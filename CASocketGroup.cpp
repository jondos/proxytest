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

#define MAX_POLLFD 8192
CASocketGroup::CASocketGroup()
	{
		#ifndef HAVE_POLL
			FD_ZERO(&m_fdset);
			FD_ZERO(&m_signaled_set);
			#ifndef _WIN32
				  m_max=0;
			#endif
		#else
			m_pollfd_read=new struct pollfd[MAX_POLLFD];
			memset(m_pollfd_read,0,sizeof(struct pollfd)*MAX_POLLFD);
			m_pollfd_write=new struct pollfd[MAX_POLLFD];
			memset(m_pollfd_write,0,sizeof(struct pollfd)*MAX_POLLFD);
			for(int i=0;i<MAX_POLLFD;i++)
				{
					m_pollfd_read[i].events=POLLIN;
					m_pollfd_write[i].events=POLLOUT;	
					m_pollfd_read[i].fd=-1;
					m_pollfd_write[i].fd=-1;	
				}
			m_max=0;
		#endif
	}
			
SINT32 CASocketGroup::add(CASocket&s)
	{
		m_csFD_SET.lock();
		#ifndef HAVE_POLL
			#ifndef _WIN32
					if(m_max<((SOCKET)s)+1)
				m_max=((SOCKET)s)+1;
			#endif
			FD_SET((SOCKET)s,&m_fdset);
//			#ifdef _DEBUG
//				CAMsg::printMsg(LOG_DEBUG,"CASocketGroutp: Added SOCKET: %u\n",(SOCKET)s);
//			#endif
		#else
			m_pollfd_read[(SOCKET)s].fd=(SOCKET)s;
			m_pollfd_write[(SOCKET)s].fd=(SOCKET)s;
			if(m_max<((SOCKET)s)+1)
				m_max=((SOCKET)s)+1;
		#endif
		m_csFD_SET.unlock();
		return E_SUCCESS;
	}

SINT32 CASocketGroup::add(CAMuxSocket&s)
	{
		m_csFD_SET.lock();
		#ifndef HAVE_POLL
			#ifndef _WIN32
					if(m_max<((SOCKET)s)+1)
				m_max=((SOCKET)s)+1;
			#endif
//			#ifdef _DEBUG
//					CAMsg::printMsg(LOG_DEBUG,"CASocketGroup: Added SOCKET: %u\n",(SOCKET)s);
//			#endif
			FD_SET((SOCKET)s,&m_fdset);
		#else
			m_pollfd_read[(SOCKET)s].fd=(SOCKET)s;
			m_pollfd_write[(SOCKET)s].fd=(SOCKET)s;			
			if(m_max<((SOCKET)s)+1)
				m_max=((SOCKET)s)+1;
		#endif
		m_csFD_SET.unlock();
		return E_SUCCESS;
	}

SINT32 CASocketGroup::remove(CASocket&s)
	{
		m_csFD_SET.lock();
		#ifndef HAVE_POLL
//			#ifdef _DEBUG
//				CAMsg::printMsg(LOG_DEBUG,"CASocketGroutp: Removed SOCKET: %u\n",(SOCKET)s);
//			#endif
			FD_CLR((SOCKET)s,&m_fdset);
		#else
			m_pollfd_read[(SOCKET)s].fd=-1;
			m_pollfd_write[(SOCKET)s].fd=-1;			
		#endif
		m_csFD_SET.unlock();
		return E_SUCCESS;
	}

SINT32 CASocketGroup::remove(CAMuxSocket&s)
	{
		m_csFD_SET.lock();
		#ifndef HAVE_POLL
//			#ifdef _DEBUG
//				CAMsg::printMsg(LOG_DEBUG,"CASocketGroutp: Removed SOCKET: %u\n",(SOCKET)s);
//			#endif
			FD_CLR((SOCKET)s,&m_fdset);
		#else
			m_pollfd_read[(SOCKET)s].fd=-1;
			m_pollfd_write[(SOCKET)s].fd=-1;			
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
						int ret=::select(0,&m_signaled_set,NULL,NULL,NULL);
				#else
						int ret=::select(m_max,&m_signaled_set,NULL,NULL,NULL);
				#endif			    
				if(ret==SOCKET_ERROR)
					{
						CAMsg::printMsg(LOG_DEBUG,"SocketGroup Select-Fehler: %i\n",GET_NET_ERROR);
					}
				return ret;
			#else
				#ifdef _WIN32
						return ::select(0,&m_signaled_set,NULL,NULL,NULL);
				#else
						return ::select(m_max,&m_signaled_set,NULL,NULL,NULL);
				#endif			    
			#endif
		#else
			m_bWriteQueried=false;
			return ::poll(m_pollfd_read,m_max,-1);
		#endif
	}

/** Waits for events on the sockets. If after ms milliseconds no event occurs, E_TIMEDOUT is returned
	* @param bWrite - if true, the functions wait for writeable sockets, otherwise for readable
	* @param time_ms - maximum milliseconds to wait
	* @retval E_TIMEDOUT, if other ms milliseconds no event occurs
	* @retval 0, if no socket was read/writeable
	* @retval E_UNKNOWN, if an error occured
	* @return number of read/writeable sockets
	*/

SINT32 CASocketGroup::select(bool bWrite,UINT32 time_ms)
	{
		SINT32 ret;
		#ifndef HAVE_POLL
			m_csFD_SET.lock();
			memcpy(&m_signaled_set,&m_fdset,sizeof(fd_set));
			m_csFD_SET.unlock();
			fd_set* set_read,*set_write;
			timeval ti;
			ti.tv_sec=0;
			ti.tv_usec=time_ms*1000;
			if(!bWrite)
				{
					set_read=&m_signaled_set;
					set_write=NULL;
						
				}
			else
				{
					set_read=NULL;
					set_write=&m_signaled_set;
				}
			#ifdef _WIN32
					if(m_signaled_set.fd_count==0)
						{
							Sleep(time_ms);
							ret=0;
						}
					else
						ret=::select(0,set_read,set_write,NULL,&ti);
			#else
					ret=::select(m_max,set_read,set_write,NULL,&ti);
			#endif
		#else
			if(bWrite)
				{
					ret=::poll(m_pollfd_write,m_max,time_ms);
					m_bWriteQueried=true;
				}
			else
				{
					ret=::poll(m_pollfd_read,m_max,time_ms);
					m_bWriteQueried=false;
				}
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
					return 0;
				return E_UNKNOWN;
			}
		return ret;
	}
			
