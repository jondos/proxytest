#pragma once
#include "../CAFirstMixChannelList.hpp"
THREAD_RETURN loopAcceptTest(void*);

class CAFirstMixChannelListTest
{
public:
	CAFirstMixChannelListTest(void);
	~CAFirstMixChannelListTest(void);
	CAFirstMixChannelList* m_pList;
	void doIterateAndRemove();
	void doTest();
	void doThreadedTest();
	volatile bool m_bRunAcceptLoop;
};
