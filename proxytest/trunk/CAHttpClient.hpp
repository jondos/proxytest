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
#ifndef __CAHTTPCLIENT_HPP
#define __CAHTTPCLIENT_HPP

#include "CASocket.hpp"
#include "CAMsg.hpp"

/**
 * Very simple http client.
 * Used by CAInfoService and CAAccountingBIInterface.
 * Must be initialized with a connected socket.
 * Note that the socket is still owned by the caller. 
 * CAHttpClient will not delete the socket
 *
 * @author Bastian Voigt
 */
class CAHttpClient
{
public:
	CAHttpClient(CASocket * pSocket)
		{
			m_pSocket = pSocket;
		}
		
	CAHttpClient()
		{
			m_pSocket = NULL;
		}

	~CAHttpClient() {}
	
	SINT32 setSocket(CASocket* pSocket)
		{
			m_pSocket = pSocket;
			return E_SUCCESS;
		}
	
	/**
	* Sends a HTTP GET request to the server
	*
	* @param url the local part of the URL requested (e.g. "/settle")
	* @return E_UNKNOWN on socket errors
	* @return E_NOT_CONNECTED if the connection was lost
	* @return E_SUCCESS if all is OK
	*/
	SINT32 sendGetRequest(const UINT8 * url);
	
	/**
	* Sends a HTTP POST request to the server
	*
	* @param url the local part of the requested URL (e.g. "/settle")
	* @param data a buffer containing the data
	* @param dataLen the length of the data in bytes
	* @return E_UNKNOWN on socket errors
	* @return E_NOT_CONNECTED if the connection was lost
	* @return E_SUCCESS if all is OK
	*/
	SINT32 sendPostRequest(const UINT8 * url, const UINT8 * data, const UINT32 dataLen);
	
	/** 
	 * receives the HTTP header and parses the content length 
	 * @param contentLength receives the parsed content length
	 * @param statusCode if set, receives the http statuscode (200, 403, 404, ...)
	 * @retval E_SUCCESS if all is OK
	 * @retval E_UNKNOWN if the server returned a http errorcode
	 * TODO: Verify that "HTTP/1.1 200 OK" must be the first line!
	 */
	SINT32 parseHTTPHeader(UINT32* contentLength, UINT32 * statusCode=NULL, UINT32 msTimeOut=3000);
	
	/** Retruns the content of the response*/
	SINT32 getContent(UINT8* a_pContent, UINT32* a_pLength);
	
private:

	/** the socket connection to the http server */
	CASocket * m_pSocket;
};

#endif
