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
#include "CALastMixChannelList.hpp"
#include "CAUtil.hpp"

CALastMixChannelList::CALastMixChannelList()
	{
		m_HashTable=new LP_lmChannelListEntry[0x10000];
		memset(m_HashTable,0,0x10000*sizeof(LP_lmChannelListEntry));
		m_listSockets=NULL;
		m_listSocketsNext=NULL;
		m_nChannels=0;
#ifdef DELAY_CHANNELS
		m_u32DelayChannelUnlimitTraffic=DELAY_CHANNEL_TRAFFIC;
		m_u32DelayChannelBucketGrow=DELAY_BUCKET_GROW;
		m_u32DelayChannelBucketGrowIntervall=DELAY_BUCKET_GROW_INTERVALL;
		m_pDelayBuckets=new UINT32*[MAX_POLLFD];
		memset(m_pDelayBuckets,0,sizeof(UINT32*)*MAX_POLLFD);
		m_pMutexDelayChannel=new CAMutex();
		m_pThreadDelayBucketsLoop=new CAThread((UINT8*)"Delay Buckets Thread");
		m_bDelayBucketsLoopRun=true;
		m_pThreadDelayBucketsLoop->setMainLoop(lml_loopDelayBuckets);
		m_pThreadDelayBucketsLoop->start(this);
#endif
#ifdef DELAY_CHANNELS_LATENCY
		m_u32DelayChannelLatency=DELAY_CHANNEL_LATENCY;
#endif
	}

CALastMixChannelList::~CALastMixChannelList()
	{
#ifdef DELAY_CHANNELS
		m_bDelayBucketsLoopRun=false;
		m_pThreadDelayBucketsLoop->join();
		delete m_pThreadDelayBucketsLoop;
		delete m_pMutexDelayChannel;
		delete []m_pDelayBuckets;
#endif
		for(UINT32 i=0;i<0x10000;i++)
			{
				lmChannelListEntry* akt=m_HashTable[i];
				lmChannelListEntry* tmp;
				while(akt!=NULL)
					{
						tmp=akt;
						akt=akt->list_Channels.next;
						delete tmp;
					}
			}
		delete[] m_HashTable;
	}

#if defined (LOG_CHANNEL) 
	SINT32 CALastMixChannelList::add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher,CAQueue* pQueue,UINT64 time, UINT32 trafficInFromUser)
#elif defined (DELAY_CHANNELS_LATENCY)
	SINT32 CALastMixChannelList::add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher,CAQueue* pQueue,UINT64 time)
#else
	SINT32 CALastMixChannelList::add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher,CAQueue* pQueue)
#endif
	{
		UINT32 hash=id&0x0000FFFF;
		lmChannelListEntry* pEntry=m_HashTable[hash];
		lmChannelListEntry* pNewEntry=new lmChannelListEntry;
		pNewEntry->channelIn=id;
		pNewEntry->pCipher=pCipher;
		pNewEntry->pSocket=pSocket;
		pNewEntry->pQueueSend=pQueue;
#if defined (LOG_CHANNEL)
		pNewEntry->timeCreated=time;
#endif
#if defined (DELAY_CHANNELS_LATENCY)
		pNewEntry->timeLatency=time+m_u32DelayChannelLatency;
#endif
#ifdef LOG_CHANNEL
		pNewEntry->trafficInFromUser=trafficInFromUser;
		pNewEntry->packetsDataInFromUser=1;
		pNewEntry->packetsDataOutToUser=0;
#endif
#if defined (LOG_CHANNEL)
		pNewEntry->trafficOutToUser=0;
#endif
#ifdef NEW_FLOW_CONTROL
		pNewEntry->sendmeCounter=0;
#endif		
#ifdef DELAY_CHANNELS
		pNewEntry->delayBucket=m_u32DelayChannelUnlimitTraffic; //can always send some first packets
		for(UINT32 i=0;i<MAX_POLLFD;i++)
			{
				if(m_pDelayBuckets[i]==NULL)
					{
						pNewEntry->delayBucketID=i;
						break;
					}
			}
		m_pDelayBuckets[pNewEntry->delayBucketID]=&pNewEntry->delayBucket;
#endif
		if(pEntry==NULL) //First Entry for Hash in HashTable
			{
				pNewEntry->list_Channels.next=NULL;
				pNewEntry->list_Channels.prev=NULL;
			}
		else //insert in Hashlist for Hashtableentry
			{
				pNewEntry->list_Channels.prev=NULL;
				pNewEntry->list_Channels.next=pEntry;
				pEntry->list_Channels.prev=pNewEntry;
			}
		//Insert in SocketList
		if(m_listSockets==NULL)
			{
				m_listSockets=pNewEntry;
				pNewEntry->list_Sockets.next=NULL;
				pNewEntry->list_Sockets.prev=NULL;
			}
		else
			{
				pNewEntry->list_Sockets.prev=NULL;
				pNewEntry->list_Sockets.next=m_listSockets;
				m_listSockets->list_Sockets.prev=pNewEntry;
				m_listSockets=pNewEntry;				
			}
		m_HashTable[hash]=pNewEntry;
		//if(m_listSocketsNext==NULL)
		//	m_listSocketsNext=m_listSockets;
		m_nChannels++;
		return E_SUCCESS;
	}

