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
#include "CAFirstMixChannelList.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"
#define MAX_HASH_KEY 8113

#ifdef PAYMENT
#include "CAAccountingInstance.hpp"
#endif

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
		m_HashTableOutChannels=new LP_fmChannelListEntry[0x10000];
		memset(m_HashTableOutChannels,0,sizeof(LP_fmChannelListEntry)*0x10000);
#ifdef DO_TRACE
	m_aktAlloc=m_maxAlloc=0;
#endif
//#ifdef PAYMENT
//	m_pAccountingInstance = CAAccountingInstance::getInstance();
//#endif
	}

CAFirstMixChannelList::~CAFirstMixChannelList()
	{
		for(int i=0;i<MAX_HASH_KEY;i++)
				{
					delete m_HashTable[i];
				}
		delete []m_HashTable;
		delete []m_HashTableOutChannels;
	}
		
/** Adds a new TCP/IP connection (a new user) to the channel list.
	* @param pMuxSocket the new connection (from a user)
	* @param peerIP the IP of the user, so that we can remove it later from the CAIPList
	* @param pQueueSend the send-queue to use for this connection
	* @retval E_UNKNOWN in case of an error
	* @retval E_SUCCESS if successful
	*/
SINT32 CAFirstMixChannelList::add(CAMuxSocket* pMuxSocket,UINT8 peerIP[4],CAQueue* pQueueSend)
	{
		if(pMuxSocket==NULL)
			return E_UNKNOWN;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		m_Mutex.lock();
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		if(pHashTableEntry->pMuxSocket!=NULL) //the entry in the hashtable for this socket (hashkey) must be empty
			{
				m_Mutex.unlock();
				return E_UNKNOWN;
			}
		pHashTableEntry->pMuxSocket=pMuxSocket;
		pHashTableEntry->pQueueSend=pQueueSend;
		pHashTableEntry->pControlChannelDispatcher=new CAControlChannelDispatcher(pQueueSend);
		pHashTableEntry->uAlreadySendPacketSize=0;
		pHashTableEntry->cNumberOfChannels=0;
#ifdef LOG_CHANNEL
		pHashTableEntry->trafficIn=0;
		pHashTableEntry->trafficOut=0;
		getcurrentTimeMillis(pHashTableEntry->timeCreated);
		getRandom((UINT8*)&pHashTableEntry->id,8);
#endif
		memcpy(pHashTableEntry->peerIP,peerIP,4);

		//now insert the new connection in the list of all open connections
		if(m_listHashTableHead==NULL) //if first one
			{
				pHashTableEntry->list_HashEntries.next=NULL;
				pHashTableEntry->list_HashEntries.prev=NULL;
				m_listHashTableHead=pHashTableEntry;				
			}
		else
			{//add to the head of the double linked list
				pHashTableEntry->list_HashEntries.next=m_listHashTableHead;
				pHashTableEntry->list_HashEntries.prev=NULL;
				m_listHashTableHead->list_HashEntries.prev=pHashTableEntry;
				m_listHashTableHead=pHashTableEntry;				
			}
#ifdef PAYMENT
		// init accounting instance for this user
		CAAccountingInstance *pAccInst;
		pAccInst = CAAccountingInstance::getInstance();
//		pAccInst->initTableEntry(pHashTableEntry);
//		m_pAccountingInstance->initTableEntry(pHashTableEntry);
#endif
		m_Mutex.unlock();
		return E_SUCCESS;
	}

///The maximum number of channels allowed per connection
#define MAX_NUMBER_OF_CHANNELS 50

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
		delete pHashTableEntry->pControlChannelDispatcher;
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
#else
				deleteChannelListEntry(pEntry);
#endif
				pEntry=pTmpEntry;
			}
#ifdef PAYMENT
		// cleanup accounting information
		CAAccountingInstance * pAccInst;
		pAccInst = CAAccountingInstance::getInstance();
		pAccInst->cleanupTableEntry(pHashTableEntry);
	//	m_pAccountingInstance->cleanupTableEntry(pHashTableEntry);
#endif
		memset(pHashTableEntry,0,sizeof(fmHashTableEntry)); //'delete' the connection from the connection hash table 
		m_Mutex.unlock();
		return E_SUCCESS;
	}

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
		pList->add(pMuxSocket,peerIP,NULL);
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
		delete pList;
		return E_SUCCESS;
	}
