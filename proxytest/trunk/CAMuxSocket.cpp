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
#include "CASocketAddr.hpp"
#include "CAMuxSocket.hpp"
#ifdef _DEBUG
	#include "CAMsg.hpp"
#endif
#include "CASocketGroup.hpp"
//#include "httptunnel/common.h"
char buff[255];

CAMuxSocket::CAMuxSocket()
	{
		m_Buff=new UINT8[MIXPACKET_SIZE];
		m_aktBuffPos=0;
		bIsCrypted=false;
		InitializeCriticalSection(&csSend);
		InitializeCriticalSection(&csReceive);
	}

SINT32 CAMuxSocket::setCrypt(bool b)
	{
		EnterCriticalSection(&csSend);
		EnterCriticalSection(&csReceive);
		bIsCrypted=b;
		if(b)
			{
				UINT8 nullkey[16];
				memset(nullkey,0,16);
				ocipherIn.setKeyAES(nullkey);
				ocipherOut.setKeyAES(nullkey);
			}
		LeaveCriticalSection(&csReceive);
		LeaveCriticalSection(&csSend);
		return E_SUCCESS;
	}


#ifndef PROT2
int CAMuxSocket::accept(UINT16 port)
	{
//		if(!bIsTunneld)
//			{
				CASocket oSocket;
				oSocket.create();
				oSocket.setReuseAddr(true);
				if(oSocket.listen(port)==SOCKET_ERROR)
					return SOCKET_ERROR;
				if(oSocket.accept(m_Socket)==SOCKET_ERROR)
					return SOCKET_ERROR;
				oSocket.close();
				m_Socket.setRecvLowWat(sizeof(MUXPACKET));
/*			}
		else
			{
				m_pTunnel=tunnel_new_server (port,0);//DEFAULT_CONTENT_LENGTH//sizeof(MUXPACKET));
				if(m_pTunnel==NULL)
				    return SOCKET_ERROR;
				tunnel_accept(m_pTunnel);
			}
*/		return 0;
	}
#else
SINT32 CAMuxSocket::accept(UINT16 port)
	{
		CASocket oSocket;
		oSocket.create();
		oSocket.setReuseAddr(true);
		if(oSocket.listen(port)==SOCKET_ERROR)
			return SOCKET_ERROR;
		if(oSocket.accept(m_Socket)==SOCKET_ERROR)
			return SOCKET_ERROR;
		oSocket.close();
		m_Socket.setRecvLowWat(sizeof(MIXPACKET));
		return E_SUCCESS;
	}

SINT32 CAMuxSocket::accept(CASocketAddr& oAddr)
	{
		CASocket oSocket;
		oSocket.create(oAddr.getType());
		oSocket.setReuseAddr(true);
		if(oSocket.listen(oAddr)==SOCKET_ERROR)
			return SOCKET_ERROR;
		if(oSocket.accept(m_Socket)==SOCKET_ERROR)
			return SOCKET_ERROR;
		oSocket.close();
		m_Socket.setRecvLowWat(sizeof(MIXPACKET));
		return E_SUCCESS;
	}

#endif

SINT32 CAMuxSocket::connect(CASocketAddr & psa)
	{
		return connect(psa,1,0);
	}

SINT32 CAMuxSocket::connect(CASocketAddr & psa,UINT retry,UINT32 time)
	{
//		if(!bIsTunneld)
//			{
				m_Socket.setRecvLowWat(MIXPACKET_SIZE);
				return m_Socket.connect(psa,retry,time);
/*			}
		else
			{

				psa->getHostName((UINT8*)buff,255);
				m_pTunnel=tunnel_new_client (buff, psa->getPort(),m_szTunnelHost,m_uTunnelPort,
				 0//DEFAULT_CONTENT_LENGTH//sizeof(MUXPACKET));
				return tunnel_connect(m_pTunnel);
			}
*/	}
			
