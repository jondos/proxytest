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
#ifndef __CALASTMIX__
#define __CALASTMIX__

#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
#include "CAASymCipher.hpp"
#include "CASocketAddrINet.hpp"
#include "CACacheLoadBalancing.hpp"
#include "CASignature.hpp"
#include "CAUtil.hpp"
#include "CAQueue.hpp"
#include "CAInfoService.hpp"
#ifdef LOG_CRIME
	#include "tre/regex.h"
#endif

class CALastMix:public CAMix

	{
		public:
			CALastMix()
				{
					m_pMuxIn=NULL;m_pSignature=NULL;
					m_pRSA=NULL;m_pInfoService=NULL;
					m_pthreadSendToMix=m_pthreadReadFromMix=NULL;
					m_pQueueSendToMix=m_pQueueReadFromMix=NULL;
				}
			virtual ~CALastMix(){clean();}
			SINT32 reconfigure();
		protected:
			virtual SINT32 loop()=0;
			SINT32 init();
			SINT32 initOnce();
			SINT32 clean();

			SINT32 processKeyExchange();
			SINT32 setTargets();
#ifdef LOG_CRIME
			bool	 checkCrime(UINT8* payLoad,UINT32 payLen);
#endif

		protected:
			volatile bool					m_bRestart;
			CAMuxSocket*					m_pMuxIn;
			CAQueue*							m_pQueueSendToMix;
			CAQueue*							m_pQueueReadFromMix;
			CACacheLoadBalancing	m_oCacheLB;
			CASocketAddrINet			maddrSocks;
			CAASymCipher*					m_pRSA;
			CASignature*					m_pSignature;
			CAInfoService*				m_pInfoService;
			CAThread*							m_pthreadSendToMix;
			CAThread*							m_pthreadReadFromMix;
#ifdef LOG_CRIME
			regex_t*							m_pCrimeRegExps;
			UINT32								m_nCrimeRegExp;
#endif

		protected:
			friend THREAD_RETURN	lm_loopSendToMix(void* param);
			friend THREAD_RETURN	lm_loopReadFromMix(void* pParam);
			friend THREAD_RETURN	lm_loopLog(void*);
			volatile bool					m_bRunLog;
			volatile UINT32				m_logUploadedPackets;
			volatile UINT64				m_logUploadedBytes;
			volatile UINT32				m_logDownloadedPackets;
			volatile UINT64				m_logDownloadedBytes;
	};

#endif
