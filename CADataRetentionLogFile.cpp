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
		m_nLogEntriesPerBlock=128;
		m_arOneBlock=new UINT8[m_nLogEntriesPerBlock*m_nBytesPerLogEntry];
		m_encBlock=new UINT8[m_nLogEntriesPerBlock*m_nBytesPerLogEntry+1024];
		m_pGCMCtx=new gcm_ctx_64k;
		m_nonceBuffForLogEntries=new UINT8[12];
	}	

CADataRetentionLogFile::~CADataRetentionLogFile()
	{
		closeLog();
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
		strftime((char*) strDate,255,"%Y%m%d-%H%M%S",theTime);
		m_Day=theTime->tm_mday;
		m_Month=theTime->tm_mon+1;
		m_Year=theTime->tm_year+1900;
		
//		m_nMaxLogTime=date-theTime->tm_min*60-theTime->tm_hour*3600-theTime->tm_sec+24*3600-1;
	
		m_nMaxLogTime=date+60;

		snprintf((char*)strFileName,4096,"%s/dataretentionlog_%s",strLogDir,strDate);
		//!! DO NOT USE O_SYNC - it is _terrible_ slow!!!!
		m_hLogFile=open((char*)strFileName,O_APPEND|O_CREAT|O_WRONLY|O_LARGEFILE|O_BINARY,S_IREAD|S_IWRITE);
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
		oHeader.year=htons(m_Year);
		oHeader.entriesPerBlock=m_nLogEntriesPerBlock;
		oHeader.keys=1;
		oHeader.loggedFields=0x00FF;//DATARETETION_LOGGED_FIELD_T_IN|DATARETETION_LOGGED_FIELD_T_OUT
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
		UINT8 keybuff[256];
		getRandom(keybuff,16);
		gcm_init_64k(m_pGCMCtx,keybuff,128);
		UINT8 encKey[2048];
		UINT32 encKeyLen=2048;
//Set date
		keybuff[16]=m_Day;
		keybuff[17]=m_Month;
		keybuff[18]=m_Year>>8;
		keybuff[19]=(m_Year&0x00FF);
//Calculate MAC
		UINT8 nonce[12];
		memset(nonce,0xFF,11);
		nonce[11]=0xFE;
		::gcm_encrypt_64k(m_pGCMCtx, nonce, 12, keybuff,20,
												NULL,0,encKey,keybuff+20);
		
		encKeyLen=256;
		pPublicKey->encryptPKCS1(keybuff,36,encKey,&encKeyLen);

//Calculate the symmetric key
		UINT8 md[SHA512_DIGEST_LENGTH];
		SHA512(keybuff+16,4,md);
		for(UINT32 i=0;i<16;i++)
			{
				keybuff[i]^=md[i];
			}
		gcm_destroy_64k(m_pGCMCtx);
		gcm_init_64k(m_pGCMCtx,keybuff,128);

		memset(keybuff,0,256);
		
		if(write(m_hLogFile,encKey,encKeyLen)!=encKeyLen)
			return E_UNKNOWN;

		//Calculate Auth tag and writ it..
		UINT8 tmpBuff[2048];
		nonce[11]=0xFD;
		memcpy(tmpBuff,&oHeader,sizeof(oHeader));
		memcpy(tmpBuff+sizeof(oHeader),encKey,encKeyLen);
		::gcm_encrypt_64k(m_pGCMCtx, nonce, 12, tmpBuff,sizeof(oHeader)+encKeyLen,
												NULL,0,tmpBuff+1024,keybuff);

		if(write(m_hLogFile,keybuff,16)!=16)
			return E_UNKNOWN;

		memset(m_nonceBuffForLogEntries,0,12);


		return E_SUCCESS;
	}

SINT32 CADataRetentionLogFile::flushLogEntries()
{
	if(m_nCurrentLogEntriesInBlock>0)
		{//Writte remaining log entries
			UINT32 nonce=htonl(m_nCurrentBlockNumber);
			memcpy(m_nonceBuffForLogEntries+8,&nonce,4);
			::gcm_encrypt_64k(m_pGCMCtx, m_nonceBuffForLogEntries ,12, m_arOneBlock,m_nCurrentLogEntriesInBlock*m_nBytesPerLogEntry,
												NULL,0,m_encBlock,m_encBlock+m_nCurrentLogEntriesInBlock*m_nBytesPerLogEntry);
			if(write(m_hLogFile,m_encBlock,m_nCurrentLogEntriesInBlock*m_nBytesPerLogEntry+16)!=m_nCurrentLogEntriesInBlock*m_nBytesPerLogEntry+16)
				{
					CAMsg::printMsg(LOG_ERR,"Error: data retention log entry was not fully written to disk!\n");
					return E_UNKNOWN;
				}
		}
	return E_SUCCESS;
}

