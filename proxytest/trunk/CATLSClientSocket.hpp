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

#ifndef __CATLSCLIENTSOCKET_HPP__
#define __CATLSCLIENTSOCKET_HPP__
#include "CASocketAddr.hpp"
#include "CASocket.hpp"
#include "CAMsg.hpp"

/**
 * This class can be used to establish a TLS / SSL encrypted connection to a server.
 * Though this class has listen() and accept() functions, these should not be used!
 * This class is meant to be used as client socket only!
 *
 * Note: This uses /dev/urandom and might cause problems on systems other than linux
 *  @author Bastian Voigt
 */
class CATLSClientSocket : public CASocket
{

public: 
	CATLSClientSocket();

	SINT32 send(const UINT8* buff,UINT32 len);
	SINT32 receive(UINT8* buff,UINT32 len);
	SINT32 close();

	/** Establishes the actual TCP/IP connection and performs the TLS handshake */
	SINT32 connect(CASocketAddr & psa, UINT32 retry, UINT32 msWaitTime);
	
	SINT32 connect(CASocketAddr & psa)
	{
		return connect(psa, 1, 0);
	}

	/** don't use this */
	SINT32 connect(CASocketAddr& psa,UINT32 msTimeOut) 
		{
			CAMsg::printMsg(LOG_ERR, "Don't use TLSClientSocket::connect(CASocketAddr&, UINT32) !!");
			return E_UNKNOWN;
		}


private:
	SINT32 initSSLObject();
	SINT32 doTCPConnect(CASocketAddr & psa, UINT32 retry, UINT32 msWaitTime);
	SINT32 doTLSConnect(CASocketAddr &psa);
	
	SSL * m_SSL;

	/** is the TLS layer established ? */
	bool m_bConnectedTLS;
};

#endif
