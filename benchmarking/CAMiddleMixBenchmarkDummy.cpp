#ifdef DO_MIDDLE_MIX_BENCHMARK
#include "../StdAfx.h"
#include "CAMiddleMixBenchmarkDummy.hpp"

SINT32 CAMiddleMixBenchmarkDummy::init()
{
	m_pMuxIn = new CAMuxSocket(OFB);
	m_pMuxOut = new CAMuxSocket(OFB);
	m_pQueueSendToMixBefore = new CAQueue(sizeof(tQueueEntry));
	m_pQueueSendToMixAfter = new CAQueue(sizeof(tQueueEntry));
	return E_SUCCESS;
}
	SINT32 CAMiddleMixBenchmarkDummy::initOnce()
{
		return E_SUCCESS;
	}
	SINT32 CAMiddleMixBenchmarkDummy::clean()
{
	return E_SUCCESS;
}
#endif //DO_MIDDLE_MIX_BENCHMARK
