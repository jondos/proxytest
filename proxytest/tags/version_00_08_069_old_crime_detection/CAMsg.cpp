/*
Copyright (c) 2000, The JAP-Team
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.

	- Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation and/or
		other materials provided with the distribution.

	- Neither the name of the University of Technology Dresden, Germany nor the names of its contributors
	  may be used to endorse or promote products derived from this software without specific
		prior written permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/
#include "StdAfx.h"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CAASymCipher.hpp"
#include "CASymCipher.hpp"
#include "CAUtil.hpp"
#include "CABase64.hpp"
#define FILENAME_ENCRYPTEDLOG "/encrypted_messages"
#define FILENAME_INFOLOG "/messages"
#define FILENAME_INFOLOG_GZ "/messages.gz"

#define MAX_MSG_SIZE 8192
extern CACmdLnOptions* pglobalOptions;

CAMsg* CAMsg::pMsg=NULL;

const char* const CAMsg::m_strMsgTypes[6]={", error   ] ",", critical] ",", info    ] ",", debug   ] ",", special ] ",", warning ] "}; //all same size!
#define STRMSGTYPES_SIZE 12

CAMsg::CAMsg()
    {
			m_pcsPrint=new CAMutex();
			setZero64(m_maxLogFileSize);
			m_strMsgBuff=new char[MAX_MSG_SIZE+1+20+STRMSGTYPES_SIZE];
			m_uLogType=MSG_STDOUT;
			m_hFileInfo=-1;
			m_hFileEncrypted=-1;
			m_strLogFile=new char[1024];
			m_strMsgBuff[0]='[';
#ifdef COMPRESSED_LOGS
			m_gzFileInfo=NULL;
#endif
			m_pCipher=NULL;
		}

CAMsg::~CAMsg()
    {
			closeLog();
#ifndef ONLY_LOCAL_PROXY
			closeEncryptedLog();
#endif
			delete[] m_strMsgBuff;
			m_strMsgBuff = NULL;
			delete[] m_strLogFile;
			m_strLogFile = NULL;
			delete m_pcsPrint;
			m_pcsPrint = NULL;
		}

SINT32 CAMsg::setLogOptions(UINT32 opt)
    {
			if(pMsg->m_uLogType==opt)
					return E_SUCCESS;
			if(pMsg->openLog(opt)==E_SUCCESS)
				{
					pMsg->closeLog(); //closes the OLD Log!
					pMsg->m_uLogType=opt;
					return E_SUCCESS;
				}
			return E_UNKNOWN;
    }

SINT32 CAMsg::printMsg(UINT32 type,const char* format,...)
	{
		if(pMsg != NULL)
		{
			pMsg->m_pcsPrint->lock();
			va_list ap;
			va_start(ap,format);
			SINT32 ret=E_SUCCESS;

			//Date is: yyyy/mm/dd-hh:mm:ss   -- the size is: 19
			time_t currtime=time(NULL);
			strftime(pMsg->m_strMsgBuff+1,255,"%Y/%m/%d-%H:%M:%S",localtime(&currtime));
			switch(type)
				{
					case LOG_DEBUG:
						strcat(pMsg->m_strMsgBuff,pMsg->m_strMsgTypes[3]);
					break;
					case LOG_INFO:
						strcat(pMsg->m_strMsgBuff,pMsg->m_strMsgTypes[2]);
					break;
					case LOG_CRIT:
						strcat(pMsg->m_strMsgBuff,pMsg->m_strMsgTypes[1]);
					break;
					case LOG_ERR:
						strcat(pMsg->m_strMsgBuff,pMsg->m_strMsgTypes[0]);
					break;
					case LOG_ENCRYPTED:
						strcat(pMsg->m_strMsgBuff,pMsg->m_strMsgTypes[4]);
					break;
					case LOG_WARNING:
						strcat(pMsg->m_strMsgBuff,pMsg->m_strMsgTypes[5]);
					break;
					default:
						va_end(ap);
						pMsg->m_pcsPrint->unlock();
						return E_UNKNOWN;
				}
	#ifdef HAVE_VSNPRINTF
			vsnprintf(pMsg->m_strMsgBuff+20+STRMSGTYPES_SIZE,MAX_MSG_SIZE,format,ap);
	#else
		  trio_vsnprintf(pMsg->m_strMsgBuff+20+STRMSGTYPES_SIZE,MAX_MSG_SIZE,format,ap);
	#endif
			va_end(ap);
			if(type==LOG_ENCRYPTED)
			{
				ret=strlen(pMsg->m_strMsgBuff);
				if(	pMsg->m_pCipher==NULL)
				{
					ret=E_UNKNOWN;
				}
				else
				{
					UINT8 bp[MAX_MSG_SIZE];
					AES_ofb128_encrypt((UINT8*)pMsg->m_strMsgBuff,
															bp,ret,
															&(pMsg->m_pCipher->oKey),
															pMsg->m_pCipher->iv,
															&(pMsg->m_pCipher->iv_off));
					if(write(pMsg->m_hFileEncrypted,bp,ret)!=ret)
						ret=E_UNKNOWN;
			 	}
			}
			else
				{
					switch(pMsg->m_uLogType)
						{
							case MSG_LOG:
	#ifndef _WIN32
						syslog(type,pMsg->m_strMsgBuff);
	#endif
							break;
							case MSG_FILE:
/*								if(pMsg->m_hFileInfo==-1)
									{
										pMsg->m_hFileInfo=open(pMsg->m_strLogFile,O_APPEND|O_CREAT|O_WRONLY|O_NONBLOCK|O_LARGEFILE|O_SYNC,S_IREAD|S_IWRITE);
									}
*///								if(pMsg->m_hFileInfo!=-1)
//									{
			#ifdef PSEUDO_LOG
										char buff[255];
										sprintf(buff,"%.15s mix AnonMix: ",ctime(&currtime)+4);
										write(pMsg->m_hFileInfo,buff,strlen(buff));
			#endif
										if(write(pMsg->m_hFileInfo,pMsg->m_strMsgBuff,strlen(pMsg->m_strMsgBuff))==-1)
											ret=E_UNKNOWN;
										pMsg->m_NrOfWrites++;
										if(pMsg->m_NrOfWrites>10000&&!isZero64(pMsg->m_maxLogFileSize)&&isGreater64(filesize64(pMsg->m_hFileInfo),pMsg->m_maxLogFileSize))
											{
												pMsg->closeLog();
												pMsg->openLog(pMsg->m_uLogType);
											}
