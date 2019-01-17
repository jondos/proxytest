#include "../StdAfx.h"
#include "../readerwriterqueue.h"
#include "../CAThread.hpp"


THREAD_RETURN lockfreequeuebenchmark_Producer(void* param)
{
	BlockingReaderWriterQueue* pQueue = (BlockingReaderWriterQueue*)param;
	UINT8* buff = new UINT8[1000];
	for (UINT32 i = 0; i < 100000000; i++)
	{
		pQueue->enqueue(buff);
	}
	delete[] buff;
	THREAD_RETURN_SUCCESS;
}

THREAD_RETURN lockfreequeuebenchmark_Consumer(void* param)
{
	BlockingReaderWriterQueue* pQueue = (BlockingReaderWriterQueue*)param;
	UINT8* buff = new UINT8[1000];
	for (UINT32 i = 0; i < 100000000; i++)
	{
		int number;
		pQueue->wait_dequeue(buff);
	}
	delete[] buff;
	THREAD_RETURN_SUCCESS;
}

TEST(TCALockfreeQueue, benchmark)
{
	BlockingReaderWriterQueue* pQueue = new BlockingReaderWriterQueue(1000);
	CAThread* pthreadProducer = new CAThread();
	CAThread* pthreadConsumer = new CAThread();
	pthreadProducer->setMainLoop(lockfreequeuebenchmark_Producer);
	pthreadConsumer->setMainLoop(lockfreequeuebenchmark_Consumer);
	pthreadConsumer->start(pQueue, false, true);
	UINT64 start, end;
	getcurrentTimeMillis(start);
	pthreadProducer->start(pQueue, false, true);
	pthreadConsumer->join();
	getcurrentTimeMillis(end);
	pthreadProducer->join();
	UINT32 diff = diff64(end, start);
	printf("CALockfreeQueueBenchmark takes %u ms\n", diff);
	delete pQueue;
}

