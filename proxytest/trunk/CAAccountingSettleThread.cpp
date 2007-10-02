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
	m_pThread->setMainLoop( mainLoop );
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


void CAAccountingSettleThread::settle()
{
	m_pCondition->lock();
	m_bSleep = false;
	m_pCondition->signal();
	m_pCondition->unlock();
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
		SettleEntry* entry;	
		SettleEntry* nextEntry;			
		bool bPICommunicationError;
	
		CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread is running...\n");
		m_pAccountingSettleThread->m_bSleep = false;
	
	
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
			
			m_pAccountingSettleThread->m_pCondition->lock();
			if (m_pAccountingSettleThread->m_bSleep)
			{
				m_pAccountingSettleThread->m_bSleep = false;
				#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread going to sleep...\n");
				#endif
				
				
#ifdef DEBUG
				UINT64 currentMillis;
				UINT8 tmpStrCurrentMillis[50];				
				getcurrentTimeMillis(currentMillis);
				print64(tmpStrCurrentMillis,currentMillis);
				CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Wait start: %s\n", tmpStrCurrentMillis);				
#endif				
				m_pAccountingSettleThread->m_pCondition->wait(sleepInterval * 1000);
#ifdef DEBUG				
				getcurrentTimeMillis(currentMillis);
				print64(tmpStrCurrentMillis,currentMillis);
				CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Wait stop:  %s\n", tmpStrCurrentMillis);
#endif				
				//sSleep((UINT16)sleepInterval);
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread Waking up...\n");
				#endif
				if(!m_pAccountingSettleThread->m_bRun)
				{
					CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Leaving run loop\n");
					break;
				}
			}
			m_pAccountingSettleThread->m_pCondition->unlock();
			
			if(!dbConn.isDBConnected() && dbConn.initDBConnection()!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR, "SettleThread could not connect to Database. Retrying later...\n");
				m_pAccountingSettleThread->m_bSleep = true;
				continue;
			}
			#ifdef DEBUG	
			CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread: DB connections established!\n");
			#endif

			dbConn.getUnsettledCostConfirmations(q, m_pAccountingSettleThread->m_settleCascade);
			if (q.isEmpty())
			{
				CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread: finished gettings CCs, found no CCs to settle\n");
				m_pAccountingSettleThread->m_bSleep = true;
				continue;
			}
			else 
			{
				UINT32 qSize = q.getSize();
				UINT32 nrOfCCs = qSize / sizeof(pCC); 
				CAMsg::printMsg(LOG_DEBUG, "SettleThread: finished gettings CCs, found %u cost confirmations to settle\n",nrOfCCs);	
			}
			
			entry = NULL;
			bPICommunicationError = false;
			while(!q.isEmpty() && m_pAccountingSettleThread->m_bRun)
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
				
				if (!pCC)
				{
					CAMsg::printMsg(LOG_CRIT, "CAAccountingSettleThread: Cost confirmation is NULL!\n");
					continue;		
				}				
				
//#ifdef DEBUG				
				CAMsg::printMsg(LOG_DEBUG, "Settle Thread: Connecting to payment instance...\n");
