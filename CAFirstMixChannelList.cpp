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
#ifndef ONLY_LOCAL_PROXY
#include "CAFirstMixChannelList.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"
#include "CAThread.hpp"
#define MAX_HASH_KEY 8200 //8113

#ifdef PAYMENT
	#include "CAAccountingInstance.hpp"
#endif

const SINT32 CAFirstMixChannelList::EXPIRATION_TIME_SECS = 300; // 5 minutes


CAFirstMixChannelList::CAFirstMixChannelList()
	{
		m_HashTable=new LP_fmHashTableEntry[MAX_HASH_KEY];
		memset(m_HashTable,0,sizeof(LP_fmHashTableEntry)*MAX_HASH_KEY);
		for(int i=0;i<MAX_HASH_KEY;i++)
			{
				m_HashTable[i]=new fmHashTableEntry;
				memset(m_HashTable[i],0,sizeof(fmHashTableEntry));
			}
		m_listHashTableHead=NULL;
		m_listHashTableNext=NULL;
#ifdef PAYMENT
		m_listTimoutHead = NULL;
		m_listTimoutFoot = NULL;
#endif
		m_HashTableOutChannels=new LP_fmChannelListEntry[0x10000];
		memset(m_HashTableOutChannels,0,sizeof(LP_fmChannelListEntry)*0x10000);
#ifdef DO_TRACE
	m_aktAlloc=m_maxAlloc=0;
#endif
#ifdef DELAY_USERS
		m_u32DelayChannelUnlimitTraffic=DELAY_USERS_TRAFFIC;
		m_u32DelayChannelBucketGrow=DELAY_USERS_BUCKET_GROW;
		m_u32DelayChannelBucketGrowIntervall=DELAY_USERS_BUCKET_GROW_INTERVALL;
		m_pDelayBuckets=new volatile UINT32*[MAX_POLLFD];
		memset(m_pDelayBuckets,0,sizeof(UINT32*)*MAX_POLLFD);
		m_pMutexDelayChannel=new CAMutex();
		m_pThreadDelayBucketsLoop=new CAThread((UINT8*)"Delay Channel Thread");
		m_bDelayBucketsLoopRun=true;
		m_pThreadDelayBucketsLoop->setMainLoop(fml_loopDelayBuckets);
		m_pThreadDelayBucketsLoop->start(this);
#endif
	}

CAFirstMixChannelList::~CAFirstMixChannelList()
	{
#ifdef DELAY_USERS
		m_bDelayBucketsLoopRun=false;
		m_pThreadDelayBucketsLoop->join();
		delete m_pThreadDelayBucketsLoop;
		m_pThreadDelayBucketsLoop = NULL;
		delete m_pMutexDelayChannel;
		m_pMutexDelayChannel = NULL;
		delete []m_pDelayBuckets;
		m_pDelayBuckets = NULL;
#endif
		for(int i=0;i<MAX_HASH_KEY;i++)
				{
					delete m_HashTable[i];
					m_HashTable[i] = NULL;
				}
		delete []m_HashTable;
		m_HashTable = NULL;
		delete []m_HashTableOutChannels;
		m_HashTableOutChannels = NULL;		
	}

/** Adds a new TCP/IP connection (a new user) to the channel list.
	* @param pMuxSocket the new connection (from a user)
	* @param peerIP the IP of the user, so that we can remove it later from the CAIPList
	* @param pQueueSend the send-queue to use for this connection
	* @retval the fmHashTableEntry of the newly added connection
	* @retval NULL if an error occured
	*/
