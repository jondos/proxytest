/*
Copyright (c) 2000, The JAP-Team All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided
with the distribution.

- Neither the name of the University of Technology Dresden,
Germany nor the names of its contributors may be used to endorse
or promote products derived from this software without specific
prior written permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/
#include "StdAfx.h"
#ifdef PAYMENT

#include "CAAccountingInstance.hpp"
#include "CAAccountingControlChannel.hpp"
#include "CABase64.hpp"
#include "CAMsg.hpp"
#include "CAUtil.hpp"
#include "CASignature.hpp"
#include "CAThreadPool.hpp"
#include "CAXMLErrorMessage.hpp"
#include "Hashtable.hpp"
#include "packetintro.h"
#include "CALibProxytest.hpp"

//for testing purposes only
#define JAP_DIGEST_LENGTH 28

XERCES_CPP_NAMESPACE::DOMDocument* CAAccountingInstance::m_preparedCCRequest;

const SINT32 CAAccountingInstance::HANDLE_PACKET_CONNECTION_OK = 1;
const SINT32 CAAccountingInstance::HANDLE_PACKET_CONNECTION_UNCHECKED = 4;
const SINT32 CAAccountingInstance::HANDLE_PACKET_HOLD_CONNECTION = 0;
const SINT32 CAAccountingInstance::HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION = 2;
const SINT32 CAAccountingInstance::HANDLE_PACKET_CLOSE_CONNECTION = 3;

SINT32 CAAccountingInstance::m_prepaidBytesMinimum = 0;

volatile UINT64 CAAccountingInstance::m_iCurrentSettleTransactionNr = 0;

/**
 * Singleton: This is the reference to the only instance of this class
 */
CAAccountingInstance * CAAccountingInstance::ms_pInstance = NULL;

const UINT64 CAAccountingInstance::PACKETS_BEFORE_NEXT_CHECK = 100;

const UINT32 CAAccountingInstance::MAX_TOLERATED_MULTIPLE_LOGINS = 10;

const UINT32 CAAccountingInstance::MAX_SETTLED_CCS = 10;

/**
 * private Constructor
 */
CAAccountingInstance::CAAccountingInstance(CAFirstMix* callingMix)
	{
		CAMsg::printMsg( LOG_DEBUG, "AccountingInstance initialising\n" );
		m_seqBIConnErrors = 0;
		m_pMutex = new CAMutex();
		//m_pIPBlockList = new CATempIPBlockList(60000);
		m_pSettlementMutex = new CAConditionVariable();
		//m_loginHashTableChangeInProgress = false; //for synchronizing settlementTransactions of loginThreads and SettlementThread.
		m_nextSettleNr = 0; //The next thread's number to alter the login hash table
		m_settleWaitNr = 0; //The wait number when it is equal to nextSettle number it's the thread's turn has to to
							//alter the login hash table
		// initialize Database connection
		//m_dbInterface = new CAAccountingDBInterface();
		m_pPiInterface = new CAAccountingBIInterface();
		m_mix = callingMix;
		/*if(m_dbInterface->initDBConnection() != E_SUCCESS)
		{
			CAMsg::printMsg( LOG_ERR, "**************** AccountingInstance: Could not connect to DB!\n");
			exit(1);
		}*/

		// initialize JPI signature tester
		m_AiName = new UINT8[256];
		CALibProxytest::getOptions()->getAiID(m_AiName, 256);
		if (CALibProxytest::getOptions()->getBI() != NULL)
		{
			//m_pJpiVerifyingInstance = CALibProxytest::getOptions()->getBI()->getVerifier();
			m_pPiInterface->setPIServerConfiguration(CALibProxytest::getOptions()->getBI());
		}
		m_iHardLimitBytes = CALibProxytest::getOptions()->getPaymentHardLimit();
		m_iSoftLimitBytes = CALibProxytest::getOptions()->getPaymentSoftLimit();

		prepareCCRequest(callingMix, m_AiName);

		m_currentAccountsHashtable =
			new Hashtable((UINT32 (*)(void *))Hashtable::hashUINT64, (SINT32 (*)(void *,void *))Hashtable::compareUINT64, 2000);

		m_certHashCC =
			new Hashtable((UINT32 (*)(void *))Hashtable::stringHash, (SINT32 (*)(void *,void *))Hashtable::stringCompare);
		for (UINT32 i = 0; i < m_allHashesLen; i++)
		{
			m_certHashCC->put(m_allHashes[i], m_allHashes[i]);
		}

		// launch BI settleThread
		m_pSettleThread = new CAAccountingSettleThread(m_currentAccountsHashtable,
														m_currentCascade);

		m_aiThreadPool = new CAThreadPool(NUM_LOGIN_WORKER_TRHEADS, MAX_LOGIN_QUEUE, false);
	}


/**
 * private destructor
 */
CAAccountingInstance::~CAAccountingInstance()
	{
		INIT_STACK;
		BEGIN_STACK("~CAAccountingInstance");

		/*
		 * avoid calling this desctructor concurrently!
		 */
		m_pMutex->lock();

		CAMsg::printMsg( LOG_DEBUG, "AccountingInstance dying\n" );
		if (m_pSettleThread)
		{
			CAMsg::printMsg( LOG_DEBUG, "deleting m_pSettleThread\n" );
			//m_pSettleThread->settle();
			delete m_pSettleThread;
			m_pSettleThread = NULL;
		}

		if (m_aiThreadPool)
		{
			CAMsg::printMsg( LOG_DEBUG, "deleting m_aiThreadPool\n" );
			delete m_aiThreadPool;
			m_aiThreadPool = NULL;
		}

		/*if (m_dbInterface)
		{
			CAMsg::printMsg( LOG_DEBUG, "termintaing dbConnection\n" );
			m_dbInterface->terminateDBConnection();
			delete m_dbInterface;
		}
		m_dbInterface = NULL;*/


		delete m_pPiInterface;
		m_pPiInterface = NULL;

		delete m_pSettlementMutex;
		m_pSettlementMutex = NULL;
		//delete m_pIPBlockList;
		//m_pIPBlockList = NULL;


		delete[] m_AiName;
		m_AiName = NULL;

		if (m_currentAccountsHashtable)
		{
			m_currentAccountsHashtable->getMutex()->lock();
			CAMsg::printMsg( LOG_DEBUG, "CAAccountingInstance: Clearing accounts hashtable...\n");
			m_currentAccountsHashtable->clear(HASH_EMPTY_NONE, HASH_EMPTY_DELETE);
			CAMsg::printMsg( LOG_DEBUG, "CAAccountingInstance: Deleting accounts hashtable...\n" );
			m_currentAccountsHashtable->getMutex()->unlock();
			delete m_currentAccountsHashtable;
			m_currentAccountsHashtable = NULL;
		}
		m_currentAccountsHashtable = NULL;
		CAMsg::printMsg( LOG_DEBUG, "CAAccountingInstance: Accounts hashtable deleted.\n" );

		delete[] m_currentCascade;
		m_currentCascade = NULL;

		if(m_certHashCC != NULL)
		{
			for (UINT32 i = 0; i < m_allHashesLen; i++)
			{
				UINT8* certHash = (UINT8*)m_certHashCC->remove(m_allHashes[i]);
			}
			m_certHashCC->clear(HASH_EMPTY_NONE, HASH_EMPTY_DELETE);
			delete m_certHashCC;
			m_certHashCC = NULL;
		}

		if (m_allHashes)
		{
			for (UINT32 i = 0; i < m_allHashesLen; i++)
			{
				delete m_allHashes[i];
				m_allHashes[i] = NULL;
			}
			delete[] m_allHashes;
		}
		m_allHashes = NULL;

		m_pMutex->unlock();

		delete m_pMutex;
		m_pMutex = NULL;
		FINISH_STACK("~CAAccountingInstance");

		CAMsg::printMsg( LOG_DEBUG, "AccountingInstance dying finished.\n" );
	}

UINT32 CAAccountingInstance::getAuthFlags(fmHashTableEntry * pHashEntry)
{
	if (pHashEntry == NULL)
	{
		return 0;
	}

	tAiAccountingInfo* pAccInfo = pHashEntry->pAccountingInfo;

	if (pAccInfo == NULL)
	{
		return 0;
	}

	return pAccInfo->authFlags;
}

UINT32 CAAccountingInstance::getNrOfUsers()
{
	UINT32 users = 0;

	if (ms_pInstance != NULL)
	{
		ms_pInstance->m_pMutex->lock();
		if(ms_pInstance->m_currentAccountsHashtable != NULL)
		{
			// getting the size is an atomic operation and does not need synchronization
			users = ms_pInstance->m_currentAccountsHashtable->getSize();
		}
		else
		{
			CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: Trying to access Hashtable after it has been disposed!!.\n");
		}
		ms_pInstance->m_pMutex->unlock();
	}

	return users;
}


THREAD_RETURN CAAccountingInstance::processThread(void* a_param)
{
	INIT_STACK;
	BEGIN_STACK("CAAccountingInstance::processThread");

	aiQueueItem* item = (aiQueueItem*)a_param;
	bool bDelete = false;
	DOMElement *elem = item->pDomDoc->getDocumentElement();

	// call the handle function
	(ms_pInstance->*(item->handleFunc))(item->pAccInfo, elem);

	item->pAccInfo->mutex->lock();
	item->pAccInfo->nrInQueue--;
	if (item->pAccInfo->authFlags & AUTH_DELETE_ENTRY &&
		item->pAccInfo->nrInQueue == 0)
	{
		/*
		 * There is no more entry of this connection in the queue,
		 * and the connection is closed. We have to delete the entry.
		 */
		bDelete = true;
		CAMsg::printMsg(LOG_INFO, "CAAccountingInstance: Deleting account entry from AI thread.\n");
	}

	if (item->pAccInfo->nrInQueue < 0)
	{
		CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: AI thread found negative handle queue!\n");
	}
	item->pAccInfo->mutex->unlock();

	if (bDelete)
	{
		delete item->pAccInfo->mutex;
		item->pAccInfo->mutex = NULL;
		delete item->pAccInfo;
		item->pAccInfo = NULL;
	}

	delete item->pDomDoc;
	item->pDomDoc = NULL;
	delete item;
	item = NULL;

	FINISH_STACK("CAAccountingInstance::processThread");

	THREAD_RETURN_SUCCESS;
}

SINT32 CAAccountingInstance::handleJapPacket(fmHashTableEntry *pHashEntry, bool a_bControlMessage, bool a_bMessageToJAP)
{
	SINT32 ret = handleJapPacket_internal(pHashEntry, a_bControlMessage, a_bMessageToJAP);

	INIT_STACK;
	FINISH_STACK("CAAccountingInstance::handleJapPacket");

	return ret;
}

/**
 * Called by FirstMix for each incoming JAP packet.
 * Determines whether the packet should be let through or not
 *
 * Possible return values, and FirstMix's reaction:
 * @return 1: everything is OK, forward packet to next mix
 * @return 2: we need something (cert, CC,...) from the user, hold packet and start timeout
 * @return 3: fatal error, or timeout exceeded -> kick the user out
 *
 */
SINT32 CAAccountingInstance::handleJapPacket_internal(fmHashTableEntry *pHashEntry, bool a_bControlMessage, bool a_bMessageToJAP)
	{
		INIT_STACK;
		BEGIN_STACK("CAAccountingInstance::handleJapPacket");

		CAAccountingDBInterface *dbInterface = NULL;

		if (pHashEntry == NULL || pHashEntry->pAccountingInfo == NULL)
		{
			return HANDLE_PACKET_CLOSE_CONNECTION;
		}

		tAiAccountingInfo* pAccInfo = pHashEntry->pAccountingInfo;
		AccountLoginHashEntry* loginEntry = NULL;
		CAXMLErrorMessage* err = NULL;

		pAccInfo->mutex->lock();

		if (pAccInfo->authFlags & AUTH_DELETE_ENTRY)
		{
			pAccInfo->mutex->unlock();
			return HANDLE_PACKET_CLOSE_CONNECTION;
		}

		//Should never happen since control flow assert that this method cannot be invoked
		//when a login is not finished.
		if ( (pAccInfo->authFlags & AUTH_LOGIN_NOT_FINISHED) && !a_bMessageToJAP )
		{
			CAMsg::printMsg(LOG_ERR, "CAAccountingInstance:  User violates login protocol");
			pAccInfo->mutex->unlock();
			return HANDLE_PACKET_CLOSE_CONNECTION;
		}

		//still preparing to close connection -> no data is forwarded upstream and only control messages are
		//sent downstream if a user connection is in this state.
		if ( pAccInfo->authFlags & AUTH_FATAL_ERROR )
		{
			pAccInfo->mutex->unlock();
			return HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION;
		}

		if (a_bControlMessage)
		{
			pAccInfo->mutex->unlock();
			return HANDLE_PACKET_CONNECTION_UNCHECKED;
		}
		else
		{
			 // count the packet and continue checkings
			pAccInfo->transferredBytes += MIXPACKET_SIZE;
			pAccInfo->sessionPackets++;
#ifdef SDTFA
			IncrementShmPacketCount();
#endif
		}

		if (pAccInfo->authFlags & AUTH_MULTIPLE_LOGIN)
		{
			return returnPrepareKickout(pAccInfo, new CAXMLErrorMessage(CAXMLErrorMessage::ERR_MULTIPLE_LOGIN, (UINT8*)"Only one login per account is allowed!"));
		}

		// do the following tests after a lot of Mix packets only (gain speed...)
		if (!(pAccInfo->authFlags & (AUTH_TIMEOUT_STARTED | AUTH_HARD_LIMIT_REACHED | AUTH_ACCOUNT_EMPTY | AUTH_WAITING_FOR_FIRST_SETTLED_CC)) &&
			pAccInfo->sessionPackets % PACKETS_BEFORE_NEXT_CHECK != 0)
		{
			//CAMsg::printMsg( LOG_DEBUG, "Now we gain some speed after %d session packets..., auth-flags: %x\n", pAccInfo->sessionPackets, pAccInfo->authFlags);
			pAccInfo->mutex->unlock();
			return HANDLE_PACKET_CONNECTION_UNCHECKED;
		}


		SAVE_STACK("CAAccountingInstance::handleJapPacket", "before accounts hash");

		if (!ms_pInstance->m_currentAccountsHashtable)
		{
			// accounting instance is dying...
			return returnKickout(pAccInfo);
		}

		ms_pInstance->m_currentAccountsHashtable->getMutex()->lock();
		//Suppose, this section checks the flags set after settlement */
		loginEntry = (AccountLoginHashEntry*)ms_pInstance->m_currentAccountsHashtable->getValue(&(pAccInfo->accountNumber));
		if (loginEntry)
		{
			pAccInfo->authFlags &= ~loginEntry->authRemoveFlags;
			//CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Remove flag: %d\n", loginEntry->authRemoveFlags);


			/*if (loginEntry->ownerRef != pHashEntry)
			{
				// this is not the latest connection of this user; kick him out...
				pAccInfo->authFlags |= AUTH_MULTIPLE_LOGIN;
				ms_pInstance->m_currentAccountsHashtable->getMutex()->unlock();
				return returnPrepareKickout(pAccInfo, new CAXMLErrorMessage(CAXMLErrorMessage::ERR_MULTIPLE_LOGIN, (UINT8*)"Only one login per account is allowed!"));
			}
			else */
			if (loginEntry->authFlags & AUTH_OUTDATED_CC)
			{
				loginEntry->authFlags &= ~AUTH_OUTDATED_CC;

				UINT8 tmp[32];
				print64(tmp,pAccInfo->accountNumber);
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Fixing bytes from outdated CC for account %s...\n", tmp);
				// we had stored an outdated CC; insert confirmed bytes from current CC here and also update client
				CAXMLCostConfirmation * pCC = NULL;
				bool bSettled;

				dbInterface = CAAccountingDBInterface::getConnection();//new CAAccountingDBInterface();
				if(dbInterface != NULL)
				{
					dbInterface->getCostConfirmation(pAccInfo->accountNumber,
													 ms_pInstance->m_currentCascade,
													 &pCC,
													 bSettled);
					CAAccountingDBInterface::releaseConnection(dbInterface);
					dbInterface = NULL;
				}
				/*ms_pInstance->m_dbInterface->getCostConfirmation(pAccInfo->accountNumber,
					ms_pInstance->m_currentCascade, &pCC, bSettled);*/

				if (pCC != NULL)
				{
					if (bSettled)
					{
						pAccInfo->transferredBytes += loginEntry->confirmedBytes - pAccInfo->confirmedBytes;
						pAccInfo->confirmedBytes = loginEntry->confirmedBytes;
						loginEntry->confirmedBytes = 0;
						pAccInfo->authFlags |= AUTH_SENT_CC_REQUEST;
						pAccInfo->pControlChannel->sendXMLMessage(pCC->getXMLDocument());
					}
					else
					{
						CAMsg::printMsg(LOG_ERR, "CAAccountingInstance: While trying to fix bytes from outdated CC,"
							"another CC was received! Waiting for settlement... \n");
					}
					delete pCC;
					pCC = NULL;
				}
				else
				{
					CAMsg::printMsg(LOG_ERR, "CAAccountingInstance: Bytes from outdated CC could not be fixed!\n");
				}
			}
			else if (loginEntry->authFlags & AUTH_ACCOUNT_EMPTY)
			{
				if(!(pAccInfo->authFlags & AUTH_ACCOUNT_EMPTY))
				{
					// Do not reset the flag, so that the confirmedBytes are always reset.
					//loginEntry->authFlags &= ~AUTH_ACCOUNT_EMPTY;
					pAccInfo->authFlags |= AUTH_ACCOUNT_EMPTY;
					/* confirmedBytes = 0 leads to immediate disconnection.
					 * If confirmedBytes > 0,  any remaining prepaid bytes may be used.
					 */
					pAccInfo->confirmedBytes = loginEntry->confirmedBytes;
					if (pAccInfo->confirmedBytes < pAccInfo->transferredBytes)
					{
						// this account is really empty; prevent an overflow in the prepaid bytes calculation
						 pAccInfo->transferredBytes = pAccInfo->confirmedBytes;
					}

					//CAMsg::printMsg(LOG_ERR, "CAAccountingInstance: Account %llu is empty!\n", pAccInfo->accountNumber);
					CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Account %llu empty with %d prepaid bytes, (transferred bytes: %llu, confirmed bytes: %llu)!\n",
											pAccInfo->accountNumber, getPrepaidBytes(pAccInfo), pAccInfo->transferredBytes, pAccInfo->confirmedBytes);
				}

			}
			else if (loginEntry->authFlags & AUTH_INVALID_ACCOUNT)
			{
				loginEntry->authFlags &= ~AUTH_INVALID_ACCOUNT;
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Found invalid account %llu ! Kicking out user...\n", pAccInfo->accountNumber);
				err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_KEY_NOT_FOUND);
			}
			else if (loginEntry->authFlags & AUTH_BLOCKED)
			{
				loginEntry->authFlags &= ~AUTH_BLOCKED;
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Kicking out user with blocked account %llu !\n", pAccInfo->accountNumber);
				err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_BLOCKED);
			}
			else if (loginEntry->authFlags & AUTH_DATABASE)
			{
				loginEntry->authFlags &= ~AUTH_DATABASE;
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Kicking out user with account %llu due to database error...\n", pAccInfo->accountNumber);
				err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_DATABASE_ERROR);
			}
			else if (loginEntry->authFlags & AUTH_UNKNOWN)
			{
				loginEntry->authFlags &= ~AUTH_UNKNOWN;
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Unknown error, account %llu! Kicking out user...\n", pAccInfo->accountNumber);
				err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR);
			}
		}
		else
		{
			CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: handleJapPacket %s,%s did not find user login hash entry for account %llu, owner/accInfo: %p/%p!\n",
					(a_bMessageToJAP ? "downstream" : "upstream"),
					(a_bControlMessage ? "ctl" : "data"),
					pAccInfo->accountNumber, pAccInfo->ownerRef, pHashEntry->pAccountingInfo);
			ms_pInstance->m_currentAccountsHashtable->getMutex()->unlock();
			return returnKickout(pAccInfo);
		}

		ms_pInstance->m_currentAccountsHashtable->getMutex()->unlock();

		if (pAccInfo->authFlags & AUTH_ACCOUNT_EMPTY)
		{
			// There should be no time limit. The connections is simply closed after all prepaid bytes are gone.
			pAccInfo->lastHardLimitSeconds = time(NULL);

			//#ifdef DEBUG

			//#endif

			if (getPrepaidBytes(pAccInfo) <= 0)
			{
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Account %llu empty! Kicking out user...\n",
						pAccInfo->accountNumber);
				err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_ACCOUNT_EMPTY);
			}
			else
			{
				/**
				 * Do not make further checkings. Let the client use the
				 * remaining prepaid bytes, and then disconnect him afterwards.
				 * As the confirmedBytes are set to zero when the client connects and
				 * the account has been empty before, no other (unauthenticated)
				 * client may use these bytes.
				 */
				pAccInfo->mutex->unlock();
				return HANDLE_PACKET_CONNECTION_OK;
			}
		}

		/** @todo We need this trick so that the program does not freeze with active AI ThreadPool!!!! */
		//pAccInfo->mutex->lock();

		SAVE_STACK("CAAccountingInstance::handleJapPacket", "before err");

		if (err)
		{
			return returnPrepareKickout(pAccInfo, err);
		}

		//----------------------------------------------------------
		// ******     Hardlimit cost confirmation check **********
		//counting unconfirmed bytes is not necessary anymore, since we deduct from prepaid bytes
		//UINT32 unconfirmedBytes=diff64(pAccInfo->transferredBytes,pAccInfo->confirmedBytes);

		//confirmed and transferred bytes are cumulative, so they use UINT64 to store potentially huge values
		//prepaid Bytes as the difference will be much smaller, but might be negative, so we cast to signed int
		SINT32 prepaidBytes = getPrepaidBytes(pAccInfo);

		if (prepaidBytes < 0 ||  prepaidBytes <= (SINT32) ms_pInstance->m_iHardLimitBytes)
		{
			UINT32 prepaidInterval = CALibProxytest::getOptions()->getPrepaidInterval();

			if ((pAccInfo->authFlags & AUTH_HARD_LIMIT_REACHED) == 0)
			{
				pAccInfo->lastHardLimitSeconds = time(NULL);
				pAccInfo->authFlags |= AUTH_HARD_LIMIT_REACHED;
			}

#ifdef DEBUG
			CAMsg::printMsg(LOG_ERR, "CAAccountingInstance: Hard limit of %d bytes triggered in %d seconds \n",
							ms_pInstance->m_iHardLimitBytes,
							(pAccInfo->lastHardLimitSeconds + HARD_LIMIT_TIMEOUT - time(NULL)));
#endif

			if ( ( (time(NULL) >= pAccInfo->lastHardLimitSeconds + HARD_LIMIT_TIMEOUT) && (prepaidBytes <= 0) )
				   || (prepaidBytes < 0 && (UINT32)(prepaidBytes * (-1)) >= prepaidInterval) )
			{
//#ifdef DEBUG
				char* strReason;
				if (time(NULL) >= pAccInfo->lastHardLimitSeconds + HARD_LIMIT_TIMEOUT)
				{
					strReason = "timeout";
				}
				else
				{
					strReason = "negative prepaid interval exceeded";
				}
				CAMsg::printMsg( LOG_INFO, "Accounting instance: User refused "
								"to send cost confirmation (HARDLIMIT EXCEEDED, %s). "
								"PrepaidBytes were: %d\n", strReason, prepaidBytes);
//#endif

				//ms_pInstance->m_pIPBlockList->insertIP( pHashEntry->peerIP );
				pAccInfo->lastHardLimitSeconds = 0;
				return returnPrepareKickout(pAccInfo, new CAXMLErrorMessage(CAXMLErrorMessage::ERR_NO_CONFIRMATION));
			}
			else
			{
				if( !(pAccInfo->authFlags & AUTH_SENT_CC_REQUEST) )
				{
					sendCCRequest(pAccInfo);
				}
			}
		}
		else
		{
			pAccInfo->authFlags &= ~AUTH_HARD_LIMIT_REACHED;
		}

		//-------------------------------------------------------
		// *** SOFT LIMIT CHECK *** is it time to request a new cost confirmation?
		if ( prepaidBytes < 0 || prepaidBytes <= (SINT32) ms_pInstance->m_iSoftLimitBytes )
		{
#ifdef DEBUG
			CAMsg::printMsg(LOG_ERR, "soft limit of %d bytes triggered \n",ms_pInstance->m_iSoftLimitBytes);
#endif
			if( !(pAccInfo->authFlags & AUTH_SENT_CC_REQUEST) )
			{//we have sent a first CC request
				// no CC request sent yet --> send a first CC request
				sendCCRequest(pAccInfo);
			}
		}// end of soft limit exceeded

		//everything is fine! let the packet pass thru
		pAccInfo->mutex->unlock();
		return HANDLE_PACKET_CONNECTION_OK;
	}

