#pragma once

#include "../CAThread.hpp"

extern class CALastMix;

class CASquidLogHelper
{
	public:
		CASquidLogHelper(CALastMix* pLastMix,UINT16 port);
		SINT32 start();
		SINT32 stop();
	protected:
		friend THREAD_RETURN	squidloghelper_ProcessingLoop(void* param);
	private:
		SINT32 processLogLine(UINT8* strLine);
		CAThread* m_pThreadProcessingLoop;
		CALastMix* m_pLastMix;
};

//int squidloghelp_main();