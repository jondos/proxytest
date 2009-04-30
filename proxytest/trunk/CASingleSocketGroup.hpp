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
#ifdef _DEBUG
	#include "CAMsg.hpp"
#endif
/** Not thread safe!*/
#if !defined(HAVE_POLL)&&!defined(HAVE_EPOLL)
	#include "CASocketGroup.hpp"
class CASingleSocketGroup:public CASocketGroup
	{
		public:
			CASingleSocketGroup(bool bWrite):CASocketGroup(bWrite)
			{
			}

			static SINT32 select_once(CASocket& s,bool bWrite,UINT32 time_ms)
				{
					fd_set fdset;
					FD_ZERO(&fdset);
					#pragma warning( push )
					#pragma warning( disable : 4127 ) //Disable: Bedingter Ausdruck ist konstant
					FD_SET(s.getSocket(),&fdset);
					#pragma warning( pop )

					SINT32 ret;
					timeval ti;
					ti.tv_sec=0;
					ti.tv_usec=time_ms*1000;
					if(bWrite)
						ret=::select(1,NULL,&fdset,NULL,&ti);
					else
						ret=::select(1,&fdset,NULL,NULL,&ti);
					if(ret>0)
						return ret;
					else if(ret==0)
						return E_TIMEDOUT;
					return E_UNKNOWN;
				}
	};

#else
#include "CAMuxSocket.hpp"
#ifdef HAVE_POLL
class CASingleSocketGroup
	{
		public:
			CASingleSocketGroup(bool bWrite)
				{
					m_pollfd=new struct pollfd;
					setPoolForWrite(bWrite);
				}
			~CASingleSocketGroup()
			{
				delete m_pollfd;
				m_pollfd = NULL;
			}
			
			SINT32 add(CASocket&s)
				{
					m_pollfd->fd=s.getSocket();
					return E_SUCCESS;
				}

			SINT32 add(CAMuxSocket&s)
				{
					m_pollfd->fd=s.getSocket();
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
			/** Waits for "events" on the socket. 
				* @param time_ms time in milli seconds to wait
				* @return E_TIMEDOUT if after time_ms milli seconds no event occured
				* @return E_UNKNOWN if an error occured
				* @return 1 if an event occured
				*/
			SINT32 select(UINT32 time_ms)
				{
					SINT32 ret=::poll(m_pollfd,1,time_ms);
					if(ret==1)
						return ret;
					else if(ret==0)
						{
							return E_TIMEDOUT;
						}
					#ifdef _DEBUG
						ret=GET_NET_ERROR;
						CAMsg::printMsg(LOG_DEBUG,"SocketGroup Select-Fehler: %i\n",ret);
					#endif
					return E_UNKNOWN;
				}

				static SINT32 select_once(CASocket& s,bool bWrite,UINT32 time_ms)
					{
						struct pollfd pollfd;
						if(bWrite)
							pollfd.events=POLLOUT;
						else
							pollfd.events=POLLIN;
						pollfd.fd=s.getSocket();
						SINT32 ret=::poll(&pollfd,1,time_ms);
						if(ret==1)
							return ret;
						else if(ret==0)
							return E_TIMEDOUT;
						return E_UNKNOWN;							
					}

		private:
			struct pollfd* m_pollfd;
	};
#elif defined(HAVE_EPOLL)
class CASingleSocketGroup
	{
		public:
			CASingleSocketGroup(bool bWrite)
				{
					m_hEPFD=epoll_create(1);
					setPoolForWrite(bWrite);
				}

			~CASingleSocketGroup()
				{
					close(hEPFD);
				}
			
			SINT32 add(CAMuxSocket&s)
				{
					if(epoll_ctl(m_hEPFD,EPOLL_CTL_ADD,s.getSocket(),&m_pollAdd)!=0)
						return E_UNKNOWN;
					return E_SUCCESS;
				}

			SINT32 add(CAMuxSocket&s)
				{
					if(epoll_ctl(m_hEPFD,EPOLL_CTL_ADD,s.getSocket(),&m_pollAdd)!=0)
						return E_UNKNOWN;
					return E_SUCCESS;
				}

			SINT32 setPoolForWrite(bool bWrite)
				{
					if(bWrite)
						m_pollAdd->events=POLLOUT|POLLERR|POLLHUP;
					else
						m_pollfd->events=POLLIN|POLLERR|POLLHUP;
					return E_SUCCESS;
				}

			SINT32 select()
				{
					return ::epoll_wait(m_hEPFD,&m_Events,1,-1);
				}
			
			SINT32 select(UINT32 time_ms)
				{
					SINT32 ret=::epoll_wait(m_hEPFD,&m_Events,1,time_ms);
					if(ret==1)
						return ret;
					else if(ret==0)
						{
							return E_TIMEDOUT;
						}
					#ifdef _DEBUG
						ret=GET_NET_ERROR;
						CAMsg::printMsg(LOG_DEBUG,"SocketGroup Select-Fehler: %i\n",ret);
					#endif
					return E_UNKNOWN;
				}

			static SINT32 select_once(CASocket& s,bool bWrite,UINT32 time_ms)
				{
					struct epool_events events;
					if(bWrite)
						events.events=POLLOUT|POLLERR|POLLHUP;
					else
						events.events=POLLIN|POLLERR|POLLHUP;
					epoll_ctl(m_hEPFD,EPOLL_CTL_ADD,s.getSocket(),&events);
					SINT32 ret=::epoll_wait(m_hEPFD,&events,1,time_ms);
					if(ret==1)
						return ret;
					else if(ret==0)
						{
							return E_TIMEDOUT;
						}
					return E_UNKNOWN;
				}

		private:
			struct epool_events m_epollAdd;
			struct epool_events m_Events;
			SINT32 m_hEPFD;
	};
#endif
#endif
#endif
