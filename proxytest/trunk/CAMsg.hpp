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
// TODO: Sylog for NT!!

#ifndef __CAMSG__
#define __CAMSG__

#define MSG_STDOUT						0x00
#define MSG_LOG								0x01
#define MSG_FILE							0x02
#define MSG_COMPRESSED_FILE		0x03

#ifdef _WIN32
	#define LOG_ERR		0
	#define LOG_CRIT	1
	#define LOG_INFO	2
	#define LOG_DEBUG 3
#endif
#define LOG_ENCRYPTED 255

#include "CAMutex.hpp"
#include "CASymCipher.hpp"

struct S_LOGENCCIPHER
{
	UINT8 iv[16];
	int iv_off;
	AES_KEY oKey;
};

typedef S_LOGENCCIPHER t_LogEncCipher;

class CAMsg
	{
		private:
			CAMsg(); //Singleton!
			static CAMsg* pMsg;
		public:
			~CAMsg();
			static SINT32 init(){pMsg=new CAMsg();return E_SUCCESS;}
			static SINT32 cleanup()
			{
				delete pMsg;
				pMsg = NULL;
				return E_SUCCESS;
			}
			static SINT32 setLogOptions(UINT32 options);
			static SINT32 setMaxLogFileSize(UINT64 size)
				{
					if(pMsg!=NULL)
						{
							set64(pMsg->m_maxLogFileSize,size);
							return E_SUCCESS;
						}
					return E_UNKNOWN;
				}

			static SINT32 printMsg(UINT32 typ,const char* format,...);
#ifndef ONLY_LOCAL_PROXY
			static SINT32 openEncryptedLog();
			static SINT32 closeEncryptedLog();
#endif //ONLY_LOCAL_PROXY
		private:
			SINT64 m_maxLogFileSize;
			UINT32 m_NrOfWrites;
			SINT32 openLog(UINT32 type);
			SINT32 closeLog();
			UINT32 m_uLogType;
			int m_hFileEncrypted;
			int m_hFileInfo;
			char *m_strMsgBuff;
			char *m_strLogFile;
			static const char* const m_strMsgTypes[6];
			CAMutex* m_pcsPrint;
#ifdef COMPRESSED_LOGS
			gzFile m_gzFileInfo;
#endif
			t_LogEncCipher* m_pCipher;
   };
#endif