#ifndef LOG_DIALOG
fmHashTableEntry* CAFirstMixChannelList::add(CAMuxSocket* pMuxSocket,const UINT8 peerIP[4],CAQueue* pQueueSend)
#else
fmHashTableEntry* CAFirstMixChannelList::add(CAMuxSocket* pMuxSocket,const UINT8 peerIP[4],CAQueue* pQueueSend,UINT8* strDialog)
#endif
	{
		INIT_STACK;
		BEGIN_STACK("CAFirstMixChannelList::add");
		
		if(pMuxSocket==NULL)
		{
			FINISH_STACK("CAFirstMixChannelList::add (null socket)");
			return NULL;
		}
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
		{
			FINISH_STACK("CAFirstMixChannelList::add (invalid hash key)");
			return NULL;
		}
		m_Mutex.lock();
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		if(pHashTableEntry->pMuxSocket!=NULL) //the entry in the hashtable for this socket (hashkey) must be empty
		{
			FINISH_STACK("CAFirstMixChannelList::add (socket exists)");
			m_Mutex.unlock();
			return NULL;
		}
		
		//SAVE_STACK("CAFirstMixChannelList::add", "initialising table entry");
		
		pHashTableEntry->pMuxSocket=pMuxSocket;
		pHashTableEntry->pQueueSend=pQueueSend;
		pHashTableEntry->pControlMessageQueue = new CAQueue();
		pHashTableEntry->pControlChannelDispatcher = new CAControlChannelDispatcher(pHashTableEntry->pControlMessageQueue);
		pHashTableEntry->uAlreadySendPacketSize=-1;
		pHashTableEntry->cNumberOfChannels=0;
#ifdef LOG_TRAFFIC_PER_USER
		pHashTableEntry->trafficIn=0;
		pHashTableEntry->trafficOut=0;
		getcurrentTimeMillis(pHashTableEntry->timeCreated);
#endif
#ifdef LOG_DIALOG
		pHashTableEntry->strDialog=new UINT8[strlen((char*)strDialog)+1];
		strcpy((char*)pHashTableEntry->strDialog,(char*)strDialog);
#endif
		getRandom(&(pHashTableEntry->id));

#ifdef PAYMENT
		pHashTableEntry->pAccountingInfo=NULL;
#endif
		
		SAVE_STACK("CAFirstMixChannelList::add", "copying peer IP");
		memcpy(pHashTableEntry->peerIP,peerIP,4);
#ifdef DATA_RETENTION_LOG
		pHashTableEntry->peerPort=((CASocket*)pMuxSocket)->getPeerPort();
#endif
#ifdef DELAY_USERS
		m_pMutexDelayChannel->lock();
		pHashTableEntry->delayBucket=m_u32DelayChannelUnlimitTraffic; //can always send some first packets
		for(UINT32 i=0;i<MAX_POLLFD;i++)
			{
				if(m_pDelayBuckets[i]==NULL)
					{
						pHashTableEntry->delayBucketID=i;
						break;
					}
			}
		m_pDelayBuckets[pHashTableEntry->delayBucketID]=&pHashTableEntry->delayBucket;
		m_pMutexDelayChannel->unlock();
#endif

		SAVE_STACK("CAFirstMixChannelList::add", "inserting in connection list");
		//now insert the new connection in the list of all open connections
		if(m_listHashTableHead==NULL) //if first one
		{
			pHashTableEntry->list_HashEntries.next=NULL;			
		}
		else
		{//add to the head of the double linked list
			pHashTableEntry->list_HashEntries.next=m_listHashTableHead;			
			m_listHashTableHead->list_HashEntries.prev=pHashTableEntry;			
		}
		pHashTableEntry->list_HashEntries.prev=NULL;
		m_listHashTableHead=pHashTableEntry;	
		
		SAVE_STACK("CAFirstMixChannelList::add", "inserting in timout list");
		// insert in timeout list; entries are added to the foot of the list
#ifdef PAYMENT
		pHashTableEntry->bRecoverTimeout = true;
		/* Hot fix: push timeout entry explicitly to avoid
		 * confusion, when timeout occurs during AI login
		 */
		//pushTimeoutEntry_internal(pHashTableEntry);
#endif		
		m_Mutex.unlock();
		
		FINISH_STACK("CAFirstMixChannelList::add");
		
		return pHashTableEntry;
	}

///The maximum number of channels allowed per connection
#define MAX_NUMBER_OF_CHANNELS CHANNELS_PER_CLIENT

/** Adds a new channel for a given connection to the channel list. 
	* Also a new out-channel id is generated and returned.
	* @param pMuxSocket the connection from the user
	* @param channelIn the channel, which should be added
	* @param pCipher the symmetric cipher associated with this channel
	* @param channelOut a pointer to the place, there the new generated out-channel id is stored
	* @retval E_SUCCESS if successful
	* @retval E_UNKNOWN in case of an error
	*/
SINT32 CAFirstMixChannelList::addChannel(CAMuxSocket* pMuxSocket,HCHANNEL channelIn,
																	CASymCipher* pCipher,HCHANNEL* channelOut)
	{
		if(pMuxSocket==NULL||channelOut==NULL)
			return E_UNKNOWN;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		m_Mutex.lock();
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		if(pHashTableEntry->pMuxSocket==NULL||pHashTableEntry->cNumberOfChannels>=MAX_NUMBER_OF_CHANNELS)
			{
				CAMsg::printMsg(LOG_DEBUG,"More than 50 channels!\n");
				m_Mutex.unlock();
				return E_UNKNOWN;
			}
		fmChannelListEntry* pEntry=pHashTableEntry->pChannelList;
#ifndef DO_TRACE
		fmChannelListEntry* pNewEntry=new fmChannelListEntry;
#else
		fmChannelListEntry* pNewEntry=newChannelListEntry();
#endif
		memset(pNewEntry,0,sizeof(fmChannelListEntry));
		pNewEntry->pCipher=pCipher;
		pNewEntry->channelIn=channelIn;	
		
		do
			{
				getRandom(channelOut); //get new Random OUT-CHANNEL-ID
			} while(*channelOut<256||get_intern_without_lock(*channelOut)!=NULL); //until it is unused...
		pNewEntry->channelOut=*channelOut;
		pNewEntry->bIsSuspended=false;
		pNewEntry->pHead=pHashTableEntry;

#ifdef LOG_CHANNEL
		pNewEntry->packetsInFromUser=0;
		pNewEntry->packetsOutToUser=0;
#endif
#ifdef SSL_HACK
		pNewEntry->downStreamBytes = 0;
#endif
		
		
		
		//add to the channel list for the given connection
		if(pEntry==NULL) //First Entry to the channel list
		{
			pNewEntry->list_InChannelPerSocket.next=NULL;
			pNewEntry->list_InChannelPerSocket.prev=NULL;				
		}
		else
		{
			pNewEntry->list_InChannelPerSocket.next=pEntry;
			pNewEntry->list_InChannelPerSocket.prev=NULL;
			pEntry->list_InChannelPerSocket.prev=pNewEntry;
		}
		pHashTableEntry->pChannelList=pNewEntry;
		
		//add to the out-channel list
		hashkey=(*channelOut)&0x0000FFFF;
		pEntry=m_HashTableOutChannels[hashkey];
		if(pEntry!=NULL) //Hash Table Bucket Over run....
			{
				pNewEntry->list_OutChannelHashTable.prev=NULL;
				pNewEntry->list_OutChannelHashTable.next=pEntry;
				pEntry->list_OutChannelHashTable.prev=pNewEntry;
			}
		m_HashTableOutChannels[hashkey]=pNewEntry;
		pHashTableEntry->cNumberOfChannels++;
		m_Mutex.unlock();
		return E_SUCCESS;
	}


