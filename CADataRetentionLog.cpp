#include "StdAfx.h"
#ifdef DATA_RETENTION_LOG
#include "CADataRetentionLog.hpp"

CADataRetentionLog::CADataRetentionLog()
	{
		m_pLogFile=NULL;
	}

SINT32 CADataRetentionLog::setLogDir(UINT8* strDir)
	{
		m_pLogFile=new CADataRetentionLogFile();
		return m_pLogFile->openLog(strDir,time(NULL));
	}

SINT32 CADataRetentionLog::log(t_dataretentionLogEntry* logEntry)
	{
		return m_pLogFile->log(logEntry);
	}
#endif //DATA_RETENTION_LOG