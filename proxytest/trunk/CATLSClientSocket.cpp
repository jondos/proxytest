/*
Copyright (c) 2000, The JAP-Team 
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.

	- Redistributions in binary form must reproduce the above copyright notice, 
	  this list of conditions and the following disclaimer in the documentation and/or 
		other materials provided with the distribution.

	- Neither the name of the University of Technology Dresden, Germany nor the names of its contributors 
	  may be used to endorse or promote products derived from this software without specific 
		prior written permission. 

	
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS 
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/
#include "StdAfx.h"
#ifdef PAYMENT
#include "CATLSClientSocket.hpp"
#include "CASSLContext.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"
	
CATLSClientSocket::CATLSClientSocket() : CASocket()
{
	m_bConnectedTLS = false;
	m_SSL = 0;
}


/**
* Shuts down the socket. This is an overridden virtual function
* which shuts down the TLS layer first
*/
SINT32 CATLSClientSocket::close()
{
	if(m_bConnectedTLS) {
     SSL_shutdown(m_SSL);
     m_bConnectedTLS = false;
	}
	CASocket::close();
/*	if(m_Socket!=0) {
		::close(m_Socket);
		m_bConnectedTCP = false;
		m_Socket = 0;
	}*/
	return E_SUCCESS;
}




/**
 * Init the SSL object. SSL_init_library() must be called before this!
 */
SINT32 CATLSClientSocket::initSSLObject()
{
	if(m_SSL != 0) return E_UNKNOWN;
	m_SSL = SSL_new( CASSLContext::getSSLContext() );
	if(m_SSL==0) 
	{
		CAMsg::printMsg(LOG_ERR, "CATLSClientSocket: Could not init SSL object\n");
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}


/**
 * Does the TLS handshake. The TCP Connection must be established first
 * and openSSL library must be initialized
 * TODO: Add certificate common name checking
 */
SINT32 CATLSClientSocket::doTLSConnect(CASocketAddr &psa)
{
/*	char *host = "frunk"; // TODO: insert BI hostname here
	X509 *peer;
	char peer_CN[256];*/

	if(m_bSocketIsClosed || m_bConnectedTLS) return E_UNKNOWN;

	// do the standard part of the ssl handshake	
	SSL_set_fd( m_SSL, m_Socket );
	if( SSL_connect( m_SSL ) != 1) 
		{
			close();
			m_bConnectedTLS = false;
			return E_UNKNOWN;
		}

	// ssl handshake ok, now let's check the server's identity
	// Note: This code was stolen from LinuxJournal
	// issue 89: An Introduction to OpenSSL Programming, Part I of II
 

	// is the certificate valid?
	if(SSL_get_verify_result( m_SSL ) != X509_V_OK ) 
		{
			CAMsg::printMsg(LOG_ERR, "SSLClientSocket: Server certificate INVALID!!\n");
			close();
			m_bConnectedTLS = false;
			return E_UNKNOWN; 
		}

	// does the server name match? Check the common name
/*	peer = SSL_get_peer_certificate( m_SSL );
	X509_NAME_get_text_by_NID( X509_get_subject_name(peer),
														 NID_commonName, peer_CN,
														 256);

	if(strcmp(peer_CN,host) != 0) {
		CAMsg::printMsg(LOG_ERR, "SSLClientSocket: Server DNS name does not match certificate common name\n");
		close();
		m_bConnectedTLS = false;
		return E_UNKNOWN;
	}*/

	m_bConnectedTLS = true;
	return E_SUCCESS;
}




/**
	* Establishes the TCP/IP connection, performs the TLS handshake and
	* checks the server certificate validity
	*/
SINT32 CATLSClientSocket::connect(CASocketAddr & psa, UINT32 retry, UINT32 msWaitTime)
{
	SINT32 rc;
	
	// call base class connect function
	if( (rc=CASocket::connect(psa, retry, msWaitTime)) != E_SUCCESS)
		return rc;

	// do our own connection initialisation (TLS handshake)
	if( (rc = initSSLObject()) !=E_SUCCESS) return rc;
	return doTLSConnect(psa);
}


/** Sends some data over the network. This may block, if socket is in blocking mode.
	@param buff the buffer of data to send
	@param len content length
	@retval E_AGAIN if non blocking socket would block or a timeout was reached
	@retval E_UNKNOWN if an error occured
	@return number of bytes send
*/
SINT32 CATLSClientSocket::send(const UINT8* buff, UINT32 len)
	{
	  if(len==0) return 0; //nothing to send
		int ret;
		SINT32 ef=0;
		do
			{
				ret=::SSL_write(m_SSL,(char*)buff,len);
				#ifdef _DEBUG
					if(ret==SOCKET_ERROR)
						CAMsg::printMsg(LOG_DEBUG,"Fehler beim TLS-Socket-send: %i",GET_NET_ERROR);
				#endif
			}
		while(ret==SOCKET_ERROR&&(ef=GET_NET_ERROR)==EINTR);
		if(ret==SOCKET_ERROR)
			{
				if(ef==ERR_INTERN_WOULDBLOCK)
					return E_AGAIN;
				return E_UNKNOWN;
			}
	  return ret;
	}
	

/** Will receive some bytes from the socket. May block or not depending on whatever this socket
	* was set to blocking or non-blocking mode.
	* Warning: If socket is in blocking mode and receive is called, receive will block until some
	* data is available, EVEN IF AN OTHER THREAD WILL CLOSE THIS SOCKET!
	* @param buff the buffer which get the received data
	* @param len size of buff
	*	@return SOCKET_ERROR if an error occured
	* @retval E_AGAIN, if socket was in non-blocking mode and receive would block or a timeout was reached
	* @retval 0 if socket was gracefully closed
	* @return the number of bytes received (always >0)
**/
SINT32 CATLSClientSocket::receive(UINT8* buff,UINT32 len)
	{
		int ret;
	  int ef=0;
	  do
			{
				ret=::SSL_read(m_SSL,(char*)buff,len);
			}
	  while(ret==SOCKET_ERROR&&(ef=GET_NET_ERROR)==EINTR);
		if(ret==SOCKET_ERROR)
			{
				if(ef==ERR_INTERN_WOULDBLOCK)
					return E_AGAIN;
			}
#ifdef _DEBUG
//		if(ret==SOCKET_ERROR)
//	      CAMsg::printMsg(LOG_DEBUG,"Receive error: %i\n",ef);
#endif
	  return ret;
	}


#endif
