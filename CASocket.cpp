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

#ifdef _DEBUG
	extern int sockets;
	#include "CAMsg.hpp"
#endif
#define CLOSE_SEND		0x01
#define CLOSE_RECEIVE 0x02
#define CLOSE_BOTH		0x03

CASocket::CASocket()
	{
		m_Socket=0;
		InitializeCriticalSection(&csClose);
		closeMode=0;
		localPort=-1;
	}

SINT32 CASocket::create()
	{
		if(m_Socket==0)
			m_Socket=socket(AF_INET,SOCK_STREAM,0);
		if(m_Socket==INVALID_SOCKET)
			return SOCKET_ERROR;
		localPort=-1;
		return E_SUCCESS;
	}

SINT32 CASocket::listen(LPCASOCKETADDR psa)
	{
		localPort=-1;
		if(m_Socket==0&&create()==SOCKET_ERROR)
			return SOCKET_ERROR;
		if(::bind(m_Socket,(LPSOCKADDR)psa,sizeof(*psa))==SOCKET_ERROR)
		    return SOCKET_ERROR;
		return ::listen(m_Socket,SOMAXCONN);
	}
			
SINT32 CASocket::listen(UINT16 port)
	{
		CASocketAddr oSocketAddr(port);
		return listen(&oSocketAddr);
	}

SINT32 CASocket::accept(CASocket &s)
	{
		s.localPort=-1;
		s.m_Socket=::accept(m_Socket,NULL,NULL);
		if(s.m_Socket==SOCKET_ERROR)
			{
				s.m_Socket=0;
				return SOCKET_ERROR;
			}
#ifdef _DEBUG
		sockets++;
#endif
		return 0;
	}
			
SINT32 CASocket::connect(LPCASOCKETADDR psa)
	{
		return connect(psa,1,0);
	}

SINT32 CASocket::connect(LPCASOCKETADDR psa,UINT retry,UINT32 time)
	{
		localPort=-1;
		if(m_Socket==0&&create()==SOCKET_ERROR)
			{
				return SOCKET_ERROR;
			}
#ifdef _DEBUG
		sockets++;
#endif
		int err=0;
		for(UINT i=0;i<retry;i++)
			{
				if(::connect(m_Socket,(LPSOCKADDR)psa,sizeof(*psa))!=0)
					{  
						err=GETERROR;
						#ifdef _DEBUG
						 CAMsg::printMsg(LOG_DEBUG,"Con-Error: %i\n",err);
						#endif
						if(err!=E_TIMEDOUT&&err!=E_CONNREFUSED)
							return SOCKET_ERROR;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"Cannot connect... retrying\n");
						#endif
						sleep(time);
					}
				else
						return E_SUCCESS;
			}
		return err;
	}
			
SINT32 CASocket::close()
	{
//		EnterCriticalSection(&csClose);
		localPort=-1;
		int ret;
		if(m_Socket!=0)
			{
	/*			LINGER linger;
				linger.l_onoff=1;
				linger.l_linger=0;
				if(::setsockopt(m_Socket,SOL_SOCKET,SO_LINGER,(char*)&linger,sizeof(linger))!=0)
					CAMsg::printMsg(LOG_DEBUG,"Fehler bei setsockopt - LINGER!\n");
	*/			::closesocket(m_Socket);
#ifdef _DEBUG
				sockets--;
#endif
				m_Socket=0;
				ret=E_SUCCESS;
			}
		else
			ret=SOCKET_ERROR;
//		LeaveCriticalSection(&csClose);
		return ret;
	}

SINT32 CASocket::close(int mode)
	{
//		EnterCriticalSection(&csClose);
		localPort=-1;
		::shutdown(m_Socket,mode);
		if(mode==SD_RECEIVE||mode==SD_BOTH)
			closeMode|=CLOSE_RECEIVE;
		if(mode==SD_SEND||mode==SD_BOTH)
			closeMode|=CLOSE_SEND;
		int ret;
		if(closeMode==CLOSE_BOTH)
			{
				close();
				ret=E_SUCCESS;
			}
		else
			ret=1;
//		LeaveCriticalSection(&csClose);
		return ret;
	}
			
int CASocket::send(UINT8* buff,UINT32 len)
	{
	  int ret;	
	  do
			{
		    ret=::send(m_Socket,(char*)buff,len,MSG_NOSIGNAL);
			 #ifdef _DEBUG
				if(ret==SOCKET_ERROR)
				 printf("Fehler beim Socket-send: %i",errno);
				#endif
			}
	  while(ret==SOCKET_ERROR&&errno==EINTR);
	  return ret;	    	    
	}

int CASocket::available()
	{
		unsigned long ul;
		if(ioctlsocket(m_Socket,FIONREAD,&ul)==SOCKET_ERROR)
			return SOCKET_ERROR;
		else
			return (int)ul;
	}

int CASocket::receive(UINT8* buff,UINT32 len)
	{
		int ret;	
	  do
			{
				ret=::recv(m_Socket,(char*)buff,len,MSG_NOSIGNAL);
			}
	  while(ret==SOCKET_ERROR&&errno==EINTR);
	  return ret;	    	    
	}

int CASocket::getLocalPort()
	{
		if(localPort==-1)
			{
				struct sockaddr_in addr;
				socklen_t namelen=sizeof(struct sockaddr_in);
				if(getsockname(m_Socket,(struct sockaddr*)&addr,&namelen)==SOCKET_ERROR)
					return SOCKET_ERROR;
				else
					localPort=ntohs(addr.sin_port);
			}
		return localPort;
	}

int CASocket::setReuseAddr(bool b)
	{
		int val=0;
		if(b) val=1;
		return setsockopt(m_Socket,SOL_SOCKET,SO_REUSEADDR,(char*)&val,sizeof(val));
	}

int CASocket::setRecvLowWat(UINT32 r)
	{
		int val=r;
		return setsockopt(m_Socket,SOL_SOCKET,SO_RCVLOWAT,(char*)&val,sizeof(val));
	}

int CASocket::setRecvBuff(UINT32 r)
	{
		int val=r;
		return setsockopt(m_Socket,SOL_SOCKET,SO_RCVBUF,(char*)&val,sizeof(val));	
	}

int CASocket::setSendBuff(UINT32 r)
	{
		int val=r;
		return setsockopt(m_Socket,SOL_SOCKET,SO_SNDBUF,(char*)&val,sizeof(val));	
	}

int CASocket::setKeepAlive(bool b)
	{
		int val=0;
		if(b) val=1;
		return setsockopt(m_Socket,SOL_SOCKET,SO_KEEPALIVE,(char*)&val,sizeof(val));	
	}