/******************************************************************/
//methods to provide a unified point of exit for handleJapPacket
/******************************************************************/


SINT32 CAAccountingInstance::getPrepaidBytes(tAiAccountingInfo* pAccInfo)
{
	if (pAccInfo == NULL)
	{
		return 0;
	}

	//most unlikely that either transferred bytes or confirmed bytes
	//are > 0x8000000000000000

	SINT64 prepaidBytes = pAccInfo->confirmedBytes - pAccInfo->transferredBytes;
	//difference must be a value that fits into a signed 32 bit integer.
	if ((prepaidBytes > 0) && (prepaidBytes & 0x7FFFFFFF00000000LL))
	{
		CAMsg::printMsg(LOG_CRIT, "PrepaidBytes overflow: %lld\n", prepaidBytes);
		CAMsg::printMsg(LOG_INFO, "TransferredBytes: %llu  ConfirmedBytes: %llu\n", pAccInfo->transferredBytes, pAccInfo->confirmedBytes);
	}
	return (SINT32) prepaidBytes;

	/*SINT32 prepaidBytes;
#ifdef DEBUG
	CAMsg::printMsg(LOG_INFO, "Calculating TransferredBytes: %llu  ConfirmedBytes: %llu\n",
			pAccInfo->transferredBytes, pAccInfo->confirmedBytes);
#endif
	if (pAccInfo->confirmedBytes > pAccInfo->transferredBytes)
	{
		prepaidBytes = pAccInfo->confirmedBytes - pAccInfo->transferredBytes;
		if (prepaidBytes < 0)
		{
			// PrepaidBytes should be greater than 0 !!!
			UINT8 tmp[32], tmp2[32];
			print64(tmp,pAccInfo->transferredBytes);
			print64(tmp2,pAccInfo->confirmedBytes);

			CAMsg::printMsg(LOG_CRIT, "PrepaidBytes are way to high! Maybe a hacker attack? Or CC did get lost?\n");
			CAMsg::printMsg(LOG_INFO, "TransferredBytes: %s  ConfirmedBytes: %s\n", tmp, tmp2);
			UINT32 prepaidInterval = CALibProxytest::getOptions()->getPrepaidInterval();
			prepaidBytes = (SINT32)prepaidInterval;
			pAccInfo->transferredBytes = pAccInfo->confirmedBytes - prepaidInterval;
		}
	}
	else
	{
		prepaidBytes = pAccInfo->transferredBytes - pAccInfo->confirmedBytes;
		prepaidBytes *= -1;
	}

	return prepaidBytes;*/
}

/**
 *  When receiving this message, the Mix should kick the user out immediately
 */
SINT32 CAAccountingInstance::returnKickout(tAiAccountingInfo* pAccInfo)
{
	UINT8 tmp[32];
	print64(tmp,pAccInfo->accountNumber);
	CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: should kick out user with account %s now...\n", tmp);
	setPrepaidBytesToZero_internal(pAccInfo);
	pAccInfo->mutex->unlock();
	return HANDLE_PACKET_CLOSE_CONNECTION;
}

void CAAccountingInstance::setPrepaidBytesToZero(tAiAccountingInfo* pAccInfo)
{
	pAccInfo->mutex->lock();
	setPrepaidBytesToZero_internal(pAccInfo);
	pAccInfo->mutex->unlock();
}

/* prepaid bytes are calculated like that */
inline void CAAccountingInstance::setPrepaidBytesToZero_internal(tAiAccountingInfo* pAccInfo)
{
	pAccInfo->transferredBytes = pAccInfo->confirmedBytes;
}

/*
 * hold packet, no timeout started
 * (Usage: send an error message before kicking out the user:
 * sets AUTH_FATAL_ERROR )
 * IMPORTANT: You need to hold a lock for pAccInfo->mutex when invoking this.
 * Postcondition is that pAccInfo->mutex is unlocked.
 */
SINT32 CAAccountingInstance::returnPrepareKickout(tAiAccountingInfo* pAccInfo, CAXMLErrorMessage* a_error)
{
	pAccInfo->authFlags |= AUTH_FATAL_ERROR;

	if (a_error)
	{
		//CAMsg::printMsg(LOG_CRIT, "AccountingInstance: Sending error message...\n");
		XERCES_CPP_NAMESPACE::DOMDocument* doc=NULL;
		a_error->toXmlDocument(doc);
		delete a_error;
		a_error = NULL;
		//pAccInfo->sessionPackets = 0; // allow some pakets to pass by to send the control message
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: before prepare Kickout send...\n");
		pAccInfo->pControlChannel->sendXMLMessage(doc);
		if (doc != NULL)
		{
			doc->release();
			doc = NULL;
		}
	}
	else
	{
		CAMsg::printMsg(LOG_CRIT, "AccountingInstance: Should send error message, but none is available!\n");
	}

	pAccInfo->mutex->unlock();
	return HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION;
}

SINT32 CAAccountingInstance::sendInitialCCRequest(tAiAccountingInfo* pAccInfo, CAXMLCostConfirmation *pCC, SINT32 prepaidBytes)
{
	XERCES_CPP_NAMESPACE::DOMDocument* doc=NULL;

	SINT32 ret = makeInitialCCRequest(pCC, doc, prepaidBytes);
	if( (ret != E_SUCCESS) || (doc == NULL))
	{
		CAMsg::printMsg(LOG_ERR, "cannot send initial CC request, ret: %d\n", ret);
		return E_UNKNOWN;
	}
#ifdef DEBUG
	UINT32 debuglen = 3000;
	UINT8 debugout[3000];
	DOM_Output::dumpToMem(doc,debugout,&debuglen);
	debugout[debuglen] = 0;
	CAMsg::printMsg(LOG_DEBUG, "the CC sent looks like this: %s \n",debugout);
#endif
	ret = pAccInfo->pControlChannel->sendXMLMessage(doc);
	if (doc != NULL)
	{
		doc->release();
		doc = NULL;
	}
	return ret;
}

SINT32 CAAccountingInstance::sendCCRequest(tAiAccountingInfo* pAccInfo)
{
	INIT_STACK;
	BEGIN_STACK("CAAccountingInstance::sendCCRequest");

	XERCES_CPP_NAMESPACE::DOMDocument* doc=NULL;
    UINT32 prepaidInterval = CALibProxytest::getOptions()->getPrepaidInterval();

    pAccInfo->authFlags |= AUTH_SENT_CC_REQUEST;

    if (pAccInfo->authFlags & AUTH_ACCOUNT_EMPTY)
    {
    	// do not send further CC requests for this account
    	return E_SUCCESS;
    }

    // prepaid bytes are "confirmed bytes - transfered bytes"
    //UINT64 bytesToConfirm = pAccInfo->confirmedBytes + (prepaidInterval) - (pAccInfo->confirmedBytes - pAccInfo->transferredBytes);
    pAccInfo->bytesToConfirm = (prepaidInterval) + pAccInfo->transferredBytes;
	makeCCRequest(pAccInfo->accountNumber, pAccInfo->bytesToConfirm, doc);
	//pAccInfo->authFlags |= AUTH_SENT_CC_REQUEST;
#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG, "CC request sent for %llu bytes, transferrred bytes: %llu bytes.\n",pAccInfo->bytesToConfirm, pAccInfo->transferredBytes);
	CAMsg::printMsg(LOG_DEBUG, "prepaid Interval: %u \n",prepaidInterval);

	UINT32 debuglen = 3000;
	UINT8 debugout[3000];
	DOM_Output::dumpToMem(doc,debugout,&debuglen);
	debugout[debuglen] = 0;
	CAMsg::printMsg(LOG_DEBUG, "the CC sent looks like this: %s \n",debugout);
#endif

	//FINISH_STACK("CAAccountingInstance::sendCCRequest");

	SINT32 ret = pAccInfo->pControlChannel->sendXMLMessage(doc);
	if (doc != NULL)
	{
		doc->release();
		doc = NULL;
	}
	return ret;
}


bool CAAccountingInstance::cascadeMatchesCC(CAXMLCostConfirmation *pCC)
{

		UINT8* certHash;
		if(m_allHashesLen !=  pCC->getNumberOfHashes() )
		{
			return false;
		}

		for (UINT32 i = 0; i < pCC->getNumberOfHashes(); i++)
		{
			certHash = pCC->getPriceCertHash(i);
			if ((certHash = (UINT8*)m_certHashCC->getValue(certHash)) != NULL)
			{
#ifdef DEBUG
				CAMsg::printMsg( LOG_INFO, "CC1: %s\n", certHash);
#endif
			}
			else
			{
#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "CC do not match current cascade.\n");
#endif
				return false;
			}
		}
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "CC matches current Cascade.\n");
#endif
		return true;
}

/**
 * creating the xml of a new CC is really the responsability of the CAXMLCostConfirmation class
 * knowledge about the structure of a CC's XML should be encapsulated in it
 * TODO: add constructor to that class that takes accountnumber, transferredbytes etc as params
 *       (should use a template internally into which it only inserts accNumber and bytes,to speed things up
 * TODO: add toXMLElement method
 * then replace manually building the xml here with contructing a CAXMLCostConfirmation
 * and just add the xml returned by its toXMLElement method
 * @param callingMix: the Mix instance to which the AI belongs
 * (needed to get cascadeInfo to extract the price certificates to include in cost confirmations)
 */
SINT32 CAAccountingInstance::prepareCCRequest(CAMix* callingMix, UINT8* a_AiName)
{
	m_preparedCCRequest = createDOMDocument();

	DOMElement* elemRoot = createDOMElement(m_preparedCCRequest,"PayRequest");
	setDOMElementAttribute(elemRoot,"version",(UINT8*) "1.0");
	m_preparedCCRequest->appendChild(elemRoot);
	DOMElement* elemCC = createDOMElement(m_preparedCCRequest,"CC");
	setDOMElementAttribute(elemCC,"version",(UINT8*) "1.2");
	elemRoot->appendChild(elemCC);
	DOMElement* elemAiName = createDOMElement(m_preparedCCRequest,"AiID");
	setDOMElementValue(elemAiName, a_AiName);
	elemCC->appendChild(elemAiName);

	//extract price certificate elements from cascadeInfo
	//get cascadeInfo from CAMix(which makeCCRequest needs to extract the price certs
	XERCES_CPP_NAMESPACE::DOMDocument* cascadeInfoDoc=NULL;
	callingMix->getMixCascadeInfo(cascadeInfoDoc);

	DOMElement* cascadeInfoElem = cascadeInfoDoc->getDocumentElement();
	DOMNodeList* allMixes = getElementsByTagName(cascadeInfoElem,"Mix");
	UINT32 nrOfMixes = allMixes->getLength();
	DOMNode** mixNodes = new DOMNode*[nrOfMixes]; //so we can use separate loops for extracting, hashing and appending

	DOMNode* curMixNode=NULL;
	for (UINT32 i = 0, j = 0, count = nrOfMixes; i < count; i++, j++){
		//cant use getDOMChildByName from CAUtil here yet, since it will always return the first child
		curMixNode = allMixes->item(i);
		if (getDOMChildByName(curMixNode,"PriceCertificate",mixNodes[j],true) != E_SUCCESS)
		{
			j--;
			nrOfMixes--;
		}
	}

	//hash'em, and get subjectkeyidentifiers
		UINT8 digest[SHA_DIGEST_LENGTH];
		m_allHashes=new UINT8*[nrOfMixes];
		m_allHashesLen = nrOfMixes;
		UINT8** allSkis=new UINT8*[nrOfMixes];
		DOMNode* skiNode=NULL;
		for (UINT32 i = 0; i < nrOfMixes; i++){
			UINT8* out=new UINT8[5000];
			UINT32 outlen=5000;

			DOM_Output::makeCanonical(mixNodes[i],out,&outlen);
			out[outlen] = 0;
#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "price cert to be hashed: %s",out);
#endif
			SHA1(out,outlen,digest);
			delete[] out;
			out = NULL;

			UINT32 len = 1024;
			UINT8* tmpBuff = new UINT8[len+1];
			memset(tmpBuff, 0, len+1);
			if(CABase64::encode(digest,SHA_DIGEST_LENGTH,tmpBuff,&len)!=E_SUCCESS)
				return E_UNKNOWN;
			//tmpBuff[len]=0;


			//line breaks might have been added, and would lead to database problems
			strtrim(tmpBuff); //return value ohny significant for NULL or all-whitespace string, ignore

			m_allHashes[i] = tmpBuff;
			CAMsg::printMsg(LOG_CRIT,"hash %u: %s\n", i, m_allHashes[i]);
			//do not delete tmpBuff here, since we're using allHashes below

			if (getDOMChildByName(mixNodes[i],"SubjectKeyIdentifier",skiNode,true) != E_SUCCESS)
				{
					CAMsg::printMsg(LOG_CRIT,"Could not get mix id from price cert");
				}

			allSkis[i] =  (UINT8*) XMLString::transcode(skiNode->getFirstChild()->getNodeValue());

	}
	//concatenate the hashes, and store for future reference to identify the cascade
    m_currentCascade = new UINT8[256];
    memset(m_currentCascade, 0, (sizeof(UINT8)*256 ));
    for (UINT32 j = 0; j < nrOfMixes; j++)
    {
        //check for hash value size (should always be OK)
        if (strlen((const char*)m_currentCascade) > ( 256 - strlen((const char*)m_allHashes[j]) )   )
        {
        	CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance::prepareCCRequest: "
        			"Too many/too long hash values, ran out of allocated memory\n");
        	return E_UNKNOWN;
        }
        if (j == 0)
        {
            m_currentCascade = (UINT8*) strcpy( (char*) m_currentCascade,(const char*)m_allHashes[j]);
        } else
        {
            m_currentCascade = (UINT8*) strcat((char*)m_currentCascade,(char*)m_allHashes[j]);
        }
    }

    //and append to CC
	DOMElement* elemPriceCerts = createDOMElement(m_preparedCCRequest,"PriceCertificates");
	DOMElement* elemCert=NULL;
	for (UINT32 i = 0; i < nrOfMixes; i++)
	{
		elemCert = createDOMElement(m_preparedCCRequest,"PriceCertHash");
		//CAMsg::printMsg(LOG_DEBUG,"hash to be inserted in cc: index %d, value %s\n",i,m_allHashes[i]);
		setDOMElementValue(elemCert,m_allHashes[i]);
		//delete[] allHashes[i];
		setDOMElementAttribute(elemCert,"id",allSkis[i]);
		setDOMElementAttribute(elemCert, "position", i);
		if (i == 0)
		{
			setDOMElementAttribute(elemCert,"isAI",(UINT8*)"true");
		}
		elemPriceCerts->appendChild(elemCert);
	}
	elemCC->appendChild(elemPriceCerts);
