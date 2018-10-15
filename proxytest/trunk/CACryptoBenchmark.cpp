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
#include "CACryptoBenchmark.hpp"
#include "CAThread.hpp"
#include "CAThreadList.hpp"
#include "CASymCipherCTR.hpp"
#include "CASymCipherOFB.hpp"
#include "CAUtil.hpp"


const UINT32 CACryptoBenchmark::ms_BlockSizes[ms_NUM_BLOCKSIZES] = { 16,64,256,512,1024 };

CACryptoBenchmark::CACryptoBenchmark()
{
}


CACryptoBenchmark::~CACryptoBenchmark()
{
}

void CACryptoBenchmark::doBenchmark()
{
	CAThreadList* pThreadList=new CAThreadList();
	CAMutex* pMutexTotlaBytes = new CAMutex();
	volatile UINT64 totalBytesProcessed;
	for (UINT32 iBlocksize = 0; iBlocksize < ms_NUM_BLOCKSIZES; iBlocksize++)
	{
		for (UINT32 numThreads = 1; numThreads <= MAX_CRYPTOBENCHMARK_THREADS; numThreads += 4)
		{
			CAMsg::printMsg(LOG_DEBUG, "Do crypto benchmark for %u threads\n", numThreads);
			totalBytesProcessed = 0;
			UINT64 startTime, endTime, time;
			getcurrentTimeMillis(startTime);
			for (UINT32 nThreads = 0; nThreads < numThreads; nThreads++)
			{
				tBenchmarkParams* pParams = new tBenchmarkParams;
				pParams->blocksize = ms_BlockSizes[iBlocksize];
				pParams->pMutexTotalBytesProcessed = pMutexTotlaBytes;
				pParams->outparam_pTotalBytesProcessed = &totalBytesProcessed;
				CAThread* pThread = new CAThread();
				pThread->setMainLoop(benchmarkThread);
				pThread->start(pParams, false, true);
				pThreadList->put(pThread);
			}
			pThreadList->waitAndRemoveAll();
			getcurrentTimeMillis(endTime);
			time = diff64(endTime, startTime);
			UINT8 tmpbuff[256];
			formatBytesPerSecond(tmpbuff, 256, (totalBytesProcessed * 1000) / time);
			CAMsg::printMsg(LOG_DEBUG, "Total crypto speed (blocksize: %u): %llu bytes in %u ms -- %s\n", ms_BlockSizes[iBlocksize],totalBytesProcessed, (UINT32)time,tmpbuff);

			numThreads -= (numThreads % 4); // Produce a series like: 1,4,8,12,...
		}
	}
	delete pThreadList;
	delete pMutexTotlaBytes;
}

THREAD_RETURN CACryptoBenchmark::timerThread(void * a_pParams)
{
	tBenchmarkTimerParams* pTimerParams = (tBenchmarkTimerParams*)a_pParams;
	msSleep(pTimerParams->timerInMillis);
	*(pTimerParams->bTimer) = false;
	THREAD_RETURN_SUCCESS;
}

THREAD_RETURN CACryptoBenchmark::benchmarkThread(void * a_pParams)
{
	tBenchmarkParams* pParams = (tBenchmarkParams*)a_pParams;
	CASymChannelCipher* pCipher = new CASymCipherCTR();
	UINT8 iv[32];
	UINT8 key[32];
	pCipher->setKeys(key, 32);
	pCipher->setIVs(iv);
	volatile bool bTimer = true;
	CAThread* pTimerThread = new CAThread();
	pTimerThread->setMainLoop(timerThread);
	tBenchmarkTimerParams* pTimerParams = new tBenchmarkTimerParams();
	pTimerParams->timerInMillis = 3000;
	pTimerParams->bTimer = &bTimer;
	UINT32 blocksize = pParams->blocksize;
	UINT8* in = new UINT8[blocksize];
	UINT8* out = new UINT8[blocksize];
	UINT64 numCrypts = 0;
	UINT64 startTime, endTime, time;
	pTimerThread->start(pTimerParams,true,true);
	getcurrentTimeMillis(startTime);
	while (bTimer)
	{
		pCipher->crypt1(in, out, blocksize);
		numCrypts++;
	}
	getcurrentTimeMillis(endTime);
	time =diff64( endTime,startTime);
	pParams->pMutexTotalBytesProcessed->lock();
	*(pParams->outparam_pTotalBytesProcessed) += blocksize * numCrypts;
	pParams->pMutexTotalBytesProcessed->unlock();
	UINT64 bytes = numCrypts * 1000 * blocksize;
	UINT8 tmpbuff[256];
	formatBytesPerSecond(tmpbuff, 256, bytes / time);

	CAMsg::printMsg(LOG_DEBUG,"Did %llu crypts in %u ms - blocksize %u - %s\n", numCrypts,(UINT32) time, blocksize, tmpbuff);
	delete pCipher;
	delete pTimerThread;
	delete pTimerParams;
	delete[] in;
	delete[] out;
	THREAD_RETURN_SUCCESS;

}