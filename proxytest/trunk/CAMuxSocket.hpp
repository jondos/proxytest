#ifndef __CAMUXSOCKET__
#define __CAMUXSOCKET__
#include "CASocket.hpp"

typedef unsigned int HCHANNEL;

class CAMuxSocket
	{
		public:
			CAMuxSocket();
			~CAMuxSocket(){}
			int accept(int port);
			int connect(LPSOCKETADDR psa);
			int close();
			int send(HCHANNEL channel_id,char* buff,int len);
			int receive(HCHANNEL* channel_id,char* buff,int len);
			int close(HCHANNEL channel_id);
			operator CASocket*(){return &m_Socket;}
			operator SOCKET(){return (SOCKET)m_Socket;}
		private:
			CASocket m_Socket;
	};
#endif