#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG, "finished method makeCCRequest\n");
#endif

	delete[] mixNodes;
	mixNodes = NULL;
	//delete[] allHashes;
	delete[] allSkis;
	allSkis = NULL;
	return E_SUCCESS;

}

/**
 * new initialCCRequest containing the last CC and the prepaid bytes
 * (this is a replacement for sending the prepaid with the challenge
 * which is now deprecated).
 */
SINT32 CAAccountingInstance::makeInitialCCRequest(CAXMLCostConfirmation *pCC, XERCES_CPP_NAMESPACE::DOMDocument* & doc, SINT32 prepaidBytes)
	{
		if( (pCC == NULL) || (pCC->getXMLDocument() == NULL) ||
			(pCC->getXMLDocument()->getDocumentElement() == NULL) )
		{
			CAMsg::printMsg(LOG_ERR, "Error creating initial CCrequest (pCC ref: %p)\n", pCC);
			return E_UNKNOWN;
		}
		DOMNode* elemCC=NULL;

		doc = createDOMDocument();
		DOMNode *ccRoot = doc->importNode(pCC->getXMLDocument()->getDocumentElement(),true);
		doc->appendChild(doc->importNode(m_preparedCCRequest->getDocumentElement(),true));
		setDOMElementAttribute(doc->getDocumentElement(), "initialCC", true);
		DOMElement *elemPrepaidBytes = createDOMElement(doc, "PrepaidBytes");
		setDOMElementValue(elemPrepaidBytes, prepaidBytes);
		getDOMChildByName(doc->getDocumentElement(),"CC",elemCC);
		if(elemCC == NULL)
		{
			return E_UNKNOWN;
		}
		doc->getDocumentElement()->replaceChild(ccRoot, elemCC);
		doc->getDocumentElement()->appendChild(elemPrepaidBytes);
		return E_SUCCESS;
	}

SINT32 CAAccountingInstance::makeCCRequest(const UINT64 accountNumber, const UINT64 transferredBytes, XERCES_CPP_NAMESPACE::DOMDocument* & doc)
	{
		INIT_STACK;
		BEGIN_STACK("CAAccountingInstance::makeCCRequest");

		DOMNode* elemCC=NULL;

		doc = createDOMDocument();
		doc->appendChild(doc->importNode(m_preparedCCRequest->getDocumentElement(),true));

		getDOMChildByName(doc->getDocumentElement(),"CC",elemCC);

		DOMElement* elemAccount = createDOMElement(doc,"AccountNumber");
		setDOMElementValue(elemAccount, accountNumber);
		elemCC->appendChild(elemAccount);
		DOMElement* elemBytes = createDOMElement(doc,"TransferredBytes");
		setDOMElementValue(elemBytes, transferredBytes);
		elemCC->appendChild(elemBytes);

		FINISH_STACK("CAAccountingInstance::makeCCRequest");

		return E_SUCCESS;
	}


SINT32 CAAccountingInstance::sendAILoginConfirmation(tAiAccountingInfo* pAccInfo,
													 const UINT32 code,
													 UINT8 * message)
	{
		SINT32 sendSuccess = E_SUCCESS;
		XERCES_CPP_NAMESPACE::DOMDocument* doc = createDOMDocument();
		DOMElement *elemRoot = createDOMElement(doc, "LoginConfirmation");
		setDOMElementAttribute(elemRoot, "code", code);
		setDOMElementValue(elemRoot, message);
		doc->appendChild(elemRoot);

#ifdef DEBUG
		UINT32 debuglen = 3000;
		UINT8 debugout[3000];
		DOM_Output::dumpToMem(doc,debugout,&debuglen);
		debugout[debuglen] = 0;
		CAMsg::printMsg(LOG_DEBUG, "the AILogin Confirmation sent looks like this: %s \n",debugout);
#endif
		sendSuccess = pAccInfo->pControlChannel->sendXMLMessage(doc);
		if (doc != NULL)
		{
			doc->release();
			doc = NULL;
		}
		return sendSuccess;
	}


/**
 * Handle a user (xml) message sent to us by the Jap.
 *
 * This function is running inside the AiThread. It determines
 * what type of message we have and calls the appropriate handle...()
 * function
 */
SINT32 CAAccountingInstance::processJapMessage(fmHashTableEntry * pHashEntry,const XERCES_CPP_NAMESPACE::DOMDocument* a_DomDoc)
	{
		INIT_STACK;
		BEGIN_STACK("CAAccountingInstance::processJapMessage");

		if (pHashEntry == NULL)
		{
			return E_UNKNOWN;
		}

		DOMElement* root = a_DomDoc->getDocumentElement();
		if(root == NULL)
		{
			CAMsg::printMsg(LOG_DEBUG, "ProcessJapMessage: getDocument Element is null!!!\n" );
			return E_UNKNOWN;
		}
		char* docElementName = XMLString::transcode(root->getTagName());
		SINT32 hf_ret = 0;


		// what type of message is it?
		if ( strcmp( docElementName, "AccountCertificate" ) == 0 )
			{
				#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Received an AccountCertificate. Calling handleAccountCertificate()\n" );
				#endif
				//handleFunc = &CAAccountingInstance::handleAccountCertificate;
				hf_ret = ms_pInstance->handleAccountCertificate( pHashEntry->pAccountingInfo, root );
				processJapMessageLoginHelper(pHashEntry, hf_ret, false);
			}
		else if ( strcmp( docElementName, "Response" ) == 0)
			{
				#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Received a Response (challenge-response)\n" );
				#endif
				//handleFunc = &CAAccountingInstance::handleChallengeResponse;
				hf_ret = ms_pInstance->handleChallengeResponse( pHashEntry->pAccountingInfo, root );
				processJapMessageLoginHelper(pHashEntry, hf_ret, false);
				/*if(hf_ret != CAXMLErrorMessage::ERR_OK)
				{
					unlockLogin(pHashEntry);
				}
				else*/
				if(hf_ret == (SINT32) CAXMLErrorMessage::ERR_OK)
				{
					//CAMsg::printMsg( LOG_DEBUG, "Prepaid bytes are: %d\n", getPrepaidBytes(pHashEntry->pAccountingInfo));

					if( (getPrepaidBytes(pHashEntry->pAccountingInfo) > 0) &&
						!(pHashEntry->pAccountingInfo->authFlags &
							(AUTH_ACCOUNT_EMPTY | AUTH_BLOCKED | AUTH_INVALID_ACCOUNT)) )
					{
						pHashEntry->pAccountingInfo->authFlags |= AUTH_LOGIN_SKIP_SETTLEMENT;
					}
				}
			}
		else if ( strcmp( docElementName, "CC" ) == 0 )
			{
				#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Received a CC. Calling handleCostConfirmation()\n" );
				#endif
				//handleFunc = &CAAccountingInstance::handleCostConfirmation;
				hf_ret = ms_pInstance->handleCostConfirmation( pHashEntry->pAccountingInfo, root );
				processJapMessageLoginHelper(pHashEntry, hf_ret, true);
				/*if(hf_ret != CAXMLErrorMessage::ERR_OK)
				{
					unlockLogin(pHashEntry);
				}*/
			}
		else
		{
			CAMsg::printMsg( LOG_ERR,
					"AI Received XML message with unknown root element \"%s\". This is not accepted!\n",
											docElementName
										);

			SAVE_STACK("CAAccountingInstance::processJapMessage", "error");
			XMLString::release(&docElementName);
			return E_UNKNOWN;
		}


		XMLString::release(&docElementName);

		/** @todo this does not work yet due to errors in CAMutex!!!
		if (handleFunc)
		{
			pItem = new aiQueueItem;
			pItem->pDomDoc = new DOM_Document(a_DomDoc);
			pItem->pAccInfo = pHashEntry->pAccountingInfo;
			pItem->handleFunc = handleFunc;

			pHashEntry->pAccountingInfo->mutex->lock();
			pItem->pAccInfo->nrInQueue++;
			ret = ms_pInstance->m_aiThreadPool->addRequest(processThread, pItem);
			if (ret !=E_SUCCESS)
			{
				pItem->pAccInfo->nrInQueue--;
				CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: Process could not add to AI thread pool!\n" );
				delete pItem->pDomDoc;
				delete pItem;
			}
			pHashEntry->pAccountingInfo->mutex->unlock();
			return ret;
		}*/

		// remove these lines if AI thread pool is used (see @todo above)
		//(ms_pInstance->*handleFunc)(pHashEntry->pAccountingInfo, root );

		FINISH_STACK("CAAccountingInstance::processJapMessage");
		return E_SUCCESS;
	}

void CAAccountingInstance::processJapMessageLoginHelper(fmHashTableEntry *pHashEntry,
														  UINT32 handlerReturnValue,
														  bool lastLoginMessage)
{
	if(pHashEntry->pAccountingInfo != NULL)
	{
		if(pHashEntry->pAccountingInfo->mutex == NULL)
		{
			return;
		}
		pHashEntry->pAccountingInfo->mutex->lock();
		if(pHashEntry->pAccountingInfo->authFlags & AUTH_LOGIN_NOT_FINISHED)
		{
			if(handlerReturnValue != CAXMLErrorMessage::ERR_OK)
			{
				pHashEntry->pAccountingInfo->authFlags &= ~AUTH_LOGIN_NOT_FINISHED;
				pHashEntry->pAccountingInfo->authFlags |= AUTH_LOGIN_FAILED;

				CAXMLErrorMessage *err = NULL;
				XERCES_CPP_NAMESPACE::DOMDocument *errDoc = NULL;

				/*if(pHashEntry->pAccountingInfo->authFlags & AUTH_BLOCKED )
				{
					err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_BLOCKED,
												(UINT8 *) "AI login: access denied because your account is blocked");
				}
				else if(pHashEntry->pAccountingInfo->authFlags & AUTH_ACCOUNT_EMPTY )
				{
					err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_ACCOUNT_EMPTY,
												(UINT8 *) "AI login: access denied because your account is empty");
				}
				else if(pHashEntry->pAccountingInfo->authFlags & AUTH_INVALID_ACCOUNT )
				{
					err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_NO_BALANCE,
												(UINT8 *) "AI login: access denied because your account is not valid");
				}
				else */ if(pHashEntry->pAccountingInfo->authFlags & AUTH_FAKE )
				{
					err = new CAXMLErrorMessage(handlerReturnValue);

				}
				else if(pHashEntry->pAccountingInfo->authFlags & AUTH_MULTIPLE_LOGIN )
				{
					err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_MULTIPLE_LOGIN,
												(UINT8*)"You are already logged in.");

				}
				else
				{
					err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR,
												(UINT8 *) "AI login: error occured while connecting, access denied");
				}


				if(err != NULL)
				{
					err->toXmlDocument(errDoc);
					pHashEntry->pAccountingInfo->pControlChannel->sendXMLMessage(errDoc);
					delete err;
					err = NULL;
				}
				if(errDoc != NULL)
				{
					errDoc->release();
					errDoc = NULL;
				}
				/*sendAILoginConfirmation(pHashEntry->pAccountingInfo,
										CAXMLErrorMessage::ERR_BLOCKED,
										(UINT8*) "AI access denied");*/
			}
			else if(lastLoginMessage)
			{
				//CAMsg::printMsg( LOG_ERR, "User successfully logged in\n");
				pHashEntry->pAccountingInfo->authFlags &= ~AUTH_LOGIN_NOT_FINISHED;
			}

		}
		pHashEntry->pAccountingInfo->mutex->unlock();
	}
}

SINT32 CAAccountingInstance::loginProcessStatus(fmHashTableEntry *pHashEntry)
{
	SINT32 ret = 0;
	if(pHashEntry == NULL)
	{
		return ret |= AUTH_LOGIN_FAILED;
	}
	if(pHashEntry->pAccountingInfo == NULL)
	{
		return ret |= AUTH_LOGIN_FAILED;
	}
	if(pHashEntry->pAccountingInfo->mutex == NULL)
	{
		return ret |= AUTH_LOGIN_FAILED;
	}
	pHashEntry->pAccountingInfo->mutex->lock();
	ret = pHashEntry->pAccountingInfo->authFlags &
		(AUTH_LOGIN_NOT_FINISHED | AUTH_LOGIN_FAILED | AUTH_LOGIN_SKIP_SETTLEMENT | AUTH_MULTIPLE_LOGIN);
	pHashEntry->pAccountingInfo->mutex->unlock();
	return ret;
}

/**
 * this method is for the corresponding CAFirstMix login thread to verify the
 * result of the settlement.
 */
SINT32 CAAccountingInstance::finishLoginProcess(fmHashTableEntry *pHashEntry)
{
	SINT32 ret = 0;
	UINT64 accountNumber = 0;
	AccountLoginHashEntry *loginEntry;
	tAiAccountingInfo* pAccInfo = pHashEntry->pAccountingInfo;

	if(ms_pInstance==NULL)
	{
		return ret |= AUTH_LOGIN_FAILED;
	}
	if(ms_pInstance->m_currentAccountsHashtable == NULL)
	{
		return ret |= AUTH_LOGIN_FAILED;
	}
	if(ms_pInstance->m_currentAccountsHashtable->getMutex() == NULL)
	{
		return ret |= AUTH_LOGIN_FAILED;
	}
	if(pHashEntry == NULL)
	{
		return ret |= AUTH_LOGIN_FAILED;
	}
	if(pHashEntry->pAccountingInfo == NULL)
	{
		return ret |= AUTH_LOGIN_FAILED;
	}
	if(pHashEntry->pAccountingInfo->mutex == NULL)
	{
		return ret |= AUTH_LOGIN_FAILED;
	}
	pAccInfo->mutex->lock();
	accountNumber = pAccInfo->accountNumber;

	//reset login flags
	pAccInfo->authFlags &= (~AUTH_LOGIN_SKIP_SETTLEMENT & ~AUTH_LOGIN_NOT_FINISHED);

	if (!(pAccInfo->authFlags & AUTH_SETTLED_ONCE))
	{
		UINT32 statusCode = 0;
		CAAccountingDBInterface* dbInterface = CAAccountingDBInterface::getConnection();
		if (dbInterface != NULL && dbInterface->getAccountStatus(accountNumber, statusCode) == E_SUCCESS)
		{
			if (statusCode > 0)
			{
				CAMsg::printMsg(LOG_WARNING, "CAAccountingInstance::finishLoginProcess: No CC was settled for account %llu. Using DB status %u.\n",
					accountNumber, statusCode);
			}
			pAccInfo->authFlags |= statusCode;
		}
		else
		{
			CAMsg::printMsg(LOG_WARNING, "CAAccountingInstance::finishLoginProcess: No CC was settled for account %llu. Could not fetch status from DB!.\n");
		}
		CAAccountingDBInterface::releaseConnection(dbInterface);
		dbInterface = NULL;
	}

	ms_pInstance->m_currentAccountsHashtable->getMutex()->lock();
	loginEntry = (AccountLoginHashEntry*)ms_pInstance->m_currentAccountsHashtable->getValue(&accountNumber);

	if (loginEntry)
	{
		if(loginEntry->authRemoveFlags)
		pAccInfo->authFlags &= ~(loginEntry->authRemoveFlags);
		pAccInfo->authFlags |= (loginEntry->authFlags & CRITICAL_SETTLE_FLAGS);
	}
	else
	{
		pAccInfo->authFlags |= AUTH_INVALID_ACCOUNT;
	}
	/*ret = pHashEntry->pAccountingInfo->authFlags &
		(AUTH_LOGIN_NOT_FINISHED | AUTH_LOGIN_FAILED);*/


	CAXMLErrorMessage *err = NULL;
	XERCES_CPP_NAMESPACE::DOMDocument *errDoc = NULL;
	/* Instead of using special login confirmation messages
	 * we rather send XMLErrorMessages for backward compatibility reasons
	 * because old JAPs (version <= 00.09.021) can handle them
	 */
	if(pAccInfo->authFlags & AUTH_BLOCKED )
	{
		err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_BLOCKED,
									(UINT8 *) "AI login: access denied because your account is blocked");
		CAMsg::printMsg(LOG_WARNING, "CAAccountingInstance::finishLoginProcess: Account %llu seems blocked.\n", accountNumber);
	}
	else if(pAccInfo->authFlags & AUTH_ACCOUNT_EMPTY )
	{
		err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_ACCOUNT_EMPTY,
									(UINT8 *) "AI login: access denied because your account is empty");
		if ((pAccInfo->authFlags & AUTH_SETTLED_ONCE))
		{
			pAccInfo->confirmedBytes = loginEntry->confirmedBytes;
			if (pAccInfo->confirmedBytes < pAccInfo->transferredBytes)
			{
				// this account is really empty; prevent an overflow in the prepaid bytes calculation
			 	pAccInfo->transferredBytes = pAccInfo->confirmedBytes;
			}
			if (getPrepaidBytes(pAccInfo) > 0)
			{
				// the user may user his last bytes and is kicked out after that
				delete err;
				err = NULL;
			}
		}
		else
		{
			// TODO I am not sure whether this is enough; check this later
			loginEntry->confirmedBytes = pAccInfo->confirmedBytes;
			pAccInfo->transferredBytes = pAccInfo->confirmedBytes;
		}
		CAMsg::printMsg(LOG_WARNING, "CAAccountingInstance::finishLoginProcess: Account %llu seems empty.\n", accountNumber);
	}
	else if(pAccInfo->authFlags & AUTH_INVALID_ACCOUNT )
	{
		err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_NO_BALANCE,
									(UINT8 *) "AI login: access denied because your account is not valid");
		CAMsg::printMsg(LOG_WARNING, "CAAccountingInstance::finishLoginProcess: Account %llu seems invalid.\n", accountNumber);
	}
	else if(pAccInfo->authFlags & AUTH_UNKNOWN )
	{
		err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_NO_ERROR_GIVEN,
									(UINT8 *) "AI login: error occured while connecting, access denied");
		CAMsg::printMsg(LOG_WARNING, "CAAccountingInstance::finishLoginProcess: Unknown error was found for account %llu.\n", accountNumber);
	}

	if(err != NULL)
	{
		err->toXmlDocument(errDoc);
		pHashEntry->pAccountingInfo->pControlChannel->sendXMLMessage(errDoc);
		delete err;
		err = NULL;
		if(errDoc != NULL)
		{
			errDoc->release();
			errDoc = NULL;
		}
		ret |= AUTH_LOGIN_FAILED;
	}
	else
	{

	//send login confirmation to user, but if the message could not be send, login will fail
	/* These login confirmation messages are necessary for the new AI login protocol to indicate
	 * that the process is finished after a settlement.
	 * They won't bother old JAPs (version <= 00.09.021) because they will ignore
	 * these confirmations.
	 */
		if(sendAILoginConfirmation(pAccInfo,
									CAXMLErrorMessage::ERR_OK,
									(UINT8*) "AI login successful") != E_SUCCESS)
		{
			ret |= AUTH_LOGIN_FAILED;
		}
	}

	/* unlock the loginEntry object for other login threads */
	//resetLoginOngoing(loginEntry, pHashEntry);
	ms_pInstance->m_currentAccountsHashtable->getMutex()->unlock();
	pAccInfo->mutex->unlock();
	return ret;
}