int CAMuxSocket::close()
	{
//		if(!bIsTunneld)
//			{
				return m_Socket.close();
/*			}
		else
			return tunnel_close(m_pTunnel);
*/	}

#ifndef PROT2
int CAMuxSocket::send(MIXPACKET *pPacket)
	{
		int MuxPacketSize=sizeof(MUXPACKET);
		int aktIndex=0;
		int len=0;
		pPacket->channel=htonl(pPacket->channel);
		pPacket->len=htons(pPacket->len);

//		if(!bIsTunneld)
//			{
				do
					{
						len=m_Socket.send(((UINT8*)pPacket)+aktIndex,MuxPacketSize);
						MuxPacketSize-=len;
						aktIndex+=len;
					} while(len>0&&MuxPacketSize>0);
/*			}
		else
			{		
				do
					{
						len=tunnel_write(m_pTunnel,(void*)"fghj",4);//((char*)pPacket)+aktIndex,MuxPacketSize);
						return 0;
//						MuxPacketSize-=len;
//						aktIndex+=len;
					} while(len>0&&MuxPacketSize>0);	
			}
*/
		if(len==SOCKET_ERROR)
			{
				#ifdef _DEBUG
					CAMsg::printMsg(LOG_DEBUG,"MuxSocket-Send-Error!\n");
					CAMsg::printMsg(LOG_DEBUG,"SOCKET-ERROR: %i\n",WSAGetLastError());
				#endif
				return SOCKET_ERROR;
			}
		return sizeof(MUXPACKET);
	}
#else

int CAMuxSocket::send(MIXPACKET *pPacket)
	{
		EnterCriticalSection(&csSend);
		int ret;
		UINT8 tmpBuff[16];
		memcpy(tmpBuff,pPacket,16);
		pPacket->channel=htonl(pPacket->channel);
		pPacket->flags=htons(pPacket->flags);
		if(bIsCrypted)
    	ocipherOut.encryptAES(((UINT8*)pPacket),((UINT8*)pPacket),16);
		ret=m_Socket.send(((UINT8*)pPacket),MIXPACKET_SIZE);
		if(ret==SOCKET_ERROR)
			{
				#ifdef _DEBUG
					CAMsg::printMsg(LOG_DEBUG,"MuxSocket-Send-Error!\n");
					CAMsg::printMsg(LOG_DEBUG,"SOCKET-ERROR: %i\n",WSAGetLastError());
				#endif
				ret=SOCKET_ERROR;
			}
		else if(ret==E_QUEUEFULL)
			ret=E_QUEUEFULL;
		else
			ret=MIXPACKET_SIZE;
		memcpy(pPacket,tmpBuff,16);
		LeaveCriticalSection(&csSend);
		return ret;
	}
#endif

#ifndef PROT2
int CAMuxSocket::receive(MIXPACKET* pPacket)
	{
		int MuxPacketSize=sizeof(MUXPACKET);
		int aktIndex=0;
		int len=0;
//		if(!bIsTunneld)
//			{
				do
					{
						len=m_Socket.receive(((UINT8*)pPacket)+aktIndex,MuxPacketSize);
						MuxPacketSize-=len;
						aktIndex+=len;
					} while(len>0&&MuxPacketSize>0);
/*			}
		else
			{
				do
					{
						char buff[6];
						len=tunnel_read(m_pTunnel,buff,1);//((char*)pPacket)+aktIndex,MuxPacketSize);
						buff[5]=0;
						printf(buff);
						return 0;
//						MuxPacketSize-=len;
//						aktIndex+=len;
					} while(len>0&&MuxPacketSize>0);
			}
*/
		if(len==SOCKET_ERROR||len==0)
			{
				#ifdef _DEBUG
					CAMsg::printMsg(LOG_DEBUG,"MuxSocket-Receive - ungültiges Packet!\n");
					CAMsg::printMsg(LOG_DEBUG,"Data-Len %i\n",len);
					if(len==SOCKET_ERROR)
						CAMsg::printMsg(LOG_DEBUG,"SOCKET-ERROR: %i\n",WSAGetLastError());
				#endif
				return SOCKET_ERROR;
			}

		pPacket->len=ntohs(pPacket->len);	
		pPacket->channel=ntohl(pPacket->channel);
		return pPacket->len;
	}
