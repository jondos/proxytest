#include "StdAfx.h"
#include "CAQueue.hpp"
#include "CAMsg.hpp"
UINT32 CAQueue::m_nMaxQueueSize=0;

CAQueue::~CAQueue()
	{
		while(m_Queue!=NULL)
			{
				delete m_Queue->pBuff;
				m_lastElem=m_Queue;
				m_Queue=m_Queue->next;
				delete m_lastElem;
			}
		DeleteCriticalSection(&csQueue);
	}

SINT32 CAQueue::add(UINT8* buff,UINT32 size)
	{
		EnterCriticalSection(&csQueue);
		if(m_Queue==NULL)
			{
				m_Queue=new QUEUE;
				m_Queue->next=NULL;
				m_Queue->pBuff=new UINT8[size];
				m_Queue->size=size;
				memcpy(m_Queue->pBuff,buff,size);
				m_lastElem=m_Queue;
			}
		else
			{
				m_lastElem->next=new QUEUE;
				m_lastElem=m_lastElem->next;
				m_lastElem->next=NULL;
				m_lastElem->pBuff=new UINT8[size];
				m_lastElem->size=size;
				memcpy(m_lastElem->pBuff,buff,size);
			}
		m_nQueueSize++;
		if(m_nQueueSize>m_nMaxQueueSize)
			{
				m_nMaxQueueSize=m_nQueueSize;
				CAMsg::printMsg(LOG_DEBUG,"Max Queue Size now: %u\n",m_nMaxQueueSize);
			}
		LeaveCriticalSection(&csQueue);
		return E_SUCCESS;
	}
			
SINT32 CAQueue::getNext(UINT8* pbuff,UINT32* psize)
	{
		EnterCriticalSection(&csQueue);
		SINT32 ret;
		if(m_Queue==NULL||pbuff==NULL||*psize<m_Queue->size)
			ret=E_UNKNOWN;
		else
			{
				memcpy(pbuff,m_Queue->pBuff,m_Queue->size);
				*psize=m_Queue->size;
				delete m_Queue->pBuff;
				QUEUE* tmp=m_Queue;
				m_Queue=m_Queue->next;
				delete tmp;
				m_nQueueSize--;
				ret=E_SUCCESS;
			}
		LeaveCriticalSection(&csQueue);
		return ret;
	}
			

