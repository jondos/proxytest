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
#include "CAUtil.hpp"
#include "CAMsg.hpp"
	
CATLSClientSocket::CATLSClientSocket()
	{
		m_bConnectedTLS = false;
		SSL_METHOD* meth;
		meth = TLSv1_client_method();
		m_pCtx = SSL_CTX_new( meth );
		m_pSSL = NULL;
		m_pRootCert=NULL;
		m_pSocket=new CASocket();
	}

CATLSClientSocket::~CATLSClientSocket()
	{
		close();
		SSL_free(m_pSSL);
		SSL_CTX_free(m_pCtx);
		delete m_pSocket;
		if(m_pRootCert!=NULL)
			delete m_pRootCert;
	}
	
/**
* Shuts down the socket. This is an overridden virtual function
* which shuts down the TLS layer first
*/
SINT32 CATLSClientSocket::close()
	{
		if(m_bConnectedTLS)
			{
				SSL_shutdown(m_pSSL);
				m_bConnectedTLS = false;
			}
		SSL_free(m_pSSL);
		return m_pSocket->close();
	}




/**
 * Init the SSL object. SSL_init_library() must be called before this!
 */
SINT32 CATLSClientSocket::setServerCertificate(CACertificate* pCert)
	{
		m_pRootCert=pCert->clone();
		return E_SUCCESS;
	}


/**
 * Does the TLS handshake. The TCP Connection must be established first
 * and openSSL library must be initialized
 */
SINT32 CATLSClientSocket::doTLSConnect(CASocketAddr &psa)
	{
		printf("starting tls connect\n");
		if(m_bConnectedTLS) 
			return E_UNKNOWN;

		m_pSSL=SSL_new(m_pCtx);
		// do the standard part of the ssl handshake	
		int s=(SOCKET)*m_pSocket;
		printf("my set fd socket is %i\n",s);
		SSL_set_fd( m_pSSL, s );
		if( SSL_connect( m_pSSL ) != 1) 
			{
				printf("connrect not paased\n");
				close();
				m_bConnectedTLS = false;
				return E_UNKNOWN;
			}
		printf("connrect paased\n");

		// ssl handshake ok, now let's check the server's identity
		// Note: This code was stolen from LinuxJournal
		// issue 89: An Introduction to OpenSSL Programming, Part I of II
 

		// is the certificate valid?
		SINT32 ret=SSL_get_verify_result( m_pSSL );
		if(ret != X509_V_OK&&ret!=X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT ) 
			{
				CAMsg::printMsg(LOG_ERR, "SSLClientSocket: the Server certificate INVALID!! Error: %i\n",ret);
				close();
				m_bConnectedTLS = false;
				return E_UNKNOWN; 
			}
		X509* peerCert=SSL_get_peer_certificate(m_pSSL);
		if(peerCert==NULL)
			{
				CAMsg::printMsg(LOG_ERR, "SSLClientSocket: the Server shows no certificate!\n");
				close();
				m_bConnectedTLS = false;
				return E_UNKNOWN; 
			}
		ret=1;	
		if(m_pRootCert!=NULL)
			{
				EVP_PKEY* pubKey=X509_get_pubkey(m_pRootCert->getX509());
				ret=X509_verify(peerCert,pubKey);
			}
		X509_free(peerCert);
		if(ret!=1)
			{
				CAMsg::printMsg(LOG_ERR, "SSLClientSocket: could not verify server certificate!\n");
				close();
				m_bConnectedTLS = false;
				return E_UNKNOWN; 				
			}
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
		if( (rc=m_pSocket->connect(psa, retry, msWaitTime)) != E_SUCCESS)
			return rc;
		// do our own connection initialisation (TLS handshake)
		return doTLSConnect(psa);
	}


/** Sends all data over the network. This may block, until all data was sent.
	@param buff the buffer of data to send
	@param len content length
	@retval E_UNKNOWN if an error occured
	@retval E_SUCCESS if successfull
*/
SINT32 CATLSClientSocket::sendFully(const UINT8* buff, UINT32 len)
	{
	  if(len==0) 
			return E_SUCCESS; //nothing to send
		SINT32 ret=::SSL_write(m_pSSL,(char*)buff,len);
	  if(ret<len)
			return E_UNKNOWN;
		return E_SUCCESS;
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
		SINT32 ret=::SSL_read(m_pSSL,(char*)buff,len);
	  if(ret<0)
			return SOCKET_ERROR;
		return ret;
	}

#endif
