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
#include "StdAfx.h"
#ifdef HAVE_POLL
#include "CASocket.hpp"
#include "CASingleSocketGroup.hpp"
//#ifdef _DEBUG
	#include "CAMsg.hpp"
//#endif

			
SINT32 CASingleSocketGroup::add(CASocket&s)
	{
		m_pollfd->fd=(SOCKET)s;
		return E_SUCCESS;
	}

SINT32 CASingleGroup::add(CAMuxSocket&s)
	{
		m_pollfd->fd=(SOCKET)s;
		return E_SUCCESS;
	}

SINT32 CASocketGroup::select()
	{
		m_poolfd->events=POLLIN;
    return ::poll(m_poolfd,1,INFTIM);

	}

SINT32 CASocketGroup::select(bool bWrite,UINT32 ms)
	{
		if(bWrite)
			m_poolfd->events=POOLOUT
		else
			m_poolfd->events=POOLIN
		int ret=::pool(m_pollfd,1,ms);
		if(ret==0)
			{
				return E_TIMEDOUT;
			}
		if(ret==SOCKET_ERROR)
			{
				ret=WSAGetLastError();
				#ifdef _DEBUG
					CAMsg::printMsg(LOG_DEBUG,"SocketGroup Select-Fehler: %i\n",WSAGetLastError());
				#endif
				return E_UNKNOWN;
			}
		return ret;
	}
			
#endif