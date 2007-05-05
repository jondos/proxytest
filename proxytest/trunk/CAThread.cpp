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
#ifndef ONLY_LOCAL_PROXY
#include "CAThread.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

pthread_once_t CAThread::ms_threadKeyInit = PTHREAD_ONCE_INIT;
pthread_key_t CAThread::ms_threadKey; 

const char* CAThread::METHOD_BEGIN = "Begin of method";
const char* CAThread::METHOD_END = "End of method";


CAThread::CAThread()
	{
		m_fncMainLoop=NULL;
		m_pThread=NULL;
		m_strName=NULL;
	}

CAThread::CAThread(const UINT8* strName)
	{
		m_fncMainLoop=NULL;
		m_pThread=NULL;
		m_strName=NULL;
		if(strName!=NULL)
			{
				UINT32 len=strlen((char*)strName);
				m_strName=new UINT8[len+1];
				memcpy(m_strName,strName,len);
				m_strName[len]=0;
			}
	}

void CAThread::destroyValue(void* a_value) 
{ 
	if (a_value)
	{
		delete a_value; 
	}
}

void CAThread::initKey() 
{ 
	pthread_key_create(&ms_threadKey, destroyValue); 
}


void CAThread::setCurrentStack(METHOD_STACK* a_value)
{
	pthread_once(&ms_threadKeyInit, initKey); 
	void *value = pthread_getspecific(ms_threadKey); 
	if (value != NULL) 
	{
		 delete value;
	}
	value = a_value; 
	pthread_setspecific(ms_threadKey, value); 
}

CAThread::METHOD_STACK* CAThread::getCurrentStack()
{
	pthread_once(&ms_threadKeyInit, initKey); 
	return (METHOD_STACK*)pthread_getspecific(ms_threadKey); 
}




SINT32 CAThread::start(void* param,bool bDaemon,bool bSilent)
	{
		if(m_fncMainLoop==NULL)
			return E_UNKNOWN;
		m_pThread=new pthread_t;
		#ifdef DEBUG
			if(!bSilent)
				CAMsg::printMsg(LOG_DEBUG, "CAThread::start() - creating thread\n");
		#endif

		if(pthread_create(m_pThread,NULL,m_fncMainLoop,param)!=0)
			{
				if(!bSilent)
					CAMsg::printMsg(LOG_ERR, "CAThread::start() - creating new thread failed!\n");
				delete m_pThread;
				m_pThread=NULL;
				return E_UNKNOWN;
			}
		#ifdef DEBUG
			if(!bSilent)
				CAMsg::printMsg(LOG_DEBUG, "CAThread::start() - thread created sucessful\n");
		#endif
		if(m_strName!=NULL&&!bSilent)
			{
				UINT8* temp=bytes2hex(m_pThread,sizeof(pthread_t));
				CAMsg::printMsg(LOG_DEBUG,"Thread with name: %s created - pthread_t: %s\n",m_strName,temp);
				delete temp;
			}
		if(bDaemon)
			pthread_detach(*m_pThread);
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
