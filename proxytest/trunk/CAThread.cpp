#include "StdAfx.h"
#include "CAThread.hpp"

CAThread::CAThread()
	{
		m_fncMainLoop=NULL;
		m_pThread=NULL;
	}

SINT32 CAThread::start(void* param)
	{
		if(m_fncMainLoop==NULL)
			return E_UNKNOWN;
		m_pThread=new pthread_t;
		pthread_create(m_pThread,NULL,m_fncMainLoop,param);
		return E_SUCCESS;
	}