UINT32 CAAccountingInstance::handleAccountCertificate(tAiAccountingInfo* pAccInfo, DOMElement* root)
{
	return handleAccountCertificate_internal(pAccInfo, root);

	INIT_STACK;
	FINISH_STACK("CAAccountingInstance::handleAccountCertificate");
}


/**
 * Handles an account certificate of a newly connected Jap.
 * Parses accountnumber and publickey, checks the signature
 * and generates and sends a challenge XML structure to the
 * Jap.
 * TODO: think about switching account without changing mixcascade
 *   (receive a new acc.cert. though we already have one)
 */
UINT32 CAAccountingInstance::handleAccountCertificate_internal(tAiAccountingInfo* pAccInfo, DOMElement* root)
	{
		INIT_STACK;
		BEGIN_STACK("CAAccountingInstance::handleAccountCertificate");

		//CAMsg::printMsg( LOG_ERR, "CAAccountingInstance::handleAccountCertificate start\n");

		//CAMsg::printMsg(LOG_DEBUG, "started method handleAccountCertificate\n");
		DOMElement* elGeneral=NULL;
		timespec now;
		getcurrentTime(now);

		// check authstate of this user
		if (pAccInfo == NULL)
		{
			return CAXMLErrorMessage::ERR_NO_RECORD_FOUND;
		}

		pAccInfo->mutex->lock();

		if (pAccInfo->authFlags & AUTH_DELETE_ENTRY)
		{
			pAccInfo->mutex->unlock();
			return CAXMLErrorMessage::ERR_NO_ERROR_GIVEN;
		}

		if (pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT)
		{
			#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "Already got an account cert. Ignoring...");
			#endif
			CAXMLErrorMessage err(
					CAXMLErrorMessage::ERR_BAD_REQUEST,
					(UINT8*)"You have already sent an Account Certificate"
				);
			XERCES_CPP_NAMESPACE::DOMDocument* errDoc=NULL;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
			if (errDoc != NULL)
			{
				errDoc->release();
				errDoc = NULL;
			}
			pAccInfo->mutex->unlock();
			return CAXMLErrorMessage::ERR_BAD_REQUEST;
		}

		// parse & set accountnumber
		if (getDOMChildByName( root, "AccountNumber", elGeneral, false ) != E_SUCCESS ||
			getDOMElementValue( elGeneral, pAccInfo->accountNumber ) != E_SUCCESS)
		{
			CAMsg::printMsg( LOG_ERR, "AccountCertificate has wrong or no accountnumber. Ignoring...\n");
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_FORMAT);
			XERCES_CPP_NAMESPACE::DOMDocument* errDoc=NULL;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
			if (errDoc != NULL)
			{
				errDoc->release();
				errDoc = NULL;
			}
			pAccInfo->mutex->unlock();
			return CAXMLErrorMessage::ERR_WRONG_FORMAT;
		}

		// parse & set payment instance id
		UINT32 len=256;
		pAccInfo->pstrBIID=new UINT8[256];
		if ( getDOMChildByName( root,"BiID", elGeneral, false ) != E_SUCCESS ||
			 getDOMElementValue( elGeneral,pAccInfo->pstrBIID, &len ) != E_SUCCESS)
			{
				delete[] pAccInfo->pstrBIID;
				pAccInfo->pstrBIID = NULL;
				CAMsg::printMsg( LOG_ERR, "AccountCertificate has no Payment Instance ID. Ignoring...\n");
				CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_FORMAT);
				XERCES_CPP_NAMESPACE::DOMDocument* errDoc=NULL;
				err.toXmlDocument(errDoc);
				pAccInfo->pControlChannel->sendXMLMessage(errDoc);
				if (errDoc != NULL)
				{
					errDoc->release();
					errDoc = NULL;
				}
				pAccInfo->mutex->unlock();
				return CAXMLErrorMessage::ERR_WRONG_FORMAT;
			}
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "Stored payment instance ID: %s\n", pAccInfo->pstrBIID);
		#endif


	// parse & set public key
		if ( getDOMChildByName( root, "JapPublicKey", elGeneral, false ) != E_SUCCESS )
		{
			CAMsg::printMsg( LOG_ERR, "AccountCertificate contains no public key. Ignoring...\n");
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_KEY_NOT_FOUND);
			XERCES_CPP_NAMESPACE::DOMDocument* errDoc=NULL;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
			if (errDoc != NULL)
			{
				errDoc->release();
				errDoc = NULL;
			}
			pAccInfo->mutex->unlock();
			return CAXMLErrorMessage::ERR_KEY_NOT_FOUND;
		}
	#ifdef DEBUG
		UINT8* aij;
		UINT32 aijsize;
		aij = DOM_Output::dumpToMem(elGeneral, &aijsize);
		aij[aijsize-1]=0;
		CAMsg::printMsg( LOG_DEBUG, "Setting user public key %s>\n", aij );
		delete[] aij;
		aij = NULL;
	#endif
	pAccInfo->pPublicKey = new CASignature();
	if ( pAccInfo->pPublicKey->setVerifyKey( elGeneral ) != E_SUCCESS )
	{
		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR);
		XERCES_CPP_NAMESPACE::DOMDocument* errDoc=NULL;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);
		if (errDoc != NULL)
		{
			errDoc->release();
			errDoc = NULL;
		}
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR;
	}

	//if ((!m_pJpiVerifyingInstance) ||
		//(m_pJpiVerifyingInstance->verifyXML( root, (CACertStore *)NULL ) != E_SUCCESS ))
	if(CAMultiSignature::verifyXML(root, CALibProxytest::getOptions()->getBI()->getCertificate()))
	{
		// signature invalid. mark this user as bad guy
		CAMsg::printMsg( LOG_INFO, "CAAccountingInstance::handleAccountCertificate(): Bad Jpi signature\n" );
		pAccInfo->authFlags |= AUTH_FAKE | AUTH_GOT_ACCOUNTCERT | AUTH_TIMEOUT_STARTED;
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_BAD_SIGNATURE;
	}


	UINT8 * arbChallenge;
	UINT8 b64Challenge[ 512 ];
	UINT32 b64Len = 512;

	//CAMsg::printMsg(LOG_DEBUG, "Almost finished handleAccountCertificate, preparing challenge\n");

	// generate random challenge data and Base64 encode it
	arbChallenge = new UINT8[222];
	getRandom( arbChallenge, 222 );
	CABase64::encode( arbChallenge, 222, b64Challenge, &b64Len );

	delete[] pAccInfo->pChallenge;
	pAccInfo->pChallenge = arbChallenge; // store challenge for later..

	// generate XML challenge structure
	XERCES_CPP_NAMESPACE::DOMDocument* doc = createDOMDocument();
	DOMElement* elemRoot = createDOMElement(doc, "Challenge" );
	DOMElement* elemPanic = createDOMElement(doc, "DontPanic" );
	DOMElement* elemPrepaid = createDOMElement(doc, "PrepaidBytes" );
	setDOMElementAttribute(elemPanic, "version",(UINT8*) "1.0" );
	doc->appendChild( elemRoot );
	elemRoot->appendChild( elemPanic );
	elemRoot->appendChild( elemPrepaid );
	setDOMElementValue( elemPanic, b64Challenge );
	SINT32 prepaidAmount = 0;  //m_dbInterface->getPrepaidAmount(pAccInfo->accountNumber, m_currentCascade, false);

	CAAccountingDBInterface *dbInterface = CAAccountingDBInterface::getConnection();

	//TODO: this is not a good moment to send the prepaid-bytes to the JonDo.
	//we cannot be sure that the database contains a consistent value here.
	if(dbInterface != NULL)
	{
		prepaidAmount = dbInterface->getPrepaidAmount(pAccInfo->accountNumber, m_currentCascade, false);
		CAAccountingDBInterface::releaseConnection(dbInterface);
		dbInterface = NULL;
	}


	SINT32 prepaidIvalLowerBound = 0; //(-1*(SINT32)CALibProxytest::getOptions()->getPrepaidInterval()); /* EXPERIMENTAL: transmit negative prepaid bytes (but not less than -PREPAID_BYTES) */
	if (prepaidAmount <  prepaidIvalLowerBound)
	{
		prepaidAmount = prepaidIvalLowerBound;
	}
	//CAMsg::printMsg( LOG_DEBUG, "handleAccountCertificate read %i prepaid bytes\n", prepaidAmount);
	setDOMElementValue( elemPrepaid, prepaidAmount);

	// send XML struct to Jap & set auth flags
	pAccInfo->pControlChannel->sendXMLMessage(doc);
	if (doc != NULL)
	{
		doc->release();
		doc = NULL;
	}
	pAccInfo->authFlags |= AUTH_CHALLENGE_SENT | AUTH_GOT_ACCOUNTCERT | AUTH_TIMEOUT_STARTED;
	pAccInfo->challengeSentSeconds = time(NULL);
	//CAMsg::printMsg("Last Account Certificate request seconds: for IP %u%u%u%u", (UINT8)pHashEntry->peerIP[0], (UINT8)pHashEntry->peerIP[1],(UINT8) pHashEntry->peerIP[2], (UINT8)pHashEntry->peerIP[3]);

	//CAMsg::printMsg( LOG_ERR, "CAAccountingInstance::handleAccountCertificate stop\n");


	pAccInfo->mutex->unlock();
	return CAXMLErrorMessage::ERR_OK;
}


UINT32 CAAccountingInstance::handleChallengeResponse(tAiAccountingInfo* pAccInfo, DOMElement* root)
{
	return handleChallengeResponse_internal(pAccInfo, root);
	INIT_STACK;
	FINISH_STACK("CAAccountingInstance::handleChallengeResponse");
}


/**
 * Handles the response to our challenge.
 * Checks the validity of the response and sets the user's authFlags
 * Also gets the last CC of the user, and sends it to the JAP
 * accordingly.
 */
UINT32 CAAccountingInstance::handleChallengeResponse_internal(tAiAccountingInfo* pAccInfo, DOMElement* root)
{
	INIT_STACK;
	BEGIN_STACK("CAAccountingInstance::handleChallengeResponse");


	//CAMsg::printMsg( LOG_ERR, "CAAccountingInstance::handleChallengeResponse start\n");


	UINT8 decodeBuffer[ 512 ];
	UINT32 decodeBufferLen = 512;
	UINT32 usedLen;
	/* DOMElement* elemPanic=NULL;
	DSA_SIG * pDsaSig=NULL; */
	SINT32 prepaidAmount = 0;
	AccountLoginHashEntry* loginEntry;
	CAXMLCostConfirmation* pCC = NULL;
	bool bSendCCRequest = true;
	UINT32 status;

	// check current authstate

	if (pAccInfo == NULL)
	{
		return CAXMLErrorMessage::ERR_NO_RECORD_FOUND;
	}

	pAccInfo->mutex->lock();

	if (pAccInfo->authFlags & AUTH_DELETE_ENTRY)
	{
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_NO_ERROR_GIVEN;
	}

	if( (!(pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT)) ||
			(!(pAccInfo->authFlags & AUTH_CHALLENGE_SENT))
		)
	{
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_NO_ERROR_GIVEN;
	}
	pAccInfo->authFlags &= ~AUTH_CHALLENGE_SENT;

	// get raw bytes of response
	if ( getDOMElementValue( root, decodeBuffer, &decodeBufferLen ) != E_SUCCESS )
	{
		CAMsg::printMsg( LOG_DEBUG, "ChallengeResponse has wrong XML format. Ignoring\n" );
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_WRONG_FORMAT;
	}
	DOMElement *elemClientVersion = NULL;
	getDOMChildByName(root, "ClientVersion", elemClientVersion, false);
	if(elemClientVersion != NULL)
	{
		UINT32 clientVersionStrLen = CLIENT_VERSION_STR_LEN;
		UINT8 *clientVersionStr = new UINT8[clientVersionStrLen];
		memset(clientVersionStr, 0, clientVersionStrLen);
		if( getDOMElementValue(elemClientVersion, clientVersionStr, &clientVersionStrLen) != E_SUCCESS)
		{
			delete [] clientVersionStr;
			clientVersionStr = NULL;
		}
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "Client Version (account %llu): %s\n",
				pAccInfo->accountNumber,
				(clientVersionStr != NULL ? clientVersionStr : (UINT8*) "<not set>"));
#endif
		pAccInfo->clientVersion = clientVersionStr;
	}
	else
	{
		pAccInfo->clientVersion = NULL;
	}
	decodeBuffer[decodeBufferLen] = 0;

	usedLen = decodeBufferLen;
	decodeBufferLen = 512;
	CABase64::decode( decodeBuffer, usedLen, decodeBuffer, &decodeBufferLen );

	/*
	UINT8 b64Challenge[ 512 ];
	UINT32 b64Len = 512;
	CABase64::encode(pHashEntry->pAccountingInfo->pChallenge, 222, b64Challenge, &b64Len);
	CAMsg::printMsg(LOG_DEBUG, "Challenge:\n%s\n", b64Challenge);
	*/

	// check signature
	//pDsaSig = DSA_SIG_new();
	CASignature * sigTester = pAccInfo->pPublicKey;
	//sigTester->decodeRS( decodeBuffer, decodeBufferLen, pDsaSig );
	if ( sigTester->verifyDER( pAccInfo->pChallenge, 222, decodeBuffer, decodeBufferLen )
		!= E_SUCCESS )
	{
		UINT8 accountNrAsString[32];
		print64(accountNrAsString, pAccInfo->accountNumber);
		CAMsg::printMsg(LOG_ERR, "Challenge-response authentication failed for account %s!\n", accountNrAsString);
		pAccInfo->authFlags |= AUTH_FAKE;
		pAccInfo->authFlags &= ~AUTH_ACCOUNT_OK;
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_BAD_SIGNATURE;
	}

	pAccInfo->authFlags |= AUTH_ACCOUNT_OK;	// authentication successful

	//EXPERIMENTAL NEW CODE for intercepting multiple login attempts
	m_currentAccountsHashtable->getMutex()->lock();
	loginEntry = (AccountLoginHashEntry*)m_currentAccountsHashtable->getValue(&(pAccInfo->accountNumber));
	struct t_fmhashtableentry *ownerRef = NULL;
	if(loginEntry != NULL) // (other) user (with same account) already logged in
	{
		ownerRef  = loginEntry->ownerRef;
		if(ownerRef != NULL)
		{
			bool isLoginFree = testAndSetLoginOwner(loginEntry, ownerRef);
			//obtaining ownership and setting loginProcessOngoing also means to cleanup the
			//old login entry after the connection belonging to the old login is closed.
			//So it can be assured that no other login-thread will find a NULL entry at the above if-statement
			m_currentAccountsHashtable->getMutex()->unlock();
			//abort if another thread is logging in but ...
			if(!isLoginFree)
			{
				CAMsg::printMsg(LOG_DEBUG, "Exiting because login is occupied for owner %p of account %llu.\n", ownerRef);
				pAccInfo->authFlags |= AUTH_MULTIPLE_LOGIN;
				pAccInfo->mutex->unlock();
				return CAXMLErrorMessage::ERR_MULTIPLE_LOGIN;
			}
			//...if the former login is finished and the connection is in use: force the previous login-connection to be kicked out.
			m_mix->getLoginMutex()->lock();
			CAXMLErrorMessage kickoutMsg(CAXMLErrorMessage::ERR_MULTIPLE_LOGIN);
			XERCES_CPP_NAMESPACE::DOMDocument* errDoc=NULL;
			kickoutMsg.toXmlDocument(errDoc);
			//Note: the ownerRef hashEntry can already be cleared at that point.
			if(  m_mix->forceKickout(ownerRef, errDoc)  )
			{
				CAMsg::printMsg(LOG_DEBUG, "Kickout was requested for owner %p of account %llu, waiting...\n", ownerRef,
						pAccInfo->accountNumber);
				//Synchronize until the main thread can sure that the connection is closed. (After FirstMixA::checkConnections()
				// in main loop)
				//not dangerous if ensured that the cleanup notifier is always locked after loginCV
				//but it is necessary to lock cleanupNotifier before releasing loginCV. otherwise we might lose
				//the signal from cleanupNotifier. This can't happen if the main thread that
				//peforms the cleanup is still blokced by loginCV before it can acquire cleanupNotifier during the cleanup.
				ownerRef->cleanupNotifier->lock();
				m_mix->getLoginMutex()->unlock();
				ownerRef->cleanupNotifier->wait();
				ownerRef->cleanupNotifier->unlock();
			}
			else
			{
				m_mix->getLoginMutex()->unlock();
				//if the forceKickout returns false the ownerRef was already cleared.
				//no need to wait any further.
				CAMsg::printMsg(LOG_INFO, "ownerRef %p of account %llu already kicked out.\n", ownerRef, pAccInfo->accountNumber);
			}
			errDoc->release();
			errDoc = NULL;
			//obtain hashtable lock again.
			m_currentAccountsHashtable->getMutex()->lock();
			if(loginEntry != NULL)
			{
				//When login ownership was obtained: cleanup the former entry.
				CAMsg::printMsg(LOG_CRIT, "finally cleaning up loginEntry %p for former owner %p of account %llu\n",
													loginEntry, loginEntry->ownerRef, pAccInfo->accountNumber);
				ms_pInstance->m_currentAccountsHashtable->remove(&(pAccInfo->accountNumber));
				delete loginEntry->ownerLock;
				delete loginEntry;
				loginEntry = NULL;
			}
		}
		else
		{
			//Impossible or Bug
			CAMsg::printMsg(LOG_CRIT, "BUG: ownerRef of an active login entry MUST NOT be null. Please report.\n");
			m_currentAccountsHashtable->getMutex()->unlock();
			pAccInfo->mutex->unlock();
			return CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR;
		}
	}
	//POST CONDITION: old login entry cleared.
	loginEntry = new AccountLoginHashEntry;
	loginEntry->accountNumber = pAccInfo->accountNumber;
	loginEntry->count = 1;
	loginEntry->confirmedBytes = 0;
	loginEntry->authRemoveFlags = 0;
	loginEntry->authFlags = 0;
	loginEntry->userID = pAccInfo->userID;
	loginEntry->ownerRef = pAccInfo->ownerRef;
	loginEntry->loginOngoing = true;
	loginEntry->ownerLock = new CAMutex();
	m_currentAccountsHashtable->put(&(loginEntry->accountNumber), loginEntry);

	//m_currentAccountsHashtable->getMutex()->unlock();

	// fetch cost confirmation from last session if available, and retrieve information; synchronized with settle thread
	bool bSettled;
	CAAccountingDBInterface *dbInterface = CAAccountingDBInterface::getConnection();
