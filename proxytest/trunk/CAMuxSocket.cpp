#include "StdAfx.h"
#include "CASocketAddr.hpp"
#include "CAMuxSocket.hpp"
#ifdef _DEBUG
	#include "CAMsg.hpp"
#endif


CAMuxSocket::CAMuxSocket()
	{
	}
	
int CAMuxSocket::accept(int port)
	{
		CASocket oSocket;
		if(oSocket.listen(port)==SOCKET_ERROR)
			return SOCKET_ERROR;
		if(oSocket.accept(m_Socket)==SOCKET_ERROR)
			return SOCKET_ERROR;
		oSocket.close();
		return 0;
	}
			
int CAMuxSocket::connect(LPSOCKETADDR psa)
	{
		return m_Socket.connect(psa);	    
	}
			
int CAMuxSocket::close()
	{
		return m_Socket.close();
	}

			
int CAMuxSocket::send(HCHANNEL channel_id,char* buff,unsigned short bufflen)
	{
		if(bufflen>DATA_SIZE)
			return SOCKET_ERROR;
		MUXPACKET MuxPacket;
		memcpy(MuxPacket.data,buff,bufflen);
		MuxPacket.channel=channel_id;
		MuxPacket.len=bufflen;
		return send(&MuxPacket);
	}
	
int CAMuxSocket::send(MUXPACKET *pPacket)
	{
		int MuxPacketSize=sizeof(MUXPACKET);
		int aktIndex=0;
		int len=0;
		pPacket->channel=htonl(pPacket->channel);
		pPacket->len=htonl(pPacket->len);
		
		do
			{
				len=m_Socket.send(((char*)pPacket)+aktIndex,MuxPacketSize);
				MuxPacketSize-=len;
				aktIndex+=len;
			} while(len>0&&MuxPacketSize>0);
		
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
		
int CAMuxSocket::receive(HCHANNEL* channel_id,char* buff,unsigned short bufflen)
	{
		MUXPACKET MuxPacket;
		if(receive(&MuxPacket)==SOCKET_ERROR)
			return SOCKET_ERROR;
		if(MuxPacket.len>bufflen)
			{
				#ifdef _DEBUG
					CAMsg::printMsg(LOG_DEBUG,"MuxSocket-Receive - ungültiges Packet!\n");
					CAMsg::printMsg(LOG_DEBUG,"Data-Len %i\n",MuxPacket.len);
					if(MuxPacket.len==SOCKET_ERROR)
						CAMsg::printMsg(LOG_DEBUG,"SOCKET-ERROR: %i\n",WSAGetLastError());
				#endif
				return SOCKET_ERROR;
			}
		*channel_id=MuxPacket.channel;
		memcpy(buff,MuxPacket.data,MuxPacket.len);
		return MuxPacket.len;
	}

int CAMuxSocket::receive(MUXPACKET* pPacket)
	{
		int MuxPacketSize=sizeof(MUXPACKET);
		int aktIndex=0;
		int len=0;
		do
			{
				len=m_Socket.receive(((char*)pPacket)+aktIndex,MuxPacketSize);
				MuxPacketSize-=len;
				aktIndex+=len;
			} while(len>0&&MuxPacketSize>0);
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
		pPacket->len=ntohl(pPacket->len);	
		pPacket->channel=ntohl(pPacket->channel);
		return pPacket->len;
	}

int CAMuxSocket::close(HCHANNEL channel_id)
	{
		char tmpBuff;
		return send(channel_id,&tmpBuff,0);
	}
