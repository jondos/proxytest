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
#include "CASocketGroupEpoll.hpp"
#ifdef HAVE_EPOLL
CASocketGroupEpoll::CASocketGroupEpoll(bool bWrite)
	{
		m_pcsFD_SET = new CAMutex();
		m_hEPFD=epoll_create(MAX_POLLFD);
		m_pEpollEvent=new struct epoll_event;
		memset(m_pEpollEvent,0,sizeof(struct epoll_event));
		m_pEvents=new struct epoll_event[MAX_POLLFD];
		setPoolForWrite(bWrite);
	}

CASocketGroupEpoll::~CASocketGroupEpoll()
	{
		epoll_close(m_hEPFD);
		delete[] m_pEvents;
		m_pEvents = NULL;
		delete m_pEpollEvent;
		m_pEpollEvent = NULL;
		delete m_pcsFD_SET;
	}

SINT32 CASocketGroupEpoll::setPoolForWrite(bool bWrite)
	{
		if(bWrite)
			m_pEpollEvent->events=EPOLLOUT|EPOLLERR|EPOLLHUP;
		else
			m_pEpollEvent->events=EPOLLIN| EPOLLERR|EPOLLHUP;
		return E_SUCCESS;
	}
#endif