/** Returns the general data stored for a given Socket (user)
	* @param pMuxSocket the connection from the user
	* @return general data for the given user
	* @retval NULL if not found
*/
fmHashTableEntry* CAFirstMixChannelList::get(CAMuxSocket* pMuxSocket)
	{
		if(pMuxSocket==NULL)
			return NULL;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return NULL;
		m_Mutex.lock();
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		m_Mutex.unlock();
		return pHashTableEntry;
	}

/** Returns the information for a given Input-Channel-ID
	* @param pMuxSocket the connection from the user
	* @param channelIn the channel id
	* @return all channel associated information (output-channel id, cipher etc.)
	* @retval NULL if not found
*/
fmChannelListEntry* CAFirstMixChannelList::get(CAMuxSocket* pMuxSocket,HCHANNEL channelIn)
	{
		if(pMuxSocket==NULL)
			return NULL;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return NULL;
		m_Mutex.lock();
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		fmChannelListEntry* pEntry=pHashTableEntry->pChannelList;
		while(pEntry!=NULL)
			{
				if(pEntry->channelIn==channelIn)
					{
						m_Mutex.unlock();
						return pEntry;
					}
				pEntry=pEntry->list_InChannelPerSocket.next;
			}
		m_Mutex.unlock();
		return NULL;		
	}
#ifdef PAYMENT	

fmHashTableEntry* CAFirstMixChannelList::popTimeoutEntry()
{
	return popTimeoutEntry(false);
}	
	
fmHashTableEntry* CAFirstMixChannelList::popTimeoutEntry(bool a_bForce)	
{
	fmHashTableEntry* ret;
	
	m_Mutex.lock();
	ret = popTimeoutEntry_internal(a_bForce);
	m_Mutex.unlock();
	
	return ret;
}
	
fmHashTableEntry* CAFirstMixChannelList::popTimeoutEntry_internal(bool a_bForce)
{
	fmHashTableEntry* pHashTableEntry;
	
	if (m_listTimoutHead == NULL)
	{
		// there are not entries in the list
		return NULL;
	}
	
	pHashTableEntry = m_listTimoutHead;
	if (a_bForce || pHashTableEntry->list_TimeoutHashEntries.timoutSecs <= time(NULL))
	{
		if (removeFromTimeoutList(pHashTableEntry) == E_SUCCESS)
		{			
			return pHashTableEntry;
		}
		else
		{
			CAMsg::printMsg(LOG_CRIT,
				"CAFirstMixChannelList:popTimeoutEntry_internal: Could not remove expired entry from timeout list!\n");	
		}
	}
	
	return NULL;
}
#endif			

#ifdef PAYMENT
SINT32 CAFirstMixChannelList::pushTimeoutEntry(fmHashTableEntry* pHashTableEntry)
{
	SINT32 ret;
	
	m_Mutex.lock();
	ret = pushTimeoutEntry_internal(pHashTableEntry);
	m_Mutex.unlock();
	
	return ret;
}


UINT32 CAFirstMixChannelList::countTimeoutEntries()
{
	fmHashTableEntry* pHashTableEntry;
	UINT32 count = 0;

	for (pHashTableEntry = m_listTimoutHead; pHashTableEntry != NULL; 
		count++, pHashTableEntry = pHashTableEntry->list_TimeoutHashEntries.next);

	return count;
}
#endif

#ifdef PAYMENT		
SINT32 CAFirstMixChannelList::pushTimeoutEntry_internal(fmHashTableEntry* pHashTableEntry)
{	
	if (pHashTableEntry == NULL)
	{
		return E_UNKNOWN;
	}
	
	INIT_STACK;
	BEGIN_STACK("CAFirstMixChannelList::pushTimeoutEntry_internal");
	
	//CAMsg::printMsg(LOG_DEBUG,"Entries in timeout list before push: %d\n", countTimeoutEntries());
	
	pHashTableEntry->list_TimeoutHashEntries.timoutSecs = time(NULL) + EXPIRATION_TIME_SECS;
	
	//SAVE_STACK("CAFirstMixChannelList::pushTimeoutEntry_internal", "removing from timeout list");
	// remove from timeout list if needed before adding it to the end	
	removeFromTimeoutList(pHashTableEntry);
	
	if (m_listTimoutFoot == NULL)
	{
		//SAVE_STACK("CAFirstMixChannelList::pushTimeoutEntry_internal", "new first entry");
		
		// this is the first entry in the list
		pHashTableEntry->list_TimeoutHashEntries.prev = NULL;
		m_listTimoutHead = pHashTableEntry;
	}
	else
	{
		//SAVE_STACK("CAFirstMixChannelList::pushTimeoutEntry_internal", "new last entry");
		// this is the new last entry in the list
		m_listTimoutFoot->list_TimeoutHashEntries.next = pHashTableEntry;
		pHashTableEntry->list_TimeoutHashEntries.prev = m_listTimoutFoot;
	}
	pHashTableEntry->list_TimeoutHashEntries.next = NULL;
	m_listTimoutFoot = pHashTableEntry;
	
	//CAMsg::printMsg(LOG_DEBUG,"Entries in timeout list after push: %d\n", countTimeoutEntries());
	
	FINISH_STACK("CAFirstMixChannelList::pushTimeoutEntry_internal");
	
	return E_SUCCESS;
	
}
#endif

