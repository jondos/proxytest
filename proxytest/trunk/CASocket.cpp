#include "StdAfx.h"
#include "CASocketAddr.hpp"
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
	}
int CASocket::create()
	{
		if(m_Socket==0)
			m_Socket=socket(AF_INET,SOCK_STREAM,0);
		if(m_Socket==INVALID_SOCKET)
			return SOCKET_ERROR;
		return 0;
	}
int CASocket::listen(LPSOCKETADDR psa)
	{
		if(m_Socket==0&&create()==SOCKET_ERROR)
			return SOCKET_ERROR;
		if(::bind(m_Socket,(LPSOCKADDR)psa,sizeof(*psa))==SOCKET_ERROR)
		    return SOCKET_ERROR;
		return ::listen(m_Socket,SOMAXCONN);
	}
			
int CASocket::listen(unsigned short port)
	{
		CASocketAddr oSocketAddr(port);
		return listen(&oSocketAddr);
	}

int CASocket::accept(CASocket &s)
	{
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
			
int CASocket::connect(LPSOCKETADDR psa)
	{
		if(m_Socket==0&&create()==SOCKET_ERROR)
			{
				return SOCKET_ERROR;
			}
#ifdef _DEBUG
		sockets++;
#endif
		if(::connect(m_Socket,(LPSOCKADDR)psa,sizeof(*psa))!=0)
		    return SOCKET_ERROR;
		else
		    return 0;	    
	}
			
int CASocket::close()
	{
//		EnterCriticalSection(&csClose);
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
				ret=0;
			}
		else
			ret=SOCKET_ERROR;
//		LeaveCriticalSection(&csClose);
		return ret;
	}

int CASocket::close(int mode)
	{
		EnterCriticalSection(&csClose);
		::shutdown(m_Socket,mode);
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

int CASocket::available()
	{
		unsigned long ul;
		if(ioctlsocket(m_Socket,FIONREAD,&ul)==SOCKET_ERROR)
			return SOCKET_ERROR;
		else
			return ul;
	}

int CASocket::receive(char* buff,int len)
	{
		return ::recv(m_Socket,buff,len,0);
	}

int CASocket::getLocalPort()
	{
		struct sockaddr_in addr;
		socklen_t namelen=sizeof(struct sockaddr_in);
		if(getsockname(m_Socket,(struct sockaddr*)&addr,&namelen)==SOCKET_ERROR)
			return SOCKET_ERROR;
		else
			return ntohs(addr.sin_port);

	}

int CASocket::setReuseAddr(bool b)
	{
		int val=0;
		if(b) val=1;
		return setsockopt(m_Socket,SOL_SOCKET,SO_REUSEADDR,(char*)&val,sizeof(val));
	}

int CASocket::setRecvLowWat(int r)
	{
		int val=r;
		return setsockopt(m_Socket,SOL_SOCKET,SO_RCVLOWAT,(char*)&val,sizeof(val));
	}

int CASocket::setRecvBuff(int r)
	{
		int val=r;
		return setsockopt(m_Socket,SOL_SOCKET,SO_RCVBUF,(char*)&val,sizeof(val));	
	}

int CASocket::setSendBuff(int r)
	{
		int val=r;
		return setsockopt(m_Socket,SOL_SOCKET,SO_SNDBUF,(char*)&val,sizeof(val));	
	}