//									}
							break;
	#ifdef COMPRESSED_LOGS
							case MSG_COMPRESSED_FILE:
								if(pMsg->m_gzFileInfo!=NULL)
								{
									if(gzwrite(pMsg->m_gzFileInfo,pMsg->m_strMsgBuff,strlen(pMsg->m_strMsgBuff))==-1)
										ret=E_UNKNOWN;
								}
							break;
	#endif
							case MSG_STDOUT:
								printf("%s",pMsg->m_strMsgBuff);
							break;
							default:
							ret=E_UNKNOWN;
						}
				}
			pMsg->m_pcsPrint->unlock();
			return ret;
		}
		else
		{
			printf("Warning logger is not active!\n");
			return E_UNKNOWN;
		}
  }

SINT32 CAMsg::closeLog()
	{
		switch(m_uLogType)
			{
				case MSG_LOG:
#ifndef _WIN32
					::closelog();
#endif
				break;
				case MSG_FILE:
					if(m_hFileInfo!=-1)
						close(m_hFileInfo);
					m_hFileInfo=-1;
#ifdef COMPRESSED_LOGS
				case MSG_COMPRESSED_FILE:
					if(m_gzFileInfo!=NULL)
						gzclose(m_gzFileInfo);
					m_gzFileInfo=NULL;
				break;
#endif
		}
		return E_SUCCESS;
	}

