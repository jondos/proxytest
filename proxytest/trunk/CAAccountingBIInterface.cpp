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
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocket.hpp"

extern CACmdLnOptions options;

/**
 * Constructor
 * Initiates the DB connection
 * @param useTLS if nonzero, a TLS socket is used.
 */
CAAccountingBIInterface::CAAccountingBIInterface(UINT32 useTLS)
{
  // init some vars
  m_lineBufferSize = 0;
	m_bUseTLS = useTLS;
	if(useTLS)
	{
		m_pSocket = new CATLSClientSocket();
	}
	else
	{
		m_pSocket = new CASocket();
	}

  // get JPI info from config file
  UINT8 jpiHost[255];
  if(options.getJPIHost(jpiHost, 255) != E_SUCCESS)
    {
      CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface: could not get JPI hostname");
    }
  UINT16 jpiPort;
  jpiPort = options.getJPIPort();
  CASocketAddrINet biAddress;
  biAddress.setAddr(jpiHost, jpiPort);

  initBIConnection(biAddress);
}



/**
 * Destructor
 */
CAAccountingBIInterface::~CAAccountingBIInterface()
{
  if(m_lineBufferSize>0)
    free(m_pLineBuffer);

  terminateBIConnection();
  delete m_pSocket;
}



/**
 * Initiate HTTP(s) connection to the BI (JPI)
 */