#else
SINT32 CAMuxSocket::receive(MIXPACKET* pPacket)
	{
		EnterCriticalSection(&csReceive);
		if(m_Socket.receiveFully((UINT8*)pPacket,MIXPACKET_SIZE)!=E_SUCCESS)
			{
				LeaveCriticalSection(&csReceive);
				return SOCKET_ERROR;
			}
		if(bIsCrypted)
    	ocipherIn.decryptAES((UINT8*)pPacket,(UINT8*)pPacket,16);
		pPacket->channel=ntohl(pPacket->channel);
		pPacket->flags=ntohs(pPacket->flags);
		LeaveCriticalSection(&csReceive);		
		return MIXPACKET_SIZE;
	}

/**Trys to receive a Mix-Packet. If after timout milliseconds not a whole packet is available
* E_AGAIN will be returned. In this case you should later try to get the rest of the packet
*/
SINT32 CAMuxSocket::receive(MIXPACKET* pPacket,UINT32 timeout)
	{
		EnterCriticalSection(&csReceive);
		SINT32 len=MIXPACKET_SIZE-m_aktBuffPos;
		SINT32 ret=m_Socket.receive(m_Buff+m_aktBuffPos,len);
		if(ret<=0)
			{
				LeaveCriticalSection(&csReceive);
				return E_UNKNOWN;
			}
		if(ret==len)
			{
				if(bIsCrypted)
        	ocipherIn.decryptAES(m_Buff,m_Buff,16);
				memcpy(pPacket,m_Buff,MIXPACKET_SIZE);
				pPacket->channel=ntohl(pPacket->channel);
				pPacket->flags=ntohs(pPacket->flags);
				m_aktBuffPos=0;
				LeaveCriticalSection(&csReceive);
				return MIXPACKET_SIZE;
			}
		m_aktBuffPos+=ret;
		if(timeout==0)
			{
				LeaveCriticalSection(&csReceive);
				return E_AGAIN;
			}
		UINT32 timeE=time(NULL)+timeout;
		SINT32 dt=timeout;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(*this);
		do
			{
				ret=oSocketGroup.select(false,dt);
				if(ret<0)
					{
						LeaveCriticalSection(&csReceive);
						return E_UNKNOWN;
					}
				if(ret==1)
					{
						ret=m_Socket.receive(m_Buff+m_aktBuffPos,len);
						if(ret<=0)
							{
								LeaveCriticalSection(&csReceive);
								return E_UNKNOWN;
							}
						if(ret==len)
							{
								if(bIsCrypted)
                	ocipherIn.decryptAES(m_Buff,m_Buff,16);
								memcpy(pPacket,m_Buff,MIXPACKET_SIZE);
								pPacket->channel=ntohl(pPacket->channel);
								pPacket->flags=ntohs(pPacket->flags);
								m_aktBuffPos=0;
								LeaveCriticalSection(&csReceive);
								return MIXPACKET_SIZE;
							}
						m_aktBuffPos+=ret;
					}
				dt=timeE-time(NULL);
			}	while(dt>0);
		LeaveCriticalSection(&csReceive);
		return E_AGAIN;
	}
#endif

#ifndef PROT2
int CAMuxSocket::close(HCHANNEL channel_id)
	{
		MUXPACKET oPacket;
		oPacket.channel=channel_id;
		oPacket.len=0;
		return send(&oPacket);
	}
#else
int CAMuxSocket::close(HCHANNEL channel_id)
	{
		MIXPACKET oPacket;
		oPacket.channel=channel_id;
		oPacket.flags=CHANNEL_CLOSE;
		return send(&oPacket);
	}
#endif