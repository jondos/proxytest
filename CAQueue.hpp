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
#ifndef ONLY_LOCAL_PROXY
#include "CAConditionVariable.hpp"
#include "CAMsg.hpp"

struct _t_queue
	{
		UINT8* pBuff;
		struct _t_queue* next;
		UINT32 size;
		UINT32 index;
	};

typedef struct _t_queue QUEUE;

/** This is a simple FIFO-Queue. You can add data and get them back.
	* This class is thread safe.
	* TODO: The handling of getAndWait is not correct because remove could intercept....
	* Maybe we do not neeed an other Mutex other than then ConVar....
	*/
class CAQueue
	{
		public:
			/** Give the size of the amount of data what you will add in one step. 
				* Used for optimizations.
				* Use expectedElementSize=0, if you have no idea about 
				* the typicall amount of data added in one call to add().
				*/
			CAQueue(UINT32 expectedElementSize=0)
				{
					m_Queue=NULL;
					m_lastElem=NULL;
					m_nExpectedElementSize=expectedElementSize;
					m_nQueueSize=0;
					m_pcsQueue=new CAMutex();
					m_pconvarSize=new CAConditionVariable();
#ifdef QUEUE_SIZE_LOG
					m_nLogSize=0;
#endif
					//m_pHeap=NULL;
					//incHeap();
				}
			~CAQueue();
			SINT32 add(const void* buff,UINT32 size);
			SINT32 get(UINT8* pbuff,UINT32* psize);
			SINT32 getOrWait(UINT8* pbuff,UINT32* psize);
			SINT32 getOrWait(UINT8* pbuff,UINT32* psize,UINT32 msTimeOut);
			SINT32 peek(UINT8* pbuff,UINT32* psize);
			SINT32 remove(UINT32* psize);
			SINT32 clean();
			
			/** Returns the size of stored data in byte.
				* @return size of Queue
				*/
			UINT32 getSize()
				{
					m_pcsQueue->lock();
					UINT32 s=m_nQueueSize;
					m_pcsQueue->unlock();
					return s;
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

#ifdef QUEUE_SIZE_LOG
			void logIfSizeGreaterThen(UINT32 size)
				{
					m_nLogSize=size;
				}
#endif
		
		private:
			QUEUE* m_Queue; 
			QUEUE* m_lastElem;
			volatile UINT32 m_nQueueSize;
			UINT32 m_nExpectedElementSize;
			//QUEUE* m_pHeap;
			CAMutex* m_pcsQueue;
			CAConditionVariable* m_pconvarSize;

#ifdef QUEUE_SIZE_LOG
			UINT32 m_nLogSize;
#endif
	/*		SINT32 incHeap()
				{
					QUEUE* pEntry;
					for(int i=0;i<100;i++)
						{
							pEntry=new QUEUE;
							pEntry->next=m_pHeap;
							if(m_nExpectedElementSize>0)
								pEntry->pBuff=new UINT8[m_nExpectedElementSize];
							else
								pEntry->pBuff=NULL;	
							m_pHeap=pEntry;
						}
					return E_SUCCESS;	
				}*/
	};
#endif
#endif //ONLY_LOCAL_PROXY
