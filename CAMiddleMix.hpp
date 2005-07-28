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
#ifndef __CAMIDDLEMIX__
#define __CAMIDDLEMIX__

#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
#include "CAASymCipher.hpp"
#include "CAMiddleMixChannelList.hpp"
#include "CASignature.hpp"
#include "CAInfoService.hpp"

class CAMiddleMix:public CAMix
	{
		public:
			CAMiddleMix():CAMix()
				{
					m_pMiddleMixChannelList=NULL;
					m_pMuxOut=NULL;m_pMuxIn=NULL;m_pRSA=NULL;m_pSignature=NULL;
					m_pInfoService=NULL;
				}
			virtual ~CAMiddleMix(){clean();};
			tMixType getType()
				{
					return CAMix::MIDDLE_MIX;
				}

		private:
			SINT32 loop();
			SINT32 init();
			SINT32 initOnce();
			SINT32 clean();
			
    /**
    * This method is not applicable to middle mixes; it does nothing
    * @param d ignored
    * @retval E_SUCCESS in any case
    */
    virtual SINT32 initMixCascadeInfo(DOM_Element& )
    {
        return E_SUCCESS;
    }
    
    virtual SINT32 processKeyExchange();

private:
			CAMuxSocket* m_pMuxIn;
			CAMuxSocket* m_pMuxOut;
			CAASymCipher* m_pRSA;
			//CASignature* m_pSignature;
			volatile bool m_bRun;
			CAMiddleMixChannelList* m_pMiddleMixChannelList;
			//CAInfoService* m_pInfoService;
			friend THREAD_RETURN mm_loopDownStream(void *p);
	};

#endif
