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
		m_bIsCrypted=false;
		InitializeCriticalSection(&csSend);
		InitializeCriticalSection(&csReceive);
	}

SINT32 CAMuxSocket::setCrypt(bool b)
	{
		EnterCriticalSection(&csSend);
		EnterCriticalSection(&csReceive);
		m_bIsCrypted=b;
		if(b)
			{
				UINT8 nullkey[16];
				memset(nullkey,0,16);
				m_oCipherIn.setKeyAES(nullkey);
				m_oCipherOut.setKeyAES(nullkey);
			}
		LeaveCriticalSection(&csReceive);
		LeaveCriticalSection(&csSend);
		return E_SUCCESS;
	}


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

SINT32 CAMuxSocket::connect(CASocketAddr & psa)
	{
		return connect(psa,1,0);
	}

SINT32 CAMuxSocket::connect(CASocketAddr & psa,UINT retry,UINT32 time)
	{
		m_Socket.setRecvLowWat(MIXPACKET_SIZE);
		return m_Socket.connect(psa,retry,time);
	}
			
int CAMuxSocket::close()
	{
		return m_Socket.close();
	}

int CAMuxSocket::send(MIXPACKET *pPacket)
	{
		EnterCriticalSection(&csSend);
		int ret;
		UINT8 tmpBuff[16];
		memcpy(tmpBuff,pPacket,16);
		pPacket->channel=htonl(pPacket->channel);
		pPacket->flags=htons(pPacket->flags);
		if(m_bIsCrypted)
    	m_oCipherOut.encryptAES(((UINT8*)pPacket),((UINT8*)pPacket),16);
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

SINT32 CAMuxSocket::receive(MIXPACKET* pPacket)
	{
		EnterCriticalSection(&csReceive);
		if(m_Socket.receiveFully((UINT8*)pPacket,MIXPACKET_SIZE)!=E_SUCCESS)
			{
				LeaveCriticalSection(&csReceive);
				return SOCKET_ERROR;
			}
		if(m_bIsCrypted)
    	m_oCipherIn.decryptAES((UINT8*)pPacket,(UINT8*)pPacket,16);
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
				if(m_bIsCrypted)
        	m_oCipherIn.decryptAES(m_Buff,m_Buff,16);
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
								if(m_bIsCrypted)
                	m_oCipherIn.decryptAES(m_Buff,m_Buff,16);
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

int CAMuxSocket::close(HCHANNEL channel_id)
	{
		MIXPACKET oPacket;
		oPacket.channel=channel_id;
		oPacket.flags=CHANNEL_CLOSE;
		return send(&oPacket);
	}
