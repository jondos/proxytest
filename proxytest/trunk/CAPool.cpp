#include "StdAfx.h"
#include "CAPool.hpp"
#include "CAUtil.hpp"

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
				tmpEntry=m_pPoolList;
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
		CAMsg::out(LOG_DEBUG,"Try to pool out %u . Element\n",v);
		tPoolListEntry* tmpEntry=m_pPoolList;
		while(v>0)
			{
				tmpEntry=tmpEntry->next;
				v--;
			}
		HCHANNEL id=tmpEntry->mixpacket.channel;
		CAMsg::out(LOG_DEBUG,"Channel id is:  %u\n",id);
	
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
		CAMsg::out(LOG_DEBUG,"pool() finished!\n",v);			
		return E_SUCCESS;
	}
		