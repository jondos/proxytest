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

#ifndef __CAFIRSTMIX__
#define __CAFIRSTMIX__

#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
#include "CAASymCipher.hpp"
#include "CASignature.hpp"
#include "CAMuxChannelList.hpp"
#include "CASocketASyncSend.hpp"
class CAFirstMix:public CAMix,CASocketASyncSendResume
	{
		public:
			CAFirstMix(){InitializeCriticalSection(&csResume);m_MixedPackets=0;}
			virtual ~CAFirstMix(){DeleteCriticalSection(&csResume);}
		private:
			SINT32 loop();
			SINT32 init();
#ifdef PROT2
			SINT32 clean();
			SINT32 initOnce();
#endif
		private:
			CASocket		socketIn;
			CAMuxSocket muxOut;
#ifndef PROT2
			UINT8* recvBuff;
#else
			UINT8* mKeyInfoBuff;
			UINT16 mKeyInfoSize;
			UINT32 m_MixedPackets;
			CAASymCipher mRSA;
			CASignature mSignature;
#endif
			CAMuxChannelList oSuspendList;
		public:
			void resume(CASocket* pSocket);
			SINT32 getMixedPackets(UINT32* ppackets)
				{
					if(ppackets!=NULL)
						{
							*ppackets=m_MixedPackets;
							return E_SUCCESS;
						}
					return E_UNKNOWN;
				}
		private:	
			CRITICAL_SECTION csResume;
			void deleteResume(CAMuxSocket* pMuxSocket);
			void deleteResume(CAMuxSocket*pMuxSocket,HCHANNEL outCahnnel);
	};

#endif