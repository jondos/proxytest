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
#include "CASocket.hpp"
#include "CASocketGroup.hpp"
//#ifdef _DEBUG
	#include "CAMsg.hpp"
//#endif
//#ifdef HAVE_EPOLL
//#else
CASocketGroup::CASocketGroup(bool bWrite)
	{
		#ifndef HAVE_POLL
			FD_ZERO(&m_fdset);
			FD_ZERO(&m_signaled_set);
			#ifndef _WIN32
				  m_max=0;
			#endif
		#else
			m_pollfd=new struct pollfd[MAX_POLLFD];
			memset((void*)m_pollfd,0,sizeof(struct pollfd)*MAX_POLLFD);
			m_max=0;
		#endif
		setPoolForWrite(bWrite);
	}

inline SINT32 CASocketGroup::setPoolForWrite(bool bWrite)
	{
		#ifndef HAVE_POLL
		if(!bWrite)
				{
					m_set_read=&m_signaled_set;
					m_set_write=NULL;
						
				}
			else
				{
					m_set_read=NULL;
					m_set_write=&m_signaled_set;
				}	
		#else
			for(int i=0;i<MAX_POLLFD;i++)
				{
					if(bWrite)
						m_pollfd[i].events=POLLOUT;
					else
						m_pollfd[i].events=POLLIN;
					m_pollfd[i].fd=-1;
				}
		#endif
		return E_SUCCESS;
	}

SINT32 CASocketGroup::remove(CASocket&s)
	{
		m_csFD_SET.lock();
		#ifndef HAVE_POLL
			#pragma warning( push )
			#pragma warning( disable : 4127 ) //Disable: Bedingter Ausdruck ist konstant
			FD_CLR(s.getSocket(),&m_fdset);
			#pragma warning (pop)
		#else
			SINT sock=s.getSocket();
			m_pollfd[sock].fd=-1;			
			//CAMsg::printMsg(LOG_DEBUG,"CASocketGroup::remove() - socket: %d\n",sock);
		#endif
		m_csFD_SET.unlock();
		return E_SUCCESS;
	}

SINT32 CASocketGroup::remove(CAMuxSocket&s)
	{
		m_csFD_SET.lock();
		#ifndef HAVE_POLL
			#pragma warning( push )
			#pragma warning( disable : 4127 ) //Disable: Bedingter Ausdruck ist konstant
			FD_CLR(s.getSocket(),&m_fdset);
			#pragma warning (pop)
		#else
			SINT sock=s.getSocket();
			m_pollfd[sock].fd=-1;			
			//CAMsg::printMsg(LOG_DEBUG,"CASocketGroup::remove() - socket: %d\n",sock);
		#endif
		m_csFD_SET.unlock();
		return E_SUCCESS;
	}

SINT32 CASocketGroup::select()
	{
		#ifndef HAVE_POLL
			m_csFD_SET.lock();
			memcpy(&m_signaled_set,&m_fdset,sizeof(fd_set));
			m_csFD_SET.unlock();
			#ifdef _DEBUG
				#ifdef _WIN32
						int ret=::select(0,m_set_read,m_set_write,NULL,NULL);
				#else
						int ret=::select(m_max,m_set_read,m_set_write,NULL,NULL);
				#endif			    
				if(ret==SOCKET_ERROR)
					{
						CAMsg::printMsg(LOG_DEBUG,"SocketGroup Select-Fehler: %i\n",GET_NET_ERROR);
					}
				return ret;
			#else
				#ifdef _WIN32
						return ::select(0,m_set_read,m_set_write,NULL,NULL);
				#else
						return ::select(m_max,m_set_read,m_set_write,NULL,NULL);
				#endif			    
			#endif
		#else
				m_csFD_SET.lock();
				SINT32 ret=::poll((struct pollfd*)m_pollfd,m_max,-1);
				m_csFD_SET.unlock();
				return ret;
		#endif
	}

/** Waits for events on the sockets. If after ms milliseconds no event occurs, E_TIMEDOUT is returned
	* @param time_ms - maximum milliseconds to wait
	* @retval E_TIMEDOUT, if other ms milliseconds no event occurs
	* @retval E_UNKNOWN, if an error occured
	* @return number of read/writeable sockets
	*/

SINT32 CASocketGroup::select(UINT32 time_ms)
	{
		SINT32 ret;
		#ifndef HAVE_POLL
			m_csFD_SET.lock();
			memcpy(&m_signaled_set,&m_fdset,sizeof(fd_set));
			m_csFD_SET.unlock();
			timeval ti;
			ti.tv_sec=0;
			ti.tv_usec=time_ms*1000;
	
			#ifdef _WIN32
					if(m_signaled_set.fd_count==0)
						{
							Sleep(time_ms);
							ret=E_TIMEDOUT;
						}
					else
						ret=::select(0,m_set_read,m_set_write,NULL,&ti);
			#else
					ret=::select(m_max,m_set_read,m_set_write,NULL,&ti);
			#endif
		#else
			m_csFD_SET.lock();
			ret=::poll((struct pollfd*)m_pollfd,m_max,time_ms);
			m_csFD_SET.unlock();
		#endif
		if(ret==0)
			{
				return E_TIMEDOUT;
			}
		if(ret==SOCKET_ERROR)
			{
				ret=GET_NET_ERROR;
				#ifdef _DEBUG
					CAMsg::printMsg(LOG_DEBUG,"SocketGroup Select-Fehler: %i\n",ret);
				#endif
				if(ret==EINTR)
					return E_TIMEDOUT;
				return E_UNKNOWN;
			}
		return ret;
	}
//#endif
