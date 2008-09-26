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

#ifndef __CAACCOUNTINGINSTANCE__
#define __CAACCOUNTINGINSTANCE__
#include "doxygen.h"
#ifdef PAYMENT
#include "CAFirstMixChannelList.hpp"
#include "CASymCipher.hpp"
#include "CAQueue.hpp"
#include "CAThread.hpp"
#include "CATempIPBlockList.hpp"
#include "CAThreadPool.hpp"
#include "CAAccountingDBInterface.hpp"
#include "CAAccountingBIInterface.hpp"
#include "CAAccountingControlChannel.hpp"
#include "CAAccountingSettleThread.hpp"
#include "CACmdLnOptions.hpp"
#include "Hashtable.hpp"
#include "CAMix.hpp"
#include "xml/DOM_Output.hpp"
#include "CAStatusManager.hpp"

// we want a costconfirmation from the user for every megabyte
// after 2megs of unconfirmed traffic we kick the user out
// todo put this in the configfile
//#define HARDLIMIT_UNCONFIRMED_BYTES 1024*1024*2
//#define SOFTLIMIT_UNCONFIRMED_BYTES 1024*512

// the number of seconds that may pass between a pay request
// and the jap sending its answer
#define CHALLENGE_TIMEOUT 15
#define HARD_LIMIT_TIMEOUT 30
#define AUTH_TIMEOUT 15
#define CRITICAL_SUBSEQUENT_BI_CONN_ERRORS 5

struct AccountLoginHashEntry
{
	UINT64 accountNumber;
	UINT64 confirmedBytes;
	UINT64 userID;
	UINT32 authFlags;
	UINT32 authRemoveFlags;
	UINT32 count;
};

struct SettleEntry
{
	UINT64 accountNumber;
	UINT32 authFlags;
	UINT32 authRemoveFlags;
	UINT64 confirmedBytes;
	UINT64 diffBytes;
	SettleEntry* nextEntry;
};

/**
 * This is the AI (accounting instance or abrechnungsinstanz in german)
 * class. Its purpose is to count packets for every user and to decide
 * wether the connection should be closed (e.g. when a user is betraying us, or
 * simply when the account is empty and the user refuses to charge it).
 *
 * It is a singleton class, only one instance exists at a time.
 * On the first call to getInstance() the initialization is performed.
 */
class CAAccountingInstance
{
public:

	/**
	 * Returns a reference to the Singleton instance
	 */
	static SINT32 init(CAMix* callingMix)
		{
				ms_pInstance = new CAAccountingInstance(callingMix);
				MONITORING_FIRE_PAY_EVENT(ev_pay_aiInited);
				return E_SUCCESS;
		}
		
	static SINT32 clean()
		{
			if(ms_pInstance != NULL)
			{
				delete ms_pInstance;
				ms_pInstance=NULL;
			}
			if(m_preparedCCRequest != NULL)
			{
				m_preparedCCRequest->release();
				m_preparedCCRequest = NULL;
			}
			MONITORING_FIRE_PAY_EVENT(ev_pay_aiShutdown);
			return E_SUCCESS;
		}

	/**
	 * @return the payment auth flags of the given connecttion or 0 if no auth flags are set
	 */
	static UINT32 getAuthFlags(fmHashTableEntry * pHashEntry);

	/**
	 * This should always be called when closing a JAP connection
	 * to cleanup the data structures
	 */
	static SINT32 cleanupTableEntry(fmHashTableEntry * pHashEntry);	
	static SINT32 initTableEntry(fmHashTableEntry * pHashEntry);
	
	/**
	 * This should be called by the FirstMix for every incoming Jap packet
	 */
	static SINT32 handleJapPacket(fmHashTableEntry *pHashEntry, bool a_bControlMessage, bool a_bMessageToJAP);
		
	/**
	 * Check if an IP address is temporarily blocked by the accounting instance.
	 * This should be called by the FirstMix when a JAP is connecting.
	 * @retval 1 if the given IP is blocked
	 * @retval 0 if it is not blocked
	 */
	static SINT32 isIPAddressBlocked(const UINT8 ip[4])
	{
		//return ms_pInstance->m_pIPBlockList->checkIP(ip); 
		return 0;
	}
	
	/**
	* Handle a user (xml) message sent to us by the Jap through the ControlChannel
	*  
	* This function determines 
	* what type of message we have and sends the appropriate handle...() 
	* function to the ai thread.
	*/
	SINT32 static processJapMessage(fmHashTableEntry * pHashEntry,const  XERCES_CPP_NAMESPACE::DOMDocument* a_DomDoc);
		
	UINT32 static getNrOfUsers();
	
	static SINT32 loginProcessStatus(fmHashTableEntry *pHashEntry);
	static SINT32 finishLoginProcess(fmHashTableEntry *pHashEntry);
	//static void forcedSettle();
	
	static SINT32 settlementTransaction();
		
	static const SINT32 HANDLE_PACKET_CONNECTION_OK; // this packet has been checked and is OK
	static const SINT32 HANDLE_PACKET_CONNECTION_UNCHECKED; // the packet might be OK (is it not checked)
	static const SINT32 HANDLE_PACKET_HOLD_CONNECTION; // queue packets until JAP has authenticated
	static const SINT32 HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION; // small grace period until kickout
	static const SINT32 HANDLE_PACKET_CLOSE_CONNECTION; // this connection should be closed immediatly
	
	
private:

	CAAccountingInstance(CAMix* callingMix); //Singleton!
	~CAAccountingInstance();