#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG, "Checking database for previously prepaid bytes...\n");
#endif

	if(dbInterface != NULL)
	{
		prepaidAmount = dbInterface->getPrepaidAmount(pAccInfo->accountNumber, m_currentCascade, false);
		dbInterface->getCostConfirmation(pAccInfo->accountNumber, m_currentCascade, &pCC, bSettled);
	}
	else
	{
		prepaidAmount = 0;
	}


	if (pCC != NULL)
	{
		if(!cascadeMatchesCC(pCC))
		{
			delete pCC;
			pCC = NULL;
			CAMsg::printMsg(LOG_INFO, "CC do not match current Cascade. Discarding CC.\n");
		}
	}

	if (pCC != NULL)
	{
		if (bSettled)
		{
			pAccInfo->authFlags &= ~AUTH_WAITING_FOR_FIRST_SETTLED_CC;
		}
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "pAccInfo->transferredBytes is %llu, confirmedBytes: %llu, pCC->transferredBytes is %llu\n",
				pAccInfo->transferredBytes, pAccInfo->confirmedBytes, pCC->getTransferredBytes());
#endif
		pAccInfo->transferredBytes += pCC->getTransferredBytes();
		pAccInfo->confirmedBytes = pCC->getTransferredBytes();
		#ifdef DEBUG
			UINT8 tmp[32];
			print64(tmp,pAccInfo->transferredBytes);
			CAMsg::printMsg(LOG_DEBUG, "Setting confirmedBytes to %llu, pAccInfo->transferredBytes is now %s\n",
					pAccInfo->confirmedBytes, tmp);
		#endif
		//delete pCC;
	}
	else
	{
		UINT8 tmp[32];
		print64(tmp,pAccInfo->accountNumber);
		CAMsg::printMsg(LOG_INFO, "CAAccountingInstance: Cost confirmation for account %s not found in database. This seems to be a new user.\n", tmp);
	}

	/** @todo We need this trick so that the program does not freeze with active AI ThreadPool!!!! */
	//pAccInfo->mutex->unlock();
//	m_currentAccountsHashtable->getMutex()->lock();
//	pAccInfo->authFlags |= AUTH_ACCOUNT_OK;	// authentication successful
//
//	loginEntry = (AccountLoginHashEntry*)m_currentAccountsHashtable->getValue(&(pAccInfo->accountNumber));
//	//now loginEntry == NULL must be asserted
//	if (!loginEntry)
//	{
//		// remember that this user is logged in at least once
//		loginEntry = new AccountLoginHashEntry;
//		loginEntry->accountNumber = pAccInfo->accountNumber;
//		loginEntry->count = 1;
//		loginEntry->confirmedBytes = 0;
//		loginEntry->authRemoveFlags = 0;
//		loginEntry->authFlags = 0;
//		loginEntry->userID = pAccInfo->userID;
//		m_currentAccountsHashtable->put(&(loginEntry->accountNumber), loginEntry);
//		if (!(AccountLoginHashEntry*)m_currentAccountsHashtable->getValue(&(pAccInfo->accountNumber)))
//		{
//			UINT8 accountNrAsString[32];
//			print64(accountNrAsString, pAccInfo->accountNumber);
//			CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: Could not insert login entry for account %s!", accountNrAsString);
//		}
//	}
//	else
//	{
//		loginEntry->count++;
//	}
//	if (loginEntry->count > 1)
//	{
//		/*
//		 * There already is a user logged in with this account.
// 		 */
//		UINT8 accountNrAsString[32];
//		print64(accountNrAsString, pAccInfo->accountNumber);
//		if (loginEntry->count < MAX_TOLERATED_MULTIPLE_LOGINS)
//		{
//			// There is now more than one user logged in with this account; kick out the other users!
//			CAMsg::printMsg(LOG_INFO,
//							"CAAccountingInstance: Multiple logins (%d) of user with account %s detected! \
//							Kicking out other users with this account...\n",
//							loginEntry->count, accountNrAsString);
//			loginEntry->userID = pAccInfo->userID; // this is the current user; kick out the others
//		}
//		else
//		{
//		 	/* The maximum of tolerated concurrent logins for this user is exceeded.
//		 	 * He won't get any new access again before the old connections have been closed!
//		 	 * @mod: in this case not more than one login is allowed at a time. The User has to wait until
//		 	 * the old login will be deleted.
//		 	 */
//		 	CAMsg::printMsg(LOG_INFO,
//		 					"CAAccountingInstance: Maximum of multiple logins exceeded (%d) for user with account %s! \
//		 					Kicking out this user!\n",
//		 					loginEntry->count, accountNrAsString);
//		 	bSendCCRequest = false; // not needed...
//		 	pAccInfo->authFlags |= AUTH_MULTIPLE_LOGIN;
//
//		 	delete pCC;
//		 	pCC = NULL;
//		 	m_currentAccountsHashtable->getMutex()->unlock();
//		 	pAccInfo->mutex->unlock();
//		 	return CAXMLErrorMessage::ERR_MULTIPLE_LOGIN;
//		}
//	}

	UINT8 tmp[32];
	print64(tmp,pAccInfo->accountNumber);
	if (prepaidAmount > 0)
	{
		CAMsg::printMsg(LOG_INFO, "CAAccountingInstance: Got %d prepaid bytes for account nr. %s.\n",prepaidAmount, tmp);

		//pAccInfo->authFlags &= ~AUTH_WAITING_FOR_FIRST_SETTLED_CC;

		if (pAccInfo->transferredBytes >= (UINT32)prepaidAmount)
		{
			pAccInfo->transferredBytes -= prepaidAmount;
		}
		else
		{
			UINT8 tmp2[32];
			print64(tmp2, pAccInfo->transferredBytes);
			CAMsg::printMsg(LOG_ERR, "CAAccountingInstance: Transferred bytes of %s for account %s are lower than prepaid amount! "
									"Maybe we lost a CC?\n",tmp2, tmp);
			prepaidAmount = 0;
		}
	}
	else
	{
		prepaidAmount = 0;
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: No database record for prepaid bytes found for account nr. %s.\n", tmp);
	}
	//CAMsg::printMsg(LOG_DEBUG, "Number of prepaid (confirmed-transferred) bytes : %d \n",pAccInfo->confirmedBytes-pAccInfo->transferredBytes);

	/**
	 * @todo Dangerous, as this may collide with previous accounts that have been
	 * used and deleted before...
	 * There should be something like an expiration date for the account status, e.g. 1 month
	 */
	 SINT32 dbRet;
	if(dbInterface != NULL);
	{
		dbRet = dbInterface->getAccountStatus(pAccInfo->accountNumber, status);
		CAAccountingDBInterface::releaseConnection(dbInterface);
		dbInterface = NULL;
	}

	if (dbRet != E_SUCCESS)
	{
		UINT8 tmp[32];
		print64(tmp,pAccInfo->accountNumber);
		CAMsg::printMsg(LOG_ERR, "CAAccountingInstance: Could not check status for account %s!\n", tmp);
	}
	else if (status > CAXMLErrorMessage::ERR_OK)
	{
		//UINT32 authFlags = 0;
		//the auth flags are set after this check, but they need to be verified during a forced settlement
		//at the end of the login. (see finishLoginProcess())
		CAMsg::printMsg(LOG_INFO, "CAAccountingInstance: Illegal status %u for account %llu found.\n",
				status, pAccInfo->accountNumber);
		if (status == CAXMLErrorMessage::ERR_BLOCKED)
		{
			pAccInfo->authFlags |= AUTH_BLOCKED;
		}
		else if (status == CAXMLErrorMessage::ERR_KEY_NOT_FOUND)
		{
			pAccInfo->authFlags |= AUTH_INVALID_ACCOUNT;
		}
		else if (status == CAXMLErrorMessage::ERR_ACCOUNT_EMPTY)
		{
			pAccInfo->authFlags |= AUTH_ACCOUNT_EMPTY;
		}

		/*if (authFlags)
		{
			pAccInfo->authFlags |= AUTH_BLOCKED;
			if (loginEntry->confirmedBytes == 0)
			{
				loginEntry->confirmedBytes = pAccInfo->confirmedBytes;
			}
		}*/
	}
	m_currentAccountsHashtable->getMutex()->unlock();

	/** @todo We need this trick so that the program does not freeze with active AI ThreadPool!!!! */
	//pAccInfo->mutex->lock();
	SINT32 sendStatus = E_SUCCESS;
	if (bSendCCRequest)
	{
		// fetch cost confirmation from last session if available, and send it
		//CAXMLCostConfirmation * pCC = NULL;
		//m_dbInterface->getCostConfirmation(pAccInfo->accountNumber, m_currentCascade, &pCC);
		if(pCC != NULL)
		{
#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Sending pcc to sign with %llu transferred bytes\n", pCC->getTransferredBytes());
#endif
			// the typical case; the user had logged in before
			/* there shouldn't be any counting synchronisation problems with the JAP
			 * because the new login protocol doesn't permit JAPs
			 * to exchange data before login is finished.
			 */
			UINT32 prepaidIval = CALibProxytest::getOptions()->getPrepaidInterval();
			pAccInfo->bytesToConfirm = (prepaidIval - prepaidAmount) + pCC->getTransferredBytes();
#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: before CC request, bytesToConfirm: %llu, prepaidIval: %u, "
					"prepaidAmount: %d, transferred bytes: %llu\n",
					pAccInfo->bytesToConfirm, prepaidIval, prepaidAmount, pAccInfo->transferredBytes);

#endif
			if( (pAccInfo->clientVersion == NULL) ||
				(strncmp((char*)pAccInfo->clientVersion, PREPAID_PROTO_CLIENT_VERSION, CLIENT_VERSION_STR_LEN) < 0) )
			{
				//old CC without payRequest and prepaid bytes.
				//CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Account %llu: Old prepaid proto version.\n", pAccInfo->accountNumber);
				sendStatus = pAccInfo->pControlChannel->sendXMLMessage(pCC->getXMLDocument());

			}
			else
			{
				//CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Account %llu: New prepaid proto version. prepaid bytes: %d\n",
				//		pAccInfo->accountNumber, prepaidAmount);
				sendStatus = sendInitialCCRequest(pAccInfo, pCC, prepaidAmount);
			}
		}
		else
		{
			// there is no CC in the database; typically this is the first connection of this user
			if (prepaidAmount > 0)
			{
				// Delete any previously stored prepaid amount; there should not be any! CC lost?
				pAccInfo->transferredBytes += prepaidAmount;
			}
			sendStatus = sendCCRequest(pAccInfo);
		}
	}


	delete pCC;
	pCC = NULL;


	if ( pAccInfo->pChallenge != NULL ) // free mem
	{
		delete[] pAccInfo->pChallenge;
		pAccInfo->pChallenge = NULL;
	}
	//CAMsg::printMsg( LOG_ERR, "CAAccountingInstance::handleChallengeResponse stop\n");

	pAccInfo->mutex->unlock();
	if(sendStatus == E_SUCCESS)
	{
		return CAXMLErrorMessage::ERR_OK;
	}
	else
	{
		return CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR;
	}
}

UINT32 CAAccountingInstance::handleCostConfirmation(tAiAccountingInfo* pAccInfo, DOMElement* root)
{
	return handleCostConfirmation_internal(pAccInfo, root);
	INIT_STACK;
	FINISH_STACK("CAAccountingInstance::handleCostConfirmation");
}

/**
 * Handles a cost confirmation sent by a jap
 */
UINT32 CAAccountingInstance::handleCostConfirmation_internal(tAiAccountingInfo* pAccInfo, DOMElement* root)
{
	INIT_STACK;
	BEGIN_STACK("CAAccountingInstance::handleCostConfirmation");

	if (pAccInfo == NULL)
	{
		return CAXMLErrorMessage::ERR_NO_RECORD_FOUND;
	}

	pAccInfo->mutex->lock();

	if ( (pAccInfo->authFlags & AUTH_DELETE_ENTRY) ||
		 (!(pAccInfo->authFlags & AUTH_LOGIN_NOT_FINISHED ) && (pAccInfo->authFlags & AUTH_ACCOUNT_EMPTY )) )
	{
		CAMsg::printMsg(LOG_ERR,
					"CAAccountingInstance::handleCostConfirmation Ignoring CC, restricted flags set: %s %s\n",
					((pAccInfo->authFlags & AUTH_DELETE_ENTRY) ? "AUTH_DELETE_ENTRY" : "") ,
					((pAccInfo->authFlags & AUTH_ACCOUNT_EMPTY) ? "AUTH_ACCOUNT_EMPTY" : ""));
		// ignore CCs for this account!
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_NO_ERROR_GIVEN;
	}

	// check authstate
	if (!(pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT) ||
		!(pAccInfo->authFlags & AUTH_ACCOUNT_OK) ||
		!(pAccInfo->authFlags & AUTH_SENT_CC_REQUEST))
	{
		CAMsg::printMsg(LOG_ERR,
			"CAAccountingInstance::handleCostConfirmation CC was received but has not been requested! Ignoring...\n");

		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_NO_ERROR_GIVEN;
	}

	CAXMLCostConfirmation* pCC = CAXMLCostConfirmation::getInstance(root);
	if(pCC==NULL)
	{
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR;
	}

	// for debugging only: test signature the oldschool way
	// warning this removes the signature from doc!!!
	if (pAccInfo->pPublicKey==NULL||
		pAccInfo->pPublicKey->verifyXML( root ) != E_SUCCESS)
	{
		// wrong signature
		CAMsg::printMsg( LOG_INFO, "CostConfirmation has INVALID SIGNATURE!\n" );
		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_SIGNATURE, (UINT8*)"CostConfirmation has bad signature");
		XERCES_CPP_NAMESPACE::DOMDocument* errDoc=NULL;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);
		if (errDoc != NULL)
		{
			errDoc->release();
			errDoc = NULL;
		}
		delete pCC;
		pCC = NULL;
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_BAD_SIGNATURE;
	}
	#ifdef DEBUG
	else
		{
			CAMsg::printMsg( LOG_DEBUG, "CostConfirmation Signature is OK.\n");
		}
	#endif


	if (pCC->getNumberOfHashes() != m_allHashesLen)
	{
		CAMsg::printMsg( LOG_INFO, "CostConfirmation has illegal number of price cert hashes!\n" );
		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_REQUEST,
			(UINT8*)"CostConfirmation has illegal number of price cert hashes");
		XERCES_CPP_NAMESPACE::DOMDocument* errDoc=NULL;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);
		if (errDoc != NULL)
		{
			errDoc->release();
			errDoc = NULL;
		}
		delete pCC;
		pCC = NULL;
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_WRONG_FORMAT;
	}

	/*Hashtable* certHashCC =
		new Hashtable((UINT32 (*)(void *))Hashtable::stringHash, (SINT32 (*)(void *,void *))Hashtable::stringCompare);
	UINT8* certHash;*/
	//bool bFailed = false;
	//for (UINT32 i = 0; i < pCC->getNumberOfHashes(); i++)
	//{
	//	certHash = pCC->getPriceCertHash(i);
	//	certHashCC->put(certHash, certHash);
		/*
		if ((certHash = (UINT8*)certHashCC->getValue(certHash)) != NULL)
		{
			CAMsg::printMsg( LOG_INFO, "CC1: %s\n", certHash);
		}*/
	//}
	//for (UINT32 i = 0; i < m_allHashesLen; i++)
	//{
		//CAMsg::printMsg( LOG_INFO, "CA:  %s\n", m_allHashes[i]);
	//	certHash = (UINT8*)certHashCC->remove(m_allHashes[i]);
	//	if (certHash == NULL)
	//	{
	//		bFailed = true;
	//		break;
	//	}
	//	else
	//	{
	//		delete[] certHash;
	//	}
	//}
	//certHashCC->clear(HASH_EMPTY_NONE, HASH_EMPTY_DELETE);
	//delete certHashCC;

	if (!cascadeMatchesCC(pCC))
	{
		CAMsg::printMsg( LOG_INFO, "CostConfirmation has invalid price cert hashes!\n" );
		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_REQUEST,
			(UINT8*)"CostConfirmation has invalid price cert hashes");
		XERCES_CPP_NAMESPACE::DOMDocument* errDoc=NULL;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);
		if (errDoc != NULL)
		{
			errDoc->release();
			errDoc = NULL;
		}
		delete pCC;
		pCC = NULL;
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_WRONG_DATA;
	}



	// parse & set transferredBytes
	//when using Prepayment, this check is outdated, but left in to notice the most crude errors/cheats
	//The CC's transferredBytes should be equivalent to
	//AccInfo's confirmed bytes + the Config's PrepaidInterval - the number of bytes transferred between
	//requesting and receiving the CC
