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
#include "CAAccountingSettleThread.hpp"
#include "CAAccountingBIInterface.hpp"
#include "CAAccountingDBInterface.hpp"
#include "CAXMLCostConfirmation.hpp"
#include "CAXMLErrorMessage.hpp"

#define SLEEP_SECONDS 120

CAAccountingSettleThread::CAAccountingSettleThread()
{
}


CAAccountingSettleThread::~CAAccountingSettleThread()
{
}

/**
 * The main loop. Sleeps for a few minutes, then contacts the BI, then sleeps again
 */
THREAD_RETURN CAAccountingSettleThread::mainLoop(void * param)
{
	CAAccountingBIInterface biConn(0);
	CAAccountingDBInterface dbConn;
	CAXMLErrorMessage * pErrMsg;
	CAXMLCostConfirmation * pCC;
	CAQueue q;
	UINT8 responseBuf[512];
	UINT8 * pTmpStr;
	UINT32 size;
	UINT32 status;

	while(true)
		{
			sSleep(SLEEP_SECONDS);
		
			dbConn.initDBConnection( host,port,dbName,userName,password);
			dbConn.getUnsettledCostConfirmations(q);
			if(!q.isEmpty())
				{
					biConn.initBIConnection(CASockAddrINet);
					do
						{
							// get the next CC from the queue
							q.get((UINT8*)&pCC, &size);
							pErrMsg = biConn.settle( *pCC );
														
							// check returncode
							if(pErrMsg->getErrorCode()==pErrMsg->ERR_OK)
							{
								dbConn.markAsSettled(pCC->getAccountNumber());
							}
							delete pCC;
							delete pErrMsg;
						}
					while(!q.isEmpty());
					biConn.terminateBIConnection();
				}
			dbConn.terminateDBConnection();
		}
		
	return THREAD_RETURN_SUCCESS;
}

#endif
