#include "CASocketAddr.hpp"
#include "CASocketList.hpp"

class CAMixChannel
	{
		public:
			CAMixChannel();
			~CAMixChannel();
			int connect(LPSOCKETADDR psa);
			int send(int id,char*buff,int len);
			int receive(int id,char* buff,int len);
			int close(int id);
			int close(int id,int mode);
		protected:
			CASocketList connections;
			CRITICAL_SECTION csSend;
			CRITICAL_SECTION csConnect;
			CRITICAL_SECTION csClose;
	};