#ifdef PAYMENT
SINT32 CAFirstMixChannelList::removeFromTimeoutList(fmHashTableEntry* pHashTableEntry)
{
	if (pHashTableEntry == NULL)
	{
		return E_UNKNOWN;
	}
	
	if (m_listTimoutHead == NULL || m_listTimoutFoot == NULL)
	{
		// there is no entry in the list; therefore this entry does not need to be removed
		return E_SUCCESS;
	}
	
	if (pHashTableEntry->list_TimeoutHashEntries.prev == NULL && 
		pHashTableEntry->list_TimeoutHashEntries.next == NULL &&
		m_listTimoutHead != pHashTableEntry)
	{
		// this entry is not in the list; it does not need to be removed
		return E_SUCCESS;
	}		
	
	if(m_listTimoutHead == pHashTableEntry) //if entry is the head of the connection list
	{
		if(m_listTimoutFoot == pHashTableEntry) //if entry is also the last (so the only one in the list..)
		{
			//list is now empty
			m_listTimoutHead = NULL; 
			m_listTimoutFoot = NULL;
		}
		else
		{
			//remove the head of the list
			m_listTimoutHead = pHashTableEntry->list_TimeoutHashEntries.next; 
		}
	}
	else
	{	//the connection is not the head of the list
		if(pHashTableEntry->list_TimeoutHashEntries.next == NULL)
		{
			//the connection is the last element in the list
			m_listTimoutFoot = pHashTableEntry->list_TimeoutHashEntries.prev;
			m_listTimoutFoot->list_TimeoutHashEntries.next = NULL;
		}
		else
		{
			//it is a simple middle element
			if (pHashTableEntry->list_TimeoutHashEntries.prev == NULL)
			{
				CAMsg::printMsg(LOG_CRIT, "CAFirstMixChannelList:removeFromTimeoutList: No previous element!!\n");
			}
			else
			{
				pHashTableEntry->list_TimeoutHashEntries.prev->list_TimeoutHashEntries.next = pHashTableEntry->list_TimeoutHashEntries.next;
			}
			if (pHashTableEntry->list_TimeoutHashEntries.next == NULL)
			{
				CAMsg::printMsg(LOG_CRIT, "CAFirstMixCahnelList:removeFromTimeoutList: No next element!!\n");
			}
			else
			{
				pHashTableEntry->list_TimeoutHashEntries.next->list_TimeoutHashEntries.prev = pHashTableEntry->list_TimeoutHashEntries.prev;			
			}
		}
	}	
	pHashTableEntry->list_TimeoutHashEntries.prev = NULL;
	pHashTableEntry->list_TimeoutHashEntries.next = NULL;
			
	return E_SUCCESS;
}	
#endif	

/** Removes all channels, which belongs to the given connection and 
	* the connection itself from the list.
	* @param pMuxSocket the connection from the user
	* @retval E_SUCCESS if successful
	* @retval E_UNKNOWN in case of an error
	*/
