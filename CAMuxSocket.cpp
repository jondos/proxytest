#include "StdAfx.h"
#include "CASocketAddr.hpp"
#include "CAMuxSocket.hpp"
#ifdef _DEBUG
	#include "CAMsg.hpp"
#endif

#include "httptunnel/common.h"
char buff[255];

CAMuxSocket::CAMuxSocket()
	{
		bIsTunneld=false;
	}
	
int CAMuxSocket::useTunnel(char* proxyhost,unsigned short proxyport)
	{
		m_szTunnelHost=new char[strlen(proxyhost)+1];
		strcpy(m_szTunnelHost,proxyhost);
		m_uTunnelPort=proxyport;
		bIsTunneld=true;
		return 0;
	}

int CAMuxSocket::accept(unsigned short port)
	{
		if(!bIsTunneld)
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
			}
		else
			{
				m_pTunnel=tunnel_new_server (port,0/*DEFAULT_CONTENT_LENGTH*//*sizeof(MUXPACKET)*/);
				if(m_pTunnel==NULL)
				    return SOCKET_ERROR;
				tunnel_accept(m_pTunnel);
			}
		return 0;
	}
			
int CAMuxSocket::connect(LPSOCKETADDR psa)
	{
		return connect(psa,1,0);
	}

int CAMuxSocket::connect(LPSOCKETADDR psa,int retry,int time)
	{
		if(!bIsTunneld)
			{
				m_Socket.setRecvLowWat(sizeof(MUXPACKET));
				return m_Socket.connect(psa,retry,time);
			}
		else
			{
				psa->getHostName(buff,255);
				m_pTunnel=tunnel_new_client (buff, psa->getPort(),m_szTunnelHost,m_uTunnelPort,
				 0/*DEFAULT_CONTENT_LENGTH*//*sizeof(MUXPACKET)*/);
				return tunnel_connect(m_pTunnel);
			}
	}
			
int CAMuxSocket::close()
	{
		if(!bIsTunneld)
			{
				return m_Socket.close();
			}
		else
			return tunnel_close(m_pTunnel);
	}

int CAMuxSocket::send(MUXPACKET *pPacket)
	{
		int MuxPacketSize=sizeof(MUXPACKET);
		int aktIndex=0;
		int len=0;
		pPacket->channel=htonl(pPacket->channel);
		pPacket->len=htons(pPacket->len);

		if(!bIsTunneld)
			{
				do
					{
						len=m_Socket.send(((char*)pPacket)+aktIndex,MuxPacketSize);
						MuxPacketSize-=len;
						aktIndex+=len;
					} while(len>0&&MuxPacketSize>0);
			}
		else
			{		
				do
					{
						len=tunnel_write(m_pTunnel,(void*)"fghj",4);//((char*)pPacket)+aktIndex,MuxPacketSize);
						return 0;
						MuxPacketSize-=len;
						aktIndex+=len;
					} while(len>0&&MuxPacketSize>0);	
			}
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

int CAMuxSocket::receive(MUXPACKET* pPacket)
	{
		int MuxPacketSize=sizeof(MUXPACKET);
		int aktIndex=0;
		int len=0;
		if(!bIsTunneld)
			{
				do
					{
						len=m_Socket.receive(((char*)pPacket)+aktIndex,MuxPacketSize);
						MuxPacketSize-=len;
						aktIndex+=len;
					} while(len>0&&MuxPacketSize>0);
			}
		else
			{
				do
					{
						char buff[6];
						len=tunnel_read(m_pTunnel,buff,1);//((char*)pPacket)+aktIndex,MuxPacketSize);
						buff[5]=0;
						printf(buff);
						return 0;
						MuxPacketSize-=len;
						aktIndex+=len;
					} while(len>0&&MuxPacketSize>0);
			}
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

int CAMuxSocket::close(HCHANNEL channel_id)
	{
		MUXPACKET oPacket;
		oPacket.channel=channel_id;
		oPacket.len=0;
		return send(&oPacket);
	}
