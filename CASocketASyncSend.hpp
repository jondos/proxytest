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
#include "CASocketGroup.hpp"
#include "CASocket.hpp"
#include "CAQueue.hpp"

class CAFirstMix;

typedef struct __t_socket_list
	{
		CASocket* pSocket;
		CAQueue* pQueue;
		bool bwasOverFull;
		__t_socket_list* next;
} _t_socket_list;
THREAD_RETURN SocketASyncSendLoop(void* p);

class CASocketASyncSend
	{	
		public:
			CASocketASyncSend(){m_Sockets=NULL;InitializeCriticalSection(&cs);}
			~CASocketASyncSend();
			SINT32 send(CASocket* pSocket,UINT8* buff,UINT32 size);
			SINT32 close(CASocket* pSocket);
			SINT32 start();
			SINT32 stop(){return E_UNKNOWN;}
			friend THREAD_RETURN SocketASyncSendLoop(void* p);

		protected:
			CASocketGroup m_oSocketGroup;
			_t_socket_list* m_Sockets;
			#ifdef _REENTRANT
				CRITICAL_SECTION cs;
			#endif

				public:
					void setFirstMix(CAFirstMix* pMix){pFirstMix=pMix;}
				CAFirstMix* pFirstMix;
	};