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

#include "CASignature.hpp"
#include "CAFirstMix.hpp"
#include "CAThread.hpp"
#include "CAMutex.hpp"
class CAInfoService
	{
		public:
			CAInfoService(CAFirstMix* pFirstMix);
			~CAInfoService();
			SINT32 sendHelo();
			int start();
			int stop();
			//SINT32 setLevel(SINT32 user,SINT32 risk,SINT32 traffic);
			SINT32 getLevel(SINT32* puser,SINT32* prisk,SINT32* ptraffic);
			//SINT32 setMixedPackets(UINT32 packets);
			SINT32 getMixedPackets(UINT32* ppackets);
			bool getRun(){return bRun;}
			SINT32 setSignature(CASignature* pSignature);
			CASignature* getSignature(){return pSignature;}
		private:
			//SINT32 iUser;
			//SINT32 iRisk;
			//SINT32 iTraffic; 
			//UINT32 m_MixedPackets;
			bool bRun;
			//CAMutex csLevel;
			CASignature* pSignature;
			CAFirstMix* m_pFirstMix;
			CAThread m_threadRunLoop;
	};
