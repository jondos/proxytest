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

//for testing purposes only
#define JAP_DIGEST_LENGTH 28

extern CACmdLnOptions* pglobalOptions;

DOM_Document CAAccountingInstance::m_preparedCCRequest;

const SINT32 CAAccountingInstance::HANDLE_PACKET_CONNECTION_OK = 1;
const SINT32 CAAccountingInstance::HANDLE_PACKET_CONNECTION_UNCHECKED = 4;
const SINT32 CAAccountingInstance::HANDLE_PACKET_HOLD_CONNECTION = 0;
const SINT32 CAAccountingInstance::HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION = 2;
const SINT32 CAAccountingInstance::HANDLE_PACKET_CLOSE_CONNECTION = 3;

SINT32 CAAccountingInstance::m_prepaidBytesMinimum = 0;


/**
 * Singleton: This is the reference to the only instance of this class
 */
CAAccountingInstance * CAAccountingInstance::ms_pInstance = NULL;

const UINT64 CAAccountingInstance::PACKETS_BEFORE_NEXT_CHECK = 100;

const UINT32 CAAccountingInstance::MAX_TOLERATED_MULTIPLE_LOGINS = 10;

/**
 * private Constructor
 */
CAAccountingInstance::CAAccountingInstance(CAMix* callingMix)
	{	
		CAMsg::printMsg( LOG_DEBUG, "AccountingInstance initialising\n" );
		
		m_pMutex = new CAMutex();
		//m_pIPBlockList = new CATempIPBlockList(60000);
		
		// initialize Database connection
		m_dbInterface = new CAAccountingDBInterface();
		if(m_dbInterface->initDBConnection() != E_SUCCESS)
		{
			CAMsg::printMsg( LOG_ERR, "**************** AccountingInstance: Could not connect to DB!\n");
			exit(1);
		}
	
		// initialize JPI signature tester
		m_AiName = new UINT8[256];
		pglobalOptions->getAiID(m_AiName, 256);
		if (pglobalOptions->getBI() != NULL)
		{
			m_pJpiVerifyingInstance = pglobalOptions->getBI()->getVerifier();
		}
		pglobalOptions->getPaymentHardLimit(&m_iHardLimitBytes);
		pglobalOptions->getPaymentSoftLimit(&m_iSoftLimitBytes);
	
		prepareCCRequest(callingMix, m_AiName);
		
		m_currentAccountsHashtable = 
			new Hashtable((UINT32 (*)(void *))Hashtable::hashUINT64, (SINT32 (*)(void *,void *))Hashtable::compareUINT64, 2000);		
		
		// launch BI settleThread				
		m_pSettleThread = new CAAccountingSettleThread(m_currentAccountsHashtable, m_currentCascade);
		
		m_aiThreadPool = new CAThreadPool(NUM_LOGIN_WORKER_TRHEADS, MAX_LOGIN_QUEUE, false);
	}
		

/**
 * private destructor
 */
