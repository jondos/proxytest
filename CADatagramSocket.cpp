#include "StdAfx.h"
#include "CASocketAddr.hpp"
#include "CADatagramSocket.hpp"



#ifdef _DEBUG
	extern int sockets;
	#include "CAMsg.hpp"
#endif

CADatagramSocket::CADatagramSocket()
	{
		m_Socket=0;
		InitializeCriticalSection(&csClose);
//		localPort=-1;
	}

SINT32 CADatagramSocket::create()
	{
		if(m_Socket==0)
			m_Socket=::socket(AF_INET,SOCK_DGRAM,0);
		if(m_Socket==INVALID_SOCKET)
			return SOCKET_ERROR;
//		localPort=-1;
		return 0;
	}

			
SINT32 CADatagramSocket::close()
	{
//		EnterCriticalSection(&csClose);
//		localPort=-1;
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

			
SINT32 CADatagramSocket::bind(LPCASOCKETADDR from)
	{
//		localPort=-1;
		if(m_Socket==0&&create()==SOCKET_ERROR)
			return SOCKET_ERROR;
		if(::bind(m_Socket,(LPSOCKADDR)from,sizeof(*from))==SOCKET_ERROR)
		    return SOCKET_ERROR;
		return 0;
	}

SINT32 CADatagramSocket::bind(UINT16 port)
	{
		CASocketAddr oSocketAddr(port);
		return bind(&oSocketAddr);
	}

SINT32 CADatagramSocket::send(UINT8* buff,UINT32 len,LPCASOCKETADDR to)
	{
    int ret=::sendto(m_Socket,(char*)buff,len,MSG_NOSIGNAL,(LPSOCKADDR)to,sizeof(*to));
			 #ifdef _DEBUG
				if(ret==SOCKET_ERROR)
				 printf("FEhler beim Socket-send: %i",errno);
				#endif
	  return ret;	    	    
	}


SINT32 CADatagramSocket::receive(UINT8* buff,UINT32 len,LPCASOCKETADDR from)
	{
		int ret;	
		if(from!=NULL)
			{
				socklen_t fromlen=sizeof(*from);
				ret=::recvfrom(m_Socket,(char*)buff,len,MSG_NOSIGNAL,(LPSOCKADDR)from,&fromlen);
			}
		else
			{
				ret=::recv(m_Socket,(char*)buff,len,MSG_NOSIGNAL);
			}
		return ret;	    	    
	}

/*
SINT32 CADatagramSocket::getLocalPort()
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

*/