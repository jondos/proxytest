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
#ifndef __CASINGLESOCKETGROUP__
#define __CASINGLESOCKETGROUP__
#ifndef HAVE_POLL
#include "CASocketGroup.hpp"
typedef CASocketGroup CASingleSocketGroup;
#else
#include "CAMuxSocket.hpp"
/** Not thread safe!*/
class CASingleSocketGroup
	{
		public:
			CASingleSocketGroup(bool bWrite)
				{
					m_pollfd=new struct pollfd;
					setPoolForWrite(bWrite);
				}
			~CASingleSocketGroup(){delete m_pollfd;}
			
			SINT32 add(CASocket&s)
				{
					m_pollfd->fd=(SOCKET)s;
					return E_SUCCESS;
				}

			SINT32 add(CAMuxSocket&s)
				{
					m_pollfd->fd=(SOCKET)s;
					return E_SUCCESS;
				}

			SINT32 setPoolForWrite(bool bWrite)
				{
					if(bWrite)
						m_pollfd->events=POLLOUT;
					else
						m_pollfd->events=POLLIN;
					return E_SUCCESS;
				}

			SINT32 select()
				{
					return ::poll(m_pollfd,1,-1);
				}
			
			SINT32 select(UINT32 time_ms)
				{
					SINT32 ret=::poll(m_pollfd,1,time_ms);
					if(ret==1)
						return ret;
					else if(ret==0)
						{
							return E_TIMEDOUT;
						}
					else if(ret==SOCKET_ERROR)
						{
							#ifdef _DEBUG
								ret=GET_NET_ERROR;
								CAMsg::printMsg(LOG_DEBUG,"SocketGroup Select-Fehler: %i\n",ret);
							#endif
							return E_UNKNOWN;
						}
					return E_UNKNOWN;
				}


		private:
			struct pollfd* m_pollfd;
	};
#endif
#endif
