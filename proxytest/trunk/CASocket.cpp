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
#include "CASocketASyncSend.hpp"
#include "CASocketAddrINet.hpp"
#ifdef _DEBUG
	extern int sockets;
#endif
#include "CAMsg.hpp"
#define CLOSE_SEND		0x01
#define CLOSE_RECEIVE 0x02
#define CLOSE_BOTH		0x03

CASocketASyncSend* CASocket::m_pASyncSend=NULL;

CASocket::CASocket()
	{
		m_Socket=0;
		InitializeCriticalSection(&m_csClose);
		m_closeMode=0;
		m_localPort=-1;
		m_aktSendBuffer=-1;
		m_bASyncSend=false;
		memset(m_ipPeer,0,4);
	}

SINT32 CASocket::create()
	{
		return create(AF_INET);
	}

SINT32 CASocket::create(int type)
	{
		m_aktSendBuffer=-1;
		if(m_Socket==0)
			m_Socket=socket(type,SOCK_STREAM,0);
		if(m_Socket==INVALID_SOCKET)
			{
				int er=errno;
				if(er==EMFILE)
					CAMsg::printMsg(LOG_CRIT,"Couldt not create a new Socket!\n");
				else
					CAMsg::printMsg(LOG_CRIT,"Couldt not create a new Socket! - Error: %i\n",er);
				return SOCKET_ERROR;
			}
		m_localPort=-1;
		return E_SUCCESS;
	}

SINT32 CASocket::listen(CASocketAddr & psa)
	{
		m_localPort=-1;
		int type=psa.getType();
		if(m_Socket==0&&create(type)==SOCKET_ERROR)
			return SOCKET_ERROR;
		if(::bind(m_Socket,psa.LPSOCKADDR(),psa.getSize())==SOCKET_ERROR)
		    return SOCKET_ERROR;
		return ::listen(m_Socket,SOMAXCONN);
	}
			
SINT32 CASocket::listen(UINT16 port)
	{
		CASocketAddrINet oSocketAddrINet(port);
		return listen(oSocketAddrINet);
	}

SINT32 CASocket::accept(CASocket &s)
	{
		s.m_localPort=-1;
		struct sockaddr_in peer;
		socklen_t peersize=sizeof(peer);
		s.m_Socket=::accept(m_Socket,(struct sockaddr*)&peer,&peersize);
		if(s.m_Socket==SOCKET_ERROR)
			{
				s.m_Socket=0;
				return SOCKET_ERROR;
			}

#ifdef _DEBUG
		sockets++;
#endif
		memcpy(s.m_ipPeer,&peer.sin_addr,4);
		return 0;
	}

			
SINT32 CASocket::connect(CASocketAddr & psa)
	{
		return connect(psa,1,0);
	}