	struct t_aiqueueitem
	{
		XERCES_CPP_NAMESPACE::DOMDocument*			pDomDoc;
		tAiAccountingInfo*		pAccInfo;
		void (CAAccountingInstance::*handleFunc)(tAiAccountingInfo*,DOMElement*);
	};
	typedef struct t_aiqueueitem aiQueueItem;

	static SINT32 handleJapPacket_internal(fmHashTableEntry *pHashEntry, bool a_bControlMessage, bool a_bMessageToJAP);
	
	static void processJapMessageLoginHelper(fmHashTableEntry *pHashEntry, 
											 UINT32 handlerReturnvalue, 
											 bool finishLogin);
	/**
	 * Handles a cost confirmation sent by a jap
	 */
	UINT32 handleCostConfirmation(tAiAccountingInfo* pAccInfo, DOMElement* root );
	UINT32 handleCostConfirmation_internal(tAiAccountingInfo* pAccInfo, DOMElement* root );

	/**
	* Handles an account certificate of a newly connected Jap.
	*/
	UINT32 handleAccountCertificate(tAiAccountingInfo* pAccInfo, DOMElement* root );
	UINT32 handleAccountCertificate_internal(tAiAccountingInfo* pAccInfo, DOMElement* root );
	
	
	/**
	 * Checks the response of the challenge-response auth.
	 */
	UINT32 handleChallengeResponse(tAiAccountingInfo* pAccInfo, DOMElement* root);
	UINT32 handleChallengeResponse_internal(tAiAccountingInfo* pAccInfo, DOMElement* root);

	bool cascadeMatchesCC(CAXMLCostConfirmation *pCC);
	
	static SINT32 getPrepaidBytes(tAiAccountingInfo* pAccInfos);
	SINT32 prepareCCRequest(CAMix* callingMix, UINT8* a_AiName);			
	static SINT32 makeCCRequest( const UINT64 accountNumber, const UINT64 transferredBytes,  XERCES_CPP_NAMESPACE::DOMDocument* & doc);
	static SINT32 sendCCRequest(tAiAccountingInfo* pAccInfo);
	static SINT32 makeAccountRequest( XERCES_CPP_NAMESPACE::DOMDocument* & doc);
	static SINT32 sendAILoginConfirmation(tAiAccountingInfo* pAccInfo, const UINT32 code, UINT8 * message);
	
	//possible replies to a JAP
	static SINT32 returnOK(tAiAccountingInfo* pAccInfo);
	static SINT32 returnWait(tAiAccountingInfo* pAccInfo);
	static SINT32 returnKickout(tAiAccountingInfo* pAccInfo);
	static SINT32 returnPrepareKickout(tAiAccountingInfo* pAccInfo, CAXMLErrorMessage* a_error);
		
	/**
	 * The main loop of the AI thread - reads messages from the queue 
	 * and starts process threads for these messages.
	 */
	//static THREAD_RETURN aiThreadMainLoop(void *param);
	
	/** Processes JAP messages asynchronously by calls to the appropriate handlers. */
	static THREAD_RETURN processThread(void* a_param);
	
	static const UINT64 PACKETS_BEFORE_NEXT_CHECK;
	
	static const UINT32 MAX_TOLERATED_MULTIPLE_LOGINS;
	
	static XERCES_CPP_NAMESPACE::DOMDocument* m_preparedCCRequest;
	
	/** reads messages from the queue and processes them */
	CAThreadPool* m_aiThreadPool;
	
	/** this is for synchronizing the write access to the HashEntries */
	CAMutex *m_pMutex;
	
	Hashtable* m_certHashCC;
	
	/** Stores the account number of all users currently logged in. */
	Hashtable* m_currentAccountsHashtable;
	
	/** the name of this accounting instance */
	UINT8* m_AiName;
	
	/** current cascade (identified by the concatenated hash values of the price certificates) */
	// (same hash values as in CCs and in the JPI, as taken from the MixInfo)
	// we concatenate price certificates rather than mix-skis because a cascade with changed prices counts as a new/different cascade
	UINT8* m_currentCascade;
	
	/** The hash values of the Mixes ordered beginning with the AI Mix. */
	UINT8** m_allHashes;
	UINT32 m_allHashesLen;

	/** the interface to the database */
	//CAAccountingDBInterface * m_dbInterface;
	CAAccountingBIInterface *m_pPiInterface;
	
	UINT32 m_iSoftLimitBytes;
	UINT32 m_iHardLimitBytes;
	
	/** 
	 * Users that get kicked out because they sent no authentication certificate
	 * get their IP appended to this list. Connections from IP Addresses contained in
	 * this list get blocked, so that evil JAP users can't use the mix cascade without 
	 * paying
	 */
	CATempIPBlockList * m_pIPBlockList;

	/**
	 * Singleton: This is the reference to the only instance of this class
	 */
	static CAAccountingInstance * ms_pInstance;
	
	/**
	 * Signature verifying instance for BI signatures
	 * @todo initialize this member
	 */
	CASignature * m_pJpiVerifyingInstance;
	
	/** this thread sends cost confirmations to the BI in regular intervals */
	CAAccountingSettleThread * m_pSettleThread;
	
	static SINT32 m_prepaidBytesMinimum;
	
	bool m_bThreadRunning;
	
	//bool m_loginHashTableChangeInProgress;
	
	/* For Thread synchronisation can only be set and read when m_pSettlementMutex lock is acquired */
	volatile UINT64 m_nextSettleNr;
	volatile UINT64 m_settleWaitNr;
	CAConditionVariable *m_pSettlementMutex;
	
	volatile UINT32 m_seqBIConnErrors;
};


#endif
#endif //Payment
