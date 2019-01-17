

#include "../StdAfx.h"
#include "../CAQueue.hpp"
#include "../CAThread.hpp"

THREAD_RETURN queuebenchmark_Producer(void* param)
{
	CAQueue* pQueue = (CAQueue*)param;
	UINT8* buff = new UINT8[1000];
	for (UINT32 i = 0; i < 100000000; i++)
	{
		pQueue->add(buff, 4);
	}
	pQueue->_close();
	delete[] buff;
	THREAD_RETURN_SUCCESS;
}

THREAD_RETURN queuebenchmark_Consumer(void* param)
{
	CAQueue* pQueue = (CAQueue*)param;
	UINT8* buff = new UINT8[1000];
	while (!pQueue->isClosed())
	{
		UINT32 len = 4;
		pQueue->get(buff, &len);
	}
	delete[] buff;
	THREAD_RETURN_SUCCESS;
}

TEST(TCAQueue, benchmark)
{
	CAQueue* pQueue = new CAQueue(1000);
	CAThread* pthreadProducer = new CAThread();
	CAThread* pthreadConsumer = new CAThread();
	pthreadProducer->setMainLoop(queuebenchmark_Producer);
	pthreadConsumer->setMainLoop(queuebenchmark_Consumer);
	pthreadConsumer->start(pQueue, false, true);
	UINT64 start, end;
	getcurrentTimeMillis(start);
	pthreadProducer->start(pQueue, false, true);
	pthreadConsumer->join();
	getcurrentTimeMillis(end);
	pthreadProducer->join();
	UINT32 diff = diff64(end, start);
	printf("CAQueueBenchmark takes %u ms\n", diff);
	delete pQueue;
}

