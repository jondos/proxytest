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
#include "CASocketAddrINet.hpp"
#include "CASingleSocketGroup.hpp"
#ifdef _DEBUG
	extern int sockets;
#endif
#include "CAMsg.hpp"
#include "CAUtil.hpp"
#define CLOSE_SEND		0x01
#define CLOSE_RECEIVE 0x02
#define CLOSE_BOTH		0x03

CASocket::CASocket()
	{
		m_Socket=0;
		m_closeMode=0;
		m_bSocketIsClosed=true;
	}

SINT32 CASocket::create()
	{
		return create(AF_INET);
	}

SINT32 CASocket::create(int type)
	{
		if(m_bSocketIsClosed)
			m_Socket=socket(type,SOCK_STREAM,0);
		else
			return E_UNKNOWN;
		if(m_Socket==INVALID_SOCKET)
			{
				int er=GET_NET_ERROR;
				if(er==EMFILE)
					CAMsg::printMsg(LOG_CRIT,"Couldt not create a new Socket!\n");
				else
					CAMsg::printMsg(LOG_CRIT,"Couldt not create a new Socket! - Error: %i\n",er);
				return SOCKET_ERROR;
			}
		m_bSocketIsClosed=false;
		return E_SUCCESS;
	}

/** Starts listening on address psa.
	* @retval E_SUCCESS, if successful
	* @retval E_SOCKET_LISTEN, if call to listen() returns an error
	* @retval E_SOCKET_BIND,	 if call to bind() returns an error
	* @retval E_UNKNOWN, otherwise
	*/
SINT32 CASocket::listen(const CASocketAddr& psa)
	{
		SINT32 type=psa.getType();
		if(m_bSocketIsClosed&&create(type)!=E_SUCCESS)
			return E_UNKNOWN;
#ifdef HAVE_UNIX_DOMAIN
		//we have to delete the file before...
		if(psa.getType()==AF_LOCAL)
			{
				UINT8* path=((CASocketAddrUnix&)psa).getPath();
				if(path!=NULL)
					{
						SINT32 ret=::unlink((char*)path);
						#ifdef _DEBUG
							if(ret!=0)
								CAMSg::printMsg(LOG_ERR,"CASocket::listen() -- could not unlink unix domain socket file name -- a call to bind or listen may fail...");
						#endif
						delete path;
					}
			}
#endif			
		if(::bind(m_Socket,psa.LPSOCKADDR(),psa.getSize())==SOCKET_ERROR)
			{
				close();
				return E_SOCKET_BIND;
			}
		if(::listen(m_Socket,SOMAXCONN)==SOCKET_ERROR)
			{
				close();
				return E_SOCKET_LISTEN;
			}
		return E_SUCCESS;
	}
			
SINT32 CASocket::listen(UINT16 port)
	{
		CASocketAddrINet oSocketAddrINet(port);
		return listen(oSocketAddrINet);
	}

SINT32 CASocket::accept(CASocket &s)
	{
		if(m_bSocketIsClosed) //the accept socket should not be closed!!
			return E_SOCKETCLOSED;
		if(!s.m_bSocketIsClosed) //but the new socket should be closed!!!
			return E_UNKNOWN;
		s.m_Socket=::accept(m_Socket,NULL,NULL);
		if(s.m_Socket==SOCKET_ERROR)
			{
				s.m_Socket=0;
				if(GET_NET_ERROR==ERR_INTERN_SOCKET_CLOSED)
					return E_SOCKETCLOSED;
				return E_UNKNOWN;
			}

#ifdef _DEBUG
		sockets++;
#endif
		s.m_bSocketIsClosed=false;
		return E_SUCCESS;
	}


/** Tries to connect to the peer described by psa*/			
SINT32 CASocket::connect(CASocketAddr & psa)
	{
		return connect(psa,1,0);
	}

/** Tries to connect to the peer described by psa.
	* @param psa - peer
	* @param retry - number of retries
	* @param time - time between retries in seconds
	*/			
