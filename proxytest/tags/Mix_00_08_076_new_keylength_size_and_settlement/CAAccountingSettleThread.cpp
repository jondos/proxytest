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
#include "CALibProxytest.hpp"

CAAccountingSettleThread::CAAccountingSettleThread(Hashtable* a_accountingHashtable, 
													UINT8* currentCascade)/*,
													CAAccountingBIInterface *pPiInterface,
													CAAccountingDBInterface *pDbInterface)*/
{
	// launch AI thread
	m_pThread = new CAThread((UINT8*)"Accounting Settle Thread");
	m_settleCascade = currentCascade;
	//m_pPiInterface = pPiInterface;
	//m_pDbInterface = pDbInterface;
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
		if(m_pThread != NULL)
		{
			settle();
			m_pThread->join();
			delete m_pThread;
			m_pThread = NULL;
		}
		if(m_pCondition != NULL)
		{
			delete m_pCondition;
			m_pCondition = NULL;
		}
	}


void CAAccountingSettleThread::settle()
{
	m_pCondition->lock();
	//m_bSleep = false;
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

		UINT32 settlement_status;
		UINT32 sleepInterval = CALibProxytest::getOptions()->getPaymentSettleInterval();
		
		CAAccountingSettleThread* m_pAccountingSettleThread=(CAAccountingSettleThread*)pParam;

		CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Start loop...\n");

		while(m_pAccountingSettleThread->m_bRun) {
			m_pAccountingSettleThread->m_pCondition->lock();
			m_pAccountingSettleThread->m_pCondition->wait(sleepInterval * 1000);
			m_pAccountingSettleThread->m_pCondition->unlock();
			settlement_status = CAAccountingInstance::newSettlementTransaction();
			if(settlement_status != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR, "AccountingSettleThread: Settlement transaction failed\n");
			}
		}

		FINISH_STACK("CAAccountingSettleThread::mainLoop");

		CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Exiting run loop!\n");
		/*dbConn.terminateDBConnection();
		delete biConn;
		biConn = NULL;*/
		THREAD_RETURN_SUCCESS;
	}
#endif //PAYMENT
