#ifndef __CAMUXSOCKET__
#define __CAMUXSOCKET__
#include "CASocket.hpp"
#include "httptunnel/tunnel.h"
#include "CASymCipher.hpp"
typedef unsigned int HCHANNEL;

#define DATA_SIZE 1000 // Size of Data in a single Mux Packet

#define MUX_HTTP  0 
#define MUX_SOCKS 1

typedef struct t_MuxPacket
	{
		HCHANNEL channel;
		unsigned short len;
		unsigned char type;
		unsigned char reserved;
		char data[DATA_SIZE];
	} MUXPACKET;

class CAMuxSocket
	{
		public:
			CAMuxSocket();
			~CAMuxSocket(){}
			int useTunnel(char* proxyhost,unsigned short proxyport);
			int accept(unsigned short port);
			int connect(LPSOCKETADDR psa);
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
			unsigned short m_uTunnelPort;
	};
#endif