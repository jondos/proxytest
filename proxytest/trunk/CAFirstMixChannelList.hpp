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
struct t_fmhashtableentry
	{
		public:
			CAMuxSocket* pMuxSocket;
			CAQueue* pQueueSend;
			UINT32 cSuspend;

		private:
			UINT32 cNumberOfChannels;
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
		
			SINT32 add(CAMuxSocket* pMuxSocket,CAQueue* pQueueSend);
			SINT32 addChannel(CAMuxSocket* pMuxSocket,HCHANNEL channelIn,CASymCipher* pCipher,HCHANNEL* channelOut);
			
			fmChannelListEntry* get(CAMuxSocket* pMuxSocket,HCHANNEL channelIn);


			SINT32 remove(CAMuxSocket* pMuxSocket);
			SINT32 removeChannel(CAMuxSocket* pMuxSocket,HCHANNEL channelIn);
					
			fmHashTableEntry* getFirst();
			fmHashTableEntry* getNext();
		
			fmChannelListEntry* getFirstChannelForSocket(CAMuxSocket* pMuxSocket);
			fmChannelListEntry* getNextChannel(fmChannelListEntry* pEntry);
		
		private:
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
			fmChannelListEntry* get(HCHANNEL channelOut)
				{
					m_Mutex.lock();
					fmChannelListEntry* pEntry=get_intern_without_lock(channelOut);
					m_Mutex.unlock();
					return pEntry;
				}	
		private:
			LP_fmHashTableEntry* m_HashTable;
			LP_fmChannelListEntry* m_HashTableOutChannels;
			fmHashTableEntry* m_listHashTableHead;
			fmHashTableEntry* m_listHashTableCurrent;
			CAMutex m_Mutex;

	};
#endif
