#include "stdafx.h"
#include "CASocketAddr.hpp"
#include "CASocket.hpp"
extern int sockets;

#define CLOSE_SEND		0x01
#define CLOSE_RECEIVE 0x02
#define CLOSE_BOTH		0x03

CASocket::CASocket()
	{
		m_Socket=0;
		InitializeCriticalSection(&csClose);
		closeMode=0;
	}
	
int CASocket::listen(LPSOCKETADDR psa)
	{
		if(m_Socket==0)
			m_Socket=socket(AF_INET,SOCK_STREAM,0);
		::bind(m_Socket,(LPSOCKADDR)psa,sizeof(*psa));
		return ::listen(m_Socket,SOMAXCONN);
	}
			
int CASocket::accept(CASocket &s)
	{
		s.m_Socket=::accept(m_Socket,NULL,NULL);
		sockets++;
		return 0;
	}
			
int CASocket::connect(LPSOCKETADDR psa)
	{
		if(m_Socket==0)
			m_Socket=socket(AF_INET,SOCK_STREAM,0);
		sockets++;
		return ::connect(m_Socket,(LPSOCKADDR)psa,sizeof(*psa));
	}
			
int CASocket::close()
	{
		EnterCriticalSection(&csClose);
		int ret;
		if(m_Socket!=0)
			{
				::closesocket(m_Socket);
				sockets--;
				m_Socket=0;
				ret=0;
			}
		else
			ret=SOCKET_ERROR;
		LeaveCriticalSection(&csClose);
		return ret;
	}

int CASocket::close(int mode)
	{
		EnterCriticalSection(&csClose);
		shutdown(m_Socket,mode);
		if(mode==SD_RECEIVE||mode==SD_BOTH)
			closeMode|=CLOSE_RECEIVE;
		if(mode==SD_SEND||mode==SD_BOTH)
			closeMode|=CLOSE_SEND;
		int ret;
		if(closeMode==CLOSE_BOTH)
			{
				close();
				ret=0;
			}
		else
			ret=1;
		LeaveCriticalSection(&csClose);
		return ret;
	}
			
int CASocket::send(char* buff,int len)
	{
		return ::send(m_Socket,buff,len,0);
	}
			
int CASocket::receive(char* buff,int len)
	{
		return ::recv(m_Socket,buff,len,0);
	}