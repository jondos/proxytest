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

CAAccountingSettleThread::CAAccountingSettleThread()
	{
		// launch AI thread
		m_pThread = new CAThread();
		m_pThread->setMainLoop( mainLoop );
		CAMsg::printMsg(LOG_DEBUG, "Now launching Accounting SettleThread...\n");
		m_pPIList = NULL;
		m_bRun=true;
		m_pThread->start(this);
	}


CAAccountingSettleThread::~CAAccountingSettleThread()
	{
		m_bRun=false;
		m_pThread->join();
		delete m_pThread;
///@todo delete m_pPIList !
	}

/**
 * The main loop. Sleeps for a few minutes, then contacts the BI to settle CCs, 
 * then sleeps again
 */
THREAD_RETURN CAAccountingSettleThread::mainLoop(void * pParam)
	{
		CAAccountingSettleThread* m_pAccountingSettleThread=(CAAccountingSettleThread*)pParam;
		CAAccountingBIInterface biConn;
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
			{
				CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread; Uuupss.. No BI given --> dying!\n");
				THREAD_RETURN_ERROR;
			}
		biAddr.setAddr(pBI->getHostName(), (UINT16)pBI->getPortNumber());
		options.getPaymentSettleInterval(&sleepInterval);

		while(m_pAccountingSettleThread->m_bRun)
			{
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread going to sleep...\n");
				#endif
				sSleep((UINT16)sleepInterval);
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread Waking up...\n");
				#endif
				if(!m_pAccountingSettleThread->m_bRun)
				{
					CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Leaving run loop\n");
					break;
				}
				if(dbConn.initDBConnection()!=E_SUCCESS)
				{
					CAMsg::printMsg(LOG_ERR, "SettleThread could not connect to Database. Retrying later...\n");
					continue;
				}
				#ifdef DEBUG	
				CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread: DB connections established!\n");
				#endif
				dbConn.getUnsettledCostConfirmations(q);
				CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread: dbConn.getUnsettledCostConfirmations(q) finished!\n");
				while(!q.isEmpty())
				{
					// get the next CC from the queue
					size = sizeof(pCC);
					if(q.get((UINT8*)(&pCC), &size)!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_ERR, "SettleThread: could not get next item from queue\n");
						q.clean();
						break;
					}
					if(biConn.initBIConnection()!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_DEBUG, "SettleThread: could not connect to BI. Retrying later...\n");
						q.clean();
						break;
					}
					CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread: try to settle...\n");
					pErrMsg = biConn.settle( *pCC );
					biConn.terminateBIConnection();
					CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread: settle done!\n");
				
					// check returncode
					if(pErrMsg==NULL)  //no returncode -> connection error
					{
						CAMsg::printMsg(LOG_ERR, "SettleThread: Communication with BI failed!\n");
					}
					else if(pErrMsg->getErrorCode()!=pErrMsg->ERR_OK)  //BI reported error
					{
						CAMsg::printMsg(LOG_ERR, "SettleThread: BI reported error no. %d (%s)\n",
							pErrMsg->getErrorCode(), pErrMsg->getDescription() );
						CAMsg::printMsg(LOG_DEBUG, "Accounting SettleThread: BI reported error!\n");
						//delete costconfirmation to avoid trying to settle an unusable CC again and again
						/*
						if(dbConn.deleteCC(pCC->getAccountNumber()) == E_SUCCESS)
						{
							CAMsg::printMsg(LOG_ERR, "SettleThread: unusable cost confirmation was deleted\n");	
						}	
						else
						{						
							CAMsg::printMsg(LOG_ERR, "SettleThread: cost confirmation is unusable, but could not delete it from database\n");
						}*/
					}
					else //settling was OK, so mark account as settled
					{
						if(dbConn.markAsSettled(pCC->getAccountNumber())==E_SUCCESS)
						{ 
							CAMsg::printMsg(LOG_ERR, "SettleThread: Costconfirmation for the account was marked as settled!\n");
						}
						else
						{	
							CAMsg::printMsg(LOG_ERR, "SettleThread: Could not mark an account as settled!\n");
						}
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
				dbConn.terminateDBConnection();
			}//main while run loop
		CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Exiting run loop!\n");
		THREAD_RETURN_SUCCESS;
	}

/** Adds information about a PI to the list of known PIs.
	* @retval E_SPACE if not enough space for storing the information is available
	* @retval E_UNKNOWN in case of an error
	* @retval E_SUCCESS if PI is added to the list successfully
	*/
SINT32 CAAccountingSettleThread::addKnownPI(const UINT8* a_pstrID, const UINT8* a_pstrHost, UINT32 a_port, const CACertificate* a_pCert) 
	{
		if(a_pstrID==NULL||a_pstrHost==NULL||a_pCert==NULL)
			return E_UNKNOWN;
		tPaymentInstanceListEntry* pEntry = new tPaymentInstanceListEntry;
		memset(pEntry,0,sizeof(tPaymentInstanceListEntry));
		UINT32 len=strlen((const char*)a_pstrID);
		if(len>255)
			return E_SPACE;
		memcpy(pEntry->arstrPIID, a_pstrID,len+1);
		len=strlen((const char*)a_pstrHost);
		if(len>255)
			return E_SPACE;
		memcpy(pEntry->arstrHost, a_pstrHost,len+1);
		pEntry->u32Port = a_port;
		pEntry->pCertificate = a_pCert->clone();
		pEntry->next = m_pPIList;
		m_pPIList = pEntry;
		return E_SUCCESS;
	}

/** Returns a tPaymentListEntry for the requested PI. If the PI is not in the list, NULL is returned
  *@note The original list item is returned NOT a copy!
	*/
tPaymentInstanceListEntry* CAAccountingSettleThread::getPI(UINT8* a_pstrID)
	{
		tPaymentInstanceListEntry* pEntry = m_pPIList;
		while(pEntry!=NULL)
			{
				if(strcmp((const char*)pEntry->arstrPIID, (const char*)a_pstrID) == 0 )
					{
						return pEntry;
					}
				pEntry = pEntry->next;
		}
		return NULL;
	}
#endif //PAYMENT
