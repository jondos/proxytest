#ifndef __CAMUXSOCKET__
#define __CAMUXSOCKET__
#include "CASocket.hpp"

typedef int HCHANNEL;

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
			operator CASocket*(){return &m_Socket;}
		private:
			CASocket m_Socket;
	};
#endif