SINT32 CALastMixChannelList::removeChannel(HCHANNEL channel)
	{
		UINT32 hash=channel&0x0000FFFF;
		lmChannelListEntry* pEntry=m_HashTable[hash];
		while(pEntry!=NULL)
			{
				if(pEntry->channelIn==channel)
					{
						if(m_listSocketsNext==pEntry) //removing next enumeration Element...
							m_listSocketsNext=m_listSocketsNext->list_Sockets.next; //adjusting!
//remove from HashTable
						if(pEntry->list_Channels.prev==NULL)
							m_HashTable[hash]=pEntry->list_Channels.next;
						else
							{
								pEntry->list_Channels.prev->list_Channels.next=pEntry->list_Channels.next;
							}
						if(pEntry->list_Channels.next!=NULL)
							{
								pEntry->list_Channels.next->list_Channels.prev=pEntry->list_Channels.prev;
							}
						//remove from SocketList
						if(pEntry->list_Sockets.prev==NULL)
							m_listSockets=pEntry->list_Sockets.next;
						else
							pEntry->list_Sockets.prev->list_Sockets.next=pEntry->list_Sockets.next;
						if(pEntry->list_Sockets.next!=NULL)
							pEntry->list_Sockets.next->list_Sockets.prev=pEntry->list_Sockets.prev;
						#ifdef DELAY_CHANNELS
							m_pMutexDelayChannel->lock();
							m_pDelayBuckets[pEntry->delayBucketID]=NULL;
							m_pMutexDelayChannel->unlock();
						#endif
						delete pEntry;
						m_nChannels--;					
						return E_SUCCESS;
					}
				pEntry=pEntry->list_Channels.next;
			}
		return E_SUCCESS;
	}

SINT32 CALastMixChannelList::test()
				{
#if !defined (LOG_CHANNEL)&&!defined(DELAY_CHANNELS_LATENCY)
					CALastMixChannelList oList;
					UINT32 c;
					UINT32 rand;
					int i,j;
					for(i=0;i<100;i++)
						{
							getRandom(&c);
							oList.add(c,NULL,NULL,NULL);

					}
					for(i=0;i<10000;i++)
						{
							lmChannelListEntry* akt=oList.getFirstSocket();
							while(akt!=NULL)
								{
									getRandom(&rand);
									if(rand<0x7FFFFFFF)
										{
											getRandom(&c);
											oList.add(c,NULL,NULL,NULL);
										}
									getRandom(&rand);
									if(rand<0x7FFFFFFF)
										oList.removeChannel(akt->channelIn);
									getRandom(&rand);
									if(rand<0x0FFFFFFF)
										for(j=0;j<5;j++)
											{
												getRandom(&c);
												oList.add(c,NULL,NULL,NULL);
											}
									getRandom(&rand);
									if(rand<0x7FFFFFFF)
										for(j=0;j<10000;j++)
											{
												getRandom(&c);
												oList.removeChannel(c);
											}
									akt=oList.getNextSocket();
								}
						}
#endif
					return 0;
				}

#ifdef DELAY_CHANNELS
	THREAD_RETURN lml_loopDelayBuckets(void* param)
		{
			CALastMixChannelList* pChannelList=(CALastMixChannelList*)param;
			UINT32** pDelayBuckets=pChannelList->m_pDelayBuckets;
			while(pChannelList->m_bDelayBucketsLoopRun)
				{
					pChannelList->m_pMutexDelayChannel->lock();
					UINT32 u32BucketGrow=pChannelList->m_u32DelayChannelBucketGrow;
					UINT32 u32MaxBucket=u32BucketGrow*10;
					for(UINT32 i=0;i<MAX_POLLFD;i++)
						{
							if(pDelayBuckets[i]!=NULL&&*(pDelayBuckets[i])<u32MaxBucket)
								*(pDelayBuckets[i])+=u32BucketGrow;
						}
					pChannelList->m_pMutexDelayChannel->unlock();		
					msSleep(pChannelList->m_u32DelayChannelBucketGrowIntervall);
				}
			THREAD_RETURN_SUCCESS;
		}
		
	void CALastMixChannelList::setDelayParameters(UINT32 unlimitTraffic,UINT32 bucketGrow,UINT32 intervall)
		{
			m_pMutexDelayChannel->lock();
			CAMsg::printMsg(LOG_DEBUG,"CALastMixChannelList - Set new traffic limit per channel- unlimit: %u bucketgrow: %u intervall %u\n",
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

#ifdef DELAY_CHANNELS_LATENCY
	void CALastMixChannelList::setDelayLatencyParameters(UINT32 latency)
		{
			CAMsg::printMsg(LOG_DEBUG,"CALastMixChannelList - Set new latency: %u ms\n",latency);
			m_u32DelayChannelLatency=latency;
		}
#endif
#endif //ONLY_LOCAL_PROXY
