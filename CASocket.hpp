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

			SINT32 listen(LPCASOCKETADDR psa);
			SINT32 listen(UINT16 port);
			int accept(CASocket &s);
			int connect(LPCASOCKETADDR psa);
			int connect(LPCASOCKETADDR psa,UINT retry,UINT32 time);
			int close();
			int close(int mode);
			int send(UINT8* buff,UINT32 len);
			int available();
			int receive(UINT8* buff,UINT32 len);
			operator SOCKET(){return m_Socket;}
			int getLocalPort();
			int setReuseAddr(bool b);
			int setRecvLowWat(UINT32 r);
			int setRecvBuff(UINT32 r);
			int setSendBuff(UINT32 r);
			int setKeepAlive(bool b);
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