//#endif				
				if(biConn.initBIConnection() != E_SUCCESS)
				{
					if (!bPICommunicationError)
					{
						CAMsg::printMsg(LOG_DEBUG, "SettleThread: could not connect to BI. Retrying later...\n");
					}
					//q.clean();
					pErrMsg = NULL; // continue in order to tell AUTH_WAITING_FOR_FIRST_SETTLED_CC for all accounts
					bPICommunicationError = true;
					biConn.terminateBIConnection(); // make sure the socket is closed					
				}
				else
				{					
#ifdef DEBUG				
					CAMsg::printMsg(LOG_DEBUG, "SettleThread: successfully connected to payment instance");
#endif				
					bPICommunicationError = false;
					pErrMsg = biConn.settle( *pCC );					
					biConn.terminateBIConnection();
					CAMsg::printMsg(LOG_DEBUG, "CAAccountingSettleThread: settle done!\n");
				}

				bool bDeleteCC = false;			
				UINT32 authFlags = 0;
				UINT32 authRemoveFlags = 0;
				UINT64 confirmedBytes = 0;							
				UINT64 diffBytes = 0;
			
				// check returncode
				if(pErrMsg == NULL)  //no returncode -> connection error
				{
					m_pAccountingSettleThread->m_bSleep = true;
					authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC; // no fault of the client
					CAMsg::printMsg(LOG_ERR, "SettleThread: Communication with BI failed!\n");
				}
				else if(pErrMsg->getErrorCode() != pErrMsg->ERR_OK)  //BI reported error
				{																																	
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
						authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC;
						authFlags |= AUTH_ACCOUNT_EMPTY;
						UINT64* msgConfirmedBytes = (UINT64*)pErrMsg->getMessageObject();
						if (msgConfirmedBytes)
						{
							confirmedBytes = *msgConfirmedBytes;
							if (confirmedBytes < pCC->getTransferredBytes())
							{
								diffBytes = pCC->getTransferredBytes() - confirmedBytes;
							}
							UINT8 tmp[32];
							print64(tmp, confirmedBytes);
							CAMsg::printMsg(LOG_ERR, "CAAccountingSettleThread: Received %s confirmed bytes!\n", tmp);
						}
						dbConn.storeAccountStatus(pCC->getAccountNumber(), CAXMLErrorMessage::ERR_ACCOUNT_EMPTY);				
						dbConn.markAsSettled(pCC->getAccountNumber(), m_pAccountingSettleThread->m_settleCascade, 
							pCC->getTransferredBytes());
					}
					/*
					else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_INVALID_PRICE_CERT)
					{
						// this should never happen; the price certs in this CC do not fit to the ones of the cascade
						// bDeleteCC = true;
					}*/
					else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_OUTDATED_CC)
					{					
						authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC; // this is a Mix not a client error
															
						//get attached CC from error message
						CAXMLCostConfirmation* attachedCC = (CAXMLCostConfirmation*) pErrMsg->getMessageObject();
						if (attachedCC)
						{
							authFlags |= AUTH_OUTDATED_CC;
							CAMsg::printMsg(LOG_DEBUG, "SettleThread: tried outdated CC, received last valid CC back\n");
							//store it in DB
							if (dbConn.storeCostConfirmation(*attachedCC, m_pAccountingSettleThread->m_settleCascade) == E_SUCCESS)
							{
								if (dbConn.markAsSettled(attachedCC->getAccountNumber(), m_pAccountingSettleThread->m_settleCascade,
														attachedCC->getTransferredBytes()) != E_SUCCESS)
								{
									CAMsg::printMsg(LOG_ERR, "SettleThread: Could not mark last valid CC as settled." 
										"Maybe a new CC has been added meanwhile?\n");
								}
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
					else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_BLOCKED)
					{
						authFlags |= AUTH_BLOCKED;
						bDeleteCC = true;
					}
					else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_DATABASE_ERROR)
					{												
						//authFlags |= AUTH_DATABASE;
						// the user is not responsible for this! 
						authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC;
						m_pAccountingSettleThread->m_bSleep = true;
					}					
					else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR ||
					  		 pErrMsg->getErrorCode() ==	CAXMLErrorMessage::ERR_SUCCESS_BUT_WITH_ERRORS)
					{
						// kick out the user and store the CC
						authFlags |= AUTH_UNKNOWN;
						m_pAccountingSettleThread->m_bSleep = true;
					}
					else
					{
						// an unknown error leads to user kickout
						CAMsg::printMsg(LOG_DEBUG, "SettleThread: Setting unknown kickout error no. %d.\n", pErrMsg->getErrorCode());
						authFlags |= AUTH_UNKNOWN;						
						bDeleteCC = true; 
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
					authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC;
					if (dbConn.markAsSettled(pCC->getAccountNumber(), m_pAccountingSettleThread->m_settleCascade, 
						pCC->getTransferredBytes()) != E_SUCCESS)
					 {
					 	CAMsg::printMsg(LOG_ERR, "SettleThread: Could not mark CC as settled. Maybe a new CC has been added meanwhile?\n");
					 }
				} 
				
				if (authFlags || authRemoveFlags)
				{
					nextEntry = new SettleEntry; 
					nextEntry->accountNumber = pCC->getAccountNumber();
					nextEntry->authFlags = authFlags;
					nextEntry->authRemoveFlags = authRemoveFlags;
					nextEntry->confirmedBytes = confirmedBytes;	
					nextEntry->diffBytes = diffBytes;
					nextEntry->nextEntry = entry;
					entry = nextEntry;
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
					AccountLoginHashEntry* loginEntry = 
						(AccountLoginHashEntry*) (m_pAccountingSettleThread->m_accountingHashtable->getValue(&(entry->accountNumber)));
					if (loginEntry)
					{				
						// the user is currently logged in											
						loginEntry->authFlags |= entry->authFlags;
						loginEntry->authRemoveFlags |= entry->authRemoveFlags;
						if (entry->confirmedBytes)
						{
							loginEntry->confirmedBytes = entry->confirmedBytes;
						}											
					}
					else if (entry->authFlags & (AUTH_INVALID_ACCOUNT | AUTH_UNKNOWN))
					{
						// user is currently not logged in; delete prepaid bytes in DB
						dbConn.storePrepaidAmount(
								entry->accountNumber, 0, m_pAccountingSettleThread->m_settleCascade);
					}
					else if (entry->diffBytes)
					{
						// user is currently not logged in; set correct prepaid bytes in DB
						SINT32 prepaidBytes = 
							dbConn.getPrepaidAmount(entry->accountNumber, 
								m_pAccountingSettleThread->m_settleCascade, true);
						if (prepaidBytes > 0)
						{ 
							if (entry->diffBytes >= (UINT32)prepaidBytes)
							{
								prepaidBytes = 0;
							}
							else
							{
								prepaidBytes -= entry->diffBytes;
							}
							dbConn.storePrepaidAmount(
								entry->accountNumber, prepaidBytes, m_pAccountingSettleThread->m_settleCascade);
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
