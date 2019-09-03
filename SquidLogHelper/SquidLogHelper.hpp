#pragma once

#include "../CAThread.hpp"
class CASquidLogHelper
{
	public:
		CASquidLogHelper(UINT16 port);
		SINT32 start();
		SINT32 stop();
	protected:
		friend THREAD_RETURN	squidloghelper_ProcessingLoop(void* param);
	private:
		SINT32 processLogLine(UINT8* strLine);
		CAThread* m_pThreadProcessingLoop;
};

//int squidloghelp_main();