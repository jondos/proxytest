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

#ifndef CAACCOUNTINGBIINTERFACE_H
#define CAACCOUNTINGBIINTERFACE_H
#include "CASSLClientSocket.hpp"
#include "CASocketAddrINet.hpp"
/**
 * This object encapsulates the connection to the JPI and
 * allows sending "settle" http(s) requests with cost confirmations
 *
 * @author Bastian Voigt
 */

class CAAccountingBIInterface 
{

public: 
	CAAccountingBIInterface();
	~CAAccountingBIInterface();

	SINT32 initBIConnection(CASocketAddrINet address);
	SINT32 terminateBIConnection();
	SINT32 settle(UINT64 accountNumber, char * xmlCC);
	SINT32 update(UINT64 accountNumber);

private:
	SINT32 readLine(char * line, int maxLen);
	SINT32 sendGetRequest(UINT8 * request, UINT8 * data, UINT32 dataSize, 
												UINT8 * response, UINT32 responseBufsize);

	bool m_connected;
	CASSLClientSocket * m_sslSocket;

	UINT8 * m_pLineBuffer; // for readLine function
	UINT32 m_lineBufferSize;
	UINT32 m_lineBufferNumBytes;
};

#endif