SINT32 CAFirstMixChannelList::remove(CAMuxSocket* pMuxSocket)
	{
		if(pMuxSocket==NULL)
			return E_UNKNOWN;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		m_Mutex.lock();
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		if(pHashTableEntry->pMuxSocket==NULL) //this connection is not in the list
			{
				m_Mutex.unlock();
				return E_UNKNOWN;
			}
	#ifdef DELAY_USERS
		m_pMutexDelayChannel->lock();
		m_pDelayBuckets[pHashTableEntry->delayBucketID]=NULL;
		m_pMutexDelayChannel->unlock();
	#endif
		pHashTableEntry->pControlChannelDispatcher->deleteAllControlChannels();
		delete pHashTableEntry->pControlChannelDispatcher; //deletes the dispatcher and all associated control channels
		delete pHashTableEntry->pControlMessageQueue;
		pHashTableEntry->pControlChannelDispatcher = NULL;
		pHashTableEntry->pControlMessageQueue = NULL;
		if(m_listHashTableNext==pHashTableEntry) //adjust the enumeration over all connections (@see getNext())
			m_listHashTableNext=pHashTableEntry->list_HashEntries.next;
		
		if(pHashTableEntry->list_HashEntries.prev==NULL) //if entry is the head of the connection list
		{
			if(pHashTableEntry->list_HashEntries.next==NULL) //if entry is also the last (so the only one in the list..)
			{
				m_listHashTableHead=NULL; //list is now empty
			}
			else
			{//remove the head of the list
				m_listHashTableHead=pHashTableEntry->list_HashEntries.next; 
				m_listHashTableHead->list_HashEntries.prev=NULL;
			}
		}
		else
		{	//the connection is not the head of the list
			if(pHashTableEntry->list_HashEntries.next==NULL)
			{//the connection is the last element in the list
				pHashTableEntry->list_HashEntries.prev->list_HashEntries.next=NULL;
			}
			else
			{//its a simple middle element
				pHashTableEntry->list_HashEntries.prev->list_HashEntries.next=pHashTableEntry->list_HashEntries.next;
				pHashTableEntry->list_HashEntries.next->list_HashEntries.prev=pHashTableEntry->list_HashEntries.prev;
			}
		}
		
		
#ifdef PAYMENT		
		removeFromTimeoutList(pHashTableEntry);
#endif		
		
		fmChannelListEntry* pEntry=pHashTableEntry->pChannelList;
		fmChannelListEntry* pTmpEntry;
		while(pEntry!=NULL)//for all channels....
			{
				//remove the out channel form the out channel hast table
				hashkey=pEntry->channelOut&0x0000FFFF;
				pTmpEntry=m_HashTableOutChannels[hashkey];
				while(pTmpEntry!=NULL)
					{
						if(pTmpEntry->channelOut==pEntry->channelOut)
							{//we have found the entry
								if(pTmpEntry->list_OutChannelHashTable.prev==NULL) //it's the head
									{
										if(pTmpEntry->list_OutChannelHashTable.next==NULL)
											{//it's also the last Element
												m_HashTableOutChannels[hashkey]=NULL; //empty this hash bucket
											}
										else
											{												
												pTmpEntry->list_OutChannelHashTable.next->list_OutChannelHashTable.prev=NULL;
												m_HashTableOutChannels[hashkey]=pTmpEntry->list_OutChannelHashTable.next;
											}
									}
								else
									{//not the head
										if(pTmpEntry->list_OutChannelHashTable.next==NULL)
											{//but the last
												pTmpEntry->list_OutChannelHashTable.prev->list_OutChannelHashTable.next=NULL;
											}
										else
											{//a middle element
												pTmpEntry->list_OutChannelHashTable.prev->list_OutChannelHashTable.next=pTmpEntry->list_OutChannelHashTable.next;
												pTmpEntry->list_OutChannelHashTable.next->list_OutChannelHashTable.prev=pTmpEntry->list_OutChannelHashTable.prev;
											}
									}
								break;
							}
						pTmpEntry=pTmpEntry->list_OutChannelHashTable.next;
				}

				pTmpEntry=pEntry->list_InChannelPerSocket.next;
#ifndef DO_TRACE				
				delete pEntry;
				pEntry = NULL;
#else
				deleteChannelListEntry(pEntry);
#endif
				pEntry=pTmpEntry;
			}
/* already done by pHashTableEntry->pControlChannelDispatcher->deleteAllControlChannels();
#ifdef PAYMENT
		// cleanup accounting information
		CAAccountingInstance::cleanupTableEntry(pHashTableEntry);
#endif
*/
#ifdef LOG_DIALOG
		delete[] pHashTableEntry->strDialog;
		pHashTableEntry->strDialog = NULL;
#endif
		memset(pHashTableEntry,0,sizeof(fmHashTableEntry)); //'delete' the connection from the connection hash table 
		m_Mutex.unlock();
		return E_SUCCESS;
	}


#ifdef NEW_MIX_TYPE
/* some additional methods for TypeB first mixes */

/** 
 * Removes all channels, which belongs to the given connection and 
 * the connection itself from the list but keeps the outgoing channels.
 * @param pMuxSocket the connection from the user
 * @retval E_SUCCESS if successful
 * @retval E_UNKNOWN in case of an error
 */
SINT32 CAFirstMixChannelList::removeClientPart(CAMuxSocket* pMuxSocket)
  {
    if(pMuxSocket==NULL)
      return E_UNKNOWN;
    SINT32 hashkey=pMuxSocket->getSocket();
    if(hashkey>MAX_HASH_KEY-1||hashkey<0)
      return E_UNKNOWN;
    m_Mutex.lock();
    fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
    if(pHashTableEntry->pMuxSocket==NULL) //this connection is not in the list
      {
        m_Mutex.unlock();
        return E_UNKNOWN;
      }
    #ifdef DELAY_USERS
      m_pMutexDelayChannel->lock();
      m_pDelayBuckets[pHashTableEntry->delayBucketID]=NULL;
      m_pMutexDelayChannel->unlock();
    #endif
    pHashTableEntry->pControlChannelDispatcher->deleteAllControlChannels();
    delete pHashTableEntry->pControlChannelDispatcher; //deletes the dispatcher and all associated control channels
    pHashTableEntry->pControlChannelDispatcher = NULL;
    if(m_listHashTableNext==pHashTableEntry) //adjust the enumeration over all connections (@see getNext())
      m_listHashTableNext=pHashTableEntry->list_HashEntries.next;
    
    if(pHashTableEntry->list_HashEntries.prev==NULL) //if entry is the head of the connection list
      {
        if(pHashTableEntry->list_HashEntries.next==NULL) //if entry is also the last (so the only one in the list..)
          {
            m_listHashTableHead=NULL; //list is now empty
          }
        else
          {//remove the head of the list
            m_listHashTableHead=pHashTableEntry->list_HashEntries.next; 
            m_listHashTableHead->list_HashEntries.prev=NULL;
          }
      }
    else
      {//the connection is not the head of the list
        if(pHashTableEntry->list_HashEntries.next==NULL)
          {//the connection is the last element in the list
            pHashTableEntry->list_HashEntries.prev->list_HashEntries.next=NULL;
          }
        else
          {//its a simple middle element
            pHashTableEntry->list_HashEntries.prev->list_HashEntries.next=pHashTableEntry->list_HashEntries.next;
            pHashTableEntry->list_HashEntries.next->list_HashEntries.prev=pHashTableEntry->list_HashEntries.prev;
          }
      }
      
		removeFromTimeoutList(pHashTableEntry);      
      
      
    fmChannelListEntry* pEntry=pHashTableEntry->pChannelList;
    while(pEntry!=NULL)//for all channels....
      {
        /* leave a dummy-entry in the out-channels-table until we receive a
         * CLOSE-message for the channel from the last mix (else we could
         * re-use it, while the last mix is still using the old channel),
         * therefore set the the pointer for the in-channel-part to NULL
         */
        pEntry->pHead = NULL;
        pEntry = pEntry->list_InChannelPerSocket.next;
      }
    #ifdef PAYMENT
      // cleanup accounting information
      CAAccountingInstance::cleanupTableEntry(pHashTableEntry);
    #endif
    memset(pHashTableEntry,0,sizeof(fmHashTableEntry)); //'delete' the connection from the connection hash table 
    m_Mutex.unlock();
    return E_SUCCESS;
  }