CAAccountingInstance::~CAAccountingInstance()
	{
		INIT_STACK;
		BEGIN_STACK("~CAAccountingInstance");
		
		m_pMutex->lock();
		
		CAMsg::printMsg( LOG_DEBUG, "AccountingInstance dying\n" );
		if (m_pSettleThread)
		{
			CAMsg::printMsg( LOG_DEBUG, "deleting m_pSettleThread\n" );
			m_pSettleThread->settle();
			delete m_pSettleThread;
		}
		m_pSettleThread = NULL;
		
		if (m_aiThreadPool)
		{
			CAMsg::printMsg( LOG_DEBUG, "deleting m_aiThreadPool\n" );
			delete m_aiThreadPool;
		}
		m_aiThreadPool = NULL;
		
		if (m_dbInterface)
		{
			CAMsg::printMsg( LOG_DEBUG, "termintaing dbConnection\n" );
			m_dbInterface->terminateDBConnection();
			delete m_dbInterface;
		}
		m_dbInterface = NULL;
		//delete m_pIPBlockList;
		//m_pIPBlockList = NULL;
		if (m_AiName)
		{
			delete[] m_AiName;
		}
		m_AiName = NULL;
		
		if (m_currentAccountsHashtable)
		{
			m_currentAccountsHashtable->getMutex()->lock();
			CAMsg::printMsg( LOG_DEBUG, "CAAccountingInstance: Clearing accounts hashtable...\n");
			m_currentAccountsHashtable->clear(HASH_EMPTY_NONE, HASH_EMPTY_DELETE);
			CAMsg::printMsg( LOG_DEBUG, "CAAccountingInstance: Deleting accounts hashtable...\n" );
			m_currentAccountsHashtable->getMutex()->unlock();
			delete m_currentAccountsHashtable;
		}
		m_currentAccountsHashtable = NULL;
		CAMsg::printMsg( LOG_DEBUG, "CAAccountingInstance: Accounts hashtable deleted.\n" );
		
		if (m_currentCascade)
		{
			delete[] m_currentCascade;
		}
		m_currentCascade = NULL;
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
	DOM_Element elem = item->pDomDoc->getDocumentElement();
	
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
		delete item->pAccInfo;
	}

	delete item->pDomDoc;
	delete item;
	
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
		
		//kick user out after previous error
		if(pAccInfo->authFlags & AUTH_FATAL_ERROR)
		{			
			// there was an error earlier.
			if (a_bMessageToJAP && a_bControlMessage)
			{				
				return returnKickout(pAccInfo);
			}
			else
			{							
				pAccInfo->mutex->unlock();
				return HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION; // don't let through messages from JAP
			}
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
		
		/*
		UINT8 tmp[32];
		//print64(tmp,pAccInfo->transferredBytes - m_countTransferred);
		//CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance1: Transferred bytes:%s\n", tmp);	
		
		print64(tmp,pAccInfo->transferredBytes);
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance2: Transferred bytes:%s\n", tmp);
		print64(tmp,pAccInfo->confirmedBytes);
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance3: Confirmed bytes:  %s\n", tmp);	
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance3: Prepaid bytes:    %d\n", getPrepaidBytes(pAccInfo));	
		*/
		
		
		
		// do the following tests after a lot of Mix packets only (gain speed...)
		if (!(pAccInfo->authFlags & (AUTH_HARD_LIMIT_REACHED | AUTH_ACCOUNT_EMPTY | AUTH_WAITING_FOR_FIRST_SETTLED_CC)) &&
			pAccInfo->sessionPackets % PACKETS_BEFORE_NEXT_CHECK != 0)
		{
			//CAMsg::printMsg( LOG_DEBUG, "Now we gain some speed after %d session packets...\n", pAccInfo->sessionPackets);
			pAccInfo->mutex->unlock();
			return HANDLE_PACKET_CONNECTION_UNCHECKED;
		}
	
		
		
		//CAMsg::printMsg( LOG_DEBUG, "Checking after %d session packets...\n", pAccInfo->sessionPackets);
		
		/** @todo We need this trick so that the program does not freeze with active AI ThreadPool!!!! */
		//pAccInfo->mutex->unlock();
			
			
		SAVE_STACK("CAAccountingInstance::handleJapPacket", "before accounts hash");
		if (pAccInfo->authFlags & AUTH_ACCOUNT_OK)
		{
			// this user is authenticated; test if he has logged in more than one time
			
			if (!ms_pInstance->m_currentAccountsHashtable)
			{
				// accounting instance is dying...
				return returnKickout(pAccInfo);
			}
			
			ms_pInstance->m_currentAccountsHashtable->getMutex()->lock();
			loginEntry = (AccountLoginHashEntry*)ms_pInstance->m_currentAccountsHashtable->getValue(&(pAccInfo->accountNumber));
			if (loginEntry)
			{
				pAccInfo->authFlags &= ~loginEntry->authRemoveFlags;
				//CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Remove flag: %d\n", loginEntry->authRemoveFlags);
				
				if (loginEntry->userID != pHashEntry->id)
				{
					// this is not the latest connection of this user; kick him out...
					pAccInfo->authFlags |= AUTH_MULTIPLE_LOGIN;
					ms_pInstance->m_currentAccountsHashtable->getMutex()->unlock();
					return returnPrepareKickout(pAccInfo, new CAXMLErrorMessage(CAXMLErrorMessage::ERR_MULTIPLE_LOGIN, (UINT8*)"Only one login per account is allowed!"));
				}
				else if (loginEntry->authFlags & AUTH_OUTDATED_CC)
				{
					loginEntry->authFlags &= ~AUTH_OUTDATED_CC;	
					
					UINT8 tmp[32];
					print64(tmp,pAccInfo->accountNumber);
					CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Fixing bytes from outdated CC for account %s...\n", tmp);	
					// we had stored an outdated CC; insert confirmed bytes from current CC here and also update client					
					CAXMLCostConfirmation * pCC = NULL;
					bool bSettled;
					ms_pInstance->m_dbInterface->getCostConfirmation(pAccInfo->accountNumber, 
						ms_pInstance->m_currentCascade, &pCC, bSettled);
					if (pCC != NULL)
					{				
						if (bSettled)
						{
							pAccInfo->transferredBytes +=  loginEntry->confirmedBytes - pAccInfo->confirmedBytes;			
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
					}
					else
					{
						CAMsg::printMsg(LOG_ERR, "CAAccountingInstance: Bytes from outdated CC could not be fixed!\n");	
					}																																
				}
				else if (loginEntry->authFlags & AUTH_ACCOUNT_EMPTY)
				{
					// Do not reset the flag, so that the confirmedBytes are always reset.
					//loginEntry->authFlags &= ~AUTH_ACCOUNT_EMPTY; 
					pAccInfo->authFlags |= AUTH_ACCOUNT_EMPTY;
					/* confirmedBytes = 0 leads to immediate disconnection.
					 * If confirmedBytes > 0,  any remaining prepaid bytes may be used.
					 */
					pAccInfo->confirmedBytes = loginEntry->confirmedBytes;
				}
				else if (loginEntry->authFlags & AUTH_INVALID_ACCOUNT)
				{
					loginEntry->authFlags &= ~AUTH_INVALID_ACCOUNT;											
					CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Found invalid account! Kicking out user...\n");																
					err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_KEY_NOT_FOUND);
				}
				else if (loginEntry->authFlags & AUTH_BLOCKED)
				{
					loginEntry->authFlags &= ~AUTH_BLOCKED;										
					CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Kicking out blocked user!\n");																
					err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_BLOCKED);
				}
				else if (loginEntry->authFlags & AUTH_DATABASE)
				{
					loginEntry->authFlags &= ~AUTH_DATABASE;										
					CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Upps - kicking out user due to database error...\n");																
					err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_DATABASE_ERROR);
				}
				else if (loginEntry->authFlags & AUTH_UNKNOWN)
				{
					loginEntry->authFlags &= ~AUTH_UNKNOWN;										
					CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Unknown error! Kicking out user...\n");																
					err = new CAXMLErrorMessage(CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR);
				}
			}
			else
			{
				CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: handleJapPacket did not find user login hash entry!\n");
				//pAccInfo->sessionPackets = 0;
				return returnKickout(pAccInfo);				
			}											
			
			ms_pInstance->m_currentAccountsHashtable->getMutex()->unlock();
		}		

		
		if (pAccInfo->authFlags & AUTH_ACCOUNT_EMPTY)
		{
			// There should be no time limit. The connections is simply closed after all prepaid bytes are gone.
			pAccInfo->lastHardLimitSeconds = time(NULL);
			
			#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Account empty with %d prepaid bytes!\n", getPrepaidBytes(pAccInfo));
			#endif
			
			if (getPrepaidBytes(pAccInfo) <= 0)
			{
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Account empty! Kicking out user...\n");				
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
				returnOK(pAccInfo);
			}
		}
	
		/** @todo We need this trick so that the program does not freeze with active AI ThreadPool!!!! */
		//pAccInfo->mutex->lock();
		
		SAVE_STACK("CAAccountingInstance::handleJapPacket", "before err");
		
		if (err)
		{						
			return returnPrepareKickout(pAccInfo, err);		
		}
	
		//CAMsg::printMsg(LOG_INFO, "CAAccountingInstance: handleJapPacket auth for account %s.\n", accountNrAsString);
	
		if (!(pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT))
		{
			//dont let the packet through for now, but still wait for an account cert
			if (!(pAccInfo->authFlags & AUTH_TIMEOUT_STARTED))						
			{
				pAccInfo->authFlags |= AUTH_TIMEOUT_STARTED;
				pAccInfo->authTimeoutStartSeconds = time(NULL);
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Wait for account certificate: %u\n", pAccInfo->authTimeoutStartSeconds);
			}
			
			//kick user out if a timeout was set and has since run out
			if (time(NULL) > pAccInfo->authTimeoutStartSeconds + AUTH_TIMEOUT)
			{
	//#ifdef DEBUG							
				CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Auth timeout has runout, will kick out user now...\n");
	//#endif											
				return returnPrepareKickout(pAccInfo, new CAXMLErrorMessage(CAXMLErrorMessage::ERR_NO_ACCOUNTCERT));				
			}						
					
			pAccInfo->mutex->unlock();
			return HANDLE_PACKET_HOLD_CONNECTION;
		}
		else 
		{									
			if(pAccInfo->authFlags & AUTH_FAKE )
			{
				// authentication process not properly finished
#ifdef DEBUG				
				CAMsg::printMsg( LOG_DEBUG, "AccountingInstance: AUTH_FAKE flag is set ... byebye\n");
#endif				
				return returnPrepareKickout(pAccInfo, new CAXMLErrorMessage(CAXMLErrorMessage::ERR_BAD_SIGNATURE, (UINT8*)"Your account certificate is invalid"));
			}
			/*
			else if (pAccInfo->authFlags & AUTH_MULTIPLE_LOGIN)
			{
				return returnPrepareKickout(pAccInfo, new CAXMLErrorMessage(CAXMLErrorMessage::ERR_MULTIPLE_LOGIN, (UINT8*)"Only one login per account is allowed!"));
			}*/
			if( !(pAccInfo->authFlags & AUTH_ACCOUNT_OK) )
			{
				// we did not yet receive the response to the challenge...
				if(time(NULL) >= pAccInfo->challengeSentSeconds + CHALLENGE_TIMEOUT)
				{
					CAMsg::printMsg( LOG_DEBUG, "AccountingInstance: Jap refused to send response to challenge (Request Timeout)...\n");
					return returnPrepareKickout(pAccInfo, new CAXMLErrorMessage(CAXMLErrorMessage::ERR_NO_ACCOUNTCERT)); //timeout over -> kick out
				}
				else //timeout still running
				{
					pAccInfo->mutex->unlock();
					// do not forward any traffic from JAP
					return HANDLE_PACKET_HOLD_CONNECTION;
				}					
			}			

			//----------------------------------------------------------
			// ******     Hardlimit cost confirmation check **********
			//counting unconfirmed bytes is not necessary anymore, since we deduct from prepaid bytes
			//UINT32 unconfirmedBytes=diff64(pAccInfo->transferredBytes,pAccInfo->confirmedBytes);
			
			//confirmed and transferred bytes are cumulative, so they use UINT64 to store potentially huge values
			//prepaid Bytes as the difference will be much smaller, but might be negative, so we cast to signed int
			SINT32 prepaidBytes = getPrepaidBytes(pAccInfo);
			
			/*
			if (prepaidBytes < m_prepaidBytesMinimum)
			{
				// this is only for logging the typical client behaviour in confirming prepaid bytes
				m_prepaidBytesMinimum = prepaidBytes;
				CAMsg::printMsg(LOG_INFO, 
					"CAAccountingInstance: New PrepaidBytes minimum: %d\n", m_prepaidBytesMinimum);
			}*/
			 
			//CAMsg::printMsg(LOG_ERR, "CAAccountingInstance: Prepaid bytes: %d!\n", prepaidBytes);
				
			if (prepaidBytes < 0 ||  prepaidBytes <= ms_pInstance->m_iHardLimitBytes)
			{	
				UINT32 prepaidInterval;
				pglobalOptions->getPrepaidInterval(&prepaidInterval);
							
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
				
				if (time(NULL) >= pAccInfo->lastHardLimitSeconds + HARD_LIMIT_TIMEOUT ||
					(prepaidBytes < 0 && (UINT32)(prepaidBytes * (-1)) >= prepaidInterval))
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
					pAccInfo->mutex->unlock();
					// do not forward any traffic from JAP
					return HANDLE_PACKET_HOLD_CONNECTION;
				}
			}
			else
			{
				pAccInfo->authFlags &= ~AUTH_HARD_LIMIT_REACHED;
			}

			//-------------------------------------------------------
			// *** SOFT LIMIT CHECK *** is it time to request a new cost confirmation?
			if ( prepaidBytes < 0 || prepaidBytes <= ms_pInstance->m_iSoftLimitBytes )
			{
#ifdef DEBUG
				CAMsg::printMsg(LOG_ERR, "soft limit of %d bytes triggered \n",ms_pInstance->m_iSoftLimitBytes);
#endif						
				if( (pAccInfo->authFlags & AUTH_SENT_CC_REQUEST) )
				{//we have sent a first CC request
					//still waiting for the answer to the CC reqeust
					//CAMsg::printMsg(LOG_ERR, "Waiting for CC request for account nr %u\n", pAccInfo->accountNumber);
					return returnOK(pAccInfo);
				}//we have sent a CC request
				// no CC request sent yet --> send a first CC request
				
				//send CC to jap
				sendCCRequest(pAccInfo);
						
				return returnOK(pAccInfo);
			}// end of soft limit exceeded

			//everything is fine! let the packet pass thru
			return returnOK(pAccInfo);
		} //end of AUTH_GOT_ACCOUNTCERT		
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
	
	SINT32 prepaidBytes;
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
			UINT32 prepaidInterval;
			pglobalOptions->getPrepaidInterval(&prepaidInterval);
			prepaidBytes = (SINT32)prepaidInterval;
			pAccInfo->transferredBytes = pAccInfo->confirmedBytes - prepaidInterval;
		}
	}
	else
	{
		prepaidBytes = pAccInfo->transferredBytes - pAccInfo->confirmedBytes;
		prepaidBytes *= -1;
	}	
	
	return prepaidBytes;
}


