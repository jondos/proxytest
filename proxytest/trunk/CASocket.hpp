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
#include "CAMutex.hpp"
#ifdef _DEBUG
	extern int sockets;
#endif

class CASocket
	{
		public:
			CASocket();
			virtual ~CASocket(){close();}

			SINT32 create();
			SINT32 create(int type);

			SINT32 listen(const CASocketAddr& psa);
			SINT32 listen(UINT16 port);
			SINT32 accept(CASocket &s);
			virtual SINT32 connect(CASocketAddr& psa)
				{
					return connect(psa,1,0);
				}
				
			virtual SINT32 connect(CASocketAddr& psa,UINT32 retry,UINT32 msWaitTime);
			virtual SINT32 connect(CASocketAddr& psa,UINT32 msTimeOut);
			
			virtual SINT32 close();
/* it seems that this function is not used:
			SINT32 close(UINT32 mode);*/
			virtual SINT32 send(const UINT8* buff,UINT32 len);
			SINT32 sendFully(const UINT8* buff,UINT32 len);
			SINT32 sendTimeOut(const UINT8* buff,UINT32 len,UINT32 msTimeOut);
			virtual SINT32 receive(UINT8* buff,UINT32 len);
			SINT32 receiveFully(UINT8* buff,UINT32 len);
			SINT32 receiveFully(UINT8* buff,UINT32 len,UINT32 msTimeOut);
			/** Returns the number of the Socket used. Which will be always the same number,
				* even after close(), until the Socket
				* is recreated using create()
				* @return number of the associated socket
			**/
			operator SOCKET(){return m_Socket;}
			SINT32 getLocalPort();
			SINT32 getPeerIP(UINT8 ip[4]);
			SINT32 setReuseAddr(bool b);
			//SINT32 setRecvLowWat(UINT32 r);
			//SINT32 setSendLowWat(UINT32 r);
			//SINT32 getSendLowWat();
			SINT32 setSendTimeOut(UINT32 msTimeOut);
			SINT32 getSendTimeOut();
			SINT32 setRecvBuff(UINT32 r);
			SINT32 getRecvBuff();
			SINT32 setSendBuff(SINT32 r);
			SINT32 getSendBuff();
			SINT32 setKeepAlive(bool b);
			SINT32 setKeepAlive(UINT32 sec);
			SINT32 setNonBlocking(bool b);
			SINT32 getNonBlocking(bool* b);
		protected:
			bool m_bSocketIsClosed; //this is a flag, which shows, if the m_Socket is valid
													//we should not set m_Socket to -1 or so after close,
													//because the Socket value ist needed sometimes even after close!!!
													// (because it is used as a Key in lookups for instance as a HashValue etc.)

			SOCKET m_Socket;
		private:			
			CAMutex m_csClose;
			UINT32 m_closeMode;
	};
#endif
