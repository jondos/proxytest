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
#include "CADatagramSocket.hpp"
#include "CASocketAddrINet.hpp"


#ifdef _DEBUG
	extern int sockets;
	#include "CAMsg.hpp"
#endif

CADatagramSocket::CADatagramSocket()
	{
		m_Socket=0;
//		localPort=-1;
	}

SINT32 CADatagramSocket::create()
	{
		return create(AF_INET);
	}

			
SINT32 CADatagramSocket::create(int type)
	{
		if(m_Socket==0)
			m_Socket=::socket(type,SOCK_DGRAM,0);
		if(m_Socket==INVALID_SOCKET)
			return SOCKET_ERROR;
//		localPort=-1;
		return E_SUCCESS;
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

			
SINT32 CADatagramSocket::bind(CASocketAddr & from)
	{
//		localPort=-1;
		if(m_Socket==0&&create(from.getType())!=E_SUCCESS)
			return SOCKET_ERROR;
		if(::bind(m_Socket,from.LPSOCKADDR(),from.getSize())==SOCKET_ERROR)
		    return SOCKET_ERROR;
		return E_SUCCESS;
	}

SINT32 CADatagramSocket::bind(UINT16 port)
	{
		CASocketAddrINet oSocketAddr(port);
		return bind(oSocketAddr);
	}

SINT32 CADatagramSocket::send(UINT8* buff,UINT32 len,CASocketAddr & to)
	{
    		if(::sendto(m_Socket,(char*)buff,len,MSG_NOSIGNAL,to.LPSOCKADDR(),to.getSize())==SOCKET_ERROR)
			return E_UNKNOWN;
		return E_SUCCESS;	    	    
	}


SINT32 CADatagramSocket::receive(UINT8* buff,UINT32 len,CASocketAddr* from)
	{
		int ret;	
		if(from!=NULL)
			{
				socklen_t fromlen=from->getSize();
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
