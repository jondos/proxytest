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
#ifndef __CAFRISTMIXCHANNELLIST__
#define __CAFRISTMIXCHANNELLIST__
#include "CAMuxSocket.hpp"
#include "CAQueue.hpp"
#include "CASymCipher.hpp"
#include "CAMutex.hpp"
#include "CAMsg.hpp"
#ifdef PAYMENT
/**
 * Structure that holds all per-user payment information
 * Included in CAFirstMixChannelList (struct fmHashTableEntry)
 */
struct t_accountinginfo
{
	UINT8 * pReceiveBuffer; //buffer for storing incoming msgs
	UINT32  msgTotalSize;   //total size of the incoming msg
	UINT32  msgCurrentSize; //number of bytes received

	// for encrypting/decryptinh  payment packets
	CASymCipher * pCipherIn; //decrypt JAP->AI packets
	CASymCipher * pCipherOut; //encrypt AI->JAP packets

	UINT8 * pLastXmlCostConfirmation;

	UINT64 accountNumber;
	UINT64 maxBalance;

};
typedef struct t_accountinginfo aiAccountingInfo;
typedef aiAccountingInfo * LP_aiAccountingInfo;
#endif

struct t_fmhashtableentry
	{
		public:
			CAMuxSocket*	pMuxSocket;
			CAQueue*			pQueueSend;
			UINT32				cSuspend;
#ifdef LOG_CHANNEL
			UINT32				trafficIn;
			UINT32				trafficOut;
			UINT64				timeCreated;
			UINT64				id;
#endif
#ifdef PAYMENT
	aiAccountingInfo * pAccountingInfo;
#endif
			UINT8					peerIP[4]; //needed for flooding control
		private:
			UINT32				cNumberOfChannels;
			struct t_firstmixchannellist* pChannelList;

			struct
				{
					struct t_fmhashtableentry* prev;
					struct t_fmhashtableentry* next;
				} list_HashEntries;

		friend class CAFirstMixChannelList;
	};

typedef struct t_fmhashtableentry fmHashTableEntry;
typedef fmHashTableEntry* LP_fmHashTableEntry;



struct t_firstmixchannellist
	{
		public:
			fmHashTableEntry* pHead;

			HCHANNEL channelIn;
			HCHANNEL channelOut;
		
			CASymCipher* pCipher;
			bool bIsSuspended;

#ifdef LOG_CHANNEL
			UINT32				packetsInFromUser;
			UINT32				packetsOutToUser;
			UINT64				timeCreated;	
#endif

		private:
			struct
				{
					struct t_firstmixchannellist* prev;
					struct t_firstmixchannellist* next;
				} list_OutChannelHashTable;

			struct
				{
					struct t_firstmixchannellist* prev;
					struct t_firstmixchannellist* next;
				} list_InChannelPerSocket;
		
		friend class CAFirstMixChannelList;
	};

typedef struct t_firstmixchannellist fmChannelList; 
typedef struct t_firstmixchannellist fmChannelListEntry; 
typedef fmChannelListEntry* LP_fmChannelListEntry;

class CAFirstMixChannelList
	{
		public:
			CAFirstMixChannelList();
			~CAFirstMixChannelList();
		
			SINT32 add(CAMuxSocket* pMuxSocket,UINT8 peerIP[4],CAQueue* pQueueSend);
			SINT32 addChannel(CAMuxSocket* pMuxSocket,HCHANNEL channelIn,CASymCipher* pCipher,HCHANNEL* channelOut);
			
			fmChannelListEntry* get(CAMuxSocket* pMuxSocket,HCHANNEL channelIn);


			SINT32 remove(CAMuxSocket* pMuxSocket);
			SINT32 removeChannel(CAMuxSocket* pMuxSocket,HCHANNEL channelIn);
					
			fmHashTableEntry* getFirst();
			fmHashTableEntry* getNext();
			fmHashTableEntry* get(CAMuxSocket* pMuxSocket);

		
			fmChannelListEntry* getFirstChannelForSocket(CAMuxSocket* pMuxSocket);
			fmChannelListEntry* getNextChannel(fmChannelListEntry* pEntry);
		
			static SINT32 test();
		private:
			/** Gets the in-channel and all associated information for the given out-channel.
				* This method is NOT thread safe (and so only for internal use)
				* @see get() 
				*	@param channelOut the out-channel id for which the information is requested
				* @return the in-channel and all associated information
				* @retval NULL if not found in list
				*/
			fmChannelListEntry* get_intern_without_lock(HCHANNEL channelOut)
				{
					fmChannelListEntry* pEntry=m_HashTableOutChannels[channelOut&0x0000FFFF];
					while(pEntry!=NULL)
						{
							if(pEntry->channelOut==channelOut)
								{
									return pEntry;
								}
							pEntry=pEntry->list_OutChannelHashTable.next;
						}
					return NULL;
				}	

		public:
			/** Gets the in-channel and all associated information for the given out-channel.
				* @param channelOut the out-channel id for which the information is requested
				* @return the in-channel and all associated information
				* @retval NULL if not found in list
				*/
			fmChannelListEntry* get(HCHANNEL channelOut)
				{
					m_Mutex.lock();
					fmChannelListEntry* pEntry=get_intern_without_lock(channelOut);
					m_Mutex.unlock();
					return pEntry;
				}	
		private:
			///The Hash-Table of all connections.
			LP_fmHashTableEntry* m_HashTable;
			///The Hash-Table of all out-channels.
			LP_fmChannelListEntry* m_HashTableOutChannels;
			
			///Pointer to the head of a list of all connections.
			fmHashTableEntry* m_listHashTableHead;
			///Next Element in the enumeration of all connections.
			fmHashTableEntry* m_listHashTableNext;
			///This mutex is used in all functions and makes them thread safe.
			CAMutex m_Mutex;
//#ifdef PAYMENT
//			CAAccountingInstance *m_pAccountingInstance;
//#endif

#ifdef DO_TRACE
			UINT32 m_aktAlloc;
			UINT32 m_maxAlloc;
			LP_fmChannelListEntry newChannelListEntry()
				{
					m_aktAlloc+=sizeof(fmChannelListEntry);
					if(m_maxAlloc<m_aktAlloc)
						{
							m_maxAlloc=m_aktAlloc;
							CAMsg::printMsg(LOG_DEBUG,"FirstMixChannelList current alloc: %u\n",m_aktAlloc);
						}
					return (LP_fmChannelListEntry)new fmChannelListEntry;
				}
			void deleteChannelListEntry(LP_fmChannelListEntry entry)
				{
					m_aktAlloc-=sizeof(fmChannelListEntry);
					delete entry;
				}
#endif
	};
#endif
