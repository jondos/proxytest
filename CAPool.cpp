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
#include "CAPool.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"
#ifdef USE_POOL
CAPool::CAPool(UINT32 poolsize)
	{
		m_uPoolSize=poolsize;
		m_uRandMax=0xFFFFFFFF;
		m_uRandMax-=m_uRandMax%m_uPoolSize;
		m_pPoolList=new tPoolListEntry;
		getRandom(m_pPoolList->poolentry.packet.data,DATA_SIZE);
		m_pPoolList->poolentry.packet.flags=CHANNEL_DUMMY;
		m_pPoolList->poolentry.packet.channel=DUMMY_CHANNEL;
#ifdef LOG_PACKET_TIMES
		setZero64(m_pPoolList->poolentry.timestamp_proccessing_start);
		setZero64(m_pPoolList->poolentry.pool_timestamp_in);
#endif				
		m_pPoolList->next=NULL;
		m_pLastEntry=m_pPoolList;
		m_arChannelIDs=new HCHANNEL[poolsize];
		m_arChannelIDs[0]=0;
		for(UINT32 i=1;i<poolsize;i++)
			{
				tPoolListEntry* tmpEntry=new tPoolListEntry;
				getRandom(tmpEntry->poolentry.packet.data,DATA_SIZE);
				tmpEntry->poolentry.packet.flags=CHANNEL_DUMMY;
				tmpEntry->poolentry.packet.channel=DUMMY_CHANNEL;
				#ifdef LOG_PACKET_TIMES
					setZero64(tmpEntry->poolentry.timestamp_proccessing_start);
					setZero64(tmpEntry->poolentry.pool_timestamp_in);
				#endif				
				tmpEntry->next=m_pPoolList;
				m_pPoolList=tmpEntry;
				m_arChannelIDs[i]=0;
			}
		m_pEntry=new tPoolListEntry;	
	}
	
CAPool::~CAPool()
	{
		tPoolListEntry* tmpEntry;
		while(m_pPoolList!=NULL)
			{
				tmpEntry=m_pPoolList;
				m_pPoolList=m_pPoolList->next;
				delete tmpEntry;
			}
		delete m_pEntry;
		delete[] m_arChannelIDs;
	}
	
SINT32 CAPool::pool(tPoolEntry* pPoolEntry)
	{
		UINT32 v;
		for(;;)
			{
				getRandom(&v);
				if(v<m_uRandMax)
					{
						v%=m_uPoolSize;
						break;
					}
			}
		HCHANNEL id=m_arChannelIDs[v];
		m_arChannelIDs[v]=pPoolEntry->packet.channel;
		tPoolListEntry* pPrevEntry=NULL;		
		tPoolListEntry* pEntryOut=m_pPoolList;
		while(pEntryOut->poolentry.packet.channel!=id)
			{
				pPrevEntry=pEntryOut;
				pEntryOut=pEntryOut->next;	
			}

		if(pEntryOut==m_pPoolList) //first element to remove)
			{
				memcpy(&m_pEntry->poolentry,pPoolEntry,sizeof(tPoolEntry));
				memcpy(pPoolEntry,&pEntryOut->poolentry,sizeof(tPoolEntry));
				m_pPoolList=m_pPoolList->next;
				m_pLastEntry->next=m_pEntry;
				m_pLastEntry=m_pEntry;
				m_pLastEntry->next=NULL;
				m_pEntry=pEntryOut;
			}
		else
			{
				memcpy(&m_pEntry->poolentry,pPoolEntry,sizeof(tPoolEntry));
				memcpy(pPoolEntry,&pEntryOut->poolentry,sizeof(tPoolEntry));
				m_pLastEntry->next=m_pEntry;
				m_pLastEntry=m_pEntry;
				m_pLastEntry->next=NULL;
				pPrevEntry->next=pEntryOut->next;
				m_pEntry=pEntryOut;		
			}		
		return E_SUCCESS;
	}
#endif
