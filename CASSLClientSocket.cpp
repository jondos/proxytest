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
#include "CASSLClientSocket.hpp"
#include "CASSLContext.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"
	
CASSLClientSocket::CASSLClientSocket()
{
	m_Socket = 0;
	m_bConnectedTCP = false;
	m_bConnectedSSL = false;
//	m_SSLCTX = 0;
	m_SSL = 0;
}


CASSLClientSocket::~CASSLClientSocket()
{
	close();
}


/**
* Shuts down the socket
*/
SINT32 CASSLClientSocket::close()
{
	if(m_bConnectedSSL) {
     SSL_shutdown(m_SSL);
     m_bConnectedSSL = false;
	}
	if(m_Socket!=0) {
		::close(m_Socket);
		m_bConnectedTCP = false;
		m_Socket = 0;
	}
	return E_SUCCESS;
}


/**
 * Creates a new socket of default type PF_INET
 */
SINT32 CASSLClientSocket::create()
{
	return create(AF_INET);
}


/**
 * Creates a new socket. Called by connect()
 *
 * @param type See documentation of glibc's socket() function
 */
SINT32 CASSLClientSocket::create(int type)
{
	if(m_Socket!=0) return E_UNKNOWN;
		
	// create basic socket
	m_Socket = socket(type, SOCK_STREAM, 0);
  if(m_Socket==INVALID_SOCKET) {
//    CAMsg::printMsg(LOG_ERROR, "CASSLClientSocket could not create SOCKET()\n");
		return SOCKET_ERROR;
	}	
	return E_SUCCESS;
}


/**
 * Init the SSL object. Connection must be established before calling this
 * WARNING: This code relies on the fact that SSL_init_library() was already
 * called (this is done in proxytest.cpp in main())
 */
