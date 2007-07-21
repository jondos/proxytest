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
#include "CAMutex.hpp"
#include "CAMsg.hpp"

#if defined (DEBUG) && defined(HAVE_PTHREAD_MUTEXES)

CAMutex::CAMutex()
	{
		m_pMutex=new pthread_mutex_t;
		ASSERT(pthread_mutex_init(m_pMutex,NULL)==0,"Muxtex init failed!");
	}

CAMutex::~CAMutex()
	{
		ASSERT(pthread_mutex_destroy(m_pMutex)==0,"Mutex detroy failed!");
		delete m_pMutex;
	}
#else

CAMutex()
{
	#ifdef HAVE_PTHREAD_MUTEXES
		m_pMutex=new pthread_mutex_t;
		m_pMutexAttributes = new pthread_mutexattr_t;						
		pthread_mutexattr_init(m_pMutexAttributes);  
		//pthread_mutexattr_settype(m_pMutexAttributes, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutexattr_settype(m_pMutexAttributes, PTHREAD_MUTEX_ERRORCHECK);
		pthread_mutex_init(m_pMutex, m_pMutexAttributes);
		//pthread_mutex_init(m_pMutex, NULL);
	#else
		m_pMutex=new CASemaphore(1);
	#endif
}
#endif

SINT32 CAMutex::lock()
{
	#ifdef	HAVE_PTHREAD_MUTEXES
	
		SINT32 ret;
		/*
		ret = pthread_mutex_trylock(m_pMutex);
		if(ret == 0)
		{
			CAMsg::printMsg(LOG_CRIT, "CAMutex: locked!\n");
			return E_SUCCESS;
		}
		else
		{			
			CAMsg::printMsg(LOG_CRIT, "CAMutex: lock error=%d\n", ret);
			//pthread_mutex_unlock(m_pMutex);
		}*/
	
		ret = pthread_mutex_lock(m_pMutex);
		if(ret == 0)
		{
			return E_SUCCESS;
		}
		CAMsg::printMsg(LOG_CRIT, "CAMutex: lock error=%d\n", ret);
		return E_UNKNOWN;
	#else
		return m_pMutex->down();
	#endif
}

SINT32 CAMutex::unlock()
{
	#ifdef HAVE_PTHREAD_MUTEXES
		SINT32 ret;
		ret = pthread_mutex_unlock(m_pMutex);
		if(ret == 0)
		{
			return E_SUCCESS;
		}
		CAMsg::printMsg(LOG_CRIT, "CAMutex: unlock error=%d\n", ret);
		return E_UNKNOWN;
	#else
		return m_pMutex->up();
	#endif
}



