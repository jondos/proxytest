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
#include "CAHttpClient.hpp"
#include "CASocketAddrINet.hpp"
#include "CAXMLCostConfirmation.hpp"
#include "CAXMLErrorMessage.hpp"
#include "CAXMLBI.hpp"

#define PI_CONNECT_TIMEOUT 5000

/**
 * This class encapsulates the connection to the JPI
 *
 * @author Bastian Voigt
 */
class CAAccountingBIInterface 
{

public: 
	
	//SINT32 setPIServerAddress(UINT8* pPiServerName, UINT16 piServerPort);
	//SINT32 setPIServerCertificate(CACertificate * pPiServerCertificate);
	CAAccountingBIInterface();
	~CAAccountingBIInterface();
	SINT32 setPIServerConfiguration(CAXMLBI* pPiServerConfig);
	
	SINT32 initBIConnection();
	SINT32 terminateBIConnection();
	
	//static CAAccountingBIInterface *getInstance(CAXMLBI *pPiServerConfig);
	//static SINT32 cleanup();
	
	/**
	 * Send a cost confirmation to the JPI
	 */
	CAXMLErrorMessage * settle(CAXMLCostConfirmation &cc);
	
/*	**
	 * Request a new Balance certificate from the JPI
	 *
	 * @param balanceCert an old balance certificate that the AI wishes to update
	 * @param response a buffer to receive the new balanceCert
	 * @param responseLen the maximum size of the buffer
	 *
	SINT32 update(UINT8 *balanceCert, UINT8 * response, UINT32 *responseLen);*/

private:
	
	//CAAccountingBIInterface();
	//~CAAccountingBIInterface();
	
	
	CATLSClientSocket *m_pSocket;
	CAHttpClient	 *m_phttpClient;
	CASocketAddrINet *m_pPiServerAddress;
	CACertificate *m_pPiServerCertificate;
	
	//static CAAccountingBIInterface *m_pPiInterfaceSingleton;
	//CAMutex *m_pPiInterfaceMutex;
	//friend class CAAccountingInstance;
};

#endif
