#include "StdAfx.h"
#include "CASocketASyncSend.hpp"
#include "CAMsg.hpp"

THREAD_RETURN SocketASyncSendLoop(void* p)
	{
		CASocketASyncSend* pASyncSend=(CASocketASyncSend*)p;
#define BUFF_SIZE 0xFFFF
		UINT8* buff=new UINT8[BUFF_SIZE];
		SINT32 ret;
		while(true)
			{
				if((ret=pASyncSend->m_oSocketGroup.select(true,50))==E_UNKNOWN||ret==E_TIMEDOUT) //bugy...
					{
						continue;
					}
				EnterCriticalSection(&pASyncSend->cs);
				_t_socket_list* akt=pASyncSend->m_Sockets;
				_t_socket_list* tmp;
				_t_socket_list* before=NULL;
				while(akt!=NULL&&ret>0)
					{
						if(pASyncSend->m_oSocketGroup.isSignaled(akt->pSocket))
							{
								ret--;
								UINT32 len=BUFF_SIZE;
								if(akt->pQueue->getNext(buff,&len)==E_SUCCESS)
									::send((SOCKET)*(akt->pSocket),(char*)buff,len,0);
								if(akt->pQueue->isEmpty())
									{
										pASyncSend->m_oSocketGroup.remove(*akt->pSocket);
										if(before!=NULL)
											before->next=akt->next;
										else
											pASyncSend->m_Sockets=akt->next;
										tmp=akt;
										akt=akt->next;
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
	}

SINT32 CASocketASyncSend::send(CASocket* pSocket,UINT8* buff,UINT32 size)
	{
		EnterCriticalSection(&cs);
		SINT32 ret;
		if(m_Sockets==NULL)
			{
				m_Sockets=new _t_socket_list;
				m_Sockets->next=NULL;
				m_Sockets->pSocket=pSocket;
				m_Sockets->pQueue=new CAQueue();
				m_Sockets->pQueue->add(buff,size);
				m_oSocketGroup.add(*pSocket);
				ret=E_SUCCESS;
			}
		else
			{
				_t_socket_list* akt=m_Sockets;
				while(akt!=NULL)
					{
						if(akt->pSocket==pSocket)
							{
								akt->pQueue->add(buff,size);
								LeaveCriticalSection(&cs);
								return E_SUCCESS;
							}
						akt=akt->next;
					}
				akt=new _t_socket_list;
				akt->next=m_Sockets;
				m_Sockets=akt;
				akt->pSocket=pSocket;
				akt->pQueue=new CAQueue();
				akt->pQueue->add(buff,size);
				m_oSocketGroup.add(*pSocket);
				ret=E_SUCCESS;
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
						if(!akt->pQueue->isEmpty())
							CAMsg::printMsg(LOG_INFO,"Deleting non empty send queue!\n");
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
		#ifdef _WIN32
		 _beginthread(SocketASyncSendLoop,0,this);
		#else
		 pthread_t othread;
		 pthread_create(&othread,NULL,SocketASyncSendLoop,this);
		#endif
		 return E_SUCCESS;
	}