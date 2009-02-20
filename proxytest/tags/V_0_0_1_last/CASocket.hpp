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

class CASocket
	{
		public:
			CASocket();
			~CASocket(){close();DeleteCriticalSection(&csClose);}

			int create();

			SINT32 listen(LPCASOCKETADDR psa);
			SINT32 listen(UINT16 port);
			int accept(CASocket &s);
			int connect(LPCASOCKETADDR psa);
			int connect(LPCASOCKETADDR psa,UINT retry,UINT32 time);
			int close();
			int close(int mode);
			int send(UINT8* buff,UINT32 len);
			int available();
			int receive(UINT8* buff,UINT32 len);
			operator SOCKET(){return m_Socket;}
			int getLocalPort();
			int setReuseAddr(bool b);
			int setRecvLowWat(UINT32 r);
			int setRecvBuff(UINT32 r);
			int setSendBuff(UINT32 r);
			int setKeepAlive(bool b);
		private:
			SOCKET m_Socket;
			#ifdef _REENTRANT
				CRITICAL_SECTION csClose;
			#endif
			int closeMode;
			// temporary hack...
			int localPort;
	};
#endif