/**
 * everything is fine, let the packet pass
 */
SINT32 CAAccountingInstance::returnOK(tAiAccountingInfo* pAccInfo)
{
	SINT32 ret;
	if (pAccInfo->authFlags & AUTH_WAITING_FOR_FIRST_SETTLED_CC)
	{
		//CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Still no settled CC...\n");
		// it is not yet sure whether this user has a charged account
		ret = HANDLE_PACKET_HOLD_CONNECTION;
	}
	else
	{
		ret = HANDLE_PACKET_CONNECTION_OK;
	}
	pAccInfo->mutex->unlock();
	
	return ret;	
}


/**
 *  When receiving this message, the Mix should kick the user out immediately
 */
SINT32 CAAccountingInstance::returnKickout(tAiAccountingInfo* pAccInfo)
{
	UINT8 tmp[32];
	print64(tmp,pAccInfo->accountNumber);
	CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: should kick out user with account %s now...\n", tmp);	
	pAccInfo->transferredBytes = pAccInfo->confirmedBytes;
	pAccInfo->mutex->unlock();
	return HANDLE_PACKET_CLOSE_CONNECTION;
}

/*
 * hold packet, no timeout started 
 * (Usage: send an error message before kicking out the user:
 * sets AUTH_FATAL_ERROR )
 */
SINT32 CAAccountingInstance::returnPrepareKickout(tAiAccountingInfo* pAccInfo, CAXMLErrorMessage* a_error)
{
	pAccInfo->authFlags |= AUTH_FATAL_ERROR;
	
	if (a_error)
	{
		//CAMsg::printMsg(LOG_CRIT, "AccountingInstance: Sending error message...\n");
		DOM_Document doc;												
		a_error->toXmlDocument(doc);			
		delete a_error;
		//pAccInfo->sessionPackets = 0; // allow some pakets to pass by to send the control message
		pAccInfo->pControlChannel->sendXMLMessage(doc);		
	}
	else
	{
		CAMsg::printMsg(LOG_CRIT, "AccountingInstance: Should send error message, but none is available!\n");
	}
	
	pAccInfo->mutex->unlock();
	return HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION;
}


