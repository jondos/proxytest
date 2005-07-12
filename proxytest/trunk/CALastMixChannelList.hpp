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
#ifndef __CALASTMIXCHANNELLIST__
#define __CALASTMIXCHANNELLIST__
#include "CASocket.hpp"
#include "CAMuxSocket.hpp"
#include "CAQueue.hpp"
#include "CASymCipher.hpp"
#include "CAMutex.hpp"
#include "CAMsg.hpp"
#include "CAThread.hpp"

struct t_lastmixchannellist
	{
		public:

			HCHANNEL channelIn;
		
			CASymCipher*  pCipher;
			CASocket*			pSocket;
			CAQueue*			pQueueSend;
#ifdef DELAY_CHANNELS
			UINT32				delayBucket;
			UINT32				delayBucketID;
#endif
#if defined (LOG_CHANNEL)||defined (DELAY_CHANNELS_LATENCY)
			UINT64				timeCreated;
#endif
#ifdef LOG_CHANNEL
			UINT32				trafficInFromUser;
			UINT32				packetsDataOutToUser;
			UINT32				packetsDataInFromUser;
#endif
#if defined (LOG_CHANNEL)
			UINT32				trafficOutToUser;
#endif
		private:
			struct
				{
					struct t_lastmixchannellist* prev;
					struct t_lastmixchannellist* next;
				} list_Channels;

			struct
				{
					struct t_lastmixchannellist* prev;
					struct t_lastmixchannellist* next;
				} list_Sockets;
		
		friend class CALastMixChannelList;
	};

typedef struct t_lastmixchannellist lmChannelList; 
typedef struct t_lastmixchannellist lmChannelListEntry; 
typedef lmChannelListEntry* LP_lmChannelListEntry;


class CALastMixChannelList
	{
		public:
			CALastMixChannelList();
			~CALastMixChannelList();

#ifdef LOG_CHANNEL			
			SINT32 add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher,CAQueue* pQueue,UINT64 time,UINT32 trafficIn);
#elif defined(DELAY_CHANNELS_LATENCY)
			SINT32 add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher,CAQueue* pQueue,UINT64 time);
#else
			SINT32 add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher,CAQueue* pQueue);
#endif
			lmChannelListEntry* get(HCHANNEL channelIn)
				{
					lmChannelListEntry* akt=m_HashTable[channelIn&0x0000FFFF];
					while(akt!=NULL)
						{
							if(akt->channelIn==channelIn)
								return akt;
							akt=akt->list_Channels.next;
						}
					return NULL;
				}

			lmChannelListEntry* getFirstSocket()
				{
					if(m_listSockets!=NULL)
						m_listSocketsNext=m_listSockets->list_Sockets.next;
					else
						m_listSocketsNext=NULL;
					return m_listSockets;
				}
			
			lmChannelListEntry* getNextSocket()
				{
					lmChannelListEntry* akt=m_listSocketsNext;
					if(m_listSocketsNext!=NULL)
						m_listSocketsNext=m_listSocketsNext->list_Sockets.next;
					return akt;
				}

//			SINT32 removeChannel(CASocket* pSocket);
			SINT32 removeChannel(HCHANNEL channelIn);
			UINT32 getSize(){return m_nChannels;}		
			static SINT32 test();

		private:
			UINT32 m_nChannels; //Number of channels in list
			///The Hash-Table of all channels.
			LP_lmChannelListEntry* m_HashTable;
			
			///Pointer to the head of a list of all sockets.
			lmChannelList* m_listSockets;
			///Next Element in the enumeration of all sockets.
			lmChannelList* m_listSocketsNext;
			///This mutex is used in all functions and makes them thread safe.
			CAMutex m_Mutex;
			#ifdef DELAY_CHANNELS
				UINT32** m_pDelayBuckets;
				CAThread* m_pThreadDelayBucketsLoop;
				CAMutex* m_pMutexDelayChannel;
				bool m_bDelayBucketsLoopRun;
				friend THREAD_RETURN lml_loopDelayBuckets(void*);
				//Parameters
				volatile UINT32	m_u32DelayChannelUnlimitTraffic;  //how much traffic without any delay?
				volatile UINT32 m_u32DelayChannelBucketGrow; //how many bytes to put in each bucket per time intervall
				volatile UINT32 m_u32DelayChannelBucketGrowIntervall; //duration of one time intervall in ms
																															//therefore the allowed bandwith=BucketGrow/Intervall*1000 [bytes/s]
				public:
					void setDelayParameters(UINT32 unlimitTraffic,UINT32 bucketGrow,UINT32 intervall);																												
			#endif
			#ifdef DELAY_CHANNELS_LATENCY
				//Parameters
				volatile UINT32	m_u32DelayChannelLatency;  //min latency in ms
				public:
					void setDelayLatencyParameters(UINT32 latency);																												
			#endif
	};
#endif
