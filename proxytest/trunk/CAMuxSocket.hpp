#ifndef __CAMUXSOCKET__
#define __CAMUXSOCKET__
#include "CASocket.hpp"
#include "httptunnel/tunnel.h"

typedef UINT32 HCHANNEL;

#define DATA_SIZE 1000 // Size of Data in a single Mux Packet

#define MUX_HTTP  0 
#define MUX_SOCKS 1

typedef struct t_MuxPacket
	{
		HCHANNEL channel;
		UINT16	len;
		UINT8		type;
		UINT8		reserved;
		UINT8		data[DATA_SIZE];
	} MUXPACKET;

class CAMuxSocket
	{
		public:
			CAMuxSocket();
			~CAMuxSocket(){}
			int useTunnel(char* proxyhost,UINT16 proxyport);
			int accept(UINT16 port);
			SINT32 connect(LPCASOCKETADDR psa);
			SINT32 connect(LPCASOCKETADDR psa,UINT retry,UINT32 time);
			int close();
			int send(MUXPACKET *pPacket);
			int receive(MUXPACKET *pPacket);
			int close(HCHANNEL channel_id);
			operator CASocket*(){return &m_Socket;}
			operator SOCKET(){if(!bIsTunneld)
														return (SOCKET)m_Socket;
												else
													return tunnel_pollin_fd(m_pTunnel);}
	
		private:
			CASocket m_Socket;
			bool bIsTunneld;
			Tunnel* m_pTunnel;
			char *m_szTunnelHost;
			UINT16 m_uTunnelPort;
	};
#endif
