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

#ifndef __CAACCOUNTINGBIINTERFACE__
#define __CAACCOUNTINGBIINTERFACE__
#include "CATLSClientSocket.hpp"
#include "CASocketAddrINet.hpp"
#include "CAXMLCostConfirmation.hpp"
#include "CAXMLErrorMessage.hpp"


/**
 * This class encapsulates the connection to the JPI
 *
 * @author Bastian Voigt
 */
class CAAccountingBIInterface 
{

public: 
	CAAccountingBIInterface(UINT32 useSSL);
	~CAAccountingBIInterface();

	SINT32 initBIConnection(CASocketAddrINet address);
	SINT32 terminateBIConnection();
	
	/**
	 * Send a cost confirmation to the JPI
	 */
	CAXMLErrorMessage * settle(CAXMLCostConfirmation &cc);
	
	/**
	 * Request a new Balance certificate from the JPI
	 *
	 * @param balanceCert an old balance certificate that the AI wishes to update
	 * @param response a buffer to receive the new balanceCert
	 * @param responseLen the maximum size of the buffer
	 */
	SINT32 update(UINT8 *balanceCert, UINT8 * response, UINT32 *responseLen);

private:
	
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
	SINT32 readLine(UINT8 * line, UINT32 * maxLen);
	
	/**
	 * Sends a HTTP GET request to the JPI
	 *
	 * @param request the request keyword as c string (e.g. "/settle" or "/update")
	 * @return E_UNKNOWN on socket errors
	 * @return E_NOT_CONNECTED if the connection was lost
	 * @return E_SUCCESS if all is OK
	 */
	SINT32 sendGetRequest(UINT8 * request);
	
	/**
	 * Sends a HTTP POST request to the JPI
	 *
	 * @param request the request keyword as c string (e.g. "/settle" or "/update")
	 * @return E_UNKNOWN on socket errors
	 * @return E_NOT_CONNECTED if the connection was lost
	 * @return E_SUCCESS if all is OK
	 */	
	SINT32 sendPostRequest(UINT8 * request, UINT8 * data, UINT32 dataLen);
	
	/**
	 * Receives the response to a HTTP request.
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
	SINT32 receiveResponse(UINT32 *status, UINT8 *buf, UINT32 *size);

	bool m_connected;
	CASocket * m_pSocket;

	UINT8 * m_pLineBuffer; // for readLine function
	UINT32 m_lineBufferSize;
	UINT32 m_lineBufferNumBytes;
	UINT32 m_bUseTLS;
};

#endif
