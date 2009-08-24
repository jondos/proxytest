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
#ifndef ONLY_LOCAL_PROXY
#include "CASocketAddr.hpp"
#include "CASocket.hpp"
#include "CAMsg.hpp"
#include "CACertificate.hpp"
/**
 * This class can be used to establish a TLS / SSL encrypted connection to a server.
 * Though this class has listen() and accept() functions, these should not be used!
 * This class is meant to be used as client socket only!
 *
 * Note: This uses /dev/urandom and might cause problems on systems other than linux
 *  @author Bastian Voigt
 * @remark This class is not thread safe!
 */

#define DEFAULT_HANDSHAKE_TIMEOUT 4

class CATLSClientSocket:public CASocket
{

public:
	CATLSClientSocket();
	~CATLSClientSocket();
	SINT32 sendFully(const UINT8* buff,UINT32 len);
	SINT32 send(const UINT8* buff,UINT32 len);
	SINT32 receive(UINT8* buff,UINT32 len);
	SINT32 close();

	/** Establishes the actual TCP/IP connection and performs the TLS handshake */
	SINT32 connect(CASocketAddr & psa, UINT32 msTimeout);

	/*
	SINT32 connect(CASocketAddr & psa)
		{
			return connect(psa, 1, 0);
		}*/

	/** Sets the Certifcate we accept as server identification. Set to NULL if you do not want
	* any certificate checking.
	*@Note At the moment only a depth of verification path of zero or one is supported!
	*/
	SINT32 setServerCertificate(CACertificate* pCert);

protected:

	/*CASocket* getSocket()
	{
		return m_pSocket;
	}*/

private:
	SINT32 doTLSConnect(CASocketAddr &psa);

	SSL *m_pSSL;
	SSL_CTX *m_pCtx;
	//CASocket*	m_pSocket;
	CACertificate *m_pRootCert;
	/** is the TLS layer established ? */
	bool m_bConnectedTLS;
};

#endif
#endif //ONLY_LOCAL_PROXY
