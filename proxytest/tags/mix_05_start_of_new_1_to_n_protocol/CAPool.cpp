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
		m_pPoolList=new tPoolListEntry;
		getRandom(m_pPoolList->mixpacket.data,DATA_SIZE);
		m_pPoolList->mixpacket.flags=CHANNEL_CLOSE;
		m_pPoolList->mixpacket.channel=0;
		m_pPoolList->next=NULL;
		m_pLastEntry=m_pPoolList;
		for(UINT32 i=1;i<poolsize;i++)
			{
				tPoolListEntry* tmpEntry=new tPoolListEntry;
				getRandom(tmpEntry->mixpacket.data,DATA_SIZE);
				tmpEntry->mixpacket.flags=CHANNEL_CLOSE;
				tmpEntry->mixpacket.channel=0;
				tmpEntry->next=m_pPoolList;
				m_pPoolList=tmpEntry;
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
	}
	
SINT32 CAPool::pool(MIXPACKET* pMixPacket)
	{
		UINT32 v;
		getRandom(&v);
		v=v%m_uPoolSize;
		tPoolListEntry* tmpEntry=m_pPoolList;
		while(v>0)
			{
				tmpEntry=tmpEntry->next;
				v--;
			}
		HCHANNEL id=tmpEntry->mixpacket.channel;
	
		tPoolListEntry* pEntryOut=tmpEntry;
		tmpEntry=m_pPoolList;
		tPoolListEntry* pPrevEntry=NULL;		
		while(tmpEntry!=pEntryOut)
			{
				if(tmpEntry->mixpacket.channel==id)
					{
						pEntryOut=tmpEntry;
						break;
					}
				pPrevEntry=tmpEntry;
				tmpEntry=tmpEntry->next;	
			}

		if(pEntryOut==m_pPoolList) //first element to remove)
			{
				memcpy(&m_pEntry->mixpacket,pMixPacket,MIXPACKET_SIZE);
				memcpy(pMixPacket,&pEntryOut->mixpacket,MIXPACKET_SIZE);
				m_pPoolList=m_pPoolList->next;
				m_pLastEntry->next=m_pEntry;
				m_pLastEntry=m_pEntry;
				m_pLastEntry->next=NULL;
				m_pEntry=pEntryOut;
			}
		else
			{
				memcpy(&m_pEntry->mixpacket,pMixPacket,MIXPACKET_SIZE);
				memcpy(pMixPacket,&pEntryOut->mixpacket,MIXPACKET_SIZE);
				m_pLastEntry->next=m_pEntry;
				m_pLastEntry=m_pEntry;
				m_pLastEntry->next=NULL;
				pPrevEntry->next=pEntryOut->next;
				m_pEntry=pEntryOut;		
			}		
		return E_SUCCESS;
	}
#endif
