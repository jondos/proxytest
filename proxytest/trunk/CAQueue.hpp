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
#ifndef __CAQUEUE__
#define __CAQUEUE__
#include "CAConditionVariable.hpp"
#include "CAMsg.hpp"

struct _t_queue
	{
		UINT8* pBuff;
		_t_queue* next;
		UINT32 size;
#ifdef DO_TRACE
		UINT32 allocSize;
		_t_queue* shadow_this;
#endif
	};

typedef struct _t_queue QUEUE;

/** This is a simple FIFO-Queue. You can add data and get them back.
	* This class is thread safe.
	* TODO: The dandling of getAndWit is not correct because remove could intercept....
	* Maybe we do not neeed an other Mutex other than then ConVar....
	*/
class CAQueue
	{
		public:
			CAQueue()
				{
					m_Queue=NULL;
					m_nQueueSize=0;
#ifdef DO_TRACE
					CAMsg::printMsg(LOG_DEBUG,"CAQueue creating QUEUE [%p]\n",this);
#endif
				}
			~CAQueue();
			SINT32 add(const UINT8* buff,UINT32 size);
			SINT32 get(UINT8* pbuff,UINT32* psize);
			SINT32 getOrWait(UINT8* pbuff,UINT32* psize);
			SINT32 peek(UINT8* pbuff,UINT32* psize);
			SINT32 remove(UINT32* psize);
			
			/** Returns the size of stored data in byte.
				* @return size of Queue
				*/
			UINT32 getSize()
				{
					return m_nQueueSize;
				}
			
			/** Returns true, if the Queue is empty
				* @retval true, if Queue is empty
				* @retval false, if Queue contains data
				*/
			bool isEmpty()
				{
					return (m_Queue==NULL);
				}

			/** Method to test the Queue
				* @retval E_SUCCESS, if Queue implementation seams to be ok
				*/
			static SINT32 test();
		
		private:
#ifdef _DEBUG 
			QUEUE* volatile m_Queue; 
			QUEUE* volatile m_lastElem;
			volatile UINT32 m_nQueueSize;
#else
			QUEUE* m_Queue;
			QUEUE* m_lastElem;
			UINT32 m_nQueueSize;
#endif
			CAMutex m_csQueue;
			CAConditionVariable m_convarSize;
#ifdef DO_TRACE
			static UINT32 m_aktAlloc;
			static UINT32 m_maxAlloc;
			
			QUEUE* newQUEUE()
				{
					m_aktAlloc+=sizeof(QUEUE);
					if(m_maxAlloc<m_aktAlloc)
						{
							m_maxAlloc=m_aktAlloc;
							CAMsg::printMsg(LOG_DEBUG,"CAQueue current alloc: %u Current Size (of this[%p] queue) %u\n",m_aktAlloc,this,m_nQueueSize);
						}
					QUEUE* pQueue=new QUEUE;
					pQueue->shadow_this=pQueue;
					return pQueue;
				}
			
			void deleteQUEUE(QUEUE* entry)
				{
					m_aktAlloc-=sizeof(QUEUE);
					if(entry!=entry->shadow_this)
							CAMsg::printMsg(LOG_CRIT,"CAQueue deleting QUEUE: this!=shadow_this!!\n");
					delete entry;
				}

			UINT8* newUINT8Buff(UINT32 size)
				{
					m_aktAlloc+=size;
					if(m_maxAlloc<m_aktAlloc)
						{
							m_maxAlloc=m_aktAlloc;
							CAMsg::printMsg(LOG_DEBUG,"CAQueue current alloc: %u Current Size (of this[%p] queue) %u\n",m_aktAlloc,this,m_nQueueSize);
						}
					return (UINT8*)new UINT8[size];
				}

			void deleteUINT8Buff(UINT8* entry,UINT32 size)
				{
					m_aktAlloc-=size;
					delete[] entry;
				}

#endif
	};
#endif
