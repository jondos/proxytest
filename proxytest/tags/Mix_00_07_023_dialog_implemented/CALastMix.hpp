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
#ifndef ONLY_LOCAL_PROXY
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
#include "CALogPacketStats.hpp"
#ifndef NEW_MIX_TYPE // not TypeB mixes
  /* TypeB mixes are using an own implementation */
  #include "CALastMixChannelList.hpp"
#endif

#ifdef REPLAY_DETECTION
	#include "CAMixWithReplayDB.hpp"
#endif

THREAD_RETURN	lm_loopLog(void*);
THREAD_RETURN	lm_loopSendToMix(void* param);
THREAD_RETURN	lm_loopReadFromMix(void* pParam);
	
class CALastMix:public 
#ifdef REPLAY_DETECTION
	CAMixWithReplayDB
#else
	CAMix
#endif
	{
		public:
			CALastMix()
				{
					m_pMuxIn=NULL;
					m_pSignature=NULL;
					m_pRSA=NULL;
					m_pInfoService=NULL;
          #ifndef NEW_MIX_TYPE // not TypeB mixes
            /* TypeB mixes are using an own implementation */
					m_pChannelList=NULL;
          #endif
					m_pthreadSendToMix=m_pthreadReadFromMix=NULL;
					m_pQueueSendToMix=m_pQueueReadFromMix=NULL;
					m_pCacheLB=new CACacheLoadBalancing();
					m_pSocksLB=new CACacheLoadBalancing();
					#ifdef LOG_PACKET_STATS
						m_pLogPacketStats=NULL;
					#endif
				}

			virtual ~CALastMix()
				{
					clean();
					delete m_pCacheLB;
					delete m_pSocksLB;
				}

			SINT32 reconfigure();
			tMixType getType() const
				{
					return CAMix::LAST_MIX;
				}

	protected:
#ifdef DYNAMIC_MIX
			void stopCascade()
			{
				m_bRestart = true;
			}
#endif
			virtual SINT32 loop()=0;
			SINT32 init();
			SINT32 initOnce();
			SINT32 clean();
      #ifdef NEW_MIX_TYPE // TypeB mixes
        virtual void reconfigureMix();
      #endif

    // added by ronin <ronin2@web.de>
    SINT32 initMixCascadeInfo(DOMElement* );

    // moved to CAMix.hpp
    virtual SINT32 processKeyExchange();
    

			SINT32 setTargets();
#ifdef LOG_CRIME
			bool	 checkCrime(const UINT8* payLoad,UINT32 payLen);
#endif

		protected:
			volatile bool					m_bRestart;
			CAMuxSocket*					m_pMuxIn;
			CAQueue* m_pQueueSendToMix;
			CAQueue* m_pQueueReadFromMix;
			#ifdef LOG_PACKET_TIMES
				CALogPacketStats* m_pLogPacketStats;
			#endif
			CACacheLoadBalancing*	m_pCacheLB;
			CACacheLoadBalancing* m_pSocksLB;
			CAASymCipher*					m_pRSA;
			CAThread*							m_pthreadSendToMix;
			CAThread*							m_pthreadReadFromMix;
      #ifndef NEW_MIX_TYPE // not TypeB mixes
        /* TypeB mixes are using an own implementation */
			CALastMixChannelList* m_pChannelList;
      #endif

#ifdef LOG_CRIME
			regex_t*							m_pCrimeRegExpsURL;
			UINT32								m_nCrimeRegExpsURL;
			regex_t*							m_pCrimeRegExpsPayload;
			UINT32								m_nCrimeRegExpsPayload;
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
#endif //ONLY_LOCAL_PROXY