/**
 * Removes an out-channel from the table, which is vacant (not connected to
 * any client).
 * @param pEntry The vacant out-channel entry.
 */
void CAFirstMixChannelList::removeVacantOutChannel(fmChannelListEntry* pEntry) {
  if (pEntry != NULL) {
    if (pEntry->pHead == NULL) {
      /* must be a vacant channel */
      m_Mutex.lock();
      fmChannelListEntry* pTmpEntry;
      /* check whether the enty is in the out-channel-table */
      SINT32 hashkey = pEntry->channelOut & 0x0000FFFF;
      pTmpEntry = m_HashTableOutChannels[hashkey];
      while (pTmpEntry != NULL) {
        if (pTmpEntry->channelOut == pEntry->channelOut) {
          //we have found the entry
          if (pTmpEntry->list_OutChannelHashTable.prev==NULL) { //it's the head      
            if (pTmpEntry->list_OutChannelHashTable.next==NULL) {
              //it's also the last Element
              m_HashTableOutChannels[hashkey] = NULL; //empty this hash bucket
            }
            else {
              pTmpEntry->list_OutChannelHashTable.next->list_OutChannelHashTable.prev=NULL;
              m_HashTableOutChannels[hashkey]=pTmpEntry->list_OutChannelHashTable.next;
            }
          }
          else {
            //not the head
            if (pTmpEntry->list_OutChannelHashTable.next==NULL) {
              //but the last
              pTmpEntry->list_OutChannelHashTable.prev->list_OutChannelHashTable.next=NULL;
            }
            else {
              //a middle element
              pTmpEntry->list_OutChannelHashTable.prev->list_OutChannelHashTable.next=pTmpEntry->list_OutChannelHashTable.next;
              pTmpEntry->list_OutChannelHashTable.next->list_OutChannelHashTable.prev=pTmpEntry->list_OutChannelHashTable.prev;
            }
          }
          break;
        }
        pTmpEntry=pTmpEntry->list_OutChannelHashTable.next;
      }
      /* entry is not in the table any more */
      #ifndef DO_TRACE        
        delete pEntry;
        pEntry = NULL;
      #else
        deleteChannelListEntry(pEntry);
      #endif
      m_Mutex.unlock();
    }
  }
}

/**
 * Removes all out-channels from the table, which are vacant (not connected to
 * any client). Also the channel-cipher is deleted.
 */
void CAFirstMixChannelList::cleanVacantOutChannels() {
  m_Mutex.lock();
  SINT32 hashkey = 0;
  do {
    fmChannelListEntry* pTmpEntry = m_HashTableOutChannels[hashkey];
    while (pTmpEntry != NULL) {
      if (pTmpEntry->pHead == NULL) {
        /* we have found a vacant channel */
        if (pTmpEntry->list_OutChannelHashTable.prev==NULL) { //it's the head      
          if (pTmpEntry->list_OutChannelHashTable.next==NULL) {
            //it's also the last Element
            m_HashTableOutChannels[hashkey] = NULL; //empty this hash bucket
          }
          else {
            pTmpEntry->list_OutChannelHashTable.next->list_OutChannelHashTable.prev=NULL;
            m_HashTableOutChannels[hashkey]=pTmpEntry->list_OutChannelHashTable.next;
          }
        }
        else {
          //not the head
          if (pTmpEntry->list_OutChannelHashTable.next==NULL) {
            //but the last
            pTmpEntry->list_OutChannelHashTable.prev->list_OutChannelHashTable.next=NULL;
          }
          else {
            //a middle element
            pTmpEntry->list_OutChannelHashTable.prev->list_OutChannelHashTable.next=pTmpEntry->list_OutChannelHashTable.next;
            pTmpEntry->list_OutChannelHashTable.next->list_OutChannelHashTable.prev=pTmpEntry->list_OutChannelHashTable.prev;
          }
        }
        /* entry is removed from the table, now delete the channel-cipher */
        delete pTmpEntry->pCipher;
        pTmpEntry->pCipher = NULL;
        fmChannelListEntry* pRemoveEntry = pTmpEntry;
        pTmpEntry = pTmpEntry->list_OutChannelHashTable.next;
        /* delete the entry */
        #ifndef DO_TRACE        
          delete pRemoveEntry;
          pRemoveEntry = NULL;
        #else
          deleteChannelListEntry(pEntry);
        #endif
      }
      else {
        /* not a vacant channel -> try the next channel in the hashtable-line */
        pTmpEntry = pTmpEntry->list_OutChannelHashTable.next;
      }
    }
    /* we have processed a whole line of the channel-table -> process the next
     * one
     */
    hashkey = (hashkey + 1) & 0x0000FFFF;
  }
  while (hashkey != 0);
  /* we have processed the whole out-channel-table */
  m_Mutex.unlock();
}
#endif //NEW_MIX_TYPE (TypeB first mixes)

