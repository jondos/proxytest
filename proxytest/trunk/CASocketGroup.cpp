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
#ifdef _DEBUG
	#include "CAMsg.hpp"
#endif

CASocketGroup::CASocketGroup()
	{
		FD_ZERO(&m_fdset);
		#ifndef _WIN32
		    max=0;
		#endif
	}
			
int CASocketGroup::add(CASocket&s)
	{
		#ifndef _WIN32
		    if(max<((SOCKET)s)+1)
			max=((SOCKET)s)+1;
		#endif
		FD_SET((SOCKET)s,&m_fdset);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CASocketGroutp: Added SOCKET: %u\n",(SOCKET)s);
		#endif
		return 0;
	}

int CASocketGroup::add(CAMuxSocket&s)
	{
		#ifndef _WIN32
		    if(max<((SOCKET)s)+1)
			max=((SOCKET)s)+1;
		#endif
		#ifdef _DEBUG
				CAMsg::printMsg(LOG_DEBUG,"CASocketGroutp: Added SOCKET: %u\n",(SOCKET)s);
		#endif
		FD_SET((SOCKET)s,&m_fdset);
		return 0;
	}

int CASocketGroup::remove(CASocket&s)
	{
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CASocketGroutp: Removed SOCKET: %u\n",(SOCKET)s);
		#endif
		FD_CLR((SOCKET)s,&m_fdset);
		return 0;
	}

int CASocketGroup::remove(CAMuxSocket&s)
	{
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CASocketGroutp: Removed SOCKET: %u\n",(SOCKET)s);
		#endif
		FD_CLR((SOCKET)s,&m_fdset);
		return 0;
	}

int CASocketGroup::select()
	{
		memcpy(&m_signaled_set,&m_fdset,sizeof(fd_set));
		#ifdef _DEBUG
			#ifdef _WIN32
			    int ret=::select(0,&m_signaled_set,NULL,NULL,NULL);
			#else
			    int ret=::select(max,&m_signaled_set,NULL,NULL,NULL);
			#endif			    
			if(ret==SOCKET_ERROR)
				{
					CAMsg::printMsg(LOG_DEBUG,"SocketGroup Select-Fehler: %i\n",WSAGetLastError());
				}
			return ret;
		#else
			#ifdef _WIN32
			    return ::select(0,&m_signaled_set,NULL,NULL,NULL);
			#else
			    return ::select(max,&m_signaled_set,NULL,NULL,NULL);
			#endif			    
		#endif

	}

SINT32 CASocketGroup::select(bool bWrite,UINT32 ms)
	{
		memcpy(&m_signaled_set,&m_fdset,sizeof(fd_set));
		fd_set* set_read,*set_write;
		timeval ti;
		ti.tv_sec=0;
		ti.tv_usec=ms*1000;
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
		SINT32 ret;
		#ifdef _WIN32
				if(m_signaled_set.fd_count==0)
					{
						Sleep(ms);
						ret=0;
					}
				else
					ret=::select(0,set_read,set_write,NULL,&ti);
		#else
			  ret=::select(max,set_read,set_write,NULL,&ti);
		#endif
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
			
bool CASocketGroup::isSignaled(CASocket&s)
	{
		return FD_ISSET((SOCKET)s,&m_signaled_set)!=0;
	}

bool CASocketGroup::isSignaled(CASocket*ps)
	{
		return FD_ISSET((SOCKET)*ps,&m_signaled_set)!=0;
	}

bool CASocketGroup::isSignaled(CAMuxSocket&s)
	{
		return FD_ISSET((SOCKET)s,&m_signaled_set)!=0;
	}