#ifdef DEBUG
	CAMsg::printMsg( LOG_DEBUG, "received cost confirmation for  %llu transferred bytes where confirmed bytes are %llu, we need %llu bytes to confirm"
			", mix already counted %llu transferred bytes\n",
			pCC->getTransferredBytes(), pAccInfo->confirmedBytes, pAccInfo->bytesToConfirm,
			pAccInfo->transferredBytes);
#endif

	if(pCC->getTransferredBytes() > (pAccInfo->transferredBytes+CALibProxytest::getOptions()->getPrepaidInterval()) )
	{
		CAMsg::printMsg( LOG_ERR, "Warning: ignoring this CC for account %llu "
				"because it tries to confirm %lld prepaid bytes where only %u prepaid bytes are allowed (cc->tranferredbytes: %llu, accInfo->tranferredBytes: %llu)\n",
				pAccInfo->accountNumber, (pCC->getTransferredBytes() - pAccInfo->transferredBytes),
				CALibProxytest::getOptions()->getPrepaidInterval(),
				pCC->getTransferredBytes(), pAccInfo->transferredBytes);

		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_DATA,
			(UINT8*)"More bytes confirmed than allowed.");
		XERCES_CPP_NAMESPACE::DOMDocument* errDoc=NULL;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);
		if (errDoc != NULL)
		{
			errDoc->release();
			errDoc = NULL;
		}
		//mark as account empty has the effect is that the user can empty his prepaid amount and then will be kicked out.
		pAccInfo->authFlags |= AUTH_FAKE;
		delete pCC;
		pCC = NULL;
		pAccInfo->mutex->unlock();
		return CAXMLErrorMessage::ERR_WRONG_DATA;
	}

	if (pCC->getTransferredBytes() < pAccInfo->confirmedBytes)
	{

		CAMsg::printMsg( LOG_ERR, "CostConfirmation has insufficient number of bytes:\n");
		CAMsg::printMsg( LOG_ERR, "CC->transferredBytes: %llu < confirmedBytesBytes: %llu\n", pCC->getTransferredBytes(), pAccInfo->confirmedBytes);

		if(pAccInfo->authFlags & AUTH_LOGIN_NOT_FINISHED)
		{
			//@todo: perhaps we should use another flag to indicate that this user should be kicked out.
			pAccInfo->authFlags |= AUTH_FAKE;
			delete pCC;
			pCC = NULL;
			pAccInfo->mutex->unlock();
			return CAXMLErrorMessage::ERR_WRONG_DATA;
		}

		/*
		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_DATA,
			(UINT8*)"Your CostConfirmation has a wrong number of transferred bytes");
		DOM_Document errDoc;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);*/
	}
	else if (pCC->getTransferredBytes() == pAccInfo->confirmedBytes && getPrepaidBytes(pAccInfo) != CALibProxytest::getOptions()->getPrepaidInterval())
	{
		CAMsg::printMsg(LOG_WARNING, "Received CostConfirmation for account %llu has no difference in bytes to current CC (%llu bytes).\n", 
			pAccInfo->accountNumber, pCC->getTransferredBytes());
	}
	else
	{
		/*
		UINT8 tmp[32];
		print64(tmp,pCC->getTransferredBytes());
		CAMsg::printMsg( LOG_ERR, "Transferredbytes in CC: %s\n", tmp);
		*/
		SINT32 dbRet = E_UNKNOWN;
		CAAccountingDBInterface *dbInterface = CAAccountingDBInterface::getConnection();
		if(dbInterface != NULL)
		{
			dbRet = dbInterface->storeCostConfirmation(*pCC, m_currentCascade);
			CAAccountingDBInterface::releaseConnection(dbInterface);
			dbInterface = NULL;
		}
		if (dbRet != E_SUCCESS)
		{
			UINT8 tmp[32];
			print64(tmp,pCC->getAccountNumber());
			CAMsg::printMsg( LOG_INFO, "CostConfirmation for account %s could not be stored in database!\n", tmp );
			pAccInfo->authFlags |= AUTH_DATABASE;
		}
		else
		{
#ifdef DEBUG
			CAMsg::printMsg( LOG_INFO, "Handle CC: pCC->transBytes: %llu\n", pCC->getTransferredBytes() );
#endif
			pAccInfo->confirmedBytes = pCC->getTransferredBytes();

			if (pAccInfo->authFlags & AUTH_WAITING_FOR_FIRST_SETTLED_CC)
			{
				// initiate immediate settling
#ifdef DEBUG
				UINT64 currentMillis;
				UINT8 tmpStrCurrentMillis[50];
				getcurrentTimeMillis(currentMillis);
				print64(tmpStrCurrentMillis,currentMillis);
				CAMsg::printMsg(LOG_DEBUG, "AccountingSettleThread: Settle ini: %s\n", tmpStrCurrentMillis);
#endif
				m_pSettleThread->settle();

			}
		}
	}
#ifdef DEBUG
	CAMsg::printMsg(LOG_ERR, "Handle CC request: pAccInfo->confirmedBytes: %llu, ppAccInfo->transferredBytes: %llu\n",
			pAccInfo->confirmedBytes, pAccInfo->transferredBytes);
#endif
	if (pAccInfo->confirmedBytes >= pAccInfo->bytesToConfirm)
	{
		// the user confirmed everything we wanted; if a timeout has been set, it should be reset
		pAccInfo->authFlags &= ~AUTH_HARD_LIMIT_REACHED;
		pAccInfo->lastHardLimitSeconds = time(NULL);
	}
	else
	{
		/*UINT8 tmp[32], tmp2[32], tmp3[32];
		print64(tmp, pCC->getTransferredBytes());
		print64(tmp2, pCC->getAccountNumber());
		print64(tmp3, pAccInfo->bytesToConfirm);*/
		CAMsg::printMsg(LOG_ERR, "AccountingSettleThread: Requested CC value has NOT been confirmed by account nr %llu! "
								 "Received Bytes: %llu/%llu "
								"Client should not be allowed to login.\n",
								pCC->getAccountNumber(),
								pCC->getTransferredBytes(),
								pAccInfo->bytesToConfirm);

		if(pAccInfo->authFlags & AUTH_LOGIN_SKIP_SETTLEMENT)
		{
			CAMsg::printMsg( LOG_INFO, "Skip settlement revoked.\n");
			pAccInfo->authFlags &= ~AUTH_LOGIN_SKIP_SETTLEMENT;
		}
		/*delete pCC;
		pCC = NULL;
		pAccInfo->mutex->unlock();
		pAccInfo->authFlags |= AUTH_FAKE;
		return CAXMLErrorMessage::ERR_WRONG_DATA;*/
		//m_pSettleThread->settle();

	}

	pAccInfo->bytesToConfirm = 0;
	pAccInfo->authFlags &= ~AUTH_SENT_CC_REQUEST;

	delete pCC;
	pCC = NULL;
	pAccInfo->mutex->unlock();

	return CAXMLErrorMessage::ERR_OK;
}

/**
 * This must be called whenever a JAP is connecting to init our per-user
 * data structures
 */
SINT32 CAAccountingInstance::initTableEntry( fmHashTableEntry * pHashEntry )
{
	INIT_STACK;
	BEGIN_STACK("CAAccountingInstance::initTableEntry");

	//ms_pInstance->m_pMutex->lock();

	if (pHashEntry == NULL)
	{
		FINISH_STACK("CAAccountingInstance::initTableEntry:NULL");
		return E_UNKNOWN;
	}

	pHashEntry->pAccountingInfo = new tAiAccountingInfo;
	memset( pHashEntry->pAccountingInfo, 0, sizeof( tAiAccountingInfo ) );

	SAVE_STACK("CAAccountingInstance::initTableEntry", "After memset");

	pHashEntry->pAccountingInfo->authFlags =
		AUTH_SENT_ACCOUNT_REQUEST | AUTH_TIMEOUT_STARTED |
		AUTH_HARD_LIMIT_REACHED | AUTH_WAITING_FOR_FIRST_SETTLED_CC	|
		AUTH_SENT_CC_REQUEST | AUTH_LOGIN_NOT_FINISHED; // prevents multiple CC requests on login
	pHashEntry->pAccountingInfo->authTimeoutStartSeconds = time(NULL);
	pHashEntry->pAccountingInfo->lastHardLimitSeconds = time(NULL);
	pHashEntry->pAccountingInfo->sessionPackets = 0;
	pHashEntry->pAccountingInfo->transferredBytes = 0;
	pHashEntry->pAccountingInfo->confirmedBytes = 0;
	pHashEntry->pAccountingInfo->bytesToConfirm = 0;
	pHashEntry->pAccountingInfo->nrInQueue = 0;
	pHashEntry->pAccountingInfo->userID = pHashEntry->id;
	pHashEntry->pAccountingInfo->ownerRef = pHashEntry;
	pHashEntry->pAccountingInfo->clientVersion = NULL;
	pHashEntry->pAccountingInfo->mutex = new CAMutex;
	//ms_pInstance->m_pMutex->unlock();


	FINISH_STACK("CAAccountingInstance::initTableEntry");

	return E_SUCCESS;
}



/**
 * This should always be called when closing a JAP connection
 * to cleanup the payment data structures and store prepaid bytes

 */
SINT32 CAAccountingInstance::cleanupTableEntry( fmHashTableEntry *pHashEntry )
	{
		INIT_STACK;
		BEGIN_STACK("CAAccountingInstance::cleanupTableEntry");

		//ms_pInstance->m_pMutex->lock();
		tAiAccountingInfo* pAccInfo = pHashEntry->pAccountingInfo;
		AccountLoginHashEntry* loginEntry;
		SINT32 prepaidBytes = 0;
		SINT32 prepaidInterval = CALibProxytest::getOptions()->getPrepaidInterval();

		if (pAccInfo == NULL)
		{
			SAVE_STACK("CAAccountingInstance::cleanupTableEntry", "acc info null");
			//ms_pInstance->m_pMutex->unlock();
			return E_UNKNOWN;
		}

		//pAccInfo->mutex->lock();

		pHashEntry->pAccountingInfo=NULL;

		if (pAccInfo->accountNumber)
		{
			CAMsg::printMsg(LOG_INFO, "cleaning up entry %p of accountno. %llu (pAccInfo ref: %p)\n",
							pHashEntry, pAccInfo->accountNumber, pAccInfo);
			if (pAccInfo->authFlags & AUTH_ACCOUNT_OK)
			{
				// remove login
				ms_pInstance->m_currentAccountsHashtable->getMutex()->lock();
				loginEntry = (AccountLoginHashEntry*)ms_pInstance->m_currentAccountsHashtable->getValue(&(pAccInfo->accountNumber));

				if (loginEntry)
				{
					// test: delete CC!!!
					//ms_pInstance->m_dbInterface->deleteCC(pAccInfo->accountNumber, ms_pInstance->m_currentCascade);
					if (testLoginEntryOwner(loginEntry, pHashEntry))// &&
						//!(pAccInfo->authFlags & AUTH_WAITING_FOR_FIRST_SETTLED_CC))
					{
						if (loginEntry->authFlags & AUTH_ACCOUNT_EMPTY)
						{
							// make sure to store the correct number of prepaid bytes
							pAccInfo->confirmedBytes = loginEntry->confirmedBytes;
						}
						//store prepaid bytes in database, so the user wont lose the prepaid amount by disconnecting
						prepaidBytes = getPrepaidBytes(pAccInfo);
						if (prepaidBytes > 0)
						{
							if (prepaidBytes > prepaidInterval)
							{
								UINT8 tmp[32];
								print64(tmp, pAccInfo->accountNumber);
								/* Client paid more than the prepaid interval -
								 * this is beyond specification and not allowed!
								 */
								CAMsg::printMsg(LOG_ERR,
									"PrepaidBytes of %d for account %s are higher than prepaid interval! "
									"The client (owner %x) did not behave according to specification. "
									"Deleting prepaid bytes!\n", prepaidBytes, tmp, pHashEntry);
									//"Deleting %d bytes...\n", prepaidBytes, tmp, pHashEntry, prepaidBytes - prepaidInterval);

								//prepaidBytes = prepaidInterval;
								prepaidBytes = 0;
							}
						}

						/*if (ms_pInstance->m_dbInterface)
						{
							ms_pInstance->m_dbInterface->storePrepaidAmount(pAccInfo->accountNumber,prepaidBytes, ms_pInstance->m_currentCascade);
						}*/
						CAAccountingDBInterface *dbInterface = CAAccountingDBInterface::getConnection();
						if(dbInterface != NULL)
						{
							dbInterface->storePrepaidAmount(pAccInfo->accountNumber,
															prepaidBytes,
															ms_pInstance->m_currentCascade);
							CAAccountingDBInterface::releaseConnection(dbInterface);
							dbInterface = NULL;
						}
						if(!isLoginOngoing(loginEntry, pHashEntry))
						{
							ms_pInstance->m_currentAccountsHashtable->remove(&(pAccInfo->accountNumber));
							delete loginEntry->ownerLock;
							delete loginEntry;
							loginEntry = NULL;
						}
						else
						{
							CAMsg::printMsg(LOG_CRIT, "cleaning: Leaving loginEntry %x cleanup of owner %x to the next owner.\n",
									loginEntry, loginEntry->ownerRef);
						}
					}

					/*if (loginEntry->count <= 1)
					{
						if (loginEntry->count < 1)
						{
							CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: Cleanup found non-positive number of user login hash entries (%d)!\n", loginEntry->count);
						}
						// this is the last active user connection; delete the entry
						ms_pInstance->m_currentAccountsHashtable->remove(&(pAccInfo->accountNumber));
						delete loginEntry->ownerLock;
						delete loginEntry;
						loginEntry = NULL;
					}
					else
					{
						// there are other connections from this user
						loginEntry->count--;
					}*/
				}
				else
				{
					CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: Cleanup did not find user login hash entry!\n");
				}
				ms_pInstance->m_currentAccountsHashtable->getMutex()->unlock();
			}
		}
		else
		{
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Cleanup method found account zero.\n");
		}

		//free memory of pAccInfo

		delete pAccInfo->pPublicKey;
		pAccInfo->pPublicKey = NULL;


		delete [] pAccInfo->pChallenge;
		pAccInfo->pChallenge = NULL;

		delete [] pAccInfo->pstrBIID;
		pAccInfo->pstrBIID = NULL;

		delete [] pAccInfo->clientVersion;
		pAccInfo->clientVersion = NULL;

		pHashEntry->pAccountingInfo=NULL;

		if (pAccInfo->nrInQueue > 0)
		{
			/*
			if (pAccInfo->accountNumber == 0)
			{
				CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: AI queue entries found for account zero: %u!\n", pAccInfo->nrInQueue);
			}
			else if (!(pAccInfo->authFlags & AUTH_ACCOUNT_OK))
			{
				UINT8 accountNrAsString[32];
				print64(accountNrAsString, pAccInfo->accountNumber);
				CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: AI queue entries found for unauthorized account %s: %u!\n", accountNrAsString, pAccInfo->nrInQueue);
			}
			else*/
			{
				// there are still entries in the ai queue; empty it before deletion; we cannot delete it now
				pAccInfo->authFlags |= AUTH_DELETE_ENTRY;
			}
		}
		else if (pAccInfo->nrInQueue < 0)
		{
			CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: Cleanup method found negative handle queue!\n");
		}
		//pAccInfo->mutex->unlock();

		if (!(pAccInfo->authFlags & AUTH_DELETE_ENTRY))
		{
			// there are no handles for this entry in the queue, we can savely delete it now
			delete pAccInfo->mutex;
			pAccInfo->mutex = NULL;
			delete [] pAccInfo->clientVersion;
			pAccInfo->clientVersion = NULL;
			delete pAccInfo;
			pAccInfo = NULL;
		}
		else
		{
			CAMsg::printMsg(LOG_INFO, "CAAccountingInstance: Cleanup method sent account deletion request to AI thread!\n");
		}
		//ms_pInstance->m_pMutex->unlock();

		FINISH_STACK("CAAccountingInstance::cleanupTableEntry");

		return E_SUCCESS;
	}

SINT32 CAAccountingInstance::newSettlementTransaction()
{
	UINT32 settledCCs = 0;
	SINT32 retVal;
	do
	{
		retVal = __newSettlementTransaction(&settledCCs);
	}
	while(settledCCs >= MAX_SETTLED_CCS && retVal == E_SUCCESS);
	return E_SUCCESS; // change to retVal if you want to block new users on failure; but remember this is not sufficient because prepaid bytes are stored even on failure!
}

