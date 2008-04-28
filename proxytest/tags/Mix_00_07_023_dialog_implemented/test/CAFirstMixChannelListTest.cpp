#include "../StdAfx.h"
#ifdef CAFirstMixChannelListTest
#include "CAFirstMixchannellisttest.hpp"
#include "../CAUtil.hpp"
#include "../CAThreadPool.hpp"
CAFirstMixChannelListTest::CAFirstMixChannelListTest(void)
{
}

CAFirstMixChannelListTest::~CAFirstMixChannelListTest(void)
{
}

void CAFirstMixChannelListTest::doTest()
	{
		m_pList=new CAFirstMixChannelList();
		for(int i=0;i<100;i++)
		{
		CAMuxSocket* pM=new CAMuxSocket;
		if(((CASocket*)pM)->create(AF_INET)==E_SUCCESS)
			{
				UINT8 peer[4];
				m_pList->add(pM,peer,new CAQueue());
			}
		else
			delete pM;
		}
		fmHashTableEntry* pEntry;
		for(int i=0;i<10;i++)
		{
		pEntry=m_pList->getFirst();
		while(pEntry!=NULL)
		{
			UINT32 r;
			getRandom(&r);
			r%=1000;
			if(r<10)
			{
				CAMuxSocket* pS=pEntry->pMuxSocket;
				delete pEntry->pQueueSend;
				m_pList->remove(pS);
				delete pS;
			}
			getRandom(&r);
			r%=1000;
			if(r<10)
			{
				CAMuxSocket* pM=new CAMuxSocket;
				if(((CASocket*)pM)->create(AF_INET)==E_SUCCESS)
					{
						UINT8 peer[4];
						m_pList->add(pM,peer,new CAQueue());
					}
				else
					delete pM;
			}
			pEntry=m_pList->getNext();
		}
		}
		pEntry=m_pList->getFirst();
		while(pEntry!=NULL)
		{
			delete pEntry->pQueueSend;
			delete pEntry->pMuxSocket;
			pEntry=m_pList->getNext();
		}
		
		delete m_pList;
	}

void CAFirstMixChannelListTest::doThreadedTest()
{
	initRandom();
	m_pList=new CAFirstMixChannelList();
	//CAThreadPool* pPool=new CAThreadPool(50,500,false);
	CAThread oAcceptThread;
	m_bRunAcceptLoop=true;
	oAcceptThread.setMainLoop(loopAcceptTest);
	oAcceptThread.start(this);
	doIterateAndRemove();
	m_bRunAcceptLoop=false;
	oAcceptThread.join();
	//pPool->destroy(true);
	//delete pPool;
	fmHashTableEntry*	pEntry=m_pList->getFirst();
		while(pEntry!=NULL)
		{
			delete pEntry->pQueueSend;
			delete pEntry->pMuxSocket;
			pEntry=m_pList->getNext();
		}
	delete m_pList;
}
void CAFirstMixChannelListTest::doIterateAndRemove()
{
	for(int i=0;i<10000;i++)
	{
		fmHashTableEntry* pEntry=m_pList->getFirst();
		while(pEntry!=NULL)
			{
				UINT32 r;
				getRandom(&r);
				r%=1000;
				if(r<100)
					{
						CAMuxSocket *pM=pEntry->pMuxSocket;
						delete pEntry->pQueueSend;
						m_pList->remove(pM);
						delete pM;
					}
				pEntry=m_pList->getNext();
		getRandom(&r);
		r%=25;
		msSleep(r);
			}
	}
}

THREAD_RETURN loopAcceptTest(void *param)
{
	CAFirstMixChannelListTest* pTest=(CAFirstMixChannelListTest*)param;
	while(pTest->m_bRunAcceptLoop)
	{
		CAMuxSocket* pM=new CAMuxSocket;
		if(((CASocket*)pM)->create(AF_INET)==E_SUCCESS)
			{
				UINT8 peer[4];
				pTest->m_pList->add(pM,peer,new CAQueue());
			}
		else
			delete pM;
		UINT32 r;
		getRandom(&r);
		r%=100;
		msSleep(r);
	}
	THREAD_RETURN_SUCCESS;
}
#endif