SINT32 CAMsg::openLog(UINT32 type)
	{
//		int tmpHandle=-1;
		time_t currtime=0;
		switch(type)
			{
				case MSG_LOG:
#ifndef _WIN32
			openlog("AnonMix",0,LOG_USER);
#endif
				break;
				case MSG_FILE:
					if(pglobalOptions->getLogDir((UINT8*)m_strLogFile,1024)!=E_SUCCESS)
						return E_UNKNOWN;
					strcat(m_strLogFile,FILENAME_INFOLOG);
					if(pglobalOptions->getMaxLogFileSize()>0)
					{
						currtime=time(NULL);
						strftime(m_strLogFile+strlen(m_strLogFile),1024-strlen(m_strLogFile),"%Y%m%d-%H%M%S",localtime(&currtime));
						//!! DO NOT USE O_SYNC - it is _terrible_ slow!!!!
						m_hFileInfo=open(m_strLogFile,O_APPEND|O_CREAT|O_WRONLY|O_NONBLOCK|O_LARGEFILE,S_IREAD|S_IWRITE);
						setMaxLogFileSize(pglobalOptions->getMaxLogFileSize());
						m_NrOfWrites=0;
					}
					else
					{
						//!! DO NOT USE O_SYNC - it is _terrible_ slow!!!!
						m_hFileInfo=open(m_strLogFile,O_APPEND|O_CREAT|O_WRONLY|O_NONBLOCK|O_LARGEFILE,S_IREAD|S_IWRITE);
					}
				break;
#ifdef COMPRESSED_LOGS
				case MSG_COMPRESSED_FILE:
			char logdir[255];
			char buff[1024];
			if(pglobalOptions->getLogDir((UINT8*)logdir,255)!=E_SUCCESS)
				return E_UNKNOWN;
			strcpy(buff,logdir);
			strcat(buff,FILENAME_INFOLOG_GZ);
			tmpHandle=open(buff,O_APPEND|O_CREAT|O_WRONLY,S_IREAD|S_IWRITE);
			m_gzFileInfo=gzdopen(tmpHandle,"wb9");
			if(m_gzFileInfo==NULL)
				return E_UNKNOWN;
				break;
#endif
				default:
					return E_UNKNOWN;
		}
		return E_SUCCESS;
	}

#ifndef ONLY_LOCAL_PROXY
/** Open a log, where the logged messages are store encrypted.
	* The file format is as follows:
	*	NEWLINE
	*	"----Start of EncryptionKey----"
	* NEWLINE
	* 128 random bytes (BASE64 encoded of RSA encrpyted block) which are used as sym key (16 bytes starting form 50. byte) and IV (next 16 bytes)
	* NEWLINE
	* "-----End of EncryptionKey-----"
	* NEWLINE
	* encrpyted messages
	*
	* The message is encrypted using AES-128-OFB.
	*/
	SINT32 CAMsg::openEncryptedLog()
	{
		CACertificate* pCert=pglobalOptions->getLogEncryptionKey();
		if(pCert==NULL)
			return E_UNKNOWN;
		CAASymCipher oRSA;
		SINT32 ret=oRSA.setPublicKey(pCert);
		delete pCert;
		pCert = NULL;
		if(ret!=E_SUCCESS)
			return E_UNKNOWN;
		UINT8 buff[1024];
		if(pglobalOptions->getEncryptedLogDir(buff,1024)!=E_SUCCESS)
			if(pglobalOptions->getLogDir(buff,1024)!=E_SUCCESS)
				return E_UNKNOWN;
		strcat((char*)buff,FILENAME_ENCRYPTEDLOG);
		pMsg->m_hFileEncrypted=open((char*)buff,O_APPEND|O_CREAT|O_WRONLY|O_LARGEFILE|O_BINARY,S_IREAD|S_IWRITE);
		if(pMsg->m_hFileEncrypted<=0)
			{
				pMsg->m_hFileEncrypted=-1;
				return E_UNKNOWN;
			}
		//create sym enc key and write it to the file (enc with pub enc key)
		write(pMsg->m_hFileEncrypted,"\n----Start of EncryptionKey----\n",32);
		UINT8 keyandiv[128];
		getRandom(keyandiv,128);
		keyandiv[0]&=0x7F;
		pMsg->m_pCipher=new t_LogEncCipher;
		AES_set_encrypt_key(keyandiv+50,128,&pMsg->m_pCipher->oKey);
		memcpy(pMsg->m_pCipher->iv,keyandiv+66,16);
		pMsg->m_pCipher->iv_off=0;
		UINT8 out[255];
		UINT32 outlen=255;
		oRSA.encrypt(keyandiv,keyandiv);
		CABase64::encode(keyandiv,128,out,&outlen);
		write(pMsg->m_hFileEncrypted,out,outlen);
		write(pMsg->m_hFileEncrypted,"-----End of EncryptionKey-----\n",31);
		return E_SUCCESS;
	}

SINT32 CAMsg::closeEncryptedLog()
	{
		if(pMsg->m_hFileEncrypted>=0)
			{
				close(pMsg->m_hFileEncrypted);
			}
		delete pMsg->m_pCipher;
		pMsg->m_pCipher=NULL;
		pMsg->m_hFileEncrypted=-1;
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