SINT32 CAAccountingBIInterface::initBIConnection(CASocketAddrINet address)
{
  if(m_pSocket->connect(address)!=E_SUCCESS)
    {
      CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface: could not connect to JPI!!");
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
  if(m_connected)
    {
      m_pSocket->close();
    }
  return E_SUCCESS;
}



/**
 * Reads a line of text from the socket. When you used this function for reading
 * and you want to read directly from the socket again, you HAVE TO EMPTY THE LINE
 * BUFFER FIRST TO AVOID LOSING DATA THAT WAS READ FROM THE SOCKET AND IS STILL
 * IN THE LINEBUFFER!
 *
 * @param line String buffer for the line
 * @param maxLen buffer size
 * @return E_SUCCESS if all is OK
 * @return E_AGAIN on repeatable errors (e.g. timeout)
 * @return SOCKET_ERROR if a socket error was signaled
 * @return E_NOT_CONNECTED if we lost the connection
 */
SINT32 CAAccountingBIInterface::readLine(UINT8 * line, UINT32 * maxLen)
{
  if(!m_connected)
    return E_NOT_CONNECTED;

  // do we need to allocate the buffer first?
  if(m_lineBufferSize==0)
    {
      m_lineBufferSize = 1024;
      m_pLineBuffer = (UINT8 *) malloc(m_lineBufferSize);
    }

  UINT32 i;
  SINT32 ret;
  bool found = false;

  while(true)
    {

      // check if there is a complete line still left in the buffer
      for(i=0; !found && i<m_lineBufferNumBytes; i++)
        {
          if(m_pLineBuffer[i]=='\n')
            found=true;
        }
      if(found)
        break;

      // no complete line left, let's read more data from the socket
      while(true)
        {
          ret = m_pSocket->receive( m_pLineBuffer+m_lineBufferNumBytes,
                                       m_lineBufferSize-m_lineBufferNumBytes);
          if(ret == SOCKET_ERROR )
            {
              return SOCKET_ERROR;
            }
          if(ret == E_AGAIN)
            {
              return E_AGAIN;
            }
          if(ret == 0)
            { // socket was closed
              m_connected = false;
              m_pSocket->close();
              return E_NOT_CONNECTED;
            }
          if(ret>0)
            {
              m_lineBufferNumBytes += ret;
              break;
            }
        }
    }

  if(i >= (*maxLen))
    { // check line length
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
  if(!m_connected)
    return E_NOT_CONNECTED;

  UINT8 requestF[] = "GET %s HTTP/1.1\r\n\r\n";
  UINT32 len = strlen((char *)requestF) + strlen((char *)request);
  UINT8* requestS=new UINT8[len+1];
  sprintf((char *)requestS, (char *)requestF, (char *)request);
  len = strlen((char *)requestS);

  // send request
  SINT32 ret = 0;
  //int offset = 0;
  do
    {
      ret = m_pSocket->send(requestS, len);
    }
  while(ret == E_AGAIN);
	delete requestS;
  if(ret == E_UNKNOWN)
    { // socket error
      return E_UNKNOWN;
    }
  return E_SUCCESS;
}



/**
 * Sends a HTTP POST request to the JPI
 *
 * @param request the request keyword as c string (e.g. "/settle" or "/update")
 * @return E_UNKNOWN on socket errors
 * @return E_NOT_CONNECTED if the connection was lost
 * @return E_SUCCESS if all is OK
 */
SINT32 CAAccountingBIInterface::sendPostRequest(UINT8 * request, UINT8 * data, UINT32 dataLen)
{
  if(!m_connected)
    return E_NOT_CONNECTED;

  UINT8 requestF[] = "POST %s HTTP/1.1\r\nContent-length: %d\r\n\r\n";
  UINT32 len = strlen((char *)requestF) + strlen((char *)request) + 30;
  UINT8* requestS=new UINT8[len+1];
  sprintf((char *)requestS, (char *)requestF, (char *)request, dataLen);
  len = strlen((char *)requestS);
  UINT32 bufsize = len + dataLen;
  UINT8* buf=new UINT8[bufsize];
  memcpy(buf, requestS, len);
  memcpy(buf+len, data, dataLen);

	// send request
	int ret = 0;
  //int offset = 0;
  do
    {
      ret = m_pSocket->send(buf, bufsize);
    }
  while(ret == E_AGAIN);
  delete requestS;
	delete buf;
	if(ret == E_UNKNOWN)
    { // socket error
      return E_UNKNOWN;
    }
  return E_SUCCESS;
}



/**
 * Receives the response to a HTTP request.
 * TODO: Better error handling
 *
 * @param status integer to take the the HTTP status code (e.g. 200 OK or 404 not found)
 * @param buf the buffer where the HTTP response data is put into
 * @param size size of the buffer
 * @return E_SPACE if the response buffer is too small (in this case,
 * dataSize contains the minimum size needed)
 * @return E_AGAIN if you can try again
 * @return E_UNKNOWN if a socket error occured or the incoming data has 
 * wrong format or we have no connection
 * @return E_NOT_CONNECTED if we are not connected
 * @return E_SUCCESS if the answer was received successfully.
 * In this case size contains the size of the data received
 */
SINT32 CAAccountingBIInterface::receiveResponse(UINT32 *status, UINT8 *buf, UINT32 *size)
{
  UINT8 line[255];
  UINT32 contentLength = 0;
  SINT32 i=0;
  SINT32 len=0;
  UINT32 j=255;
  if(readLine(line, &j) == E_SUCCESS)
    { // read first line (status)
      len = strlen((char *)line);

      // extract status information
      for(i=0; i<len && line[i]!=' '; i++)
        ;
      if(line[i]!=' ')
        return E_UNKNOWN;
      line[i]=0;
      *status = atoi((char *)line);
    }
  else
    return E_UNKNOWN;

  do
    { // read all header lines and parse content length
      j=255;
      if(readLine(line,&j) == E_SUCCESS)
        {
          len = strlen((char *)line);
          if(len>15)
            {
              UINT8 temp[15];
              memcpy(temp, line, 15);
              if(memcmp(temp, "Content-length:", 15)==0)
                {
                  contentLength = atoi((char *)line+15);
                }
            }
        }
      else
        return E_UNKNOWN;
    }
  while(len>0);

  if(contentLength>0)
    {
      // read data part
      if(contentLength> *size)
        {
          *size = contentLength;
          return E_SPACE;
        }
      if(m_lineBufferNumBytes>0)
        { // empty LineBuffer first !!!
          int numBytes = (m_lineBufferNumBytes>=contentLength?contentLength:m_lineBufferNumBytes);
          memcpy(buf, m_pLineBuffer, numBytes);
          *size = numBytes;
          if(m_lineBufferNumBytes - numBytes > 0)
            {
              memmove(m_pLineBuffer, m_pLineBuffer + numBytes, m_lineBufferNumBytes-numBytes);
              m_lineBufferNumBytes -= numBytes;
            }
        }
      while( *size<contentLength)
        { // now LineBuffer is empty, read from real socket
          int ret = m_pSocket->receive(buf + *size, contentLength - *size);
          if(ret == SOCKET_ERROR )
            {
              return SOCKET_ERROR;
            }
          if(ret == E_AGAIN)
            {
              return E_AGAIN;
            }
          if(ret == 0)
            { // socket was closed
              m_connected = false;
              m_pSocket->close();
              return E_NOT_CONNECTED;
            }
          if(ret>0)
            *size += ret;
        }
    }
  if(*status!=200)
    {
      CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface: JPI returned http error %i\n", *status);
      return E_UNKNOWN;
    }
  return E_SUCCESS;
}



/**
 * Send a cost confirmation to the JPI
 * TODO: Error handling
 */
CAXMLErrorMessage * CAAccountingBIInterface::settle(CAXMLCostConfirmation &cc)
{
	UINT8 * pStrCC;
	UINT8 response[512];
	UINT32 size, status;
	CAXMLErrorMessage *pErrMsg;
	
	pStrCC = cc.toXmlString(size);
	sendPostRequest((UINT8*)"/settle", pStrCC, strlen((char*)pStrCC));
	delete[] pStrCC;
	
	size = 512;
	receiveResponse(&status, response, &size);
	if( (!response) || (status!=200) )
	{
		return NULL;
	}
	else
	{
		pErrMsg = new CAXMLErrorMessage(response);
		return pErrMsg;
	}

/*  UINT8 requestF[] = "<?xml version=\"1.0\">\n<Confirmations>\n%s</Confirmations>\n";
  UINT32 sendbuflen = strlen((char *)costConfirmation) + strlen((char *)requestF) + 10;
  UINT8* sendbuf=new UINT8[sendbuflen];
  UINT32 status;
  sprintf((char *)sendbuf, (char *)requestF, (char *)costConfirmation);
  sendPostRequest((UINT8 *)"/settle", sendbuf, strlen((char *)sendbuf));
  delete sendbuf;
	UINT32 responseLen = 500;
  UINT8* response=new UINT8[responseLen];
  receiveResponse(&status, response, &responseLen);
  delete response;
	return E_SUCCESS;*/
}



/**
 * Request a new Balance certificate from the JPI
 * TODO: Error handling
 *
 * @param balanceCert an old balance certificate that the AI wishes to update
 * @param response a buffer to receive the new balanceCert
 * @param responseLen the maximum size of the buffer
 */
SINT32 CAAccountingBIInterface::update(UINT8 *balanceCert, UINT8 * response, UINT32 *responseLen)
{
  UINT8 requestF[] = "<?xml version=\"1.0\">\n<Balances>\n%s</Balances>\n";
  UINT32 sendbuflen = strlen((char *)balanceCert) + strlen((char *)requestF) + 10;
  UINT8* sendbuf=new UINT8[sendbuflen];
  UINT32 status;
  sprintf((char *)sendbuf, (char *)requestF, (char *)balanceCert);
  sendPostRequest((UINT8 *)"/update", sendbuf, strlen((char *)sendbuf));
  delete sendbuf;
	receiveResponse(&status, response, responseLen);
  return E_SUCCESS;
}


#endif
