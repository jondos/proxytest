#ifndef __CATIMEDQUEUE__
#define __CATIMEDQUEUE__
#include "CAQueue.hpp"

class CATimedQueue
{
	public:
		CATimedQueue();
		SINT32 add(const void* pbuff,UINT32 size,const UINT64& timestamp);
		SINT32 getOrWait(UINT8* pbuff,UINT32* psize,UINT64& timestamp);
		SINT32 getOrWait(UINT8* pbuff,UINT32* psize,UINT64& timestamp,UINT32 ms_TimeOut);
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