SINT32 CAAccountingInstance::__newSettlementTransaction(UINT32 *nrOfSettledCCs)
{

	CAXMLErrorMessage **pErrMsgs = NULL, *settleException = NULL;
	CAXMLCostConfirmation **allUnsettledCCs = NULL;
	SettleEntry *entry = NULL, *nextEntry = NULL;
	CAAccountingDBInterface *dbInterface = NULL;

	UINT32 nrOfCCs = 0, i = 0;
	SINT32 biConnectionStatus = 0, ret = E_SUCCESS;
	UINT64 myWaitNr = 0;

	dbInterface = CAAccountingDBInterface::getConnection();
	if(dbInterface == NULL)
	{
		CAMsg::printMsg(LOG_ERR, "Settlement transaction: could not connect to Database. Retry later...\n");
		return E_NOT_CONNECTED;
	}

	//settlement transactions need to be synchronized globally because the settlement thread as well as all login threads
	//may start a settlement transaction concurrently.
	ms_pInstance->m_pSettlementMutex->lock();

	
	UINT64 iCurrentSettleTransactionNr = (++m_iCurrentSettleTransactionNr) % 50;

	/* First part get unsettled CCs from the AI database */
	#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: DB connections established!\n");
	#endif

		dbInterface->getUnsettledCostConfirmations(&allUnsettledCCs, ms_pInstance->m_currentCascade, &nrOfCCs, MAX_SETTLED_CCS);
		*nrOfSettledCCs = nrOfCCs;
		//no unsettled CCs found.
		if (allUnsettledCCs == NULL)
		{
			CAMsg::printMsg(LOG_INFO, "Settlement transaction %llu: looked for unsettled CCs, found no CCs to settle\n", iCurrentSettleTransactionNr);
			ms_pInstance->m_pSettlementMutex->unlock();
			ret = E_SUCCESS;
			goto cleanup;
		}


		CAMsg::printMsg(LOG_DEBUG, "Settlement transaction %llu: looked for unsettled CCs, found %u cost confirmations to settle\n", iCurrentSettleTransactionNr,  nrOfCCs);

		//connect to the PI
		biConnectionStatus = ms_pInstance->m_pPiInterface->initBIConnection();
		if(biConnectionStatus != E_SUCCESS)
		{
			ms_pInstance->m_seqBIConnErrors++;
			if(ms_pInstance->m_seqBIConnErrors >= CRITICAL_SUBSEQUENT_BI_CONN_ERRORS)
			{
				//transition to critical BI conn payment state
				MONITORING_FIRE_PAY_EVENT(ev_pay_biConnectionCriticalSubseqFailures);
			}
			else
			{
				MONITORING_FIRE_PAY_EVENT(ev_pay_biConnectionFailure);
			}
			ms_pInstance->m_pPiInterface->terminateBIConnection(); // make sure the socket is closed
			ms_pInstance->m_pSettlementMutex->unlock();

			CAMsg::printMsg(LOG_WARNING, "Settlement transaction: could not connect to BI. Try later...\n");
			//ret = E_SUCCESS;
			ret = E_NOT_CONNECTED;
			goto cleanup;
		}
		else
		{
			pErrMsgs = ms_pInstance->m_pPiInterface->settleAll(allUnsettledCCs, nrOfCCs, &settleException);
			ms_pInstance->m_pPiInterface->terminateBIConnection();
		}

	//workaround because usage of exceptions is not allowed!
	if(settleException != NULL)
	{
		ms_pInstance->m_pSettlementMutex->unlock();
		CAMsg::printMsg(LOG_ERR, "Settlement transaction: BI reported settlement not successful: "
						"code: %i, %s \n", settleException->getErrorCode(),
							(settleException->getDescription() != NULL ?
								settleException->getDescription() : (UINT8*) "<no description given>") );
		ret = E_UNKNOWN;
		goto cleanup;
	}

	if(pErrMsgs == NULL)
	{
		//this must never happen because in any case where NULL is returned for pErrMsgs
		//'settleException' must not be NULL.
		ms_pInstance->m_pSettlementMutex->unlock();
		CAMsg::printMsg(LOG_CRIT, "Settlement transaction: ErrorMessages are null.\n");
		ret = E_UNKNOWN;
		goto cleanup;
	}
	else
	{
		for(i = 0; i < nrOfCCs; i++)
		{
			nextEntry = __handleSettleResult(allUnsettledCCs[i], pErrMsgs[i], dbInterface, iCurrentSettleTransactionNr);
			if(nextEntry != NULL)
			{
				nextEntry->nextEntry = entry;
				entry = nextEntry;
			}
		}
	}

	//after settling: apply the corresponding account state changes reported from the PI.
	if (entry)
	{
		//wait numbers are used to obtain a synchronisation with FCFS assertion.
		//This additional synchronisation is necessary because before acquiring the login hashtable
		//locks the settlementMutex should be released. (Nested locking should be avoided).
		if(ms_pInstance->m_nextSettleNr == ms_pInstance->m_settleWaitNr)
		{
			//CAMsg::printMsg(LOG_INFO, "Thread %x: resetting the wait numbers.\n", pthread_self() );
			//no one is waiting, we use this occasion to reset the wait numbers
			ms_pInstance->m_nextSettleNr = 0;
			ms_pInstance->m_settleWaitNr = 1;
		}
		else
		{
			//get global wait number and wait but release the DBConnection first.
			CAAccountingDBInterface::releaseConnection(dbInterface);
			dbInterface = NULL;
			myWaitNr = ms_pInstance->m_settleWaitNr;
			ms_pInstance->m_settleWaitNr++;
			while(myWaitNr != ms_pInstance->m_nextSettleNr)
			{
				CAMsg::printMsg(LOG_INFO, "Thread %x must wait to alter login table after settling: %llu before him in the queue\n", pthread_self(),
										(myWaitNr - ms_pInstance->m_nextSettleNr));
				ms_pInstance->m_pSettlementMutex->wait();

			}
			CAMsg::printMsg(LOG_INFO, "Thread %x may continue.\n", pthread_self());
			dbInterface = CAAccountingDBInterface::getConnection();
		}

		//first apply changes to the database ...
		__commitSettlementToDatabase(entry, dbInterface);
		CAAccountingDBInterface::releaseConnection(dbInterface);
		dbInterface = NULL;
		ms_pInstance->m_pSettlementMutex->unlock();

		//... then to the login hashtable after releasing the global settlement lock.
		ms_pInstance->m_currentAccountsHashtable->getMutex()->lock();
		#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "Settlement thread with wait nr %llu alters hashtable.\n", myWaitNr);
		#endif
		__commitSettlementToLoginTable(entry);
		ms_pInstance->m_currentAccountsHashtable->getMutex()->unlock();

		//indicate that the next thread may proceed with its settlement changes by incrementing the nextSettleNr.
		ms_pInstance->m_pSettlementMutex->lock();
		ms_pInstance->m_nextSettleNr++;
		if(ms_pInstance->m_settleWaitNr != ms_pInstance->m_nextSettleNr)
		{
			//There are threads waiting for applying their settlement results.
			CAMsg::printMsg(LOG_INFO, "Thread %x Waking up next Thread %llu are waiting.\n", pthread_self(),
							(ms_pInstance->m_settleWaitNr - ms_pInstance->m_nextSettleNr));
			ms_pInstance->m_pSettlementMutex->broadcast();
		}
	}
	ms_pInstance->m_pSettlementMutex->unlock();

cleanup:
	if(dbInterface != NULL)
	{
		CAAccountingDBInterface::releaseConnection(dbInterface);
		dbInterface = NULL;
	}

	if(allUnsettledCCs != NULL)
	{
		for(i = 0; i < nrOfCCs; i++)
		{
			delete allUnsettledCCs[i];
			allUnsettledCCs[i] = NULL;
		}
		delete [] allUnsettledCCs;
		allUnsettledCCs = NULL;
	}

	if(pErrMsgs != NULL)
	{
		for(i = 0; i < nrOfCCs; i++)
		{
			delete pErrMsgs[i];
			pErrMsgs[i] = NULL;
		}
		pErrMsgs = NULL;
	}

	while (entry != NULL)
	{
		nextEntry = entry->nextEntry;
		delete entry;
		entry = nextEntry;
	}

	delete settleException;
	settleException = NULL;

	
	CAMsg::printMsg(LOG_INFO, "Settlement transaction %llu finished.\n", iCurrentSettleTransactionNr);

	return ret;
}

SettleEntry *CAAccountingInstance::__handleSettleResult(CAXMLCostConfirmation *pCC, CAXMLErrorMessage *pErrMsg, CAAccountingDBInterface *dbInterface, 
	UINT64 a_iCurrentSettleTransactionNr)
{
	bool bDeleteCC = false;
	UINT32 authFlags = 0;
	UINT32 authRemoveFlags = 0;
	UINT64 confirmedBytes = 0;
	UINT64 diffBytes = 0;
	UINT32 storedStatus = 0;
	SettleEntry *entry = NULL;

	dbInterface->getAccountStatus(pCC->getAccountNumber(), storedStatus);

	// check returncode
	if(pErrMsg == NULL)  //no returncode -> connection error
	{
		authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC; // no fault of the client
		CAMsg::printMsg(LOG_ERR, "Settlement transaction: Communication with BI failed!\n");
	}
	else if(pErrMsg->getErrorCode() != pErrMsg->ERR_OK)  //BI reported error
	{
		CAMsg::printMsg(LOG_ERR, "Settlement transaction: BI reported error no. %d (%s)\n",
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
				UINT8 tmp2[32];
				print64(tmp2, diffBytes);
				UINT8 tmp3[32];
				print64(tmp3, pCC->getTransferredBytes());
				CAMsg::printMsg(LOG_INFO, "Settlement transaction: Received %s confirmed bytes and %s diff bytes while having %s transferred bytes!\n", tmp, tmp2, tmp3);
			}
			else
			{
				CAMsg::printMsg(LOG_INFO, "Settlement transaction: Account empty, no message object received. User will be kicked out.\n");
			}

			dbInterface->storeAccountStatus(pCC->getAccountNumber(), CAXMLErrorMessage::ERR_ACCOUNT_EMPTY);
			authFlags |= AUTH_SETTLED_ONCE;
			dbInterface->markAsSettled(pCC->getAccountNumber(), ms_pInstance->m_currentCascade,
										pCC->getTransferredBytes());
//#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: settling %llu bytes for account %llu\n",
										pCC->getTransferredBytes(), pCC->getAccountNumber());
//#endif
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
				CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: tried outdated CC, received last valid CC back\n");
				//store it in DB
				if (dbInterface->storeCostConfirmation(*attachedCC,
						ms_pInstance->m_currentCascade) == E_SUCCESS)
				{
					authFlags |= AUTH_SETTLED_ONCE;
					if (dbInterface->markAsSettled(attachedCC->getAccountNumber(), ms_pInstance->m_currentCascade,
											attachedCC->getTransferredBytes()) != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_ERR, "Settlement transaction: Could not mark last valid CC as settled."
							"Maybe a new CC has been added meanwhile?\n");
					}
				}
				else
				{
					CAMsg::printMsg(LOG_ERR, "Settlement transaction: storing last valid CC in db failed!\n");
				}
				// set the confirmed bytes to the value of the CC got from the PI
				confirmedBytes = attachedCC->getTransferredBytes();
			}
			else
			{
				CAMsg::printMsg(LOG_INFO, "Settlement transaction: Did not receive last valid CC - maybe old Payment instance?\n");
			}
		}
		else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_BLOCKED)
		{
			authFlags |= AUTH_BLOCKED;
			bDeleteCC = true;

			dbInterface->storeAccountStatus(pCC->getAccountNumber(), CAXMLErrorMessage::ERR_BLOCKED);
		}

		else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_DATABASE_ERROR)
		{
			//authFlags |= AUTH_DATABASE;
			// the user is not responsible for this!
			authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC;
		}
		else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR ||
				 pErrMsg->getErrorCode() ==	CAXMLErrorMessage::ERR_SUCCESS_BUT_WITH_ERRORS)
		{
			// kick out the user and store the CC
			authFlags |= AUTH_UNKNOWN;
		}
		else
		{
			// an unknown error leads to user kickout
			CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: Setting unknown kickout error no. %d.\n", pErrMsg->getErrorCode());
			authFlags |= AUTH_UNKNOWN;
			bDeleteCC = true;
		}

		if (bDeleteCC)
		{
			//delete costconfirmation to avoid trying to settle an unusable CC again and again
			if(dbInterface->deleteCC(pCC->getAccountNumber(), ms_pInstance->m_currentCascade) == E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR, "Settlement transaction: unusable cost confirmation was deleted\n");
			}
			else
			{
				CAMsg::printMsg(LOG_ERR, "Settlement transaction: cost confirmation is unusable, but could not delete it from database\n");
			}
		}
	}
	else //settling was OK, so mark account as settled
	{
		authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC;
		authFlags |= AUTH_SETTLED_ONCE;
		if (dbInterface->markAsSettled(pCC->getAccountNumber(),
								ms_pInstance->m_currentCascade,
								pCC->getTransferredBytes()) != E_SUCCESS)
		 {
			CAMsg::printMsg(LOG_ERR, "Settlement transaction: Could not mark CC as settled. Maybe a new CC has been added meanwhile?\n");
		 }
#ifdef DEBUG
		CAMsg::printMsg(LOG_INFO, "Settlement transaction %llu: CC OK for account %llu with %llu transferred bytes!\n", 
			a_iCurrentSettleTransactionNr, pCC->getAccountNumber(), pCC->getTransferredBytes());
#endif
	}

	if (authFlags || authRemoveFlags)
	{
		entry = new SettleEntry;
		entry->accountNumber = pCC->getAccountNumber();
		entry->authFlags = authFlags;
		entry->authRemoveFlags = authRemoveFlags;
		entry->confirmedBytes = confirmedBytes;
		entry->diffBytes = diffBytes;
		entry->storedStatus = storedStatus;
	}
	return entry;
}

/** only for internal use during the settleTransaction because the global settlement lock is not acquired */
void CAAccountingInstance::__commitSettlementToDatabase(SettleEntry *entryList, CAAccountingDBInterface *dbInterface)
{
	SettleEntry *entry = entryList, *nextEntry = NULL;
	while (entry != NULL && dbInterface != NULL)
	{
		if (entry->authFlags & (AUTH_INVALID_ACCOUNT | AUTH_UNKNOWN))
		{
			dbInterface->storePrepaidAmount(
					entry->accountNumber, 0, ms_pInstance->m_currentCascade);
		}
		else if (entry->diffBytes)
		{
			// user is currently not logged in; set correct prepaid bytes in DB
			SINT32 prepaidBytes =
				dbInterface->getPrepaidAmount(entry->accountNumber,
						ms_pInstance->m_currentCascade, true);
			if (prepaidBytes > 0)
			{
				if (entry->diffBytes < 0 || entry->diffBytes >= (UINT64)prepaidBytes)
				{
					prepaidBytes = 0;
				}
				else
				{
					prepaidBytes -= entry->diffBytes;
				}
				dbInterface->storePrepaidAmount(
					entry->accountNumber, prepaidBytes, ms_pInstance->m_currentCascade);
			}
		}

		if( !(entry->authFlags & (AUTH_ACCOUNT_EMPTY | AUTH_BLOCKED | AUTH_INVALID_ACCOUNT)) &&
			entry->storedStatus != 0)
		{
			dbInterface->clearAccountStatus(entry->accountNumber);
			switch (entry->storedStatus)
			{
				case CAXMLErrorMessage::ERR_ACCOUNT_EMPTY:
				{
					CAMsg::printMsg(LOG_INFO, "Settlement transaction: clearing account empty status for %llu\n",
							entry->accountNumber);
					entry->authRemoveFlags |= AUTH_ACCOUNT_EMPTY;
					break;
				}
				case CAXMLErrorMessage::ERR_BLOCKED:
				{
					CAMsg::printMsg(LOG_INFO, "Settlement transaction: clearing account blocked status for %llu\n",
							entry->accountNumber);
					entry->authRemoveFlags |= AUTH_BLOCKED;
					break;
				}
				case CAXMLErrorMessage::ERR_KEY_NOT_FOUND:
				{
					CAMsg::printMsg(LOG_INFO, "Settlement transaction: clearing account invalid status for %llu\n",
							entry->accountNumber);
					entry->authRemoveFlags |= AUTH_INVALID_ACCOUNT;
					break;
				}
			}
		}
		nextEntry = entry->nextEntry;
		entry = nextEntry;
	}
}

/** only for internal use during the settleTransaction because no login table locks are acquired */
void CAAccountingInstance::__commitSettlementToLoginTable(SettleEntry *entryList)
{
	SettleEntry *entry = entryList, *nextEntry = NULL;
	while (entry != NULL)
	{
		AccountLoginHashEntry* loginEntry =
						(AccountLoginHashEntry*) (ms_pInstance->m_currentAccountsHashtable->getValue(&(entry->accountNumber)));
		if (loginEntry)
		{
			// the user is currently logged in
			loginEntry->authFlags |= entry->authFlags;
			loginEntry->authRemoveFlags |= entry->authRemoveFlags;
			if (entry->confirmedBytes > 0 &&
				loginEntry->confirmedBytes < entry->confirmedBytes)
			{
				loginEntry->confirmedBytes = entry->confirmedBytes;
			}
		}
		nextEntry = entry->nextEntry;
		entry = nextEntry;
	}
}

/* Settlement transaction procedure as it is called by the SettleThread
 * NEVER INVOKE THIS METHOD IF YOU'RE HOLDING A LOCK ON m_currentAccountsHashtable !!
 * */
