#include "CASocketGroup.hpp"
#include "CASocket.hpp"
#include "CAQueue.hpp"


typedef struct __t_socket_list
	{
		CASocket* pSocket;
		CAQueue* pQueue;
		__t_socket_list* next;
} _t_socket_list;
THREAD_RETURN SocketASyncSendLoop(void* p);

class CASocketASyncSend
	{	
		public:
			CASocketASyncSend(){m_Sockets=NULL;InitializeCriticalSection(&cs);}
			~CASocketASyncSend();
			SINT32 send(CASocket* pSocket,UINT8* buff,UINT32 size);
			SINT32 start();
			SINT32 stop(){return E_UNKNOWN;}
			friend THREAD_RETURN SocketASyncSendLoop(void* p);

		protected:
			CASocketGroup m_oSocketGroup;
			_t_socket_list* m_Sockets;
			#ifdef _REENTRANT
				CRITICAL_SECTION cs;
			#endif
	
	};