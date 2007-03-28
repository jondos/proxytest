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
#include "CAAccountingDBInterface.hpp"
#include "CAAccountingBIInterface.hpp"
#include "CAAccountingControlChannel.hpp"
#include "CAAccountingSettleThread.hpp"
#include "CACmdLnOptions.hpp"
#include "CAMix.hpp"
#include "xml/DOM_Output.hpp"

// we want a costconfirmation from the user for every megabyte
// after 2megs of unconfirmed traffic we kick the user out
// todo put this in the configfile
//#define HARDLIMIT_UNCONFIRMED_BYTES 1024*1024*2
//#define SOFTLIMIT_UNCONFIRMED_BYTES 1024*512

// the number of seconds that may pass between a pay request
// and the jap sending its answer
#define REQUEST_TIMEOUT 60
#define HARD_LIMIT_TIMEOUT 30
#define GOODWILL_TIMEOUT 60
#define MIN_BALANCE 50
#define MIN_BYTES 1024*512


struct t_aiqueueitem
	{
		DOM_Document*				pDomDoc;
		fmHashTableEntry*		pHashEntry;
	};
typedef struct t_aiqueueitem aiQueueItem;

extern CACmdLnOptions options;

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
	static SINT32 init()
		{
				ms_pInstance = new CAAccountingInstance();
				return E_SUCCESS;
		}
		
	static SINT32 clean()
		{
			delete ms_pInstance;
			ms_pInstance=NULL;
			return E_SUCCESS;
		}

	/**
	 * This should always be called when closing a JAP connection
	 * to cleanup the data structures
	 */
	static SINT32 cleanupTableEntry(fmHashTableEntry * pHashEntry);	
	static SINT32 initTableEntry(fmHashTableEntry * pHashEntry);
	static SINT32 queueItem(aiQueueItem* pItem)
		{
			return ms_pInstance->m_pQueue->add(pItem,sizeof(aiQueueItem));
		}
	/**
	 * This should be called by the FirstMix for every incoming Jap packet
	 */
	static SINT32 handleJapPacket(fmHashTableEntry *pHashEntry,CAMix* callingMix );

	/**
	 * Check if an IP address is temporarily blocked by the accounting instance.
	 * This should be called by the FirstMix when a JAP is connecting.
	 * @retval 1 if the given IP is blocked
	 * @retval 0 if it is not blocked
	 */
	static SINT32 isIPAddressBlocked(const UINT8 ip[4])
		{
			return ms_pInstance->m_pIPBlockList->checkIP(ip); 
		}
	
	/**
	* Handle a user (xml) message sent to us by the Jap through the ControlChannel
	*  
	* This function is running inside the AiThread. It determines 
	* what type of message we have and calls the appropriate handle...() 
	* function
	*/
	SINT32 static processJapMessage(fmHashTableEntry * pHashEntry,const DOM_Document& a_DomDoc);

private:

	CAAccountingInstance(); //Singleton!
	~CAAccountingInstance();

	/**
	* Handles a cost confirmation sent by a jap
	*/
	void handleCostConfirmation( fmHashTableEntry *pHashEntry, DOM_Element &root );

	/**
	* Handles an account certificate of a newly connected Jap.
	*/
	void handleAccountCertificate( fmHashTableEntry *pHashEntry, DOM_Element &root );
	
	
	/**
	 * Checks the response of the challenge-response auth.
	 */
	void handleChallengeResponse(fmHashTableEntry *pHashEntry, const DOM_Element &root);

				
	static SINT32 makeCCRequest( const UINT64 accountNumber, const UINT64 transferredBytes, DOM_Document& doc, DOM_Document& cascadeInfo);
	static SINT32 makeAccountRequest(DOM_Document &doc);
	
	static SINT32 returnOK(tAiAccountingInfo* pAccInfo);
	static SINT32 returnWait(tAiAccountingInfo* pAccInfo);
	static SINT32 returnKickout();
	static SINT32 returnHold();
	
	/**
	 * The main loop of the AI thread - reads messages from the queue 
	 * and calls the appropriate handlers
	 */
	static THREAD_RETURN aiThreadMainLoop(void *param);
	
	/** this thread reads messages from the queue and processes them */
	CAThread * m_pThread;
	
	/** this is for synchronizing the write access to the HashEntries */
	CAMutex m_Mutex;
	
	/** the name of this accounting instance */
	UINT8 * m_AiName;

	/** the interface to the database */
	CAAccountingDBInterface * m_dbInterface;
	
	UINT32 m_iSoftLimitBytes;
	UINT32 m_iHardLimitBytes;
	// /** the interface to the payment instance */
	// CAAccountingBIInterface * m_biInterface;
	
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
	
	/** internal receiving queue for messages coming from Japs */
	CAQueue * m_pQueue;
	
	/** this thread sends cost confirmations to the BI in regular intervals */
	CAAccountingSettleThread * m_pSettleThread;
	
	bool m_bThreadRunning;
};


#endif
#endif //Payment
