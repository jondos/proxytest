#ifndef __CASOCKET__
#define __CASOCKET__
#include "CASocketAddr.hpp"
#include "CASymCipher.hpp"
class CASocket
	{
		public:
			CASocket();
			~CASocket(){close();DeleteCriticalSection(&csClose);}
			int listen(LPSOCKETADDR psa);
			int listen(unsigned short port);
			int accept(CASocket &s);
			int connect(LPSOCKETADDR psa);
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
		private:
			SOCKET m_Socket;
			CRITICAL_SECTION csClose;
			int closeMode;
	};
#endif