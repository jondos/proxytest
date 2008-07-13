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
#include "CASingleSocketGroup.hpp"
#include "CAUtil.hpp"

CAMuxSocket::CAMuxSocket()
	{
		m_Buff=new UINT8[MIXPACKET_SIZE];
		m_aktBuffPos=0;
		m_bIsCrypted=false;
	}
	
CAMuxSocket::~CAMuxSocket()
	{
		close();
		delete []m_Buff;
		m_Buff = NULL;
	}	

SINT32 CAMuxSocket::setCrypt(bool b)
	{
		m_csSend.lock();
		m_csReceive.lock();
		m_bIsCrypted=b;
		m_csReceive.unlock();
		m_csSend.unlock();
		return E_SUCCESS;
	}


SINT32 CAMuxSocket::accept(UINT16 port)
	{
		CASocket oSocket;
		oSocket.create();
		oSocket.setReuseAddr(true);
		if(oSocket.listen(port)!=E_SUCCESS)
			return E_UNKNOWN;
		if(oSocket.accept(m_Socket)!=E_SUCCESS)
			return E_UNKNOWN;
		oSocket.close();
		//m_Socket.setRecvLowWat(MIXPACKET_SIZE);
		m_aktBuffPos=0;
		return E_SUCCESS;
	}

/** Waits for an incoming connection on oAddr.
  * @retval E_SUCCESS, if successful
	* @retval E_SOCKET_BIND, E_SOCKET_LISTEN
	* @retval E_UNKOWN
	*/
SINT32 CAMuxSocket::accept(const CASocketAddr& oAddr)
	{
		CASocket oSocket;
		oSocket.create(oAddr.getType());
		oSocket.setReuseAddr(true);
		SINT32 ret=oSocket.listen(oAddr);
		if(ret!=E_SUCCESS)
			return ret;
		ret=oSocket.accept(m_Socket);
		if(ret!=E_SUCCESS)
			return E_UNKNOWN;
		oSocket.close();
		//m_Socket.setRecvLowWat(MIXPACKET_SIZE);
		m_aktBuffPos=0;
		return E_SUCCESS;
	}

SINT32 CAMuxSocket::connect(CASocketAddr & psa)
	{
		return connect(psa,1,0);
	}

SINT32 CAMuxSocket::connect(CASocketAddr & psa,UINT retry,UINT32 time)
	{
		//m_Socket.setRecvLowWat(MIXPACKET_SIZE);
		m_aktBuffPos=0;
		return m_Socket.connect(psa,retry,time);
	}
/** Closes the underlying socket.*/			
SINT32 CAMuxSocket::close()
	{
		m_aktBuffPos=0;
		return m_Socket.close();
	}
/** Sends a MixPacket over the Network. Will block until the whole packet is 
	* send.
	* @param pPacket MixPacket to send
	* @retval MIXPACKET_SIZE if MixPacket was successful send
	* @retval E_UNKNOWN otherwise
*/
SINT32 CAMuxSocket::send(MIXPACKET *pPacket)
	{
		m_csSend.lock();
		int ret;
		UINT8 tmpBuff[16];
		memcpy(tmpBuff,pPacket,16);
		pPacket->channel=htonl(pPacket->channel);
		pPacket->flags=htons(pPacket->flags);
		if(m_bIsCrypted)
    	m_oCipherOut.crypt1(((UINT8*)pPacket),((UINT8*)pPacket),16);
		ret=m_Socket.sendFully(((UINT8*)pPacket),MIXPACKET_SIZE);
		if(ret!=E_SUCCESS)
			{
				#ifdef _DEBUG
					CAMsg::printMsg(LOG_DEBUG,"MuxSocket-Send-Error!\n");
					CAMsg::printMsg(LOG_DEBUG,"SOCKET-ERROR: %i\n",GET_NET_ERROR);
				#endif
				ret=E_UNKNOWN;
			}
		else
		{
			ret=MIXPACKET_SIZE;
		}
		memcpy(pPacket,tmpBuff,16);
		m_csSend.unlock();
		return ret;
	}

SINT32 CAMuxSocket::send(MIXPACKET *pPacket,UINT8* buff)
	{
		m_csSend.lock();
		int ret;
		UINT8 tmpBuff[16];
		memcpy(tmpBuff,pPacket,16);
		pPacket->channel=htonl(pPacket->channel);
		pPacket->flags=htons(pPacket->flags);
		if(m_bIsCrypted)
			m_oCipherOut.crypt1(((UINT8*)pPacket),((UINT8*)pPacket),16);
		memcpy(buff,((UINT8*)pPacket),MIXPACKET_SIZE);
		ret=MIXPACKET_SIZE;
		memcpy(pPacket,tmpBuff,16);
		m_csSend.unlock();
		return ret;
	}
	
SINT32 CAMuxSocket::prepareForSend(MIXPACKET *pinoutPacket)
	{	
		m_csSend.lock();
		pinoutPacket->channel=htonl(pinoutPacket->channel);
		pinoutPacket->flags=htons(pinoutPacket->flags);
		m_oCipherOut.crypt1(((UINT8*)pinoutPacket),((UINT8*)pinoutPacket),16);
		m_csSend.unlock();
		return MIXPACKET_SIZE;
	}
	
