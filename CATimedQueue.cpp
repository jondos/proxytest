#include "StdAfx.h"
#include "CATimedQueue.hpp"

CATimedQueue::CATimedQueue()
{
	m_pQueue=new CAQueue();
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