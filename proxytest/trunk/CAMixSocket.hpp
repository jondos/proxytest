#include "CASocketAddr.hpp"
class CAMixSocket
	{
		public:
			CAMixSocket();
			~CAMixSocket();
			int connect(LPSOCKETADDR psa);
			int send(char* buff,int len);
			int receive(char* buff,int len);
//			int close();
			int close(int mode);
		protected:
			int id;
			#ifdef _REENTRANT
				CRITICAL_SECTION csClose;
			#endif
	};