/** Receives a whole MixPacket. Blocks until a packet is received or
	* a socket error occurs.
	*
	* @param pPacket on return stores the received MixPacket
	* @retval SOCKET_ERROR, in case of an error
	* @retval MIXPACKET_SIZE otherwise
	*/
SINT32 CAMuxSocket::receive(MIXPACKET* pPacket)
	{
		UINT32 retLock = m_csReceive.lock();
		if (retLock != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,
				"Could not lock MuxSocket receive method! Error code: %d\n", retLock);
			return E_UNKNOWN;
		}
		
		if(m_Socket.receiveFully((UINT8*)pPacket,MIXPACKET_SIZE)!=E_SUCCESS)
		{
			m_csReceive.unlock();
			return SOCKET_ERROR;
		}
		if(m_bIsCrypted)
    	m_oCipherIn.crypt1((UINT8*)pPacket,(UINT8*)pPacket,16);
		pPacket->channel=ntohl(pPacket->channel);
		pPacket->flags=ntohs(pPacket->flags);
		m_csReceive.unlock();
		return MIXPACKET_SIZE;
	}

/**Trys to receive a Mix-Packet. If after timout milliseconds not 
* a whole packet is available
* E_AGAIN will be returned. 
* In this case you should later try to get the rest of the packet
*/

//TODO: Bug if socket is not in non_blocking mode!!
SINT32 CAMuxSocket::receive(MIXPACKET* pPacket,UINT32 msTimeout)
	{
		UINT32 retLock = m_csReceive.lock();
		if (retLock != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,
				"Could not lock MuxSocket timed receive method! Error code: %d\n", retLock);
			return E_UNKNOWN;
		}
		SINT32 len=MIXPACKET_SIZE-m_aktBuffPos;
		SINT32 ret=m_Socket.receive(m_Buff+m_aktBuffPos,len);
		if(ret<=0&&ret!=E_AGAIN) //if socket was set in non-blocking mode
			{
				m_csReceive.unlock();
				return E_UNKNOWN;
			}
		if(ret==len) //whole packet recieved
			{
				if(m_bIsCrypted)
					m_oCipherIn.crypt1(m_Buff,m_Buff,16);
				memcpy(pPacket,m_Buff,MIXPACKET_SIZE);
				pPacket->channel=ntohl(pPacket->channel);
				pPacket->flags=ntohs(pPacket->flags);
				m_aktBuffPos=0;
				m_csReceive.unlock();
				return MIXPACKET_SIZE;
			}
		if(ret>0) //some new bytes arrived
			m_aktBuffPos+=ret;
		if(msTimeout==0) //we should not wait any more
			{
				m_csReceive.unlock();
				return E_AGAIN;
			}
		UINT64 timeE;
		UINT64 timeC;
		getcurrentTimeMillis(timeE);
		add64(timeE,msTimeout);
		UINT32 dt=msTimeout;
		CASingleSocketGroup oSocketGroup(false);
		oSocketGroup.add(*this);
		for(;;)
			{
				ret=oSocketGroup.select(dt);
				if(ret!=1)
					{
						m_csReceive.unlock();
						return E_UNKNOWN;
					}
				len=MIXPACKET_SIZE-m_aktBuffPos;
				ret=m_Socket.receive(m_Buff+m_aktBuffPos,len);
				if(ret<=0&&ret!=E_AGAIN)
					{
						m_csReceive.unlock();
						return E_UNKNOWN;
					}
				if(ret==len)
					{
						if(m_bIsCrypted)
							m_oCipherIn.crypt1(m_Buff,m_Buff,16);
						memcpy(pPacket,m_Buff,MIXPACKET_SIZE);
						pPacket->channel=ntohl(pPacket->channel);
						pPacket->flags=ntohs(pPacket->flags);
						m_aktBuffPos=0;
						m_csReceive.unlock();
						return MIXPACKET_SIZE;
					}
				if(ret>0)
					m_aktBuffPos+=ret;
				getcurrentTimeMillis(timeC);
				if(isGreater64(timeC,timeE)||isEqual64(timeC,timeE))
					break;
				dt=diff64(timeE,timeC);
			}
		m_csReceive.unlock();
		return E_AGAIN;
	}

/*int CAMuxSocket::close(HCHANNEL channel_id)
	{
		MIXPACKET oPacket;
		oPacket.channel=channel_id;
		oPacket.flags=CHANNEL_CLOSE;
		return send(&oPacket);
	}

int CAMuxSocket::close(HCHANNEL channel_id,UINT8* buff)
	{
		MIXPACKET oPacket;
		oPacket.channel=channel_id;
		oPacket.flags=CHANNEL_CLOSE;
		return send(&oPacket,buff);
	}*/

#ifdef LOG_CRIME
UINT32 CAMuxSocket::sigCrime(HCHANNEL channel_id,MIXPACKET* sigPacket)
	{
		sigPacket->channel=channel_id;
		UINT32 v;
		getRandom(&v);
		v&=CHANNEL_SIG_CRIME_ID_MASK;
		sigPacket->flags=(CHANNEL_SIG_CRIME|v);
		getRandom(sigPacket->data,DATA_SIZE);
		return (v>>8);
	}
#endif
