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
#include "CAAccountingBIInterface.hpp"


/**
 * Constructor
 */
CAAccountingBIInterface::CAAccountingBIInterface()
{
	m_lineBufferSize = 0;
	
	m_pSslSocket = new CASSLClientSocket();
	
	CASocketAddrINet biAddress = config.getBiAddress();
	initBIConnection(biAddress, true);
}



/**
 * Destructor
 */
CAAccountingBIInterface::~CAAccountingBIInterface()
{	
  if(m_lineBufferSize>0) free(m_pLineBuffer);

	terminateBIConnection();
	delete m_pSslSocket;
}



/**
 * Initiate HTTP(s) connection to the BI (JPI)
 */
SINT32 CAAccountingBIInterface::initBIConnection(CASocketAddrINet address)
{
	if(m_pSslSocket->connect(address)!=E_SUCCESS) {
		CAMsg::printMsg(LOG_ERROR, "AccountingInstance could not connect to JPI!!");
		m_connected = false;
		return E_UNKNOWN;
	}
	m_connected = true;
	return E_SUCCESS;
}



/**
 * Terminate HTTP(s) connection to the BI (JPI)
 */
SINT32 CAAccountingBIInterface::terminateBIConnection()
{
	if(m_connected) {
		m_pSslSocket->close();
	}
	return E_SUCCESS;
}



/**
 * Reads a line of text from the socket. When using this function for reading,
 * please use ONLY this function. Otherwise you will lose data
 *
 * @param line String buffer for the line
 * @param maxLen buffer size
 * @return E_SUCCESS if all is OK
 * @return E_AGAIN on repeatable errors (e.g. timeout)
 * @return SOCKET_ERROR if a socket error was signaled
 * @return E_NOT_CONNECTED if we lost the connection
 */
SINT32 readLine(UINT8 * line, UINT32 * maxLen)
{
	if(!m_connected) return E_NOT_CONNECTED;
	
	// do we need to allocate the buffer first?
	if(m_lineBufferSize==0) {
		m_lineBufferSize = 1024;
		m_lineBuffer = (UINT8 *) malloc(m_lineBufferSize);
	}

	int i, ret;
	bool found = false;
	
  while(true) {
		
		// check if there is a complete line still left in the buffer
		for(i=0; !found && i<m_lineBufferOffset; i++) {
			if(m_pLineBuffer[i]=='\n') found=true;
		}
		if(found) break;

		// no complete line left, let's read more data from the socket
		while(true) {
			ret = m_pSslSocket->read( m_pLineBuffer+m_lineBufferNumBytes,
																m_lineBufferSize-m_lineBufferNumBytes);
			if(ret == SOCKET_ERROR ) {
				return SOCKET_ERROR;
			}
			if(ret == E_AGAIN) {
				return E_AGAIN;
			}
	    if(len == 0) { // socket was closed
				m_connected = false;
				m_pSslSocket->close();
				return E_NOT_CONNECTED;
			}
			if(ret>0) {
				m_lineBufferNumBytes += ret;
				break;
			}
		}
	}

	if(i >= (*maxLen)) { // check line length
		*maxLen = i+1;
		return E_SPACE;
	}
	memcpy(line, m_pLineBuffer, i);
	line[i]='\0';
	m_lineBufferNumBytes -= (i+1);
	memmove(m_pLineBuffer, m_pLineBuffer+i+1, m_lineBufferNumBytes);
	return E_SUCCESS;
}


   
/**
 * Sends a HTTP GET request to the JPI
 *
 * @param request the request keyword as c string (e.g. "/settle" or "/update")
 * @return E_UNKNOWN on socket errors
 * @return E_NOT_CONNECTED if the connection was lost
 * @return E_SUCCESS if all is OK
 */
SINT32 CAAccountingBIInterface::sendGetRequest(UINT8 * request)
{
  if(!m_connected) return E_NOT_CONNECTED;
	
	char requestF[] = "GET %s HTTP/1.1\r\n\r\n";
	UINT32 len = strlen(requestF) + strlen(request);
	char requestS[len];
	sprintf(requestS, requestF, request);
  len = strlen(requestS);
  UINT32 bufsize = len + dataSize;
  UINT8 buf[bufsize];
  memcpy(buf, requestS, len);
  memcpy(buf+len, data, dataSize);
	
	// send request
	int ret = 0;
	int offset = 0;
	do {
		ret = m_pSslSocket->send(buf, bufsize);
	} while(ret == E_AGAIN);
	if(ret == E_UNKNOWN) { // socket error
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}



/**
 * Receives the response to a HTTP request.
 *
 * @param data the buffer where the response data should go
 * @param dataSize buffer size
 * @param status integer to take the the HTTP status code (e.g. 200 OK or 404 not found)
 * @return E_SPACE if the response buffer is too small (in this case,
 * dataSize contains the minimum size needed)
 * @return E_UNKNOWN if a socket error occured or the incoming data has wrong format
 * @return E_NOT_CONNECTED if we are not connected
 * @return E_SUCCESS if the answer was received successfully
 */
SINT32 CAAccountingBIInterface::receiveResponse(UINT32 *status, UINT8 *buf, UINT32 *size)
{
	char line[255];
	int contentLength = 0;
	int i=0;
	int len=0;
	
	if(readLine(line, 255) == E_SUCCESS) { // read first line (status)
		len = strlen(line);
		
		// extract status information
		for(i=0; i<len && line[i]!=' '; i++);
		if(line[i]!=' ') return E_UNKNOWN;
    line[i]=0;
    *status = atoi(line);
	}
	else return E_UNKNOWN;

	do { // read all header lines and look for "Content-length"
		if(readLine(line,255) == E_SUCCESS) {
			len = strlen(line);
			
		}
		else return E_UNKNOWN;
				
	} while(len>0);

	if(contentLength>0) {
		// read data part
		if(contentLength> *size) {
			*size = contentLength;
			return E_SPACE;
		}
		if(m_lineBufferNumBytes>0) { // empty LineBuffer first !!!
			int numBytes = (m_lineBufferNumBytes>=contentLength?contentLength:m_lineBufferNumBytes);
			memcpy(buf, m_lineBuffer, numBytes);
			*size = numBytes;
			if(m_lineBufferNumBytes - numBytes > 0) {
				memmove(m_lineBuffer, m_lineBuffer + numBytes, m_lineBufferNumBytes-numBytes);
				m_lineBufferNumBytes -= numBytes;
			}
		}
		if( *size<contentLength) { // now LineBuffer is empty, read from real socket

		}
	}
	if(status!=200) {
		CAMsg::printMsg(LOG_ERROR, "CAAccountingBIInterface: JPI returned http error %i\n", *status);
		return E_UNKNOWN;
	}
}



/**
 * Send a cost confirmation to the JPI
 */
UINT32 CAAccountingBIInterface::settle(UINT64 accountNumber, char * xmlCC)
{
	sendRequest();
}
#endif