SINT32 CAAccountingInstance::sendCCRequest(tAiAccountingInfo* pAccInfo)
{
	//INIT_STACK;
	//BEGIN_STACK("CAAccountingInstance::sendCCRequest");
	
	DOM_Document doc;                
    UINT32 prepaidInterval;
    
    pAccInfo->authFlags |= AUTH_SENT_CC_REQUEST;
    
    if (pAccInfo->authFlags & AUTH_ACCOUNT_EMPTY)
    {
    	// do not send further CC requests for this account    	
    	return E_SUCCESS;
    }
    
    pglobalOptions->getPrepaidInterval(&prepaidInterval);
    // prepaid bytes are "confirmed bytes - transfered bytes"
    //UINT64 bytesToConfirm = pAccInfo->confirmedBytes + (prepaidInterval) - (pAccInfo->confirmedBytes - pAccInfo->transferredBytes);			
    pAccInfo->bytesToConfirm = (prepaidInterval) + pAccInfo->transferredBytes;
	makeCCRequest(pAccInfo->accountNumber, pAccInfo->bytesToConfirm, doc);				
	//pAccInfo->authFlags |= AUTH_SENT_CC_REQUEST;
#ifdef DEBUG	
	CAMsg::printMsg(LOG_DEBUG, "CC request sent for %u bytes \n",pAccInfo->bytesToConfirm);
	CAMsg::printMsg(LOG_DEBUG, "transferrred bytes: %u bytes \n",pAccInfo->transferredBytes);
	CAMsg::printMsg(LOG_DEBUG, "prepaid Interval: %u \n",prepaidInterval);	
	
		

	UINT32 debuglen = 3000;
	UINT8 debugout[3000];
	DOM_Output::dumpToMem(doc,debugout,&debuglen);
	debugout[debuglen] = 0;			
	CAMsg::printMsg(LOG_DEBUG, "the CC sent looks like this: %s \n",debugout);
#endif			
	
	//FINISH_STACK("CAAccountingInstance::sendCCRequest");
	
	return pAccInfo->pControlChannel->sendXMLMessage(doc);
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
	m_preparedCCRequest = DOM_Document::createDocument();
	
	DOM_Element elemRoot = m_preparedCCRequest.createElement("PayRequest");
	elemRoot.setAttribute("version", "1.0");
	m_preparedCCRequest.appendChild(elemRoot);
	DOM_Element elemCC = m_preparedCCRequest.createElement("CC");
	elemCC.setAttribute("version", "1.2");
	elemRoot.appendChild(elemCC);
	DOM_Element elemAiName = m_preparedCCRequest.createElement("AiID");
	setDOMElementValue(elemAiName, a_AiName);
	elemCC.appendChild(elemAiName);	

	//extract price certificate elements from cascadeInfo
	//get cascadeInfo from CAMix(which makeCCRequest needs to extract the price certs
	DOM_Document cascadeInfoDoc; 
	callingMix->getMixCascadeInfo(cascadeInfoDoc);
	
	DOM_Element cascadeInfoElem = cascadeInfoDoc.getDocumentElement();
	DOM_NodeList allMixes = cascadeInfoElem.getElementsByTagName("Mix");
	UINT32 nrOfMixes = allMixes.getLength();
	DOM_Node* mixNodes = new DOM_Node[nrOfMixes]; //so we can use separate loops for extracting, hashing and appending
	
	DOM_Node curMixNode;
	for (UINT32 i = 0, j = 0, count = nrOfMixes; i < count; i++, j++){
		//cant use getDOMChildByName from CAUtil here yet, since it will always return the first child
		curMixNode = allMixes.item(i); 
		if (getDOMChildByName(curMixNode,(UINT8*)"PriceCertificate",mixNodes[j],true) != E_SUCCESS)
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
		DOM_Node skiNode;
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
			
			UINT8* tmpBuff = new UINT8[1024];
			UINT32 len=1024;
			if(CABase64::encode(digest,SHA_DIGEST_LENGTH,tmpBuff,&len)!=E_SUCCESS)
				return E_UNKNOWN;
			tmpBuff[len]=0;
						
			
			//line breaks might have been added, and would lead to database problems
			strtrim(tmpBuff); //return value ohny significant for NULL or all-whitespace string, ignore   
						
			m_allHashes[i] = tmpBuff;
			//do not delete tmpBuff here, since we're using allHashes below

		if (getDOMChildByName(mixNodes[i],(UINT8*)"SubjectKeyIdentifier",skiNode,true) != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not get mix id from price cert");	
			}	
		
		allSkis[i] =  (UINT8*) skiNode.getFirstChild().getNodeValue().transcode();

	}
	    //concatenate the hashes, and store for future reference to indentify the cascade
    m_currentCascade = new UINT8[256];
    for (UINT32 j = 0; j < nrOfMixes; j++)
    {
        //check for hash value size (should always be OK)
        if (strlen((const char*)m_currentCascade) > ( 256 - strlen((const char*)m_allHashes[j]) )   )
        {
            return E_UNKNOWN;
            CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance::prepareCCRequest: Too many/too long hash values, ran out of allocated memory\n");
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
	DOM_Element elemPriceCerts = m_preparedCCRequest.createElement("PriceCertificates");
	DOM_Element elemCert;
	for (UINT32 i = 0; i < nrOfMixes; i++) 
	{
		elemCert = m_preparedCCRequest.createElement("PriceCertHash");
		//CAMsg::printMsg(LOG_DEBUG,"hash to be inserted in cc: index %d, value %s\n",i,m_allHashes[i]);
		setDOMElementValue(elemCert,m_allHashes[i]);
		//delete[] allHashes[i];
		elemCert.setAttribute("id", DOMString( (const char*)allSkis[i]));
		setDOMElementAttribute(elemCert, "position", i);
		if (i == 0) 
		{
			elemCert.setAttribute("isAI","true");
		}
		elemPriceCerts.appendChild(elemCert);
	}
	elemCC.appendChild(elemPriceCerts);
#ifdef DEBUG 		
	CAMsg::printMsg(LOG_DEBUG, "finished method makeCCRequest\n");
#endif		

	delete[] mixNodes;
	//delete[] allHashes;
	delete[] allSkis;	
	return E_SUCCESS;

}


SINT32 CAAccountingInstance::makeCCRequest(const UINT64 accountNumber, const UINT64 transferredBytes, DOM_Document& doc)
	{
		INIT_STACK;
		BEGIN_STACK("CAAccountingInstance::makeCCRequest");
		
		DOM_Node elemCC;
		
		doc = DOM_Document::createDocument();
		doc.appendChild(doc.importNode(m_preparedCCRequest.getDocumentElement(),true));
		
	    getDOMChildByName(doc.getDocumentElement(),(UINT8*)"CC",elemCC);
		
		DOM_Element elemAccount = doc.createElement("AccountNumber");
		setDOMElementValue(elemAccount, accountNumber);
		elemCC.appendChild(elemAccount);
		DOM_Element elemBytes = doc.createElement("TransferredBytes");
		setDOMElementValue(elemBytes, transferredBytes);
		elemCC.appendChild(elemBytes);
		
		FINISH_STACK("CAAccountingInstance::makeCCRequest");

		return E_SUCCESS;
	}




/**
 * Handle a user (xml) message sent to us by the Jap. 
 *	
 * This function is running inside the AiThread. It determines 
 * what type of message we have and calls the appropriate handle...() 
 * function
 */
SINT32 CAAccountingInstance::processJapMessage(fmHashTableEntry * pHashEntry,const DOM_Document& a_DomDoc)
	{
		INIT_STACK;
		BEGIN_STACK("CAAccountingInstance::processJapMessage");
		
		if (pHashEntry == NULL)
		{
			return E_UNKNOWN;
		}

		DOM_Element root = a_DomDoc.getDocumentElement();
		char* docElementName = root.getTagName().transcode();		
		aiQueueItem* pItem;
		void (CAAccountingInstance::*handleFunc)(tAiAccountingInfo*,DOM_Element&) = NULL;
		SINT32 ret;


		// what type of message is it?
		if ( strcmp( docElementName, "AccountCertificate" ) == 0 )
			{
				#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Received an AccountCertificate. Calling handleAccountCertificate()\n" );
				#endif
				handleFunc = &CAAccountingInstance::handleAccountCertificate;
				//ms_pInstance->handleAccountCertificate( pHashEntry->pAccountingInfo, root );
			}
		else if ( strcmp( docElementName, "Response" ) == 0)
			{
				#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Received a Response (challenge-response)\n");
				#endif
				handleFunc = &CAAccountingInstance::handleChallengeResponse;
				//ms_pInstance->handleChallengeResponse( pHashEntry->pAccountingInfo, root );
			}
		else if ( strcmp( docElementName, "CC" ) == 0 )
			{
				#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Received a CC. Calling handleCostConfirmation()\n" );
				#endif
				handleFunc = &CAAccountingInstance::handleCostConfirmation;
				//ms_pInstance->handleCostConfirmation( pHashEntry->pAccountingInfo, root );
			}
		else
		{
			CAMsg::printMsg( LOG_ERR, 
					"AI Received XML message with unknown root element \"%s\". This is not accepted!\n",
											docElementName 
										);
										
			SAVE_STACK("CAAccountingInstance::processJapMessage", "error");
			return E_UNKNOWN;
		}

		delete [] docElementName;

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
		(ms_pInstance->*handleFunc)(pHashEntry->pAccountingInfo, root );
		
		FINISH_STACK("CAAccountingInstance::processJapMessage");
		return E_SUCCESS;
	}

void CAAccountingInstance::handleAccountCertificate(tAiAccountingInfo* pAccInfo, DOM_Element &root)
{
	handleAccountCertificate_internal(pAccInfo, root);
	
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
void CAAccountingInstance::handleAccountCertificate_internal(tAiAccountingInfo* pAccInfo, DOM_Element &root)
	{			
		INIT_STACK;
		BEGIN_STACK("CAAccountingInstance::handleAccountCertificate");
		
		//CAMsg::printMsg( LOG_ERR, "CAAccountingInstance::handleAccountCertificate start\n");
		
		//CAMsg::printMsg(LOG_DEBUG, "started method handleAccountCertificate\n");
		DOM_Element elGeneral;
		timespec now;
		getcurrentTime(now);

		// check authstate of this user
		if (pAccInfo == NULL)
		{
			return;
		}
							
		pAccInfo->mutex->lock();
		
		if (pAccInfo->authFlags & AUTH_DELETE_ENTRY)
		{
			pAccInfo->mutex->unlock();
			return;
		}
		
		if (pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT)
		{
			//#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "Already got an account cert. Ignoring...");
			//#endif
			CAXMLErrorMessage err(
					CAXMLErrorMessage::ERR_BAD_REQUEST, 
					(UINT8*)"You have already sent an Account Certificate"
				);
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
			pAccInfo->mutex->unlock();
			return ;
		}

		// parse & set accountnumber
		if (getDOMChildByName( root, (UINT8 *)"AccountNumber", elGeneral, false ) != E_SUCCESS ||
			getDOMElementValue( elGeneral, pAccInfo->accountNumber ) != E_SUCCESS)
		{
			CAMsg::printMsg( LOG_ERR, "AccountCertificate has wrong or no accountnumber. Ignoring...\n");
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_FORMAT);
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
			pAccInfo->mutex->unlock();
			return ;
		}		

		// parse & set payment instance id
		UINT32 len=256;
		pAccInfo->pstrBIID=new UINT8[256];
		if ( getDOMChildByName( root, (UINT8 *)"BiID", elGeneral, false ) != E_SUCCESS ||
			 getDOMElementValue( elGeneral,pAccInfo->pstrBIID, &len ) != E_SUCCESS)
			{
				delete[] pAccInfo->pstrBIID;
				pAccInfo->pstrBIID=NULL;
				CAMsg::printMsg( LOG_ERR, "AccountCertificate has no Payment Instance ID. Ignoring...\n");
				CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_FORMAT);
				DOM_Document errDoc;
				err.toXmlDocument(errDoc);
				pAccInfo->pControlChannel->sendXMLMessage(errDoc);
				pAccInfo->mutex->unlock();
				return ;
			}
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "Stored payment instance ID: %s\n", pAccInfo->pstrBIID);
		#endif


	// parse & set public key
	if ( getDOMChildByName( root, (UINT8 *)"JapPublicKey", elGeneral, false ) != E_SUCCESS )
		{
			CAMsg::printMsg( LOG_ERR, "AccountCertificate contains no public key. Ignoring...\n");
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_KEY_NOT_FOUND);
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
			pAccInfo->mutex->unlock();
			return ;
		}
	#ifdef DEBUG
		UINT8* aij;
		UINT32 aijsize;
		aij = DOM_Output::dumpToMem(elGeneral, &aijsize);
		aij[aijsize-1]=0;
		CAMsg::printMsg( LOG_DEBUG, "Setting user public key %s>\n", aij );
		delete[] aij;
	#endif
	pAccInfo->pPublicKey = new CASignature();
	if ( pAccInfo->pPublicKey->setVerifyKey( elGeneral ) != E_SUCCESS )
	{
		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR);
		DOM_Document errDoc;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);
		pAccInfo->mutex->unlock();
		return ;
	}

	if ((!m_pJpiVerifyingInstance) ||
		(m_pJpiVerifyingInstance->verifyXML( (DOM_Node &)root, (CACertStore *)NULL ) != E_SUCCESS ))
	{
		// signature invalid. mark this user as bad guy
		CAMsg::printMsg( LOG_INFO, "CAAccountingInstance::handleAccountCertificate(): Bad Jpi signature\n" );
		pAccInfo->authFlags |= AUTH_FAKE | AUTH_GOT_ACCOUNTCERT | AUTH_TIMEOUT_STARTED;
		pAccInfo->mutex->unlock();
		return ;
	}		
	
		
	UINT8 * arbChallenge;
	UINT8 b64Challenge[ 512 ];
	UINT32 b64Len = 512;

	//CAMsg::printMsg(LOG_DEBUG, "Almost finished handleAccountCertificate, preparing challenge\n");

	// generate random challenge data and Base64 encode it
	arbChallenge = ( UINT8* ) malloc( 222 );
	getRandom( arbChallenge, 222 );
	CABase64::encode( arbChallenge, 222, b64Challenge, &b64Len );
	if ( pAccInfo->pChallenge != NULL )
		delete[] pAccInfo->pChallenge;
	pAccInfo->pChallenge = arbChallenge; // store challenge for later..

	// generate XML challenge structure
	DOM_Document doc = DOM_Document::createDocument();
	DOM_Element elemRoot = doc.createElement( "Challenge" );
	DOM_Element elemPanic = doc.createElement( "DontPanic" );
	DOM_Element elemPrepaid = doc.createElement( "PrepaidBytes" );
	elemPanic.setAttribute( "version", "1.0" );
	doc.appendChild( elemRoot );
	elemRoot.appendChild( elemPanic );
	elemRoot.appendChild( elemPrepaid );
	setDOMElementValue( elemPanic, b64Challenge );
	SINT32 prepaidAmount = m_dbInterface->getPrepaidAmount(pAccInfo->accountNumber, m_currentCascade, false);
	if (prepaidAmount < 0)
	{
		prepaidAmount = 0;
	}
	setDOMElementValue( elemPrepaid, (UINT32)prepaidAmount);

	// send XML struct to Jap & set auth flags
	pAccInfo->pControlChannel->sendXMLMessage(doc);
	pAccInfo->authFlags |= AUTH_CHALLENGE_SENT | AUTH_GOT_ACCOUNTCERT | AUTH_TIMEOUT_STARTED;	
	pAccInfo->challengeSentSeconds = time(NULL);
	//CAMsg::printMsg("Last Account Certificate request seconds: for IP %u%u%u%u", (UINT8)pHashEntry->peerIP[0], (UINT8)pHashEntry->peerIP[1],(UINT8) pHashEntry->peerIP[2], (UINT8)pHashEntry->peerIP[3]);
	
	//CAMsg::printMsg( LOG_ERR, "CAAccountingInstance::handleAccountCertificate stop\n");
	
	
	pAccInfo->mutex->unlock();
}


