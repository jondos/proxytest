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

#ifndef CA_ACCOUNTING_INSTANCE_HPP
#define CA_ACCOUNTING_INSTANCE_HPP

//#include "CAMuxSocket.hpp"
#include "CAFirstMixChannelList.hpp"
#include "CASymCipher.hpp"
#include "CAQueue.hpp"
#include "CAThread.hpp"
#include "CATempIPBlockList.hpp"
#include "CAAccountingDBInterface.hpp"
#include "CAAccountingBIInterface.hpp"


struct t_aiqueueitem 
{
	UINT8 * pData;
	UINT32 dataLen;
	fmHashTableEntry * pHashEntry;
};
typedef struct t_aiqueueitem aiQueueItem;
typedef aiQueueItem* p_aiQueueItem;



/**
 * This is the AI (accounting instance or abrechnungsinstanz in german)
 * class. It creates an instance on the first call to getInstance and 
 * launches its msg processing thread internally.
 */
class CAAccountingInstance 
{
public:

	/**
	 * Returns a reference to the Singleton instance
	 */
	static CAAccountingInstance *getInstance();

	/**
	 * This must be called by the FirstMix when a JAP connected and sent 
	 * its symmetric keys
	 */
	UINT32 setJapKeys( fmHashTableEntry * pHashEntry,
										 UINT8 * out, UINT8 * in);

	/**
	 * This should always be called when closing a JAP connection
	 * to cleanup the data structures
	 */
	UINT32 cleanupTableEntry(fmHashTableEntry * pHashEntry);
	UINT32 initTableEntry(fmHashTableEntry * pHashEntry);

	/**
	 * This should be called by the FirstMix for every incoming Jap packet
	 */
	UINT32 handleJapPacket( MIXPACKET *packet,
													fmHashTableEntry *pHashEntry
													);
													


	/**
	 * Check if an IP address is temporarily blocked by the accounting instance.
	 * This should be called by the FirstMix when a JAP is connecting.
	 * @retval 1 if the given IP is blocked
	 * @retval 0 if it is not blocked
	 */
	SINT32 isIPAddressBlocked(const UINT8 ip[4])
	{ return m_pIPBlockList->checkIP(ip); }

private:

	CAAccountingInstance(); //Singleton!
	~CAAccountingInstance();

	void addToReceiveBuffer( UINT8 *data, 
												fmHashTableEntry *pHashEntry
												);

	/**
	* Handle a user (xml) message sent to us by the Jap. 
	*  
	* This function is running inside the AiThread. It determines 
	* what type of message we have and calls the appropriate handle...() 
	* function
	*/
	void processJapMessage( fmHashTableEntry * pHashEntry, 
											UINT8 * pData,
											UINT32 dataLen
											);

	/**
	* Handles a cost confirmation sent by a jap
	*/
	void handleCostConfirmation(fmHashTableEntry *pHashEntry, 
								const DOM_Element &root,
								UINT8 * pData, UINT32 dataLen);

	/**
	* Handles an account certificate of a newly connected Jap.
	*/
	void handleAccountCertificate(fmHashTableEntry *pHashEntry, 
							const DOM_Element &root,
							UINT8 * pData, UINT32 dataLen);

	void handleAccountCertificateError(fmHashTableEntry *pHashEntry, UINT32 num);
	
	void handleBalanceCertificate(fmHashTableEntry *pHashEntry, 
				const DOM_Element &root,
				UINT8 * pData, UINT32 dataLen);
				
	void handleChallengeResponse(fmHashTableEntry *pHashEntry, 
				const DOM_Element &root,
				UINT8 * pData, UINT32 dataLen);



	/**
	* Sends a message to the Jap. The message is automatically splitted into 
	* several mixpackets if necessary and is encrypted using the pCipherOut
	* AES cipher
	*/
  void sendAIMessageToJap(fmHashTableEntry pHashEntry,
                          UINT8 *msgString, UINT32 msgLen);

	static THREAD_RETURN aiThreadMainLoop(void *param);

	/** internal receiving queue for messages coming from japs */
	CAQueue * m_pQueue;

	/** this thread reads messages from the queue and processes them */
	CAThread * m_pThread;
	
	/** this is for synchronizing the write access to the HashEntries */
	CAMutex m_Mutex;

	CAAccountingDBInterface * m_dbInterface;
	CAAccountingBIInterface * m_biInterface;
	
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
	static CAAccountingInstance * m_pInstance;	
    

};


#endif
