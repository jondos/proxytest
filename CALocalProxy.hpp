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
#ifndef __CALOCALPROXY__
#define __CALOCALPROXY__

#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
#include "CAASymCipher.hpp"

// How many packets do you want to replay at a time?
#define REPLAY_COUNT 16
#ifndef NEW_MIX_TYPE
class CALocalProxy
	{
		public:
			CALocalProxy()
				{
					m_arRSA=NULL;
					m_pSymCipher=NULL;
				}
			
			~CALocalProxy(){clean();}
			
			// signals the main loop whether to capture or replay packets
			static bool bCapturePackets;
			static bool bReplayPackets;
			static int iCapturedPackets;
			SINT32 start();
	
		private:
			SINT32 loop();
			SINT32 init();
			SINT32 initOnce();
			SINT32 clean();
			SINT32 processKeyExchange(UINT8* buff,UINT32 size);
		private:
			CASocket m_socketIn;
			CASocket m_socketSOCKSIn;
			CAMuxSocket m_muxOut;
			UINT32 m_chainlen;
			UINT32 m_MixCascadeProtocolVersion;
			CAASymCipher* m_arRSA;
			CASymCipher* m_pSymCipher;
			UINT32 m_nFlowControlDownstreamSendMe;
			bool m_bWithNewFlowControl;
			bool m_bWithEnhancedChannelEncryption;
			UINT32 m_SymChannelEncryptedKeySize;
			UINT32 m_SymChannelKeySize;
			bool m_bWithFirstMixSymmetric;
	};
#endif //!NEW_MIX_TYPE
#endif
