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

//#include "httptunnel/common.h"
char buff[255];

CAMuxSocket::CAMuxSocket()
	{
//		bIsTunneld=false;
	}
/*	
int CAMuxSocket::useTunnel(char* proxyhost,UINT16 proxyport)
	{
		m_szTunnelHost=new char[strlen(proxyhost)+1];
		strcpy(m_szTunnelHost,proxyhost);
		m_uTunnelPort=proxyport;
		bIsTunneld=true;
		return 0;
	}
*/
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
int CAMuxSocket::accept(UINT16 port)
	{
		CASocket oSocket;
		oSocket.create();
		oSocket.setReuseAddr(true);
		if(oSocket.listen(port)==SOCKET_ERROR)
			return SOCKET_ERROR;
		if(oSocket.accept(m_Socket)==SOCKET_ERROR)
			return SOCKET_ERROR;
		oSocket.close();
		m_Socket.setRecvLowWat(sizeof(MUXPACKET));
		return E_SUCCESS;
	}
#endif		
	
SINT32 CAMuxSocket::connect(LPCASOCKETADDR psa)
	{
		return connect(psa,1,0);
	}

SINT32 CAMuxSocket::connect(LPCASOCKETADDR psa,UINT retry,UINT32 time)
	{
//		if(!bIsTunneld)
//			{
				m_Socket.setRecvLowWat(MUXPACKET_SIZE);
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
int CAMuxSocket::send(MUXPACKET *pPacket)
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
int CAMuxSocket::send(MUXPACKET *pPacket)
	{
		int ret;
		HCHANNEL tmpChannel=pPacket->channel;
		UINT16 tmpFlags=pPacket->flags;
		pPacket->channel=htonl(tmpChannel);
		pPacket->flags=htons(tmpFlags);
		ret=m_Socket.send(((UINT8*)pPacket),MUXPACKET_SIZE);
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
			ret=MUXPACKET_SIZE;
		pPacket->channel=tmpChannel;
		pPacket->flags=tmpFlags;
		return ret;
	}
#endif

#ifndef PROT2
int CAMuxSocket::receive(MUXPACKET* pPacket)
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
SINT32 CAMuxSocket::receive(MUXPACKET* pPacket)
	{
		if(m_Socket.receiveFully((UINT8*)pPacket,MUXPACKET_SIZE)!=E_SUCCESS)
			return SOCKET_ERROR;
		pPacket->channel=ntohl(pPacket->channel);
		pPacket->flags=ntohs(pPacket->flags);
		return MUXPACKET_SIZE;
	}

SINT32 CAMuxSocket::receive(MUXPACKET* pPacket,UINT32 timeout)
	{
		SINT32 ret=m_Socket.receiveFully((UINT8*)pPacket,MUXPACKET_SIZE,timeout);
		if(ret==E_SUCCESS)
			{
				pPacket->channel=ntohl(pPacket->channel);
				pPacket->flags=ntohs(pPacket->flags);
				return MUXPACKET_SIZE;
			}
		if(ret==E_TIMEDOUT)
			return E_TIMEDOUT;
		return SOCKET_ERROR;
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
		MUXPACKET oPacket;
		oPacket.channel=channel_id;
		oPacket.flags=CHANNEL_CLOSE;
		return send(&oPacket);
	}
#endif