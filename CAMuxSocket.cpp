#include "StdAfx.h"
#include "CASocketAddr.hpp"
#include "CAMuxSocket.hpp"
#ifdef _DEBUG
	#include "CAMsg.hpp"
#endif

#define DATA_SIZE 1000 // Size of Data in a single Mux Packet

typedef struct t_MuxPacket
	{
		HCHANNEL channel;
		int len;
		char data[DATA_SIZE];
	} MUXPACKET;

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

			
int CAMuxSocket::send(HCHANNEL channel_id,char* buff,int bufflen)
	{
		if(bufflen>DATA_SIZE)
			return SOCKET_ERROR;
		MUXPACKET MuxPacket;
		MuxPacket.channel=channel_id;
		memcpy(MuxPacket.data,buff,bufflen);
		MuxPacket.len=bufflen;
		int MuxPacketSize=sizeof(MuxPacket);
		int aktIndex=0;
		int len=0;
		do
			{
				len=m_Socket.send(((char*)&MuxPacket)+aktIndex,MuxPacketSize);
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
		return sizeof(MuxPacket);
	}
			
int CAMuxSocket::receive(HCHANNEL* channel_id,char* buff,int bufflen)
	{
		MUXPACKET MuxPacket;
		int MuxPacketSize=sizeof(MuxPacket);
		int aktIndex=0;
		int len=0;
		do
			{
				len=m_Socket.receive(((char*)&MuxPacket)+aktIndex,MuxPacketSize);
				MuxPacketSize-=len;
				aktIndex+=len;
			} while(len>0&&MuxPacketSize>0);
		
		if(len==SOCKET_ERROR||MuxPacket.len<0||MuxPacket.len>bufflen)
			{
				#ifdef _DEBUG
					CAMsg::printMsg(LOG_DEBUG,"MuxSocket-Receive - ungültiges Packet!\n");
					CAMsg::printMsg(LOG_DEBUG,"Data-Len %i\n",len);
					if(len==SOCKET_ERROR)
						CAMsg::printMsg(LOG_DEBUG,"SOCKET-ERROR: %i\n",WSAGetLastError());
				#endif
				return SOCKET_ERROR;
			}
		*channel_id=MuxPacket.channel;
		memcpy(buff,MuxPacket.data,MuxPacket.len);
		return MuxPacket.len;
	}

int CAMuxSocket::close(HCHANNEL channel_id)
	{
		char tmpBuff;
		return send(channel_id,&tmpBuff,0);
	}
