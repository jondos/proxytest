#include "StdAfx.h"
#include "CASocket.hpp"
#include "CASocketGroup.hpp"
CASocketGroup::CASocketGroup()
	{
		FD_ZERO(&m_fdset);
	}
			
int CASocketGroup::add(CASocket&s)
	{
		FD_SET((SOCKET)s,&m_fdset);
		return 0;
	}

int CASocketGroup::select()
	{
		memcpy(&m_signaled_set,&m_fdset,sizeof(fd_set));
		return ::select(0,&m_signaled_set,NULL,NULL,NULL);
	}
			
bool CASocketGroup::isSignaled(CASocket&s)
	{
		return FD_ISSET((SOCKET)s,&m_signaled_set)!=0;
	}
