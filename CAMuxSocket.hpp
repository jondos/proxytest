#ifndef __CAMUXSOCKET__
#define __CAMUXSOCKET__
#include "CASocket.hpp"

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
			int accept(int port);
			int connect(LPSOCKETADDR psa);
			int close();
			int send(HCHANNEL channel_id,char* buff,unsigned short len);
			int send(MUXPACKET *pPacket);
			int send(MUXPACKET *pPacket,CASymCipher oCipher);
			int receive(HCHANNEL* channel_id,char* buff,unsigned short len);
			int receive(MUXPACKET *pPacket);
			int receive(MUXPACKET *pPacket,CASymCipher oCipher);
			int close(HCHANNEL channel_id);
			operator CASocket*(){return &m_Socket;}
			operator SOCKET(){return (SOCKET)m_Socket;}
		private:
			CASocket m_Socket;
	};
#endif