#ifndef __CASOCKET__
#define __CASOCKET__
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
#include "CASocketAddr.hpp"
class CASocketASyncSend;
class CASocketASyncSendResume;
class CASocket
	{
		public:
			CASocket();
			~CASocket(){close();DeleteCriticalSection(&csClose);}

			SINT32 create();
			SINT32 create(int type);

			SINT32 listen(CASocketAddr& psa);
			SINT32 listen(UINT16 port);
			SINT32 accept(CASocket &s);
			SINT32 connect(CASocketAddr& psa);
			SINT32 connect(CASocketAddr& psa,UINT retry,UINT32 msWaitTime);
			SINT32 connect(CASocketAddr& psa,UINT32 msTimeOut);
			SINT32 close();
			SINT32 close(int mode);
			int send(UINT8* buff,UINT32 len,bool bDisableAsync=false);
			int sendTimeOut(UINT8* buff,UINT32 len,UINT32 msTimeOut);
#ifdef HAVE_FIONREAD
			#define HAVE_AVAILABLE
			SINT32 available();
#endif
			int receive(UINT8* buff,UINT32 len);
			SINT32 receiveFully(UINT8* buff,UINT32 len);
			SINT32 receiveFully(UINT8* buff,UINT32 len,SINT32 timeout);
			operator SOCKET(){return m_Socket;}
			int getLocalPort();
			SINT32 getPeerIP(UINT8 ip[4]);
			SINT32 setReuseAddr(bool b);
			SINT32 setRecvLowWat(UINT32 r);
			SINT32 setSendLowWat(UINT32 r);
			SINT32 getSendLowWat();
			SINT32 setSendTimeOut(UINT32 msTimeOut);
			SINT32 getSendTimeOut();
			SINT32 setRecvBuff(UINT32 r);
			SINT32 getRecvBuff();
			SINT32 setSendBuff(UINT32 r);
			SINT32 getSendBuff();
			SINT32 setKeepAlive(bool b);
			SINT32 setKeepAlive(UINT32 sec);
			SINT32 setNonBlocking(bool b);
			SINT32 getNonBlocking(bool* b);
			SINT32 setASyncSend(bool b,SINT32 size,UINT32 lowwater,UINT32 SendQueueSoftLimit,CASocketASyncSendResume* pResume);
		private:
			UINT8 m_ipPeer[4];
			SOCKET m_Socket;
			#ifdef _REENTRANT
				CRITICAL_SECTION csClose;
			#endif
			int closeMode;
			// temporary hack...
			int localPort;
			bool m_bASyncSend;
			static CASocketASyncSend* m_pASyncSend;
	};
#endif