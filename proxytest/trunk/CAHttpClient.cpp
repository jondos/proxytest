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
#include "CAHttpClient.hpp"

/**
* Sends a HTTP GET request to the server
*
* @param url the local part of the URL requested (e.g. "/settle")
* @return E_UNKNOWN on socket errors
* @return E_NOT_CONNECTED if the connection was lost
* @return E_SUCCESS if all is OK
*/
SINT32 CAHttpClient::sendGetRequest(UINT8 * url)
	{
		UINT8 requestF[] = "GET %s HTTP/1.0\r\n\r\n";
		UINT32 len;
		UINT8* requestS;
		
		if(!m_pSocket)
			{
				return E_NOT_CONNECTED;
			}
		
		// put request together
		len = strlen((char *)requestF) + strlen((char *)url);
		requestS = new UINT8[len+1];
		sprintf((char *)requestS, (char *)requestF, (char *)url);
		len = strlen((char *)requestS);
	
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "HttpClient now sending: %s\n", requestS);
		#endif
		
		// send it
		/// TODO: use sendFully() here
		SINT32 ret = 0;
		do
			{
				ret = m_pSocket->send(requestS, len);
			}
		while(ret == E_AGAIN);
		delete[] requestS;
		if(ret == E_UNKNOWN)
			{ // socket error
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}



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
SINT32 CAHttpClient::sendPostRequest(const UINT8 * url, const UINT8 * data, const UINT32 dataLen)
	{
		UINT8 requestF[] = "POST %s HTTP/1.0\r\nContent-length: %d\r\n\r\n";
		UINT32 len;
		UINT8* requestS;
		UINT32 bufsize;
		UINT8* buf;
		
		if(!m_pSocket)
			{
				return E_NOT_CONNECTED;
			}
	
		// put request together
		len = strlen((char *)requestF) + strlen((char *)url) + 30;
		requestS=new UINT8[len+1];
		sprintf((char *)requestS, (char *)requestF, (char *)url, dataLen);
		len = strlen((char *)requestS);
		bufsize = len + dataLen +1;
		buf=new UINT8[bufsize];
		memcpy(buf, requestS, len);
		memcpy(buf+len, data, dataLen);
		buf[len+dataLen]=0;
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "HttpClient now sending: %s\n", buf);
		#endif
	
		// send it
		/// TODO: use sendFully() here
		int ret = 0;
		do
			{
				ret = m_pSocket->send(buf, bufsize);
			}
		while(ret == E_AGAIN);
		delete[] requestS;
		delete[] buf;
		if(ret == E_UNKNOWN)
			{ // socket error
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

/** 
	* receives the HTTP header and parses the content length 
	* @param contentLength receives the parsed content length
	* @param statusCode if set, receives the http statuscode (200, 403, 404, ...)
	* @return E_SUCCESS if all is OK, E_UNKNOWN if the server returned a http errorcode
	* TODO: Verify that "HTTP/1.1 200 OK" must be the first line!
	*/
SINT32 CAHttpClient::parseHTTPHeader(UINT32* contentLength, UINT32 * statusCode)
	{
		char *line = new char[255];
		SINT32 ret = 0;
		SINT32 ret2 = E_SUCCESS;
		if(!m_pSocket)
			{
				return E_NOT_CONNECTED;
			}
		
		do
		{
			int i=0;
			UINT8 byte = 0;
			do
			{
				ret = m_pSocket->receive(&byte, 1);
				if(byte == '\r' || byte == '\n')
				{
					line[i++] = 0;
				}
				else
				{
					line[i++] = byte;
				}
			}
			while(byte != '\n' && i<255 && ret > 0);
	
			if(ret < 0)
				break;
	
			if(strncmp(line, "HTTP", 4) == 0)
			{
				if(statusCode!=NULL && strlen(line)>9) // parse statusCode
				{
					*statusCode = atoi(line+9);
				}
				if(strstr(line, "200 OK") == NULL)
				{
					CAMsg::printMsg(LOG_CRIT,"HttpClient: Error: Server returned: '%s'.\n",line);
					if(strstr(line, "404") != NULL)
						{
							CAMsg::printMsg(
									LOG_CRIT,
									"HttpClient: Error: Maybe the desired mix is not online? Retry later.\n"
								);
						}
					ret2 = E_UNKNOWN;
					break;
				}
			}
			/// TODO: do it better (case insensitive compare!)
			else if( (strncmp(line, "Content-length: ", 16) == 0) ||
				(strncmp(line, "Content-Length: ", 16) == 0))
			{
				*contentLength = (UINT32) atol(line+16);
			}
		}
		while(strlen(line) > 0);
		delete[] line;
		return ret2;
	}
