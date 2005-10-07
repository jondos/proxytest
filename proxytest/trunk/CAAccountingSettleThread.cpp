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
#include "CACmdLnOptions.hpp"
#include "CASocketAddrINet.hpp"
#include "CAXMLCostConfirmation.hpp"
#include "CAXMLErrorMessage.hpp"

//#define SLEEP_SECONDS 20




CAAccountingSettleThread::CAAccountingSettleThread()
	{
		// launch AI thread
		m_pThread = new CAThread();
		m_pThread->setMainLoop( mainLoop );
		CAMsg::printMsg(LOG_DEBUG, "Now launching Accounting SettleThread...\n");
		m_bRun=true;
		m_pThread->start((void*)&m_bRun);
	}


CAAccountingSettleThread::~CAAccountingSettleThread()
	{
		m_bRun=false;
		m_pThread->join();
		delete m_pThread;
	}

/**
 * The main loop. Sleeps for a few minutes, then contacts the BI to settle CCs, 
 * then sleeps again
 */
THREAD_RETURN CAAccountingSettleThread::mainLoop(void * pParam)
	{
		CAAccountingBIInterface biConn(0 /* no tls, for now */);
		CAAccountingDBInterface dbConn;
		CAXMLErrorMessage * pErrMsg;
		CAXMLCostConfirmation * pCC;
		UINT32 sleepInterval;
		CAQueue q;
		UINT32 size;
		CASocketAddrINet biAddr;
	
		CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread is running...\n");
	
		CAXMLBI* pBI = options.getBI();
		if(pBI==NULL)
			THREAD_RETURN_ERROR;
		biAddr.setAddr(pBI->getHostName(), (UINT16)pBI->getPortNumber());
		options.getPaymentSettleInterval(&sleepInterval);

		volatile bool* bRun=(volatile bool*)pParam;
	
		while(*bRun)
			{
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread going to sleep...\n");
				#endif
				sSleep((UINT16)sleepInterval);
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread Waking up...\n");
				#endif
				if(!(*bRun))
					break;
				if(dbConn.initDBConnection()!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_ERR, "SettleThread could not connect to Database. Retrying later...\n");
						continue;
					}
				dbConn.getUnsettledCostConfirmations(q);
				if(!q.isEmpty())
					{
						if(biConn.initBIConnection()!=E_SUCCESS)
							{
								CAMsg::printMsg(LOG_DEBUG, "SettleThread: could not connect to BI. Retrying later...\n");
								//delet all CC's from the queue
								while(!q.isEmpty())
									{
										size = sizeof(CAXMLCostConfirmation*);
										if(q.get((UINT8*)(&pCC), &size)!=E_SUCCESS)
											{
												CAMsg::printMsg(LOG_ERR, "SettleThread: could not get next item from queue\n");
												break;
											}
										delete pCC;
									}
								q.clean();
								biConn.terminateBIConnection();
								dbConn.terminateDBConnection();
								continue;
							}
						do
							{
								// get the next CC from the queue
								size = sizeof(pCC);
								if(q.get((UINT8*)(&pCC), &size)!=E_SUCCESS)
									{
										CAMsg::printMsg(LOG_ERR, "SettleThread: could not get next item from queue\n");
										q.clean();
										break;
									}
								pErrMsg = biConn.settle( *pCC );
							
								// check returncode
								if(!pErrMsg)
									{
										CAMsg::printMsg(LOG_ERR, "SettleThread: Communication with BI failed!\n");
									}
								else if(pErrMsg->getErrorCode()!=pErrMsg->ERR_OK)
									{
										CAMsg::printMsg(LOG_ERR, "SettleThread: BI reported error no. %d (%s)\n",
											pErrMsg->getErrorCode(), pErrMsg->getDescription() );
									}
								else
									{
										if(dbConn.markAsSettled(pCC->getAccountNumber())!=E_SUCCESS)
											CAMsg::printMsg(LOG_ERR, "SettleThread: Could not mark an account as settled!\n");
									}
								delete pCC;
								delete pErrMsg;
							}
						while(!q.isEmpty());
						biConn.terminateBIConnection();
					}
				dbConn.terminateDBConnection();
			}//while
		THREAD_RETURN_SUCCESS;
	}
#endif //PAYMENT
