#ifndef __CADATAGRAMSOCKET__
#define __CADATAGRAMSOCKET__
#include "CASocketAddr.hpp"

class CADatagramSocket
	{
		public:
			CADatagramSocket();
			~CADatagramSocket(){close();DeleteCriticalSection(&csClose);}

			SINT32 create();
			
			SINT32 close();
			SINT32 bind(LPCASOCKETADDR psa);
			SINT32 bind(UINT16 port);
			SINT32 send(UINT8* buff,UINT32 len,LPCASOCKETADDR to);
			SINT32 receive(UINT8* buff,UINT32 len,LPCASOCKETADDR from);
			operator SOCKET(){return m_Socket;}
//			SINT32 getLocalPort();
/*			int setReuseAddr(bool b);
			int setRecvLowWat(UINT32 r);
			int setRecvBuff(UINT32 r);
			int setSendBuff(UINT32 r);
			int setKeepAlive(bool b);
*/
		private:
			SOCKET m_Socket;
			#ifdef _REENTRANT
				CRITICAL_SECTION csClose;
			#endif
			// temporary hack...
//			int localPort;
	};
#endif
