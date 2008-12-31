#include "StdAfx.h"
#ifdef DATA_RETENTION_LOG
#include "CADataRetentionLog.hpp"

CADataRetentionLog::CADataRetentionLog()
	{
		m_pLogFile=NULL;
		m_pPublicEncryptionKey=NULL;
	}

SINT32 CADataRetentionLog::setPublicEncryptionKey(CAASymCipher* pPublicKey)
	{
		m_pPublicEncryptionKey=pPublicKey;
		return E_SUCCESS;
	}

SINT32 CADataRetentionLog::setLogDir(UINT8* strDir)
	{
		m_pLogFile=new CADataRetentionLogFile();
		return m_pLogFile->openLog(strDir,time(NULL),m_pPublicEncryptionKey);
	}

SINT32 CADataRetentionLog::log(t_dataretentionLogEntry* logEntry)
	{
		return m_pLogFile->log(logEntry);
	}
#endif //DATA_RETENTION_LOG