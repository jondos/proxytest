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
#include "CATimedQueue.hpp"

/*CATimedQueue::CATimedQueue()
{
	m_pQueue=new CAQueue();
}*/

CATimedQueue::CATimedQueue(UINT32 expectedSize)
{
	m_pQueue=new CAQueue(expectedSize+sizeof(UINT64));
}

SINT32 CATimedQueue::add(const void* pbuff,UINT32 size,const UINT64& timestamp)
{
	m_pQueue->add(pbuff,size);
	return m_pQueue->add(&timestamp,sizeof(timestamp));
}

SINT32 CATimedQueue::getOrWait(UINT8* pbuff,UINT32* psize,UINT64& timestamp)
{
	m_pQueue->getOrWait(pbuff,psize);
	UINT32 len=sizeof(timestamp);
	return m_pQueue->getOrWait((UINT8*)&timestamp,&len);
}

SINT32 CATimedQueue::getOrWait(UINT8* pbuff,UINT32* psize,UINT64& timestamp,UINT32 ms_TimeOut)
{
	SINT32 ret=m_pQueue->getOrWait(pbuff,psize,ms_TimeOut);
	if(ret!=E_SUCCESS)
		return ret;
	UINT32 len=sizeof(timestamp);
	return m_pQueue->getOrWait((UINT8*)&timestamp,&len);
}
