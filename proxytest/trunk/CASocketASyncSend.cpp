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
#include "CASocketASyncSend.hpp"
#include "CAMsg.hpp"
#include "CAFirstMix.hpp"

THREAD_RETURN SocketASyncSendLoop(void* p)
	{
		CASocketASyncSend* pASyncSend=(CASocketASyncSend*)p;
#define BUFF_SIZE 0xFFFF
		UINT8* buff=new UINT8[BUFF_SIZE];
		SINT32 ret;
		while(pASyncSend->m_bRun)
			{
				if((ret=pASyncSend->m_oSocketGroup.select(true,50))==E_UNKNOWN||ret==E_TIMEDOUT) //bugy...
					{
						continue;
					}
				EnterCriticalSection(&pASyncSend->cs);
				if(!pASyncSend->m_bRun)
					{
						LeaveCriticalSection(&pASyncSend->cs);
						break;
					}
				_t_socket_list* akt=pASyncSend->m_Sockets;
				_t_socket_list* tmp;
				_t_socket_list* before=NULL;
				while(akt!=NULL&&ret>0)
					{
						if(pASyncSend->m_oSocketGroup.isSignaled(akt->pSocket))
							{
								ret--;
								UINT32 len=BUFF_SIZE;
								SINT32 sendSpace=akt->pSocket->getSendSpace();
								if(sendSpace>0)
									{
										CAMsg::printMsg(LOG_DEBUG,"Send space now: %u\n",sendSpace);
										len=min(sendSpace,len);
									}
								if(akt->pQueue->getNext(buff,&len)==E_SUCCESS)
									akt->pSocket->send(buff,len,true);
								if(akt->bwasOverFull&&akt->pQueue->getSize()<pASyncSend->m_SendQueueLowWater)
									{
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_INFO,"Resumeing...\n");
										#endif
										pASyncSend->pResume->resume(akt->pSocket);
										akt->bwasOverFull=false;
									}
								if(akt->pQueue->isEmpty())
									{
										pASyncSend->m_oSocketGroup.remove(*akt->pSocket);
										if(before!=NULL)
											before->next=akt->next;
										else
											pASyncSend->m_Sockets=akt->next;
										tmp=akt;
										akt=akt->next;
										delete tmp->pQueue;
										delete tmp;
									}
								else
									{
										before=akt;
										akt=akt->next;
									}
							}
						else
							{
								before=akt;
								akt=akt->next;
							}
					}
				LeaveCriticalSection(&pASyncSend->cs);
			}
		THREAD_RETURN_SUCCESS;
	}

//#define SENDQUEUEFULLSIZE 100

SINT32 CASocketASyncSend::send(CASocket* pSocket,UINT8* buff,UINT32 size)
	{
		if(pSocket==NULL||buff==NULL)
			return E_UNKNOWN;
		EnterCriticalSection(&cs);
		if(!m_bRun)
			{
				LeaveCriticalSection(&cs);
				return E_UNKNOWN;
			}
		SINT32 ret;
		if(m_Sockets==NULL)
			{
				m_Sockets=new _t_socket_list;
				if(m_Sockets==NULL)
					{
						LeaveCriticalSection(&cs);
						return E_UNKNOWN;
					}
				m_Sockets->pQueue=new CAQueue();
				if(	m_Sockets->pQueue==NULL||
						m_Sockets->pQueue->add(buff,size)<0)
					{
						delete m_Sockets->pQueue;
						delete m_Sockets;
						m_Sockets=NULL;
						LeaveCriticalSection(&cs);
						return E_UNKNOWN;
					}
				m_Sockets->next=NULL;
				m_Sockets->pSocket=pSocket;
				m_Sockets->bwasOverFull=false;
				m_oSocketGroup.add(*pSocket);
				ret=size;
			}
		else
			{
				_t_socket_list* akt=m_Sockets;
				while(akt!=NULL)
					{
						if(akt->pSocket==pSocket)
							{
								ret=akt->pQueue->add(buff,size);
								if(ret<0)
									ret =E_UNKNOWN;
								else if((UINT32)ret>m_SendQueueSoftLimit)
									{
										akt->bwasOverFull=true;
										ret=E_QUEUEFULL;
									}
								else 
									ret=size;
								LeaveCriticalSection(&cs);
								return ret;
							}
						akt=akt->next;
					}
				akt=new _t_socket_list;
				if(akt==NULL)
					{
						LeaveCriticalSection(&cs);
						return E_UNKNOWN;
					}
				akt->pQueue=new CAQueue();
				if(akt->pQueue==NULL||akt->pQueue->add(buff,size)<0)
					{
						delete akt->pQueue;
						delete akt;
						LeaveCriticalSection(&cs);
						return E_UNKNOWN;
					}
				akt->next=m_Sockets;
				m_Sockets=akt;
				akt->pSocket=pSocket;
				akt->bwasOverFull=false;
				m_oSocketGroup.add(*pSocket);
				ret=size;
			}
		LeaveCriticalSection(&cs);
		return ret;
	}

SINT32 CASocketASyncSend::close(CASocket* pSocket)
	{
		EnterCriticalSection(&cs);
		SINT32 ret;
		_t_socket_list* akt=m_Sockets;
		_t_socket_list* before=NULL;
		while(akt!=NULL)
			{
				if(akt->pSocket==pSocket)
					{
#ifdef _DEBUG						
						if(!akt->pQueue->isEmpty())
							CAMsg::printMsg(LOG_INFO,"Deleting non empty send queue!\n");
#endif
						delete akt->pQueue;
						if(before!=NULL)
							before->next=akt->next;
						else
							m_Sockets=akt->next;
						m_oSocketGroup.remove(*akt->pSocket);
						delete akt;
						LeaveCriticalSection(&cs);
						return E_SUCCESS;
					}
				before=akt;
				akt=akt->next;
			}
		ret=E_SUCCESS;
		LeaveCriticalSection(&cs);
		return ret;
	}

SINT32 CASocketASyncSend::start()
	{
		EnterCriticalSection(&cs);
		if(!m_bRun)
			{
				m_bRun=true;
				#ifdef _WIN32
					_beginthread(SocketASyncSendLoop,0,this);
				#else
					pthread_t othread;
					pthread_create(&othread,NULL,SocketASyncSendLoop,this);
				#endif
			}
		LeaveCriticalSection(&cs);
		return E_SUCCESS;
	}

SINT32 CASocketASyncSend::stop()
	{
		EnterCriticalSection(&cs);
		m_bRun=false;
		//Insert: WAIT FOR THREAD Termination...
		_t_socket_list* akt=m_Sockets;
		while(m_Sockets!=NULL)
			{
				if(!m_Sockets->pQueue->isEmpty())
					CAMsg::printMsg(LOG_INFO,"Deleting non empty send queue!\n");
					delete m_Sockets->pQueue;
				akt=m_Sockets;
				m_oSocketGroup.remove(*m_Sockets->pSocket);
				m_Sockets=m_Sockets->next;
				delete akt;
			}
		LeaveCriticalSection(&cs);
		return E_SUCCESS;
	}