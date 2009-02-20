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
typedef struct _t_queue
	{
		UINT8* pBuff;
		UINT32 size;
		_t_queue* next;
	}QUEUE;

/** this is a simple FIFO-Queue. You can add data and get them back.*/
class CAQueue
	{
		public:
			CAQueue(){InitializeCriticalSection(&m_csQueue);m_Queue=NULL;m_nQueueSize=0;}
			~CAQueue();
			SINT32 add(const UINT8* buff,UINT32 size);
			SINT32 get(UINT8* pbuff,UINT32* psize);
			/** Returns the size of stored data.
				* @retrun size of Queue
				*/
			UINT32 getSize(){return m_nQueueSize;}
			
			/** Returns true, if the Queue is empty
				* @retval true, if Queue is empty
				* @retval false, if Queue contains data
				*/
			bool isEmpty(){return m_Queue==NULL;}

			/** Method to test the Queue
				* @retval E_SUCCESS, if Queue implementation seams to be ok
				*/
			static SINT32 test();
		private:
			QUEUE* m_Queue;
			QUEUE* m_lastElem;
			#ifdef _REENTRANT
				CRITICAL_SECTION m_csQueue;
			#endif
			UINT32 m_nQueueSize;
	};
#endif