SINT32 CAAccountingInstance::settlementTransaction()
{
	INIT_STACK;
	BEGIN_STACK("CAAccountingInstance::settlementTransaction");

	SINT32 ret = 0;
	CAXMLErrorMessage * pErrMsg = NULL;
	CAXMLCostConfirmation * pCC = NULL;
	//CAQueue q;
	CAXMLCostConfirmation **allUnsettledCCs = NULL;
	UINT32 /*size, qSize, */ nrOfCCs, i;
	SettleEntry *entry = NULL, *nextEntry = NULL;

	CAAccountingDBInterface *dbInterface = NULL;

	/* This should never happen */
	if(ms_pInstance == NULL)
	{
		FINISH_STACK("CAAccountingInstance::settlementTransaction");
		return E_UNKNOWN;
	}
	/* This should never happen */
	if(ms_pInstance->m_pSettlementMutex == NULL)
	{
		FINISH_STACK("CAAccountingInstance::settlementTransaction");
		return E_UNKNOWN;
	}
	//sleep(5);
	dbInterface = CAAccountingDBInterface::getConnection();
	if(dbInterface == NULL)
	{
		CAMsg::printMsg(LOG_ERR, "Settlement transaction: could not connect to Database. Retry later...\n");
		//MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
		FINISH_STACK("CAAccountingInstance::settlementTransaction");
		return E_NOT_CONNECTED;
	}
	//MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
	ms_pInstance->m_pSettlementMutex->lock();
	/* First part get unsettled CCs from the AI database */
	#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: DB connections established!\n");
	#endif

	dbInterface->getUnsettledCostConfirmations(&allUnsettledCCs, ms_pInstance->m_currentCascade, &nrOfCCs, MAX_SETTLED_CCS);
	if (nrOfCCs <= 0)
	{
		CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: finished gettings CCs, found no CCs to settle\n");
		if(dbInterface != NULL)
		{
			CAAccountingDBInterface::releaseConnection(dbInterface);
			dbInterface = NULL;
		}
		ms_pInstance->m_pSettlementMutex->unlock();
		FINISH_STACK("CAAccountingInstance::settlementTransaction");
		return E_SUCCESS;
	}
	//qSize = q.getSize();
	//nrOfCCs = qSize / sizeof(pCC);
	CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: finished gettings CCs, found %u cost confirmations to settle\n",nrOfCCs);

	SAVE_STACK("CAAccountingInstance::settlementTransaction", "After getting unsettled CCs");
	/* Second part: We found unsettled CCs. Now contact the Payment Instance to settle them */

	//while(!q.isEmpty())
	for(i = 0; i < nrOfCCs; i++)
	{
		// get the next CC from the queue
		/*size = sizeof(pCC);
		ret = q.get((UINT8*)(&pCC), &size);
		if(ret != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_ERR, "Settlement transaction: could not get next item from queue\n");
			q.clean();
			if(dbInterface != NULL)
			{
				CAAccountingDBInterface::releaseConnection(dbInterface);
				dbInterface = NULL;
			}
			ms_pInstance->m_pSettlementMutex->unlock();
			FINISH_STACK("CAAccountingInstance::settlementTransaction");
			return ret;
			//break;
		}*/
		pCC = allUnsettledCCs[i];
		entry = NULL;
		nextEntry = NULL;
		if (!pCC)
		{
			CAMsg::printMsg(LOG_CRIT, "Settlement transaction: Cost confirmation is NULL!\n");
			continue;
		}

#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: Connecting to payment instance...\n");
#endif
		ret = ms_pInstance->m_pPiInterface->initBIConnection();
		if(ret != E_SUCCESS)
		{
			ms_pInstance->m_seqBIConnErrors++;
			if(ms_pInstance->m_seqBIConnErrors >= CRITICAL_SUBSEQUENT_BI_CONN_ERRORS)
			{
				//transition to critical BI conn payment state
				MONITORING_FIRE_PAY_EVENT(ev_pay_biConnectionCriticalSubseqFailures);
			}
			else
			{
				MONITORING_FIRE_PAY_EVENT(ev_pay_biConnectionFailure);
			}
			CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: could not connect to BI. Try later...\n");
			//q.clean();
			pErrMsg = NULL; // continue in order to tell AUTH_WAITING_FOR_FIRST_SETTLED_CC for all accounts
			ms_pInstance->m_pPiInterface->terminateBIConnection(); // make sure the socket is closed
		}
		else
		{
			MONITORING_FIRE_PAY_EVENT(ev_pay_biConnectionSuccess);
#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: successfully connected to payment instance");
#endif
			ms_pInstance->m_seqBIConnErrors = 0;
			pErrMsg = ms_pInstance->m_pPiInterface->settle( *pCC );
			ms_pInstance->m_pPiInterface->terminateBIConnection();
			CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: settle done!\n");
		}

		bool bDeleteCC = false;
		UINT32 authFlags = 0;
		UINT32 authRemoveFlags = 0;
		UINT64 confirmedBytes = 0;
		UINT64 diffBytes = 0;
		UINT32 storedStatus = 0;

		dbInterface->getAccountStatus(pCC->getAccountNumber(), storedStatus);

		// check returncode
		if(pErrMsg == NULL)  //no returncode -> connection error
		{
			authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC; // no fault of the client
			CAMsg::printMsg(LOG_ERR, "Settlement transaction: Communication with BI failed!\n");
		}
		else if(pErrMsg->getErrorCode() != pErrMsg->ERR_OK)  //BI reported error
		{
			CAMsg::printMsg(LOG_ERR, "Settlement transaction: BI reported error no. %d (%s)\n",
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
					UINT8 tmp2[32];
					print64(tmp2, diffBytes);
					UINT8 tmp3[32];
					print64(tmp3, pCC->getTransferredBytes());
					CAMsg::printMsg(LOG_INFO, "Settlement transaction: Received %s confirmed bytes and %s diff bytes while having %s transferred bytes!\n", tmp, tmp2, tmp3);
				}
				else
				{
					CAMsg::printMsg(LOG_ERR, "Settlement transaction: Account empty, but no message object received! "
							"This may lead to too much prepaid bytes!\n");
				}

				dbInterface->storeAccountStatus(pCC->getAccountNumber(), CAXMLErrorMessage::ERR_ACCOUNT_EMPTY);
				authFlags |= AUTH_SETTLED_ONCE;
				dbInterface->markAsSettled(pCC->getAccountNumber(), ms_pInstance->m_currentCascade,
											pCC->getTransferredBytes());
//#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: settling %llu bytes for account %llu\n",
											pCC->getTransferredBytes(), pCC->getAccountNumber());
//#endif
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
					CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: tried outdated CC, received last valid CC back\n");
					//store it in DB
					if (dbInterface->storeCostConfirmation(*attachedCC,
							ms_pInstance->m_currentCascade) == E_SUCCESS)
					{
						authFlags |= AUTH_SETTLED_ONCE;
						if (dbInterface->markAsSettled(attachedCC->getAccountNumber(), ms_pInstance->m_currentCascade,
												attachedCC->getTransferredBytes()) != E_SUCCESS)
						{
							CAMsg::printMsg(LOG_ERR, "Settlement transaction: Could not mark last valid CC as settled."
								"Maybe a new CC has been added meanwhile?\n");
						}
					}
					else
					{
						CAMsg::printMsg(LOG_ERR, "Settlement transaction: storing last valid CC in db failed!\n");
					}
					// set the confirmed bytes to the value of the CC got from the PI
					confirmedBytes = attachedCC->getTransferredBytes();
				}
				else
				{
					CAMsg::printMsg(LOG_INFO, "Settlement transaction: Did not receive last valid CC - maybe old Payment instance?\n");
				}
			}
			else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_BLOCKED)
			{
				authFlags |= AUTH_BLOCKED;
				bDeleteCC = true;

				dbInterface->storeAccountStatus(pCC->getAccountNumber(), CAXMLErrorMessage::ERR_BLOCKED);
			}

			else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_DATABASE_ERROR)
			{
				//authFlags |= AUTH_DATABASE;
				// the user is not responsible for this!
				authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC;
			}
			else if (pErrMsg->getErrorCode() == CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR ||
			  		 pErrMsg->getErrorCode() ==	CAXMLErrorMessage::ERR_SUCCESS_BUT_WITH_ERRORS)
			{
				// kick out the user and store the CC
				authFlags |= AUTH_UNKNOWN;
			}
			else
			{
				// an unknown error leads to user kickout
				CAMsg::printMsg(LOG_DEBUG, "Settlement transaction: Setting unknown kickout error no. %d.\n", pErrMsg->getErrorCode());
				authFlags |= AUTH_UNKNOWN;
				bDeleteCC = true;
			}

			if (bDeleteCC)
			{
				//delete costconfirmation to avoid trying to settle an unusable CC again and again
				if(dbInterface->deleteCC(pCC->getAccountNumber(), ms_pInstance->m_currentCascade) == E_SUCCESS)
				{
					CAMsg::printMsg(LOG_ERR, "Settlement transaction: unusable cost confirmation was deleted\n");
				}
				else
				{
					CAMsg::printMsg(LOG_ERR, "Settlement transaction: cost confirmation is unusable, but could not delete it from database\n");
				}
			}
		}
		else //settling was OK, so mark account as settled
		{
			authRemoveFlags |= AUTH_WAITING_FOR_FIRST_SETTLED_CC;
			authFlags |= AUTH_SETTLED_ONCE;
			if (dbInterface->markAsSettled(pCC->getAccountNumber(),
									ms_pInstance->m_currentCascade,
									pCC->getTransferredBytes()) != E_SUCCESS)
			 {
			 	CAMsg::printMsg(LOG_ERR, "Settlement transaction: Could not mark CC as settled. Maybe a new CC has been added meanwhile?\n");
			 }
			CAMsg::printMsg(LOG_INFO, "Settlement transaction: CC OK!\n");
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
			nextEntry->storedStatus = storedStatus;
			entry = nextEntry;
		}

		/*if (pCC != NULL)
		{
			delete pCC;
			pCC = NULL;
		}*/
		if (pErrMsg != NULL)
		{
			delete pErrMsg;
			pErrMsg = NULL;
		}
	}

	if(allUnsettledCCs != NULL)
	{
		for(i = 0; i < nrOfCCs; i++)
		{
			delete allUnsettledCCs[i];
			allUnsettledCCs[i] = NULL;
		}
		delete [] allUnsettledCCs;
		allUnsettledCCs = NULL;
	}

	SAVE_STACK("CAAccountingInstance::settlementTransaction", "After settling unsettled CCs with BI");

	SettleEntry *first = entry;
	UINT64 myWaitNr = 0;
	if (entry)
	{
		//wait numbers are used to obtain a synchronisation with FCFS assertion.
		//This additional synchronisation is necessary because before acquiring the login hashtable
		//locks the settlementMutex should be released. (Nested locking should be avoided).
		if(ms_pInstance->m_nextSettleNr == ms_pInstance->m_settleWaitNr)
		{
			//CAMsg::printMsg(LOG_INFO, "Thread %x: resetting the wait numbers.\n", pthread_self() );
			//no one is waiting, we use this occasion to reset the wait numbers
			ms_pInstance->m_nextSettleNr = 0;
			ms_pInstance->m_settleWaitNr = 1;
		}
		else
		{
			SAVE_STACK("CAAccountingInstance::settlementTransaction", "wait for altering hashtable");
			//get global wait number and wait but release the DBConnection first.
			CAAccountingDBInterface::releaseConnection(dbInterface);
			dbInterface = NULL;
			myWaitNr = ms_pInstance->m_settleWaitNr;
			ms_pInstance->m_settleWaitNr++;
			while(myWaitNr != ms_pInstance->m_nextSettleNr)
			{
				CAMsg::printMsg(LOG_INFO, "Thread %x must wait to alter login table after settling: %llu before him in the queue\n", pthread_self(),
										(myWaitNr - ms_pInstance->m_nextSettleNr));
				ms_pInstance->m_pSettlementMutex->wait();

			}
			CAMsg::printMsg(LOG_INFO, "Thread %x may continue.\n", pthread_self());
			dbInterface = CAAccountingDBInterface::getConnection();
		}
		SAVE_STACK("CAAccountingInstance::settlementTransaction", "altering DB entries");
		while (entry != NULL && dbInterface != NULL)
		{
			if (entry->authFlags & (AUTH_INVALID_ACCOUNT | AUTH_UNKNOWN))
			{
				dbInterface->storePrepaidAmount(
						entry->accountNumber, 0, ms_pInstance->m_currentCascade);
			}
			else if (entry->diffBytes)
			{
				// user is currently not logged in; set correct prepaid bytes in DB
				SINT32 prepaidBytes =
					dbInterface->getPrepaidAmount(entry->accountNumber,
							ms_pInstance->m_currentCascade, true);
				if (prepaidBytes > 0)
				{
					if (entry->diffBytes < 0 || entry->diffBytes >= (UINT64)prepaidBytes)
					{
						prepaidBytes = 0;
					}
					else
					{
						prepaidBytes -= entry->diffBytes;
					}
					dbInterface->storePrepaidAmount(
						entry->accountNumber, prepaidBytes, ms_pInstance->m_currentCascade);
				}
			}

			if( !(entry->authFlags & (AUTH_ACCOUNT_EMPTY | AUTH_BLOCKED | AUTH_INVALID_ACCOUNT)) &&
				entry->storedStatus != 0)
			{
				dbInterface->clearAccountStatus(entry->accountNumber);
				switch (entry->storedStatus)
				{
					case CAXMLErrorMessage::ERR_ACCOUNT_EMPTY:
					{
						CAMsg::printMsg(LOG_INFO, "Settlement transaction: clearing account empty status for %llu\n",
								entry->accountNumber);
						entry->authRemoveFlags |= AUTH_ACCOUNT_EMPTY;
						break;
					}
					case CAXMLErrorMessage::ERR_BLOCKED:
					{
						CAMsg::printMsg(LOG_INFO, "Settlement transaction: clearing account blocked status for %llu\n",
								entry->accountNumber);
						entry->authRemoveFlags |= AUTH_BLOCKED;
						break;
					}
					case CAXMLErrorMessage::ERR_KEY_NOT_FOUND:
					{
						CAMsg::printMsg(LOG_INFO, "Settlement transaction: clearing account invalid status for %llu\n",
								entry->accountNumber);
						entry->authRemoveFlags |= AUTH_INVALID_ACCOUNT;
						break;
					}
				}
			}

			nextEntry = entry->nextEntry;
			entry = nextEntry;
		}
	}
	CAAccountingDBInterface::releaseConnection(dbInterface);
	dbInterface = NULL;
	ms_pInstance->m_pSettlementMutex->unlock();

	if(first != NULL)
	{
		SAVE_STACK("CAAccountingInstance::settlementTransaction", "altering hashtable");
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "Settlement thread with wait nr %llu alters hashtable.\n", myWaitNr);
#endif
		entry = first;
		ms_pInstance->m_currentAccountsHashtable->getMutex()->lock();
		while (entry != NULL)
		{
			AccountLoginHashEntry* loginEntry =
							(AccountLoginHashEntry*) (ms_pInstance->m_currentAccountsHashtable->getValue(&(entry->accountNumber)));
			if (loginEntry)
			{
				// the user is currently logged in
				loginEntry->authFlags |= entry->authFlags;
				loginEntry->authRemoveFlags |= entry->authRemoveFlags;
				if (entry->confirmedBytes > 0 &&
					loginEntry->confirmedBytes < entry->confirmedBytes)
				{
					loginEntry->confirmedBytes = entry->confirmedBytes;
				}
			}
			nextEntry = entry->nextEntry;
			delete entry;
			entry = nextEntry;
		}
		ms_pInstance->m_currentAccountsHashtable->getMutex()->unlock();

		ms_pInstance->m_pSettlementMutex->lock();
		ms_pInstance->m_nextSettleNr++;
		if(ms_pInstance->m_settleWaitNr != ms_pInstance->m_nextSettleNr)
		{
			SAVE_STACK("CAAccountingInstance::settlementTransaction", "waking up waiting threads for altering hashtable");
			//There are threads waiting for applying their settlement results.
			CAMsg::printMsg(LOG_INFO, "Thread %x Waking up next Thread %llu are waiting.\n", pthread_self(),
							(ms_pInstance->m_settleWaitNr - ms_pInstance->m_nextSettleNr));
			ms_pInstance->m_pSettlementMutex->broadcast();
		}
		ms_pInstance->m_pSettlementMutex->unlock();
	}

	/*if(dbInterface != NULL)
	{
		CAAccountingDBInterface::releaseConnection(dbInterface);
		dbInterface = NULL;
	}*/
	FINISH_STACK("CAAccountingInstance::settlementTransaction");
	return E_SUCCESS;
}

/**
 * if the current login entry isn't locked by a login thread
 * the calling login-thread obtains ownership
 */
bool testAndSetLoginOwner(struct AccountLoginHashEntry *loginEntry, fmHashTableEntry *ownerRef)
{
	bool success = false;
	loginEntry->ownerLock->lock();
	success = !loginEntry->loginOngoing;
	if(success)
	{
		//CAMsg::printMsg(LOG_DEBUG,"login free, owner %x \n", loginEntry->ownerRef);
		loginEntry->ownerRef = ownerRef;
		loginEntry->loginOngoing = true;
	}
	/*else
	{
		CAMsg::printMsg(LOG_DEBUG,"login ongoing, owner %x \n", loginEntry->ownerRef);
	}*/
	loginEntry->ownerLock->unlock();
	return success;
}

/**
 * release login (particularly for use in error case)
 * this function is thread-safe.
 */
void CAAccountingInstance::unlockLogin(fmHashTableEntry *ownerRef)
{
	if(ownerRef == NULL || ownerRef->pAccountingInfo == NULL)
	{
		return;
	}
	UINT64 accountNumber = ownerRef->pAccountingInfo->accountNumber;
	ms_pInstance->m_currentAccountsHashtable->getMutex()->lock();
	AccountLoginHashEntry *loginEntry = (AccountLoginHashEntry*)ms_pInstance->m_currentAccountsHashtable->getValue(&accountNumber);
	if (loginEntry != NULL)
	{
		resetLoginOngoing(loginEntry, ownerRef);
	}
	ms_pInstance->m_currentAccountsHashtable->getMutex()->unlock();
}

/**
 * indicates that the ongoing login process for this entry is finished but doesn't reset ownership.
 */
bool resetLoginOngoing(struct AccountLoginHashEntry *loginEntry, fmHashTableEntry *ownerRef)
{
	bool success = false;
	loginEntry->ownerLock->lock();
	success = testLoginEntryOwner_internal(loginEntry, ownerRef);
	if(success)
	{
		loginEntry->loginOngoing = false;
		//CAMsg::printMsg(LOG_DEBUG,"resetted for %x.\n", loginEntry->ownerRef);
	}
	/*else
	{
		CAMsg::printMsg(LOG_DEBUG,"not resetted for %x.\n", loginEntry->ownerRef);
	}*/
	loginEntry->ownerLock->unlock();
	return success;
}

/**
 * tests whether the corresponding ownerEntry owns this loginEntry.
 */
bool testLoginEntryOwner(struct AccountLoginHashEntry *loginEntry, fmHashTableEntry *ownerRef)
{
	bool ret;
	loginEntry->ownerLock->lock();
	ret = testLoginEntryOwner_internal(loginEntry, ownerRef);
	loginEntry->ownerLock->unlock();
	return ret;
}

/**
 * test whether this entry is currently resrved by an ongoing login process.
 */
bool isLoginOngoing(struct AccountLoginHashEntry *loginEntry, fmHashTableEntry *ownerRef)
{
	bool ret;
	loginEntry->ownerLock->lock();
	ret = loginEntry->loginOngoing;
	loginEntry->ownerLock->unlock();
	return ret;
}

inline bool testLoginEntryOwner_internal(struct AccountLoginHashEntry *loginEntry, fmHashTableEntry *ownerRef)
{
	return (loginEntry->ownerRef == ownerRef);
}
#endif /* ifdef PAYMENT */