SINT32 CASocket::connect(CASocketAddr & psa,UINT32 retry,UINT32 time)
	{
//		CAMsg::printMsg(LOG_DEBUG,"Socket:connect\n");
		if(m_bSocketIsClosed&&create()!=E_SUCCESS)
			{
				return E_UNKNOWN;
			}
#ifdef _DEBUG
		sockets++;
#endif
		int err=0;
		const SOCKADDR* addr=psa.LPSOCKADDR();
		int addr_len=psa.getSize();
		for(UINT32 i=0;i<retry;i++)
			{
//				CAMsg::printMsg(LOG_DEBUG,"Socket:connect-connect\n");
				err=::connect(m_Socket,addr,addr_len);
//				CAMsg::printMsg(LOG_DEBUG,"Socket:connect-connect-finished err: %i\n",err);
				if(err!=0)
					{  
						err=GET_NET_ERROR;
						#ifdef _DEBUG
						 CAMsg::printMsg(LOG_DEBUG,"Con-Error: %i\n",err);
						#endif
						if(err!=ERR_INTERN_TIMEDOUT&&err!=ERR_INTERN_CONNREFUSED)
							return E_UNKNOWN; //Should be better.....
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"Cannot connect... retrying\n");
						#endif						
						sSleep(time);
					}
				else
						return E_SUCCESS;
			}
		return err;
	}
			

/** Tries to connect to peeer psa.
	*
	*	@param psa - peer
	* @param msTimeOut - abort after msTimeOut MilliSeconds
	*/
SINT32 CASocket::connect(CASocketAddr & psa,UINT32 msTimeOut)
	{
		if(m_bSocketIsClosed&&create(psa.getType())!=E_SUCCESS)
			{
				return E_UNKNOWN;
			}
#ifdef _DEBUG
		sockets++;
#endif
		bool bWasNonBlocking=false;
		getNonBlocking(&bWasNonBlocking);
		setNonBlocking(true);
		int err=0;
		const LPSOCKADDR addr=(const LPSOCKADDR)psa.LPSOCKADDR();
		int addr_len=psa.getSize();
		
		err=::connect(m_Socket,addr,addr_len);
		if(err==0)
			return E_SUCCESS;
		err=GET_NET_ERROR;
#ifdef _WIN32
		if(err!=WSAEWOULDBLOCK)
			return E_UNKNOWN;
#else
		if(err!=EINPROGRESS)
			return E_UNKNOWN;
#endif

#ifndef HAVE_POLL
		struct timeval tval;
		tval.tv_sec=msTimeOut/1000;
		tval.tv_usec=(msTimeOut%1000)*1000;
		fd_set readSet,writeSet;
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_SET(m_Socket,&readSet);
		FD_SET(m_Socket,&writeSet);
		err=::select(m_Socket+1,&readSet,&writeSet,NULL,&tval);
#else
		struct pollfd opollfd;
		opollfd.fd=m_Socket;
		opollfd.events=POLLIN|POLLOUT;
		err=::poll(&opollfd,1,msTimeOut);
#endif		
		if(err!=1) //timeout or error
			{
				close();
				return E_UNKNOWN;
			}
		socklen_t len=sizeof(err);
		err=0;
		if(::getsockopt(m_Socket,SOL_SOCKET,SO_ERROR,(char*)&err,&len)<0||err!=0) //error by connect
			{
				close();
				return E_UNKNOWN;
			}
		setNonBlocking(bWasNonBlocking);
		return E_SUCCESS;	
	}
			
SINT32 CASocket::close()
	{
		m_csClose.lock();
		int ret;
		if(!m_bSocketIsClosed)
			{
#ifdef _DEBUG				
				if(::closesocket(m_Socket)==SOCKET_ERROR)
					{
						CAMsg::printMsg(LOG_DEBUG,"Fehler bei CASocket::closesocket\n -- %i",GET_NET_ERROR);
					}
				sockets--;
#else
				::closesocket(m_Socket);
#endif
				m_bSocketIsClosed=true;
				m_closeMode=0;
				ret=E_SUCCESS;
			}
		else
			ret=E_UNKNOWN;
		m_csClose.unlock();
		return ret;
	}