/** Removes a single channel from the list.
	* @param pMuxSocket the connection from the user
	* @param channelIn the channel, which should be removed
	* @retval E_SUCCESS if successful
	* @retval E_UNKNOWN in case of an error
	*/
SINT32 CAFirstMixChannelList::removeChannel(CAMuxSocket* pMuxSocket,HCHANNEL channelIn)
	{
		if(pMuxSocket==NULL)
			return E_UNKNOWN;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		m_Mutex.lock();
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		if(pHashTableEntry->pMuxSocket==NULL)
			{
				m_Mutex.unlock();
				return E_UNKNOWN;
			}
		fmChannelListEntry* pEntry=pHashTableEntry->pChannelList;
		while(pEntry!=NULL)
			{
				if(pEntry->channelIn==channelIn) //search for the channel
					{
						hashkey=pEntry->channelOut&0x0000FFFF; //remove the out channel from the out channel hash table
						fmChannelListEntry*pTmpEntry=m_HashTableOutChannels[hashkey];
						while(pTmpEntry!=NULL)
							{
								if(pTmpEntry->channelOut==pEntry->channelOut)
									{//found it in the out channel hash table
										if(pTmpEntry->list_OutChannelHashTable.prev==NULL) //head
											{
												if(pTmpEntry->list_OutChannelHashTable.next==NULL)
													{
														m_HashTableOutChannels[hashkey]=NULL;
													}
												else
													{
														
														pTmpEntry->list_OutChannelHashTable.next->list_OutChannelHashTable.prev=NULL;
														m_HashTableOutChannels[hashkey]=pTmpEntry->list_OutChannelHashTable.next;
													}
											}
										else
											{
												if(pTmpEntry->list_OutChannelHashTable.next==NULL)
													{//last element
														pTmpEntry->list_OutChannelHashTable.prev->list_OutChannelHashTable.next=NULL;
													}
												else
													{//middle element
														pTmpEntry->list_OutChannelHashTable.prev->list_OutChannelHashTable.next=pTmpEntry->list_OutChannelHashTable.next;
														pTmpEntry->list_OutChannelHashTable.next->list_OutChannelHashTable.prev=pTmpEntry->list_OutChannelHashTable.prev;
													}
											}
										break;
									}
								pTmpEntry=pTmpEntry->list_OutChannelHashTable.next;
						}

						//remove the channel from the channel hast table
						if(pEntry->list_InChannelPerSocket.prev==NULL) //head
							{
								if(pEntry->list_InChannelPerSocket.next==NULL)
									{//the only element
										pHashTableEntry->pChannelList=NULL;
									}
								else
									{
										pEntry->list_InChannelPerSocket.next->list_InChannelPerSocket.prev=NULL;
										pHashTableEntry->pChannelList=pEntry->list_InChannelPerSocket.next;
									}
							}
						else
							{
								if(pEntry->list_InChannelPerSocket.next==NULL)
									{//the last element
										pEntry->list_InChannelPerSocket.prev->list_InChannelPerSocket.next=NULL;
									}
								else
									{//a middle element
										pEntry->list_InChannelPerSocket.prev->list_InChannelPerSocket.next=pEntry->list_InChannelPerSocket.next;
										pEntry->list_InChannelPerSocket.next->list_InChannelPerSocket.prev=pEntry->list_InChannelPerSocket.prev;
									}
							}
						#ifndef DO_TRACE				
							delete pEntry;
							pEntry = NULL;
						#else
							deleteChannelListEntry(pEntry);
						#endif
						pHashTableEntry->cNumberOfChannels--;
						m_Mutex.unlock();
						return E_SUCCESS;
					}
				pEntry=pEntry->list_InChannelPerSocket.next; //try next channel
			}
		m_Mutex.unlock();
		return E_UNKNOWN;//not found
	}

/** Gets the first connection of all connections in the list.
	* @see getNext()
	* @return first connection in the list
	* @retval NULL if no connection is in the list
	*/
fmHashTableEntry* CAFirstMixChannelList::getFirst()
	{
		m_Mutex.lock();
		if(m_listHashTableHead!=NULL)
			m_listHashTableNext=m_listHashTableHead->list_HashEntries.next;
		else
			m_listHashTableNext=NULL;
		m_Mutex.unlock();
		return m_listHashTableHead;
	}

/** Gets the next entry in the connections-list.
	* @see getFirst()
	* @return next entry in the connection list
	* @retval NULL in case of an error
	*/
fmHashTableEntry* CAFirstMixChannelList::getNext()
	{
		m_Mutex.lock();
		fmHashTableEntry* tmpEntry=m_listHashTableNext;
		if(m_listHashTableNext!=NULL)
			m_listHashTableNext=m_listHashTableNext->list_HashEntries.next;
		m_Mutex.unlock();
		return tmpEntry;
	}

/** Gets the first channel for a given connection.
	* @see getNextChannel()
	* @param pMuxSocket the connection from the user
	* @return the channel and the associated information
	* @retval NULL if no channel for this connection exists at the moment
	*/
