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
#ifndef __CASOCKETGROUP__
#define __CASOCKETGROUP__
#include "CAMuxSocket.hpp"
#include "CAMutex.hpp"
class CASocketGroup
	{
		public:
			CASocketGroup(bool bWrite);
			~CASocketGroup()
				{
					#ifdef HAVE_POLL
						delete[] m_pollfd;
					#endif
				}
			
			inline SINT32 setPoolForWrite(bool bWrite);
			inline SINT32 add(CASocket&s);
			inline SINT32 add(CAMuxSocket&s);
			SINT32 remove(CASocket&s);
			SINT32 remove(CAMuxSocket&s);
			SINT32 select();
			SINT32 select(UINT32 time_ms);

			bool isSignaled(CASocket&s)
				{
					#ifndef HAVE_POLL
						return FD_ISSET((SOCKET)s,&m_signaled_set)!=0;
					#else
						return m_pollfd[(SOCKET)s].revents!=0;
					#endif
				}

			bool isSignaled(CASocket*ps)
				{
					#ifndef HAVE_POLL
						return FD_ISSET((SOCKET)*ps,&m_signaled_set)!=0;
					#else
						return m_pollfd[(SOCKET)*ps].revents!=0;
					#endif
				}

			bool isSignaled(CAMuxSocket&s)
				{
					#ifndef HAVE_POLL
						return FD_ISSET((SOCKET)s,&m_signaled_set)!=0;
					#else
						return m_pollfd[(SOCKET)s].revents!=0;
					#endif
				}

		private:
			#ifndef HAVE_POLL
				fd_set m_fdset;
				fd_set m_signaled_set;
				fd_set* m_set_read,*m_set_write;
				#ifndef _WIN32
					UINT32 m_max;
				#endif
			#else
				struct pollfd* m_pollfd;
				UINT32 m_max;
			#endif
			CAMutex m_csFD_SET;
	};
#endif
