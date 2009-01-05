#include "StdAfx.h"
#ifdef DATA_RETENTION_LOG
#include "CADataRetentionLog.hpp"

CADataRetentionLog::CADataRetentionLog()
	{
		m_pLogFile=NULL;
		m_pPublicEncryptionKey=NULL;
		m_strLogDir=NULL;
	}

CADataRetentionLog::~CADataRetentionLog()
	{
		closeLog();
		delete[] m_strLogDir;
	}

SINT32 CADataRetentionLog::setPublicEncryptionKey(CAASymCipher* pPublicKey)
	{
		m_pPublicEncryptionKey=pPublicKey;
		return E_SUCCESS;
	}

SINT32 CADataRetentionLog::setLogDir(UINT8* strDir)
	{
		UINT32 l=strlen((char*)strDir);
		m_strLogDir=new UINT8[l+1];
		memcpy(m_strLogDir,strDir,l);
		m_strLogDir[l]=0;
		m_max_t_out=0;
		return E_SUCCESS;
	}

SINT32 CADataRetentionLog::openLogFile(UINT32 time)
	{
		m_pLogFile=new CADataRetentionLogFile();
		if(m_pLogFile->openLog(m_strLogDir,time,m_pPublicEncryptionKey)!=E_SUCCESS)
			return E_UNKNOWN;
		m_max_t_out=m_pLogFile->getMaxLogTime();
		return E_SUCCESS;
	}

SINT32 CADataRetentionLog::closeLog()
	{
		if(m_pLogFile!=NULL)
			{
				m_pLogFile->closeLog();
				delete m_pLogFile;
				m_pLogFile=NULL;
			}
		return E_SUCCESS;
	}

SINT32 CADataRetentionLog::log(t_dataretentionLogEntry* logEntry)
	{
		if(ntohl(logEntry->t_out)>m_max_t_out)
			{
				if(m_pLogFile!=NULL)
					{
						m_pLogFile->closeLog();
						delete m_pLogFile;
						m_pLogFile=NULL;
					}
				if(openLogFile(ntohl(logEntry->t_out))!=E_SUCCESS)
					return E_UNKNOWN;
			}
		return m_pLogFile->log(logEntry);
	}
#endif //DATA_RETENTION_LOG
