#ifndef __CASOCKET__
#define __CASOCKET__
#include "CASocketAddr.hpp"
class CASocket
	{
		public:
			CASocket();
			~CASocket(){close();DeleteCriticalSection(&csClose);}
			int listen(LPSOCKETADDR psa);
			int accept(CASocket &s);
			int connect(LPSOCKETADDR psa);
			int close();
			int close(int mode);
			int send(char* buff,int len);
			int receive(char* buff,int len);
			operator SOCKET(){return m_Socket;}
		private:
			SOCKET m_Socket;
			CRITICAL_SECTION csClose;
			int closeMode;
	};
#endif