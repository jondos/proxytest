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
#ifndef __CATIMEDQUEUE__
#define __CATIMEDQUEUE__
#include "CAQueue.hpp"

class CATimedQueue
{
	public:
		//CATimedQueue();
		CATimedQueue(UINT32 expectedSize);
		SINT32 add(const void* pbuff,UINT32 size,const UINT64& timestamp)
			{
				m_pQueue->add(pbuff,size);
				return m_pQueue->add(&timestamp,sizeof(timestamp));
			}

		SINT32 getOrWait(UINT8* pbuff,UINT32* psize,UINT64& timestamp)
			{
				m_pQueue->getOrWait(pbuff,psize);
				UINT32 len=sizeof(timestamp);
				return m_pQueue->getOrWait((UINT8*)&timestamp,&len);
			}

		SINT32 getOrWait(UINT8* pbuff,UINT32* psize,UINT64& timestamp,UINT32 ms_TimeOut)
			{
				SINT32 ret=m_pQueue->getOrWait(pbuff,psize,ms_TimeOut);
				if(ret!=E_SUCCESS)
					return ret;
				UINT32 len=sizeof(timestamp);
				return m_pQueue->getOrWait((UINT8*)&timestamp,&len);
			}

		SINT32 get(UINT8* pbuff,UINT32* psize,UINT64& timestamp)
			{
				UINT32 len=sizeof(UINT64);
				m_pQueue->get(pbuff,psize);
				return m_pQueue->get((UINT8*)&timestamp,&len);
			}

		SINT32 get(UINT8* pbuff,UINT32* psize)
			{
				return m_pQueue->get(pbuff,psize);
			}
		UINT32 getSize()
			{
				return m_pQueue->getSize();
			}
		bool isEmpty()
			{
				return m_pQueue->isEmpty();
			}
		SINT32 peek(UINT8* pbuff,UINT32* psize)
			{
				return m_pQueue->peek(pbuff,psize);
			}
		SINT32 remove(UINT32* psize)
			{
				return m_pQueue->remove(psize);
			}
	
	private:
		CAQueue* m_pQueue;
};
#endif
