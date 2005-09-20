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
#include "CAThread.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

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

SINT32 CAThread::start(void* param,bool bDaemon)
	{
		if(m_fncMainLoop==NULL)
			return E_UNKNOWN;
		m_pThread=new pthread_t;
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAThread::start() - creating thread\n");
		#endif

		if(pthread_create(m_pThread,NULL,m_fncMainLoop,param)!=0)
			{
				CAMsg::printMsg(LOG_ERR, "CAThread::start() - creating new thread failed!\n");
				delete m_pThread;
				m_pThread=NULL;
				return E_UNKNOWN;
			}
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAThread::start() - thread created sucessful\n");
		#endif
		if(m_strName!=NULL)
			{
				UINT8* temp=bytes2hex(m_pThread,sizeof(pthread_t));
				CAMsg::printMsg(LOG_DEBUG,"Thread with name: %s created - pthread_t: %s\n",m_strName,temp);
				delete temp;
			}
		if(bDaemon)
			pthread_detach(*m_pThread);
		return E_SUCCESS;
	}

