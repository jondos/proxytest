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
		return 0;
	}

int CASocketGroup::remove(CASocket&s)
	{
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
			    int ret=::select(0,&m_signaled_set,NULL,NULL,NULL);
			#else
			    int ret=::select(max,&m_signaled_set,NULL,NULL,NULL);
			#endif			    
		#endif
	}
			
bool CASocketGroup::isSignaled(CASocket&s)
	{
		return FD_ISSET((SOCKET)s,&m_signaled_set)!=0;
	}
