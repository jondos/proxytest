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
#include "CAAccountingInstance.hpp"
#include "CASocketAddrINet.hpp"
#include "CAXMLCostConfirmation.hpp"
#include "CAXMLErrorMessage.hpp"
#include "Hashtable.hpp"

extern CACmdLnOptions* pglobalOptions;

CAAccountingSettleThread::CAAccountingSettleThread(Hashtable* a_accountingHashtable, UINT8* currentCascade)
{
	// launch AI thread
	m_pThread = new CAThread();
	m_settleCascade = currentCascade;
	m_pThread->setMainLoop((THREAD_RETURN (*)(void *))mainLoop);
	CAMsg::printMsg(LOG_DEBUG, "Now launching Accounting SettleThread...\n");
	m_bRun=true;
	m_accountingHashtable = a_accountingHashtable;
	m_pCondition = new CAConditionVariable;
	m_pThread->start(this);
}


CAAccountingSettleThread::~CAAccountingSettleThread()
	{
		m_bRun=false;
		m_pThread->join();
		delete m_pThread;
		delete m_pCondition;
	}

/**
 * The main loop. Sleeps for a few minutes, then contacts the BI to settle CCs, 
 * then sleeps again
 */
THREAD_RETURN CAAccountingSettleThread::mainLoop(void * pParam)
	{
		INIT_STACK;
		BEGIN_STACK("CAAccountingSettleThread::mainLoop");
		
		CAAccountingSettleThread* m_pAccountingSettleThread=(CAAccountingSettleThread*)pParam;
		CAAccountingBIInterface biConn;
		CAAccountingDBInterface dbConn;
		CAXMLErrorMessage * pErrMsg;
		CAXMLCostConfirmation * pCC;
		UINT32 sleepInterval;
		CAQueue q;
		UINT32 size;
		CASocketAddrINet biAddr;	
		AccountLoginHashEntry* entry;	
		AccountLoginHashEntry* nextEntry;	
	
		CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread is running...\n");
	
		CAXMLBI* pBI = pglobalOptions->getBI();
		if(pBI==NULL)
		{
			CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread; Uuupss.. No BI given --> dying!\n");
			THREAD_RETURN_ERROR;
		}
		biAddr.setAddr(pBI->getHostName(), (UINT16)pBI->getPortNumber());
		pglobalOptions->getPaymentSettleInterval(&sleepInterval);

		CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Start loop...\n");

		while(m_pAccountingSettleThread->m_bRun)
		{
			SAVE_STACK("CAAccountingSettleThread::mainLoop", "Loop");
			
			#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread going to sleep...\n");
			#endif
			m_pCondition->wait(sleepInterval * 1000);
			//sSleep((UINT16)sleepInterval);
			#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread Waking up...\n");
			#endif
			if(!m_pAccountingSettleThread->m_bRun)
			{
				CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Leaving run loop\n");
				break;
			}
			if(!dbConn.isDBConnected() && dbConn.initDBConnection()!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR, "SettleThread could not connect to Database. Retrying later...\n");
				continue;
			}
			#ifdef DEBUG	
			CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread: DB connections established!\n");
			#endif

			dbConn.getUnsettledCostConfirmations(q, m_pAccountingSettleThread->m_settleCascade);
			if (q.isEmpty() )
			{
				CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread: finished gettings CCs, found no CCs to settle\n");
			}
			else 
			{
				UINT32 qSize = q.getSize();
				UINT32 nrOfCCs = qSize / sizeof(pCC); 
				CAMsg::printMsg(LOG_DEBUG, "SettleThread: finished gettings CCs, found %u cost confirmations to settle\n",nrOfCCs);	
			}
			
			entry = NULL;
			while(!q.isEmpty())
			{
				SAVE_STACK("CAAccountingSettleThread::mainLoop", "Settling");
				// get the next CC from the queue
				size = sizeof(pCC);
				if(q.get((UINT8*)(&pCC), &size)!=E_SUCCESS)
				{
					CAMsg::printMsg(LOG_ERR, "SettleThread: could not get next item from queue\n");
					q.clean();
					break;
				}
#ifdef DEBUG				
				CAMsg::printMsg(LOG_DEBUG, "Settle Thread: trying to connect to payment instance");
#endif				
				if(biConn.initBIConnection()!=E_SUCCESS)
				{
					CAMsg::printMsg(LOG_DEBUG, "SettleThread: could not connect to BI. Retrying later...\n");
					q.clean();
					biConn.terminateBIConnection(); // make sure the socket is closed
					break;
				}
#ifdef DEBUG				
				CAMsg::printMsg(LOG_DEBUG, "SettleThread: successfully connected to payment instance");
#endif				
				if (!pCC)
				{
					CAMsg::printMsg(LOG_CRIT, "CAAccountingSettleThread: Cost confirmation is NULL!\n");
					biConn.terminateBIConnection();
					continue;
				}
				
				//CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread: try to settle...\n");
				pErrMsg = biConn.settle( *pCC );
				biConn.terminateBIConnection();
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingSettleThread: settle done!\n");
			
				if (!pCC)
				{
					CAMsg::printMsg(LOG_CRIT, "CAAccountingSettleThread: Cost confirmation is NULL!\n");
					continue;
				}
			
				// check returncode
				if(pErrMsg==NULL)  //no returncode -> connection error
				{
					CAMsg::printMsg(LOG_ERR, "SettleThread: Communication with BI failed!\n");
				}
				else if(pErrMsg->getErrorCode() != pErrMsg->ERR_OK)  //BI reported error
				{																												
					bool bDeleteCC = false;
					UINT32 authFlags = 0;
					UINT64 confirmedBytes = 0;														
					
					CAMsg::printMsg(LOG_ERR, "CAAccountingSettleThread: BI reported error no. %d (%s)\n",
						pErrMsg->getErrorCode(), pErrMsg->getDescription() );
					if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_KEY_NOT_FOUND)
					{
						authFlags |= AUTH_INVALID_ACCOUNT;	
						//dbConn.storeAccountStatus(pCC->getAccountNumber(), CAXMLErrorMessage::ERR_KEY_NOT_FOUND);				
						bDeleteCC = true;													
					}
					else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_ACCOUNT_EMPTY)
					{
						authFlags |= AUTH_ACCOUNT_EMPTY;
						UINT64* msgConfirmedBytes = (UINT64*)pErrMsg->getMessageObject();
						if (msgConfirmedBytes)
						{
							confirmedBytes = *msgConfirmedBytes;
							UINT8 tmp[32];
							print64(tmp, confirmedBytes);
							CAMsg::printMsg(LOG_ERR, "CAAccountingSettleThread: Received %s confirmed bytes!\n", tmp);
						}
						dbConn.storeAccountStatus(pCC->getAccountNumber(), CAXMLErrorMessage::ERR_ACCOUNT_EMPTY);				
						dbConn.markAsSettled(pCC->getAccountNumber(), m_pAccountingSettleThread->m_settleCascade);
					}
					/*
					else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_INVALID_PRICE_CERT)
					{
						// this should never happen; the price certs in this CC do not fit to the ones of the cascade
						// bDeleteCC = true;
					}*/
					else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_OUTDATED_CC)
					{														
						//get attached CC from error message
						CAXMLCostConfirmation* attachedCC = (CAXMLCostConfirmation*) pErrMsg->getMessageObject();
						if (attachedCC)
						{
							authFlags |= AUTH_OUTDATED_CC;
							CAMsg::printMsg(LOG_DEBUG, "SettleThread: tried outdated CC, received last valid CC back\n");
							//store it in DB
							if (dbConn.storeCostConfirmation(*attachedCC, m_pAccountingSettleThread->m_settleCascade) == E_SUCCESS)
							{
								dbConn.markAsSettled(attachedCC->getAccountNumber(), m_pAccountingSettleThread->m_settleCascade);
							}
							else
							{
								CAMsg::printMsg(LOG_ERR, "SettleThread: storing last valid CC in db failed!\n");	
							}								
							// set the confirmed bytes to the value of the CC got from the PI
							confirmedBytes = attachedCC->getTransferredBytes();	
						}
						else
						{
							CAMsg::printMsg(LOG_DEBUG, "SettleThread: Did not receive last valid CC - maybe old Payment instance?\n");
						}																		
					}
					else
					{
						CAMsg::printMsg(LOG_DEBUG, "SettleThread: Setting unknown kickout error no. %d.\n", pErrMsg->getErrorCode());
						authFlags |= AUTH_UNKNOWN;
						bDeleteCC = true; // an unknown error leads to user kickout
					}		
					
					if (authFlags)
					{
						nextEntry = new AccountLoginHashEntry; 
						nextEntry->accountNumber = pCC->getAccountNumber();
						nextEntry->authFlags = authFlags;
						nextEntry->confirmedBytes = confirmedBytes;	
						nextEntry->nextEntry = entry;
						entry = nextEntry;
					}																	
					
					if (bDeleteCC)
					{
						//delete costconfirmation to avoid trying to settle an unusable CC again and again					
						if(dbConn.deleteCC(pCC->getAccountNumber(), m_pAccountingSettleThread->m_settleCascade) == E_SUCCESS)
						{
							CAMsg::printMsg(LOG_ERR, "SettleThread: unusable cost confirmation was deleted\n");	
						}	
						else
						{						
							CAMsg::printMsg(LOG_ERR, "SettleThread: cost confirmation is unusable, but could not delete it from database\n");
						}
					}
				}
				else //settling was OK, so mark account as settled
				{
					dbConn.markAsSettled(pCC->getAccountNumber(), m_pAccountingSettleThread->m_settleCascade);
				} 

				if (pCC != NULL)
				{
					delete pCC;
					pCC = NULL;
				}
				if (pErrMsg != NULL)
				{
					delete pErrMsg;
					pErrMsg = NULL;
				}
			}
			
			/*
			 * Now alter the hashtable entries if needed. This blocks communication with JAP clients
			 * and should therefore be done as quickly as possible.
			 */
			if (entry)
			{
				m_pAccountingSettleThread->m_accountingHashtable->getMutex().lock();
				while (entry)
				{			
					AccountLoginHashEntry* oldEntry = 
						(AccountLoginHashEntry*) (m_pAccountingSettleThread->m_accountingHashtable->getValue(&(entry->accountNumber)));
					if (oldEntry)
					{				
						// the user is currently logged in											
						oldEntry->authFlags |= entry->authFlags;
						if (entry->confirmedBytes)
						{
							oldEntry->confirmedBytes = entry->confirmedBytes;
						}						
					}	
					nextEntry = entry->nextEntry;
					delete entry;
					entry = nextEntry;									
				}
				m_pAccountingSettleThread->m_accountingHashtable->getMutex().unlock();		
			}
						
		}//main while run loop
		
		FINISH_STACK("CAAccountingSettleThread::mainLoop");
		
		CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Exiting run loop!\n");
		dbConn.terminateDBConnection();	
		
		THREAD_RETURN_SUCCESS;
	}
#endif //PAYMENT
