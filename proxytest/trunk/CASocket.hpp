#ifndef __CASOCKET__
#define __CASOCKET__
#include "CASocketAddr.hpp"
#include "CASymCipher.hpp"



class CASocket
	{
		public:
			CASocket();
			~CASocket(){close();DeleteCriticalSection(&csClose);}

			int create();

			int listen(LPSOCKETADDR psa);
			int listen(unsigned short port);
			int accept(CASocket &s);
			int connect(LPSOCKETADDR psa);
			int connect(LPSOCKETADDR psa,int retry,int time);
			int close();
			int close(int mode);
			int send(char* buff,int len);
			int send(char* buff,int len,CASymCipher& oCipher);
			int available();
			int receive(char* buff,int len);
			int receive(char* buff,int len,CASymCipher& oCipher);
			operator SOCKET(){return m_Socket;}
			int getLocalPort();
			int setReuseAddr(bool b);
			int setRecvLowWat(int r);
			int setRecvBuff(int r);
			int setSendBuff(int r);
		private:
			SOCKET m_Socket;
			#ifdef _REENTRANT
				CRITICAL_SECTION csClose;
			#endif
			int closeMode;
			// temporary hack...
			int localPort;
	};
#endif