SINT32 CASSLClientSocket::initSSLObject()
{
	m_SSL = SSL_new( CASSLContext::getSSLContext() );
	if(m_SSL==0) {
//		CAMsg::printMsg(LOG_ERROR, "CASSLClientSocket: Could not init SSL object\n");
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}


/**
 * Initiates the TCP connection layer.
 * After the connection is established, initSSLObject and doSSLConnect
 * must be called to establish the SSL layer
 */
SINT32 CASSLClientSocket::doTCPConnect(CASocketAddr &psa, UINT32 retry, UINT32 msWaitTime)
{
	int err=0;
	const SOCKADDR* addr=psa.LPSOCKADDR();
	int addr_len=psa.getSize();

	for(UINT32 i=0;i<retry;i++) {
//				CAMsg::printMsg(LOG_DEBUG,"Socket:connect-connect\n");
		err=::connect(m_Socket,addr,addr_len);
//				CAMsg::printMsg(LOG_DEBUG,"Socket:connect-connect-finished err: %i\n",err);
		if(err!=0) {
				err=GET_NET_ERROR;
				#ifdef _DEBUG
//				CAMsg::printMsg(LOG_DEBUG,"Con-Error: %i\n",err);
				#endif
				if(err!=ERR_INTERN_TIMEDOUT&&err!=ERR_INTERN_CONNREFUSED)
					return E_UNKNOWN; //Should be better.....
				#ifdef _DEBUG
//				CAMsg::printMsg(LOG_DEBUG,"Cannot connect... retrying\n");
				#endif
				sSleep(msWaitTime);
		}
		else {
			m_bConnectedTCP = true;
			break;
		}
	}
	return err;
}


/**
 * Does the SSL handshake. The TCP Connection must be established first
 * and SSL library must be initialized
 */
SINT32 CASSLClientSocket::doSSLConnect(CASocketAddr &psa)
{
	if(!m_bConnectedTCP) return E_UNKNOWN;
	if(m_bConnectedSSL) return E_UNKNOWN;

	// do the standard part of the ssl handshake	
	SSL_set_fd( m_SSL, m_Socket );
	// sbio = BIO_new_socket(m_Socket, BIO_NOCLOSE);
	// SSL_set_bio(m_SSL,sbio,sbio);
	if( SSL_connect( m_SSL ) != 1) {
		close();
		m_bConnectedSSL = false;
		return E_UNKNOWN;
	}

	// ssl handshake ok, now let's check the server's identity
	// Note: This code was stolen from LinuxJournal
	// issue 89: An Introduction to OpenSSL Programming, Part I of II
 
	char *host = "frunk" /* TODO: insert BI hostname here */;
	X509 *peer;
	char peer_CN[256];

	// is the certificate valid?
	if(SSL_get_verify_result( m_SSL ) != X509_V_OK ) {
//		CAMsg::printMsg(LOG_ERROR, "SSLClientSocket: Server certificate INVALID!!\n");
		close();
		m_bConnectedSSL = false;
		return E_UNKNOWN; 
	}

	// does the server name match? Check the common name
	peer = SSL_get_peer_certificate( m_SSL );
	X509_NAME_get_text_by_NID( X509_get_subject_name(peer),
														 NID_commonName, peer_CN,
														 256);

	if(strcmp(peer_CN,host) != 0) {
//		CAMsg::printMsg(LOG_ERROR, "SSLClientSocket: Server DNS name does not match certificate common name\n");
		close();
		m_bConnectedSSL = false;
		return E_UNKNOWN;
	}

	m_bConnectedSSL = true;	
	return E_SUCCESS;
}


/** Tries to connect to the peer described by psa */
SINT32 CASSLClientSocket::connect(CASocketAddr & psa)
{
	return connect(psa,1,0);
}


/**
	* Establishes the TCP/IP connection, performs the SSL handshake and
	* checks the server certificate validity
	*/
SINT32 CASSLClientSocket::connect(CASocketAddr & psa, UINT32 retry, UINT32 msWaitTime)
{
  if( (m_Socket == 0) && (create()!=E_SUCCESS))
		return E_UNKNOWN;
	if(doTCPConnect(psa, retry, msWaitTime)!=E_SUCCESS)
		return E_UNKNOWN;
	if(initSSLObject()!=E_SUCCESS)
		return E_UNKNOWN;
	if(doSSLConnect(psa)!=E_SUCCESS)
		return E_UNKNOWN;
	return E_SUCCESS;
}


/** Sends some data over the network. This may block, if socket is in blocking mode.
	@param buff the buffer of data to send
	@param len content length
	@retval E_AGAIN if non blocking socket would block or a timeout was reached
	@retval E_UNKNOWN if an error occured
	@return number of bytes send
*/
SINT32 CASSLClientSocket::send(const UINT8* buff, UINT32 len)
	{
	  if(len==0) return 0; //nothing to send
		int ret;
		SINT32 ef=0;
		do
			{
				ret=::SSL_write(m_SSL,(char*)buff,len);
				#ifdef _DEBUG
					if(ret==SOCKET_ERROR)
						CAMsg::printMsg(LOG_DEBUG,"Fehler beim SSL-Socket-send: %i",GET_NET_ERROR);
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
SINT32 CASSLClientSocket::receive(UINT8* buff,UINT32 len)
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


SINT32 CASSLClientSocket::setNonBlocking(bool b)
	{
		if(b)
			{
				#ifndef _WIN32
						int flags=fcntl(m_Socket,F_GETFL,0);
						fcntl(m_Socket,F_SETFL,flags|O_NONBLOCK);
				#else
						unsigned long flags=1;
						ioctlsocket(m_Socket,FIONBIO,&flags);
				#endif
			}
		else
			{
				#ifndef _WIN32
						int flags=fcntl(m_Socket,F_GETFL,0);
						fcntl(m_Socket,F_SETFL,flags&~O_NONBLOCK);
				#else
						unsigned long flags=0;
						ioctlsocket(m_Socket,FIONBIO,&flags);
				#endif
			}
		return E_SUCCESS;
	}

	
SINT32 CASSLClientSocket::getNonBlocking(bool* b)
	{
		#ifndef _WIN32
				int flags=fcntl(m_Socket,F_GETFL,0);
				*b=((flags&O_NONBLOCK)==O_NONBLOCK);
				return E_SUCCESS;
		#else
				return SOCKET_ERROR;
		#endif
	}
#endif