SINT32 CASocket::close(UINT32 mode)
	{
//		EnterCriticalSection(&csClose);
		::shutdown(m_Socket,mode);
		if(mode==SD_RECEIVE||mode==SD_BOTH)
			m_closeMode|=CLOSE_RECEIVE;
		if(mode==SD_SEND||mode==SD_BOTH)
			{
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
			
/** Sends some data over the network. This may block, if socket is in blocking mode.
	@param buff the buffer of data to send
	@param len content length
	@retval E_AGAIN if non blocking socket would block or a timeout was reached
	@retval E_UNKNOWN if an error occured
	@return number of bytes send
*/
SINT32 CASocket::send(const UINT8* buff,UINT32 len)
	{
	  if(len==0)
			return 0; //nothing to send
		int ret;	
		SINT32 ef=0;
		do
			{
				ret=::send(m_Socket,(char*)buff,len,MSG_NOSIGNAL);
				#ifdef _DEBUG
					if(ret==SOCKET_ERROR)
						CAMsg::printMsg(LOG_DEBUG,"Fehler beim Socket-send: %i",GET_NET_ERROR);
				#endif
			}
		while(ret==SOCKET_ERROR&&(ef=GET_NET_ERROR)==EINTR);
		if(ret==SOCKET_ERROR)
			{
				if(ef==ERR_INTERN_WOULDBLOCK)
					return E_AGAIN;
				return E_UNKNOWN;
			}
	  return ret;	    	    
	}

/** Sends some data over the network. Using a Timeout if socket is in blocking mode. 
	Otherwise E_AGAIN may returned
	@param buff the buffer to send
	@param len content length
	@param msTimeOut Maximum MilliSeconds to wait
	@retval E_AGAIN if Operation would block on a non-blocking socket
	@retval E_TIMEDOUT if the timeout was reached
	@return number of bytes send, or -1 in case of an error
*/
SINT32 CASocket::sendTimeOut(const UINT8* buff,UINT32 len,UINT32 msTimeOut)
	{
	  int ret;
		SINT32 aktTimeOut=0;	
		bool bWasNonBlocking;
		getNonBlocking(&bWasNonBlocking);
		if(bWasNonBlocking) //we are in non-blocking mode
			{
				ret=send(buff,len);				
			}
		else
			{
				aktTimeOut=getSendTimeOut();
				if(aktTimeOut>0&&(UINT32)aktTimeOut==msTimeOut) //we already have the right timeout
					{
						ret=send(buff,len);
					}
				else if(aktTimeOut<0||setSendTimeOut(msTimeOut)!=E_SUCCESS)
					{//do it complicate but still to simple!!!! more work TODO
						setNonBlocking(true);
						ret=send(buff,len);
						if(ret==E_AGAIN)
							{
								msSleep(msTimeOut);
								ret=send(buff,len);
							}
						setNonBlocking(bWasNonBlocking);
					}
				else
					{
						ret=send(buff,len);
						setSendTimeOut(aktTimeOut);
					}
			}
		return ret;	    	    
	}

/** Sends all data over the network. This may block until all data is send.
	@param buff - the buffer of data to send
	@param len - content length
	@retval E_UNKNOWN, if an error occured
	@retval E_SUCCESS, if successful
*/
SINT32 CASocket::sendFully(const UINT8* buff,UINT32 len)
	{
	  if(len==0)
			return E_SUCCESS; //nothing to send
		SINT32 ret;
		for(;;)
			{
				ret=send(buff,len);
				if((UINT32)ret==len)
					return E_SUCCESS;
				else if(ret==E_AGAIN)
					{
						ret=CASingleSocketGroup::select_once(*this,true,1000);
						if(ret>=0||ret==E_TIMEDOUT)
							continue;
						return E_UNKNOWN;
					}
				else if(ret<0)
					{
						return E_UNKNOWN;
					}
				len-=ret;
				buff+=ret;
			}
	}

/** Will receive some bytes from the socket. May block or not depending on whatever this socket
	* was set to blocking or non-blocking mode.
	* Warning: If socket is in blocking mode and receive is called, receive will block until some
	* data is available, EVEN IF AN OTHER THREAD WILL CLOSE THIS SOCKET!
	* @param buff the buffer which get the received data
	* @param len size of buff
	*	@return SOCKET_ERROR if an error occured
	* @retval E_AGAIN, if socket was in non-blocking mode and receive would block or a timeout was reached
	* @retval 0 if socket was gracefully closed
	* @return the number of bytes received (always >0)
**/
SINT32 CASocket::receive(UINT8* buff,UINT32 len)
	{
		int ret;	
	  int ef=0;
	  do
			{
				ret=::recv(m_Socket,(char*)buff,len,MSG_NOSIGNAL);
			}
	  while(ret==SOCKET_ERROR&&(ef=GET_NET_ERROR)==EINTR);
		if(ret==SOCKET_ERROR)
			{
				if(ef==ERR_INTERN_WOULDBLOCK)
					return E_AGAIN;
			}
#ifdef _DEBUG
		if(ret==SOCKET_ERROR)
	      CAMsg::printMsg(LOG_DEBUG,"Receive error: %i\n",ef);
#endif
	  return ret;	    	    
	}

/**Receives all bytes. This blocks until all bytes are received or an error occured.
@return E_UNKNOWN, in case of an error
@return E_SUCCESS otherwise

//TODO:: bugy for non-blocking socket...
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
						if(ret==0||(ret!=E_AGAIN&&ret!=E_TIMEDOUT))
							return E_UNKNOWN;
					}
				pos+=ret;
				len-=ret;
			}
	  while(len>0);
	  return E_SUCCESS;	    	    
	}

/** Trys to receive all bytes. After the timout value has elpased, the error E_TIMEDOUT is returned
	* Would not work correctly on Windows....
	*	@param buff byte array, where the received bytes would be stored 
	*	@param len	on input holds the number of bytes which should be read,
	*	@param msTimeOut the timout in milli seconds
	* @retval E_TIMEDOUT if not all byts could be read
	* @retval E_UNKNOWN if an error occured
	* @retval E_SUCCESS if all bytes could be read
	*
	* TODO: Lots of work TODO!!!!!
	*/
SINT32 CASocket::receiveFully(UINT8* buff,UINT32 len,UINT16 msTimeOut)
	{
		SINT32 ret;
		UINT16 dt=msTimeOut/2+1;
		SINT32 timeoutLeft=msTimeOut;
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
				msSleep(dt);
				timeoutLeft-=dt;
			}
		while(timeoutLeft>0);
		return E_TIMEDOUT;
	}

SINT32 CASocket::getLocalPort()
	{
		struct sockaddr_in addr;
		socklen_t namelen=sizeof(struct sockaddr_in);
		if(getsockname(m_Socket,(struct sockaddr*)&addr,&namelen)==SOCKET_ERROR)
			return SOCKET_ERROR;
		return ntohs(addr.sin_port);
	}

SINT32 CASocket::getPeerIP(UINT8 ip[4])
	{
		struct sockaddr_in addr;
		socklen_t namelen=sizeof(struct sockaddr_in);
		if(getpeername(m_Socket,(struct sockaddr*)&addr,&namelen)==SOCKET_ERROR)
			return SOCKET_ERROR;
		memcpy(ip,&addr.sin_addr,4);
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
/** Returns < 0 on error, otherwise the new sendbuffersize (which may be less than r)*/
SINT32 CASocket::setSendBuff(SINT32 r)
	{
		if(r<0)
			return E_UNKNOWN;
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
			return val;
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
				return E_SUCCESS;
		#else
				return SOCKET_ERROR;
		#endif
	}