void CAAccountingInstance::handleChallengeResponse(tAiAccountingInfo* pAccInfo, DOM_Element &root)
{
	handleChallengeResponse_internal(pAccInfo, root);
	INIT_STACK;
	FINISH_STACK("CAAccountingInstance::handleChallengeResponse");
}


/**
 * Handles the response to our challenge.
 * Checks the validity of the response and sets the user's authFlags
 * Also gets the last CC of the user, and sends it to the JAP
 * accordingly.
 */
void CAAccountingInstance::handleChallengeResponse_internal(tAiAccountingInfo* pAccInfo, DOM_Element &root)
{
	INIT_STACK;
	BEGIN_STACK("CAAccountingInstance::handleChallengeResponse");
	
	
	//CAMsg::printMsg( LOG_ERR, "CAAccountingInstance::handleChallengeResponse start\n");

	
	UINT8 decodeBuffer[ 512 ];
	UINT32 decodeBufferLen = 512;
	UINT32 usedLen;
	DOM_Element elemPanic;
	DSA_SIG * pDsaSig;
	SINT32 prepaidAmount = 0;
	AccountLoginHashEntry* loginEntry;
	CAXMLCostConfirmation* pCC = NULL;
	bool bSendCCRequest = true;
	UINT32 status;
	
	// check current authstate
	
	if (pAccInfo == NULL)
	{
		return;
	}

	pAccInfo->mutex->lock();
	
	if (pAccInfo->authFlags & AUTH_DELETE_ENTRY)
	{
		pAccInfo->mutex->unlock();
		return;
	}

	if( (!(pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT)) ||
			(!(pAccInfo->authFlags & AUTH_CHALLENGE_SENT))
		)
	{
		pAccInfo->mutex->unlock();
		return ;
	}
	pAccInfo->authFlags &= ~AUTH_CHALLENGE_SENT;

	// get raw bytes of response
	if ( getDOMElementValue( root, decodeBuffer, &decodeBufferLen ) != E_SUCCESS )
	{
		CAMsg::printMsg( LOG_DEBUG, "ChallengeResponse has wrong XML format. Ignoring\n" );
		pAccInfo->mutex->unlock();
		return;
	}
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
	pDsaSig = DSA_SIG_new();
	CASignature * sigTester = pAccInfo->pPublicKey;
	sigTester->decodeRS( decodeBuffer, decodeBufferLen, pDsaSig );
	if ( sigTester->verifyDER( pAccInfo->pChallenge, 222, decodeBuffer, decodeBufferLen ) 
		!= E_SUCCESS )
	{
		UINT8 accountNrAsString[32];
		print64(accountNrAsString, pAccInfo->accountNumber);
		CAMsg::printMsg(LOG_ERR, "Challenge-response authentication failed for account %s!\n", accountNrAsString);
		pAccInfo->authFlags |= AUTH_FAKE;
		pAccInfo->authFlags &= ~AUTH_ACCOUNT_OK;
		pAccInfo->mutex->unlock();
		return ;
	}	
	
	
	
	// fetch cost confirmation from last session if available, and retrieve information; synchronized with settle thread
	bool bSettled;
	m_dbInterface->getCostConfirmation(pAccInfo->accountNumber, m_currentCascade, &pCC, bSettled);
	if (pCC != NULL)
	{
		if (bSettled)
		{
			pAccInfo->authFlags &= ~AUTH_WAITING_FOR_FIRST_SETTLED_CC;
		}
		
		pAccInfo->transferredBytes += pCC->getTransferredBytes();
		pAccInfo->confirmedBytes = pCC->getTransferredBytes();
		#ifdef DEBUG
			UINT8 tmp[32];
			print64(tmp,pAccInfo->transferredBytes);
			CAMsg::printMsg(LOG_DEBUG, "TransferredBytes is now %s\n", tmp);
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
	m_currentAccountsHashtable->getMutex()->lock();	
	pAccInfo->authFlags |= AUTH_ACCOUNT_OK;	// authentication successful
	
	loginEntry = (AccountLoginHashEntry*)m_currentAccountsHashtable->getValue(&(pAccInfo->accountNumber));	
	if (!loginEntry)
	{
		// remember that this user is logged in at least once
		loginEntry = new AccountLoginHashEntry;
		loginEntry->accountNumber = pAccInfo->accountNumber;
		loginEntry->count = 1;
		loginEntry->confirmedBytes = 0;
		loginEntry->authRemoveFlags = 0;
		loginEntry->authFlags = 0;
		loginEntry->userID = pAccInfo->userID;
		m_currentAccountsHashtable->put(&(loginEntry->accountNumber), loginEntry);
		if (!(AccountLoginHashEntry*)m_currentAccountsHashtable->getValue(&(pAccInfo->accountNumber)))
		{
			UINT8 accountNrAsString[32];
			print64(accountNrAsString, pAccInfo->accountNumber);
			CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: Could not insert login entry for account %s!", accountNrAsString);
		}
	}	
	else
	{
		loginEntry->count++;
	}
	if (loginEntry->count > 1)
	{
		/*
		 * There already is a user logged in with this account.
 		 */ 		 
		UINT8 accountNrAsString[32];
		print64(accountNrAsString, pAccInfo->accountNumber);
		if (loginEntry->count < MAX_TOLERATED_MULTIPLE_LOGINS)
		{
			// There is now more than one user logged in with this account; kick out the other users!		
			CAMsg::printMsg(LOG_INFO, 
							"CAAccountingInstance: Multiple logins (%d) of user with account %s detected! \
							Kicking out other users with this account...\n", 
							loginEntry->count, accountNrAsString);
			loginEntry->userID = pAccInfo->userID; // this is the current user; kick out the others
		}
		else
		{
		 	/* The maximum of tolerated concurrent logins for this user is exceeded.
		 	 * He won't get any new access again before the old connections have been closed!
		 	 */
		 	CAMsg::printMsg(LOG_INFO, 
		 					"CAAccountingInstance: Maximum of multiple logins exceeded (%d) for user with account %s! \
		 					Kicking out this user!\n", 
		 					loginEntry->count, accountNrAsString);
		 	bSendCCRequest = false; // not needed...
		 	//pAccInfo->authFlags |= AUTH_MULTIPLE_LOGIN;
		}
	}
	
	#ifdef DEBUG		
	CAMsg::printMsg(LOG_DEBUG, "Checking database for previously prepaid bytes...\n");
	#endif
	prepaidAmount = m_dbInterface->getPrepaidAmount(pAccInfo->accountNumber, m_currentCascade, true);
	UINT8 tmp[32];
	print64(tmp,pAccInfo->accountNumber);
	if (prepaidAmount > 0)
	{		
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Got %d prepaid bytes for account nr. %s.\n",prepaidAmount, tmp);
						
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
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: No database record for prepaid bytes found for account nr. %s.\n", tmp);	
	}	
	//CAMsg::printMsg(LOG_DEBUG, "Number of prepaid (confirmed-transferred) bytes : %d \n",pAccInfo->confirmedBytes-pAccInfo->transferredBytes);	
	
	/** 
	 * @todo Dangerous, as this may collide with previous accounts that have been 
	 * used and deleted before... 
	 * There should be something like an expiration date for the account status, e.g. 1 month
	 */
	if (m_dbInterface->getAccountStatus(pAccInfo->accountNumber, status) != E_SUCCESS)
	{
		UINT8 tmp[32];
		print64(tmp,pAccInfo->accountNumber);
		CAMsg::printMsg(LOG_ERR, "CAAccountingInstance: Could not check status for account %s!\n", tmp);			
	}
	else if (status > CAXMLErrorMessage::ERR_OK)
	{
		UINT32 authFlags = 0;
		UINT8 tmp[32];
		print64(tmp,pAccInfo->accountNumber);
		CAMsg::printMsg(LOG_ERR, "CAAccountingInstance: The user with account %s should be kicked out due to error %u!\n", tmp, status);

		/*if (status == CAXMLErrorMessage::ERR_KEY_NOT_FOUND)
		{
			authFlags |= AUTH_INVALID_ACCOUNT;
		}
		else if (status == CAXMLErrorMessage::ERR_ACCOUNT_EMPTY)
		{
			authFlags |= AUTH_ACCOUNT_EMPTY;
			pAccInfo->authFlags |= AUTH_ACCOUNT_EMPTY;
		}*/
		
		if (authFlags)
		{	
			loginEntry->authFlags |= authFlags;
			if (loginEntry->confirmedBytes == 0)
			{
				loginEntry->confirmedBytes = pAccInfo->confirmedBytes;
			}
		}
	}	
	m_currentAccountsHashtable->getMutex()->unlock();	
	
	/** @todo We need this trick so that the program does not freeze with active AI ThreadPool!!!! */
	//pAccInfo->mutex->lock();
	
	if (bSendCCRequest)
	{		
		// fetch cost confirmation from last session if available, and send it
		//CAXMLCostConfirmation * pCC = NULL;
		//m_dbInterface->getCostConfirmation(pAccInfo->accountNumber, m_currentCascade, &pCC);
		if(pCC != NULL)
		{			
			// the typical case; the user had logged in before
			pAccInfo->pControlChannel->sendXMLMessage(pCC->getXMLDocument());
			//delete pCC;
		}
		else
		{
			// there is no CC in the database; typically this is the first connection of this user			
			if (prepaidAmount > 0)
			{
				// Delete any previously stored prepaid amount; there should not be any! CC lost?
				pAccInfo->transferredBytes += prepaidAmount;
			}
			sendCCRequest(pAccInfo);
		}
	}
	
	if(pCC != NULL)
	{
		delete pCC;
	}
	
	if ( pAccInfo->pChallenge != NULL ) // free mem
	{
		delete[] pAccInfo->pChallenge;
		pAccInfo->pChallenge = NULL;
	}
	//CAMsg::printMsg( LOG_ERR, "CAAccountingInstance::handleChallengeResponse stop\n");
	
	pAccInfo->mutex->unlock();
}

void CAAccountingInstance::handleCostConfirmation(tAiAccountingInfo* pAccInfo, DOM_Element &root)
{
	handleCostConfirmation_internal(pAccInfo, root);
	INIT_STACK;
	FINISH_STACK("CAAccountingInstance::handleCostConfirmation");
}

/**
 * Handles a cost confirmation sent by a jap
 */
void CAAccountingInstance::handleCostConfirmation_internal(tAiAccountingInfo* pAccInfo, DOM_Element &root)
{
	INIT_STACK;
	BEGIN_STACK("CAAccountingInstance::handleCostConfirmation");


	if (pAccInfo == NULL)
	{
		return;
	}

	pAccInfo->mutex->lock();
	
	
	if (pAccInfo->authFlags & (AUTH_DELETE_ENTRY | AUTH_ACCOUNT_EMPTY))
	{
		// ignore CCs for this account!
		pAccInfo->mutex->unlock();
		return ;
	}
	
	// check authstate	
	if (!(pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT) ||
		!(pAccInfo->authFlags & AUTH_ACCOUNT_OK) ||  
		!(pAccInfo->authFlags & AUTH_SENT_CC_REQUEST)) 
	{		
		CAMsg::printMsg(LOG_ERR, 
			"CAAccountingInstance::handleCostConfirmation CC was received but has not been requested! Ignoring...\n");
		
		pAccInfo->mutex->unlock();
		return ;
	}
		
	CAXMLCostConfirmation* pCC = CAXMLCostConfirmation::getInstance(root);
	if(pCC==NULL)
	{
		pAccInfo->mutex->unlock();
		return ;
	}
	
	
		
	// for debugging only: test signature the oldschool way
	// warning this removes the signature from doc!!!
	if (pAccInfo->pPublicKey==NULL||
		pAccInfo->pPublicKey->verifyXML( (DOM_Node &)root ) != E_SUCCESS)
	{
		// wrong signature
		CAMsg::printMsg( LOG_INFO, "CostConfirmation has INVALID SIGNATURE!\n" );
		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_SIGNATURE, (UINT8*)"CostConfirmation has bad signature");
		DOM_Document errDoc;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);
		delete pCC;
		pAccInfo->mutex->unlock();
		return;
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
		DOM_Document errDoc;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);
		delete pCC;
		pAccInfo->mutex->unlock();
		return;
	}
	
	Hashtable* certHashCC = 
		new Hashtable((UINT32 (*)(void *))Hashtable::stringHash, (SINT32 (*)(void *,void *))Hashtable::stringCompare);
	UINT8* certHash;
	bool bFailed = false;
	for (UINT32 i = 0; i < pCC->getNumberOfHashes(); i++)
	{
		certHash = pCC->getPriceCertHash(i);
		certHashCC->put(certHash, certHash);
		/*
		if ((certHash = (UINT8*)certHashCC->getValue(certHash)) != NULL)
		{
			CAMsg::printMsg( LOG_INFO, "CC1: %s\n", certHash);
		}*/
	}
	for (UINT32 i = 0; i < m_allHashesLen; i++)
	{
		//CAMsg::printMsg( LOG_INFO, "CA:  %s\n", m_allHashes[i]);
		certHash = (UINT8*)certHashCC->remove(m_allHashes[i]);
		if (certHash == NULL)
		{
			bFailed = true;
			break;
		}
		else
		{
			delete certHash;
		}
	}
	certHashCC->clear(HASH_EMPTY_NONE, HASH_EMPTY_DELETE);
	delete certHashCC;
	
	if (bFailed)
	{
		CAMsg::printMsg( LOG_INFO, "CostConfirmation has invalid price cert hashes!\n" );
		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_REQUEST, 
			(UINT8*)"CostConfirmation has invalid price cert hashes");
		DOM_Document errDoc;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);
		delete pCC;
		pAccInfo->mutex->unlock();
		return;
	}
	


	// parse & set transferredBytes
	//when using Prepayment, this check is outdated, but left in to notice the most crude errors/cheats
	//The CC's transferredBytes should be equivalent to 
	//AccInfo's confirmed bytes + the Config's PrepaidInterval - the number of bytes transferred between
	//requesting and receiving the CC
	if (pCC->getTransferredBytes() <= pAccInfo->confirmedBytes)
	{
		if (pCC->getTransferredBytes() < pAccInfo->confirmedBytes)
		{
			UINT8 tmp[32], tmpOther[32];
			print64(tmp,pCC->getTransferredBytes());
			print64(tmpOther,pAccInfo->confirmedBytes);		
			CAMsg::printMsg( LOG_ERR, "CostConfirmation has insufficient number of bytes (%s < %s). Ignoring...\n", 
				tmp, tmpOther );
		}
		
		/*
		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_DATA, 
			(UINT8*)"Your CostConfirmation has a wrong number of transferred bytes");
		DOM_Document errDoc;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendXMLMessage(errDoc);*/
	}
	else
	{	
		/*		
		UINT8 tmp[32];
		print64(tmp,pCC->getTransferredBytes());
		CAMsg::printMsg( LOG_ERR, "Transferredbytes in CC: %s\n", tmp);
		*/
		
		if (m_dbInterface->storeCostConfirmation(*pCC, m_currentCascade) != E_SUCCESS)
		{
			UINT8 tmp[32];
			print64(tmp,pCC->getAccountNumber());
			CAMsg::printMsg( LOG_INFO, "CostConfirmation for account %s could not be stored in database!\n", tmp );
			pAccInfo->authFlags |= AUTH_DATABASE;
		}
		else 
		{
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
	
	if (pAccInfo->confirmedBytes >= pAccInfo->bytesToConfirm)
	{
		// the user confirmed everything we wanted; if a timeout has been set, it should be reset
		pAccInfo->lastHardLimitSeconds = time(NULL);
	}
	else
	{
		UINT8 tmp[32], tmp2[32], tmp3[32];
		print64(tmp, pCC->getTransferredBytes());
		print64(tmp2, pCC->getAccountNumber());
		print64(tmp3, pAccInfo->bytesToConfirm);
		CAMsg::printMsg(LOG_ERR, "AccountingSettleThread: Requested CC value has NOT been confirmed by account nr %s! "
								 "Received Bytes: %s/%s "
								"Maybe client and Mix count differently?\n", tmp2, tmp, tmp3);
		m_pSettleThread->settle();
	}
	
	pAccInfo->bytesToConfirm = 0;
	pAccInfo->authFlags &= ~AUTH_SENT_CC_REQUEST;

	
	delete pCC;
	pAccInfo->mutex->unlock();
	
	return;
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
		AUTH_SENT_CC_REQUEST; // prevents multiple CC requests on login
	pHashEntry->pAccountingInfo->authTimeoutStartSeconds = time(NULL);
	pHashEntry->pAccountingInfo->lastHardLimitSeconds = time(NULL);
	pHashEntry->pAccountingInfo->sessionPackets = 0;
	pHashEntry->pAccountingInfo->transferredBytes = 0;
	pHashEntry->pAccountingInfo->confirmedBytes = 0;
	pHashEntry->pAccountingInfo->bytesToConfirm = 0;
	pHashEntry->pAccountingInfo->nrInQueue = 0;
	pHashEntry->pAccountingInfo->userID = pHashEntry->id;
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
		UINT32 prepaidInterval;
		
		if (pAccInfo == NULL)
		{
			SAVE_STACK("CAAccountingInstance::cleanupTableEntry", "acc info null");
			//ms_pInstance->m_pMutex->unlock();
			return E_UNKNOWN;
		}
		
		pAccInfo->mutex->lock();
		
		pHashEntry->pAccountingInfo=NULL;
		
		if (pAccInfo->accountNumber)
		{
			if (pAccInfo->authFlags & AUTH_ACCOUNT_OK)
			{
				// remove login
				ms_pInstance->m_currentAccountsHashtable->getMutex()->lock();
				loginEntry = (AccountLoginHashEntry*)ms_pInstance->m_currentAccountsHashtable->getValue(&(pAccInfo->accountNumber));																	
				if (loginEntry)
				{
					// test: delete CC!!!
					//ms_pInstance->m_dbInterface->deleteCC(pAccInfo->accountNumber, ms_pInstance->m_currentCascade);
					
					if (pAccInfo->userID == loginEntry->userID)// &&
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
							pglobalOptions->getPrepaidInterval(&prepaidInterval);
							if (prepaidBytes > prepaidInterval)
							{
								UINT8 tmp[32];
								print64(tmp, pAccInfo->accountNumber);
								/* Client paid more than the prepaid interval - 
								 * this is beyond specification and not allowed!
								 */
								CAMsg::printMsg(LOG_ERR, 
									"PrepaidBytes of %d for account %s are higher than prepaid interval! "
									"The client did not behave according to specification. "
									"Deleting %d bytes...\n", prepaidBytes, tmp, prepaidBytes - prepaidInterval);
								
								prepaidBytes = prepaidInterval;
							}
						}												
						
						if (ms_pInstance->m_dbInterface)
						{
							ms_pInstance->m_dbInterface->storePrepaidAmount(pAccInfo->accountNumber,prepaidBytes, ms_pInstance->m_currentCascade);
						}
					}					

					if (loginEntry->count <= 1)
					{
						if (loginEntry->count < 1)
						{
							CAMsg::printMsg(LOG_CRIT, "CAAccountingInstance: Cleanup found non-positive number of user login hash entries (%d)!\n", loginEntry->count);
						}
						// this is the last active user connection; delete the entry
						ms_pInstance->m_currentAccountsHashtable->remove(&(pAccInfo->accountNumber));
						delete loginEntry;
					}
					else
					{
						// there are other connections from this user
						loginEntry->count--;
					}
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
		if ( pAccInfo->pPublicKey!=NULL )
		{
			delete pAccInfo->pPublicKey;
		}
		if ( pAccInfo->pChallenge!=NULL )
		{
			delete [] pAccInfo->pChallenge;
		}
		if ( pAccInfo->pstrBIID!=NULL )
		{
			delete [] pAccInfo->pstrBIID;
		}
					
		
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
		pAccInfo->mutex->unlock();
		
		if (!(pAccInfo->authFlags & AUTH_DELETE_ENTRY))
		{
			// there are no handles for this entry in the queue, we can savely delete it now
			delete pAccInfo->mutex;
			pAccInfo->mutex = NULL;
			delete pAccInfo;
		}
		else
		{
			CAMsg::printMsg(LOG_INFO, "CAAccountingInstance: Cleanup method sent account deletion request to AI thread!\n");
		}
		//ms_pInstance->m_pMutex->unlock();
		
		FINISH_STACK("CAAccountingInstance::cleanupTableEntry");
		
		return E_SUCCESS;
	}



#endif /* ifdef PAYMENT */