SINT32 CASocket::connect(CASocketAddr & psa,UINT retry,UINT32 time)
	{
//		CAMsg::printMsg(LOG_DEBUG,"Socket:connect\n");
		m_localPort=-1;
		if(m_Socket==0&&create()==SOCKET_ERROR)
			{
				return SOCKET_ERROR;
			}
#ifdef _DEBUG
		sockets++;
#endif
		int err=0;
		LPSOCKADDR addr=psa.LPSOCKADDR();
		int addr_len=psa.getSize();
		for(UINT i=0;i<retry;i++)
			{
//				CAMsg::printMsg(LOG_DEBUG,"Socket:connect-connect\n");
				err=::connect(m_Socket,addr,addr_len);
//				CAMsg::printMsg(LOG_DEBUG,"Socket:connect-connect-finished err: %i\n",err);
				if(err!=0)
					{  
						err=GETERROR;
						#ifdef _DEBUG
						 CAMsg::printMsg(LOG_DEBUG,"Con-Error: %i\n",err);
						#endif
						if(err!=ERR_INTERN_TIMEDOUT&&err!=ERR_INTERN_CONNREFUSED)
							return SOCKET_ERROR; //Should be better.....
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
			

SINT32 CASocket::connect(CASocketAddr & psa,UINT msTimeOut)
	{
		m_localPort=-1;
		if(m_Socket==0&&create(psa.getType())==SOCKET_ERROR)
			{
				return SOCKET_ERROR;
			}
#ifdef _DEBUG
		sockets++;
#endif
		bool bWasNonBlocking=false;
		getNonBlocking(&bWasNonBlocking);
		setNonBlocking(true);
		int err=0;
		LPSOCKADDR addr=psa.LPSOCKADDR();
		int addr_len=psa.getSize();
		
		err=::connect(m_Socket,addr,addr_len);
		if(err==0)
			return E_SUCCESS;
		err=GETERROR;
#ifdef _WIN32
		if(err!=WSAEWOULDBLOCK)
			return E_UNKNOWN;
#else
		if(err!=EINPROGRESS)
			return E_UNKNOWN;
#endif
		fd_set readSet,writeSet;
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_SET(m_Socket,&readSet);
		FD_SET(m_Socket,&writeSet);
		struct timeval tval;
		tval.tv_sec=msTimeOut/1000;
		tval.tv_usec=(msTimeOut%1000)*1000;
		err=::select(m_Socket+1,&readSet,&writeSet,NULL,&tval);
		if(err==0) //timeout
			{
				::close(m_Socket);
				m_Socket=-1;
				return E_UNKNOWN;
			}
		if(FD_ISSET(m_Socket,&readSet)||FD_ISSET(m_Socket,&writeSet))
			{
				socklen_t len=sizeof(err);
				err=0;
				if(::getsockopt(m_Socket,SOL_SOCKET,SO_ERROR,(char*)&err,&len)<0||err!=0)
					{
						::close(m_Socket);
						m_Socket=-1;
						return E_UNKNOWN;
					}
				setNonBlocking(bWasNonBlocking);
				return E_SUCCESS;
			}
		::close(m_Socket);
		m_Socket=-1;
		return E_UNKNOWN;
	
	}
			
SINT32 CASocket::close()
	{
//		EnterCriticalSection(&csClose);
		m_localPort=-1;
		int ret;
		if(m_Socket!=0)
			{
	/*			LINGER linger;
				linger.l_onoff=1;
				linger.l_linger=0;
				if(::setsockopt(m_Socket,SOL_SOCKET,SO_LINGER,(char*)&linger,sizeof(linger))!=0)
					CAMsg::printMsg(LOG_DEBUG,"Fehler bei setsockopt - LINGER!\n");
	*/			
				if(m_bASyncSend)
					m_pASyncSend->close(this);			
				if(::closesocket(m_Socket)==SOCKET_ERROR)
					{
						CAMsg::printMsg(LOG_DEBUG,"Fehler bei CASocket::closesocket\n -- %i",GETERROR);
					}
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
		m_localPort=-1;
		::shutdown(m_Socket,mode);
		if(mode==SD_RECEIVE||mode==SD_BOTH)
			m_closeMode|=CLOSE_RECEIVE;
		if(mode==SD_SEND||mode==SD_BOTH)
			{
				if(m_bASyncSend)
					m_pASyncSend->close(this);
				m_closeMode|=CLOSE_SEND;				
			}
		int ret;
		if(m_closeMode==CLOSE_BOTH)
			{
				close();
				ret=E_SUCCESS;
			}
		else
			ret=1;
//		LeaveCriticalSection(&csClose);
		return ret;
	}
			
/** Sends the buff over the network.
	@param buff - the buffer to send
	@param len - content length
	@param bDisableAsync - force to send this asynchronous (even if Async-Modus was set via setAsync)
					default: false
	@ret number of bytes send, or -1 in case of an error
*/
int CASocket::send(UINT8* buff,UINT32 len,bool bDisableAsync)
	{
	  int ret;	
	  if(!m_bASyncSend||bDisableAsync) //sync send...
			{
				do
					{
						ret=::send(m_Socket,(char*)buff,len,MSG_NOSIGNAL);
						#ifdef _DEBUG
							if(ret==SOCKET_ERROR)
								printf("Fehler beim Socket-send: %i",errno);
						#endif
					}
				while(ret==SOCKET_ERROR&&errno==EINTR);
			}
		else
			{
				ret=m_pASyncSend->send(this,buff,len);
			}
	  return ret;	    	    
	}

/** Sends the buff over the network. Using a Timeout
	@param buff - the buffer to send
	@param len - content length
	@param msTimeOut MilliSeconds to wait
	@ret number of bytes send, or -1 in case of an error
*/
int CASocket::sendTimeOut(UINT8* buff,UINT32 len,UINT32 msTimeOut)
	{
	  int ret;
		SINT32 aktTimeOut=0;	
		if((aktTimeOut=getSendTimeOut())==SOCKET_ERROR||setSendTimeOut(msTimeOut)!=E_SUCCESS)
			{//do it complicate but still to simple!!!! more work TODO
				bool bWasNonBlocking=false;
				getNonBlocking(&bWasNonBlocking);
				setNonBlocking(true);
				ret=send(buff,len,true);
				if(ret==SOCKET_ERROR)
					{
						msleep(msTimeOut);
						ret=send(buff,len,true);
					}
				setNonBlocking(bWasNonBlocking);
			}
		else
			{
				ret=send(buff,len,true);
				setSendTimeOut(aktTimeOut);
			}
		return ret;	    	    
	}

#ifdef HAVE_FIONREAD
SINT32 CASocket::available()
	{
		unsigned long ul;
		if(ioctlsocket(m_Socket,FIONREAD,&ul)==SOCKET_ERROR)
			return SOCKET_ERROR;
		else
			return (int)ul;
	}
#endif

SINT32 CASocket::getSendSpace()
	{
#ifdef HAVE_TIOCOUTQ
		UINT32 ul;
		SINT32 sl=m_aktSendBuffer;
		if(sl<=0)
			{
				sl=getSendBuff();
				if(sl<=0)
					return E_UNKNOWN;
			}
		if(ioctlsocket(m_Socket,TIOCOUTQ,&ul)==SOCKET_ERROR)
			return SOCKET_ERROR;
		else
			return sl-(SINT32)ul;
#else
		return E_UNKNOWN;
#endif
	}
/**
@return SOCKET_ERROR if an error occured
@return 0 if socket was gracefully closed
@return the number of bytes received (always >0)
**/
int CASocket::receive(UINT8* buff,UINT32 len)
	{
		int ret;	
	  int ef=0;
	  do
			{
				ret=::recv(m_Socket,(char*)buff,len,MSG_NOSIGNAL);
			}
	  while(ret==SOCKET_ERROR&&(ef=errno)==EINTR);
#ifdef _DEBUG
		if(ret==SOCKET_ERROR)
	      CAMsg::printMsg(LOG_DEBUG,"Receive error: %i\n",ef);
#endif
	  return ret;	    	    
	}

/**Receives all bytes
@return E_UNKOWN, in case of an error
@return E_SUCCESS otherwise
*/
SINT32 CASocket::receiveFully(UINT8* buff,UINT32 len)
	{
		SINT32 ret;
		UINT32 pos=0;
	  do
			{
				ret=receive(buff+pos,len);
				if(ret<=0)
					{
						CAMsg::printMsg(LOG_DEBUG,"ReceiveFully receive error ret=%i\n",ret);
				    return E_UNKNOWN;
					}
				pos+=ret;
				len-=ret;
			}
	  while(len>0);
	  return E_SUCCESS;	    	    
	}

/** Trys to receive all bytes. After the timout value has elpased, the error E_TIMEDOUT is returned
*Woudn't work correctly on Windows....
*/
SINT32 CASocket::receiveFully(UINT8* buff,UINT32 len,SINT32 timeout)
	{
		SINT32 ret;
		SINT32 dt=timeout/2;
		do
			{
				#ifdef HAVE_AVAILABLE
					ret=available();
				#else
					ret=::recv(m_Socket,(char*)buff,len,MSG_NOSIGNAL|MSG_PEEK|MSG_DONTWAIT);
				#endif		
				if(ret<=0) //This may be a bug (=0 ?)
					return SOCKET_ERROR;
				if((UINT32)ret>=len)
					{
						ret=receive(buff,len);
						if(ret<=0||(UINT32)ret!=len)
							{
								CAMsg::printMsg(LOG_DEBUG,"ReceiveFully receive error ret=%i\n",ret);
								return E_UNKNOWN;
							}
						return E_SUCCESS;	    	    
					}
				msleep(dt);
				timeout-=dt;
			}
		while(timeout>0);
		return E_TIMEDOUT;
	}

int CASocket::getLocalPort()
	{
		if(m_localPort==-1)
			{
				struct sockaddr_in addr;
				socklen_t namelen=sizeof(struct sockaddr_in);
				if(getsockname(m_Socket,(struct sockaddr*)&addr,&namelen)==SOCKET_ERROR)
					return SOCKET_ERROR;
				else
					m_localPort=ntohs(addr.sin_port);
			}
		return m_localPort;
	}

SINT32 CASocket::getPeerIP(UINT8 ip[4])
	{
		if(m_ipPeer[0]==0)
			return E_UNKNOWN;
		memcpy(ip,m_ipPeer,4);
		return E_SUCCESS;
	}

SINT32 CASocket::setReuseAddr(bool b)
	{
		int val=0;
		if(b) val=1;
		return setsockopt(m_Socket,SOL_SOCKET,SO_REUSEADDR,(char*)&val,sizeof(val));
	}

SINT32 CASocket::setRecvLowWat(UINT32 r)
	{
		int val=r;
		return setsockopt(m_Socket,SOL_SOCKET,SO_RCVLOWAT,(char*)&val,sizeof(val));
	}

SINT32 CASocket::setSendLowWat(UINT32 r)
	{
		int val=r;
		return setsockopt(m_Socket,SOL_SOCKET,SO_SNDLOWAT,(char*)&val,sizeof(val));
	}

SINT32 CASocket::getSendLowWat()
	{
		int val;
		socklen_t size=sizeof(val);
		if(getsockopt(m_Socket,SOL_SOCKET,SO_SNDLOWAT,(char*)&val,&size)==SOCKET_ERROR)
			return E_UNKNOWN;
		else
			return val;
	}

SINT32 CASocket::setRecvBuff(UINT32 r)
	{
		int val=r;
		return setsockopt(m_Socket,SOL_SOCKET,SO_RCVBUF,(char*)&val,sizeof(val));	
	}

SINT32 CASocket::getRecvBuff()
	{
		int val;
		socklen_t size=sizeof(val);
		if(getsockopt(m_Socket,SOL_SOCKET,SO_RCVBUF,(char*)&val,&size)==SOCKET_ERROR)
			return E_UNKNOWN;
		else
			return val;
	}
/** Returuns < 0 on error, otherwise the new sendbuffersize (which may be less than r)*/
SINT32 CASocket::setSendBuff(SINT32 r)
	{
		if(r<0)
			return E_UNKNOWN;
		m_aktSendBuffer=-1;
		SINT32 val=r;
		SINT32 ret=setsockopt(m_Socket,SOL_SOCKET,SO_SNDBUF,(char*)&val,sizeof(val));	
		if(ret!=0)
			return E_UNKNOWN;
		return getSendBuff();
	}

SINT32 CASocket::getSendBuff()
	{
		SINT32 val;
		socklen_t size=sizeof(val);
		if(getsockopt(m_Socket,SOL_SOCKET,SO_SNDBUF,(char*)&val,&size)==SOCKET_ERROR)
			return E_UNKNOWN;
		else
			{
				m_aktSendBuffer=val;
				return val;
			}
	}

SINT32 CASocket::setSendTimeOut(UINT32 msTimeOut)
	{
		timeval t;
		t.tv_sec=msTimeOut/1000;
		t.tv_usec=(msTimeOut%1000)*1000;
		return setsockopt(m_Socket,SOL_SOCKET,SO_SNDTIMEO,(char*)&t,sizeof(t));	
	}

SINT32 CASocket::getSendTimeOut()
	{
		timeval val;
		socklen_t size=sizeof(val);
		if(getsockopt(m_Socket,SOL_SOCKET,SO_SNDTIMEO,(char*)&val,&size)==SOCKET_ERROR)
			return E_UNKNOWN;
		else
			return val.tv_sec*1000+val.tv_usec/1000;
	}

/** Enables/disables the socket keep-alive option.
@param b true if option should be enabled, false otherwise
@return E_SUCCES if no error occured
@return E_UNKOWN otherwise
*/
SINT32 CASocket::setKeepAlive(bool b)
	{
		int val=0;
		if(b) val=1;
		if(setsockopt(m_Socket,SOL_SOCKET,SO_KEEPALIVE,(char*)&val,sizeof(val))==SOCKET_ERROR)
			return E_UNKNOWN;
		return E_SUCCESS;
	}

/** Enables the socket keep-alive option with a given ping time (in seconds).
@param sec the time intervall(in seconds) of a keep-alive message
@return E_SUCCES if no error occured
@return E_UNKOWN otherwise
*/
SINT32 CASocket::setKeepAlive(UINT32 sec)
	{
#ifdef HAVE_TCP_KEEPALIVE
		int val=sec;
		if(setKeepAlive(true)!=E_SUCCESS)
			return E_UNKNOWN;
		if(setsockopt(m_Socket,IPPROTO_TCP,TCP_KEEPALIVE,(char*)&val,sizeof(val))==SOCKET_ERROR)
			return E_UNKNOWN;
		return E_SUCCESS;
#else
		return E_UNKNOWN;
#endif
	}


SINT32 CASocket::setNonBlocking(bool b)
	{
		if(b)
			{
				#ifndef _WIN32
						int flags=fcntl(m_Socket,F_GETFL,0);
						fcntl(m_Socket,F_SETFL,flags|O_NONBLOCK);
				#else
						unsigned long flags=1;
						ioctlsocket(m_Socket,FIONBIO,&flags);
				#endif
			}
		else
			{
				#ifndef _WIN32
						int flags=fcntl(m_Socket,F_GETFL,0);
						fcntl(m_Socket,F_SETFL,flags&~O_NONBLOCK);
				#else
						unsigned long flags=0;
						ioctlsocket(m_Socket,FIONBIO,&flags);
				#endif
			}
		return E_SUCCESS;
	}

SINT32 CASocket::getNonBlocking(bool* b)
	{
		#ifndef _WIN32
				int flags=fcntl(m_Socket,F_GETFL,0);
				*b=((flags&O_NONBLOCK)==O_NONBLOCK);
		#else
				return SOCKET_ERROR;
		#endif
		return E_SUCCESS;
	}

SINT32 CASocket::setASyncSend(bool b,SINT32 estimatedSendSize,UINT32 lowwater,UINT32 SendQueueSoftLimit,CASocketASyncSendResume* pResume)
	{
		if(b)
			{
				if(estimatedSendSize!=-1)
					setSendLowWat(estimatedSendSize);
				if(m_pASyncSend==NULL)
					{
						m_pASyncSend=new CASocketASyncSend();
						m_pASyncSend->setResume(pResume);
						if(SendQueueSoftLimit!=0)
							m_pASyncSend->setSendQueueSoftLimit(SendQueueSoftLimit);
						if(lowwater!=0)
							m_pASyncSend->setSendQueueLowWater(lowwater);
						m_pASyncSend->start();
					}
			}
		m_bASyncSend=b;

		return E_SUCCESS;
	}
