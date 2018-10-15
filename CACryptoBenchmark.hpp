#pragma once

#define MAX_CRYPTOBENCHMARK_THREADS 16


struct t_benchmarkParams
{
	UINT32 blocksize;
	UINT32 runtimeInMillis;
	volatile UINT64* outparam_pTotalBytesProcessed;
	CAMutex* pMutexTotalBytesProcessed;
};

typedef struct t_benchmarkParams tBenchmarkParams;

struct t_benchmarkTimerParams
{
	UINT32 timerInMillis;
	volatile bool* bTimer;
};

typedef struct t_benchmarkTimerParams tBenchmarkTimerParams;


class CACryptoBenchmark
{
public:
	CACryptoBenchmark();
	~CACryptoBenchmark();
	void doBenchmark();
private:

	static const UINT32 ms_NUM_BLOCKSIZES = 5;
	static const UINT32 ms_BlockSizes[ms_NUM_BLOCKSIZES];
	static THREAD_RETURN benchmarkThread(void * param);
	static THREAD_RETURN timerThread(void * param);

};

