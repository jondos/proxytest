#include "StdAfx.h"
#include "CAPool.hpp"
#include "CAUtil.hpp"

CAPool::CAPool(UINT32 poolsize)
	{
		m_pPoolList=new tPoolListEntry;
		getRandom(m_pPoolList->mixpacket.data,DATA_SIZE);
		m_pPoolList->mixpacket.flags=CHANNEL_CLOSE;
		m_pPoolList->mixpacket.channel=0;
		m_pPoolList->next=NULL;
		for(UINT32 i=1;i<poolsize;i++)
			{
				tPoolListEntry* tmpEntry=new tPoolListEntry;
				getRandom(tmpEntry->mixpacket.data,DATA_SIZE);
				tmpEntry->mixpacket.flags=CHANNEL_CLOSE;
				tmpEntry->mixpacket.channel=0;
				tmpEntry=m_pPoolList;
				m_pPoolList=tmpEntry;
			}
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
	}
	
SINT32 CAPool::pool(MIXPACKET* pMixPacket)
	{
		return E_SUCCESS;
	}
		