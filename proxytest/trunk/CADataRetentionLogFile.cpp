#include "StdAfx.h"
#ifdef DATA_RETENTION_LOG
#include "CADataRetentionLogFile.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
extern CACmdLnOptions* pglobalOptions;

CADataRetentionLogFile::CADataRetentionLogFile()
	{
		m_nCurrentLogEntriesInBlock=0;
		m_hLogFile=-1;
		m_nBytesPerLogEntry=sizeof(t_dataretentionLogEntry);
		m_nLogEntriesPerBlock=32;
		m_arOneBlock=new UINT8[m_nLogEntriesPerBlock*m_nBytesPerLogEntry];
		m_encBlock=new UINT8[m_nLogEntriesPerBlock*m_nBytesPerLogEntry+1024];
		m_pGCMCtx=new gcm_ctx_64k;
	}	

SINT32 CADataRetentionLogFile::openLog(UINT8* strLogDir,UINT32 date,CAASymCipher* pPublicKey)
	{
		m_nCurrentLogEntriesInBlock=0;
		m_nCurrentBlockNumber=0;
		UINT8* strFileName=new UINT8[4096];
		UINT8 strDate[256];
		struct tm* theTime;
		time_t t=date;
		theTime=gmtime(&t);
		strftime((char*) strDate,255,"%Y%m%d-%H%M",theTime);
		m_Day=theTime->tm_mday;
		m_Month=theTime->tm_mon+1;
		m_Year=theTime->tm_year+1900;
		snprintf((char*)strFileName,4096,"%s/dataretentionlog_%s",strLogDir,strDate);
		m_hLogFile=open((char*)strFileName,O_APPEND|O_CREAT|O_WRONLY|O_SYNC|O_LARGEFILE|O_SYNC,S_IREAD|S_IWRITE);
		delete [] strFileName; 
		if(m_hLogFile<=0)
			return E_UNKNOWN;
		return writeHeader(pPublicKey);
	}

SINT32 CADataRetentionLogFile::writeHeader(CAASymCipher* pPublicKey)
	{
		t_dataretentionLogFileHeader oHeader;
		memset(&oHeader,0,sizeof(oHeader));
		oHeader.day=m_Day;
		oHeader.month=m_Month;
		oHeader.year=m_Year;
		oHeader.entriesPerBlock=m_nLogEntriesPerBlock;
		oHeader.keys=1;
		oHeader.loggedFields=0;//DATARETETION_LOGGED_FIELD_T_IN|DATARETETION_LOGGED_FIELD_T_OUT
		if(pglobalOptions->isFirstMix())
			{
				oHeader.entity=DATARETENTION_ENTITY_FIRST_MIX;
			}
		else if(pglobalOptions->isMiddleMix())
			{
				oHeader.entity=DATARETENTION_ENTITY_MIDDLE_MIX;
			}
		else if(pglobalOptions->isLastMix())
			{
				oHeader.entity=DATARETENTION_ENTITY_LAST_MIX;
			}
		if(write(m_hLogFile,&oHeader,sizeof(oHeader))!=sizeof(oHeader))
			return E_UNKNOWN;

//Generate sym key and write it to header
		UINT8 key[16];
		getRandom(key,16);
		gcm_init_64k(m_pGCMCtx,key,128);
		UINT8 encKey[2048];
		UINT32 encKeyLen=2048;
		pPublicKey->encryptOAEP(key,16,encKey,&encKeyLen);
		memset(key,0,16);	
		
		if(write(m_hLogFile,encKey,encKeyLen)!=encKeyLen)
			return E_UNKNOWN;
		
		return E_SUCCESS;
	}

SINT32 CADataRetentionLogFile::writeFooter()
	{
		return E_SUCCESS;
	}

SINT32 CADataRetentionLogFile::closeLog()
	{
		SINT32 ret=writeFooter();
		if(ret!=E_SUCCESS)
			return ret;
		ret=close(m_hLogFile);
		m_hLogFile=-1;
		if(ret==0)
			return E_SUCCESS;
		return E_UNKNOWN;
	}

SINT32 CADataRetentionLogFile::log(t_dataretentionLogEntry* logEntry)
	{
		SINT32 ret=E_SUCCESS;
		memcpy(m_arOneBlock+m_nBytesPerLogEntry*m_nCurrentLogEntriesInBlock,logEntry,m_nBytesPerLogEntry);
		m_nCurrentLogEntriesInBlock++;
		
		if(m_nCurrentLogEntriesInBlock>=m_nLogEntriesPerBlock)
			{//Block is full -->encrypt and write them
				UINT32 nonce=htonl(m_nCurrentBlockNumber);
				::gcm_encrypt_64k(m_pGCMCtx, (UINT8*)&nonce, 4, m_arOneBlock,m_nLogEntriesPerBlock*m_nBytesPerLogEntry,
												NULL,0,m_encBlock,m_encBlock+m_nLogEntriesPerBlock*m_nBytesPerLogEntry);

				if(write(m_hLogFile,m_encBlock,m_nLogEntriesPerBlock*m_nBytesPerLogEntry+16)!=m_nLogEntriesPerBlock*m_nBytesPerLogEntry+16)
					{
						CAMsg::printMsg(LOG_ERR,"Error: data retention log entry was not fully written to disk!\n");
						ret=E_UNKNOWN;
					}
				m_nCurrentLogEntriesInBlock=0;
				m_nCurrentBlockNumber++;
			}
		return ret;
	}
#endif //DATA_RETENTION_LOG