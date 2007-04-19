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
* @retval E_UNKNOWN on socket errors
* @retval E_NOT_CONNECTED if the connection was lost
* @retval E_SUCCESS if all is OK
*/
SINT32 CAHttpClient::sendGetRequest(const UINT8 * url)
	{
		static const UINT8 requestF[] = "GET %s HTTP/1.0\r\n\r\n";
		static const UINT32 requestFLen=strlen((char *)requestF);
		if(m_pSocket==NULL)
			{
				return E_NOT_CONNECTED;
			}
		
		// put request together
		UINT32 len = requestFLen + strlen((char *)url);
		UINT8* requestS = new UINT8[len+1];
		sprintf((char *)requestS, (const char *)requestF, (const char *)url);
		len = strlen((char *)requestS);
	
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "HttpClient now sending: %s\n", requestS);
		#endif
		
		// send it
		SINT32 ret = m_pSocket->sendFully(requestS,len);
		delete[] requestS;
		if(ret != E_SUCCESS)
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
		UINT8 requestF[] = "POST %s HTTP/1.0\r\nContent-length: %u\r\n\r\n";
		UINT32 len;
		UINT8* requestS;
		
		if(!m_pSocket)
			{
				return E_NOT_CONNECTED;
			}
	
		// put request together
		len = strlen((char *)requestF) + strlen((char *)url) + 30;
		requestS=new UINT8[len+1];
		sprintf((char *)requestS, (char *)requestF, (char *)url, dataLen);
		len = strlen((char *)requestS);
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "HttpClient now sending: %s\n", requestS);
		#endif
		SINT32 ret=m_pSocket->sendFully(requestS,len);
		delete[] requestS;
		if(ret!=E_SUCCESS)
			{
				return E_UNKNOWN;
			}
		ret=m_pSocket->sendFully(data,dataLen);
		if(ret != E_SUCCESS)
			{ // socket error
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

/** 
	* Receives the HTTP header and parses the content length 
	* @param contentLength receives the parsed content length
	* @param statusCode if set, receives the http statuscode (200, 403, 404, ...)
	* @retval E_SUCCESS if all is OK
	* @retval E_NOT_CONNECTED if the connection to the Web-Server was lost 
	* @retval E_UNKNOWN if the server returned a http errorcode, in this case statusCode is set to 0 and contentLength is set to 0
	* @todo: Verify that "HTTP/1.1 200 OK" must be the first line!
	* @todo: Maybe set an other statusCode in case of an error ?
	*/
SINT32 CAHttpClient::parseHTTPHeader(UINT32* contentLength, UINT32 * statusCode)
	{
		if(contentLength!=NULL)
			*contentLength=0;
		if(statusCode!=NULL)
			*statusCode=0;
		char *line = new char[255];
		SINT32 ret = 0;
		SINT32 ret2 = E_UNKNOWN;
		if(!m_pSocket)
		{
			return E_NOT_CONNECTED;
		}
		do
			{
				int i=0;
				UINT8 byte = 0;
				do//Read a line from the WebServer
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
	
				if(ret < 0||i>=255)
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
								break;
							}
						else
							ret2=E_SUCCESS;
					}
				/// TODO: do it better (case insensitive compare!)
				else if( (strncmp(line, "Content-length: ", 16) == 0) ||
										(strncmp(line, "Content-Length: ", 16) == 0))
					{
						*contentLength = (UINT32) atol(line+16);
					}
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG,"Server returned: '%s'.\n",line);
				#endif	
			} 
		while(strlen(line) > 0);//Stop reading of response lines, if an empty line was reveived...
		delete[] line;
		return ret2;
	}

/** Gets the content of a HTTP response
* @param a_pContent buff which receives the content 
* @param a_pLength on input contains the size of a_pContent, on return contains the number of received bytes
* @retval E_NOT_CONNECTED if socket is not connected
* @retval E_SUCCESS if successful
*/
SINT32 CAHttpClient::getContent(UINT8* a_pContent, UINT32* a_pLength)
	{
		if(m_pSocket==NULL)
			{
				return E_NOT_CONNECTED;
			}
			
		UINT32 aktIndex = 0;
		UINT32 len=*a_pLength;
		while(len>0)
			{
				SINT32 ret=m_pSocket->receive(a_pContent+aktIndex,len);
				if(ret<=0)
					break;
				aktIndex+=ret;
				len-=ret;	
			}
		*a_pLength=aktIndex;	
		return E_SUCCESS;
	}