fmChannelListEntry* CAFirstMixChannelList::getFirstChannelForSocket(CAMuxSocket* pMuxSocket)
	{
		if(pMuxSocket==NULL)
			return NULL;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return NULL;
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		return pHashTableEntry->pChannelList;
	}

/** Gets the next channel for a given connection.
	* @see getFirstChannelForSocket()
	* @param pEntry a entry returned by a previos call to getFirstChannelForSocket() or getNextChannel()
	* @return the next channel and all associated information
	* @retval NULL if there are no more channels for this connection
	*/
fmChannelListEntry* CAFirstMixChannelList::getNextChannel(fmChannelListEntry* pEntry)
	{
		if(pEntry==NULL)
			return NULL;
		return pEntry->list_InChannelPerSocket.next;
	}

SINT32 CAFirstMixChannelList::test()
	{
		CAFirstMixChannelList* pList=new CAFirstMixChannelList();
		CAMuxSocket *pMuxSocket=new CAMuxSocket();
		((CASocket*)pMuxSocket)->create();
		UINT8 peerIP[4];
#ifndef LOG_DIALOG
		pList->add(pMuxSocket,peerIP,NULL);
#else
		pList->add(pMuxSocket,peerIP,NULL,(UINT8*)"1");
#endif
#if defined(HAVE_CRTDBG)
		_CrtMemState s1, s2, s3;
		_CrtMemCheckpoint( &s1 );
#endif
		UINT32 /*channelIn,*/i,channelOut;
		for(i=0;i<50;i++)
			pList->addChannel(pMuxSocket,i,NULL,&channelOut);
		for(i=0;i<50;i++)
			pList->removeChannel(pMuxSocket,i);
#if defined(HAVE_CRTDBG)
		_CrtMemCheckpoint( &s2 );
		if ( _CrtMemDifference( &s3, &s1, &s2 ) )
      _CrtMemDumpStatistics( &s3 );
#endif
		
		pList->remove(pMuxSocket);
		delete pMuxSocket;
		pMuxSocket = NULL;
		delete pList;
		pList = NULL;
		return E_SUCCESS;
	}
	
#ifdef DELAY_USERS
	THREAD_RETURN fml_loopDelayBuckets(void* param)
		{
			INIT_STACK;
			BEGIN_STACK("CAFirstMixChannelList::fml_loopDelayBuckets");
			
			CAFirstMixChannelList* pChannelList=(CAFirstMixChannelList*)param;
			volatile UINT32** pDelayBuckets=pChannelList->m_pDelayBuckets;
			while(pChannelList->m_bDelayBucketsLoopRun)
				{
					pChannelList->m_pMutexDelayChannel->lock();
					UINT32 u32BucketGrow=pChannelList->m_u32DelayChannelBucketGrow;
					UINT32 u32MaxBucket=u32BucketGrow*10;
					for(UINT32 i=0;i<MAX_POLLFD;i++)
						{
							if(pDelayBuckets[i]!=NULL&&*(pDelayBuckets[i])<u32MaxBucket)
							{
								*(pDelayBuckets[i])+=u32BucketGrow;
							}
						}
					pChannelList->m_pMutexDelayChannel->unlock();		
					msSleep(pChannelList->m_u32DelayChannelBucketGrowIntervall);
				}
			
			FINISH_STACK("CAFirstMixChannelList::fml_loopDelayBuckets");	
				
			THREAD_RETURN_SUCCESS;
		}
	
	void CAFirstMixChannelList::decDelayBuckets(UINT32 delayBucketID)
	{
		m_pMutexDelayChannel->lock();
		if(delayBucketID < MAX_POLLFD)
		{
			if(m_pDelayBuckets[delayBucketID] != NULL)
			{
				*(m_pDelayBuckets[delayBucketID]) -= ( (*(m_pDelayBuckets[delayBucketID])) > 0 ) ? 1 : 0;
			}
			/*CAMsg::printMsg(LOG_DEBUG,"DelayBuckets decrementing ID %u downto %u\n", 
								delayBucketID, (*(m_pDelayBuckets[delayBucketID])) );*/
		}
		m_pMutexDelayChannel->unlock();
	}
	
	bool CAFirstMixChannelList::hasDelayBuckets(UINT32 delayBucketID)
	{
		bool ret = false;
		m_pMutexDelayChannel->lock();
		if(delayBucketID < MAX_POLLFD)
		{
			if(m_pDelayBuckets[delayBucketID] != NULL)
			{
				ret = ( (*(m_pDelayBuckets[delayBucketID])) > 0 );
			}
		}
		m_pMutexDelayChannel->unlock();
		return ret;
	}
	
	void CAFirstMixChannelList::setDelayParameters(UINT32 unlimitTraffic,UINT32 bucketGrow,UINT32 intervall)
		{
			m_pMutexDelayChannel->lock();
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMixChannelList - Set new traffic limit per user- unlimit: %u bucketgrow: %u intervall %u\n",
				unlimitTraffic,bucketGrow,intervall);
			m_u32DelayChannelUnlimitTraffic=unlimitTraffic;
			m_u32DelayChannelBucketGrow=bucketGrow;
			m_u32DelayChannelBucketGrowIntervall=intervall;
			for(UINT32 i=0;i<MAX_POLLFD;i++)
				if(m_pDelayBuckets[i]!=NULL)
					*(m_pDelayBuckets[i])=m_u32DelayChannelUnlimitTraffic;
			m_pMutexDelayChannel->unlock();		
		}																												
		
#endif	
#endif //ONLY_LOCAL_PROXY