SINT32 CADataRetentionLogFile::writeFooter()
	{
		SINT32 ret=flushLogEntries();
		if(ret!=E_SUCCESS)
			return E_UNKNOWN;
		UINT32 u=htonl(m_nCurrentLogEntriesInBlock+m_nCurrentBlockNumber*m_nLogEntriesPerBlock);
		UINT8 out[32];
		UINT8 nonce[12];
		memset(nonce,0xFF,12);
		::gcm_encrypt_64k(m_pGCMCtx, nonce ,12,(UINT8*) &u,4,
												NULL,0,out,out+4);
		if(write(m_hLogFile,out,20)!=20)
			return E_UNKNOWN;
		return E_SUCCESS;
	}

SINT32 CADataRetentionLogFile::closeLog()
	{
		if(m_hLogFile==-1)
			return E_SUCCESS;
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
				memcpy(m_nonceBuffForLogEntries+8,&nonce,4);
				::gcm_encrypt_64k(m_pGCMCtx, m_nonceBuffForLogEntries ,12, m_arOneBlock,m_nLogEntriesPerBlock*m_nBytesPerLogEntry,
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

SINT32 CADataRetentionLogFile::doCheckAndPerformanceTest()
{
		UINT8 keybuff[256];
		UINT8 tag[128];
		UINT8 nonce[12];
		memset(nonce,0xFF,12);
		UINT8 oneBlock[8192];
		memset(oneBlock,0xd4,8192);
		UINT8 encBlock[10000];
		memset(keybuff,0xC1,16);
		UINT32 lenBlock=64;
		UINT64 start;
		const UINT32 runs=100000;
		gcm_ctx_4k* pGCMCtx=new gcm_ctx_4k;
		
		for(int l=0;l<7;l++)
		{
			gcm_init_4k(pGCMCtx,keybuff,128);
			getcurrentTimeMillis(start);
			for(UINT32 i =0;i<runs;i++)
				::gcm_encrypt_4k(pGCMCtx, nonce ,12, oneBlock,lenBlock,NULL,0,encBlock,tag);
			UINT64 end;
			getcurrentTimeMillis(end);
			print64(encBlock,diff64(end,start));
			double bytes=(double)(runs*lenBlock);
			double thetime=(double)diff64(end,start);
			printf("Time for %u run of 4k encypt of %u bytes: %s [ms] (%f bytes/s)\n",runs,lenBlock,encBlock,bytes/thetime*1000.0); 
			lenBlock<<=1;
		}
		gcm_ctx_64k* pGCMCtx64=new gcm_ctx_64k;
		lenBlock=64;
		UINT8 tmpBlock[8192];
	
		for(int l=0;l<7;l++)
		{
			gcm_init_64k(pGCMCtx64,keybuff,128);
			getcurrentTimeMillis(start);
			for(UINT32 i =0;i<runs;i++)
				::gcm_encrypt_64k(pGCMCtx64, nonce ,12, oneBlock,lenBlock,NULL,0,encBlock,tag);
			UINT64 end;
			getcurrentTimeMillis(end);
			print64(tmpBlock,diff64(end,start));
			double bytes=(double)(runs*lenBlock);
			double thetime=(double)diff64(end,start);
			printf("Time for %u run of 64k encypt of %u bytes: %s [ms] (%f bytes/s)\n",runs,lenBlock,tmpBlock,bytes/thetime*1000.0); 
			lenBlock<<=1;
		}
		lenBlock>>=1;
		printf("Test finished!\n");

		memset(tmpBlock,1,8192);
		if(gcm_decrypt_4k(pGCMCtx,nonce,12,encBlock,lenBlock,tag,16,NULL,0,tmpBlock)==0||memcmp(oneBlock,tmpBlock,lenBlock)!=0)
			printf("Check failed!\n");
		else
			printf("Check success!\n");

		printf("Check finished!\n");
		return E_SUCCESS;
}

#endif //DATA_RETENTION_LOG
