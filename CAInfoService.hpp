#include "CASignature.hpp"
class CAInfoService
	{
		public:
			CAInfoService();
			~CAInfoService();
			int sendHelo();
			int start();
			int stop();
			SINT32 setLevel(UINT32 user,UINT32 risk,UINT32 traffic);
			SINT32 getLevel(UINT32* puser,UINT32* prisk,UINT32* ptraffic);
			bool getRun(){return bRun;}
			SINT32 setSignature(CASignature* pSignature);
			CASignature* getSignature(){return pSignature;}
		private:
			UINT32 nUser;
			UINT32 nRisk;
			UINT32 nTraffic; 
			bool bRun;
			CRITICAL_SECTION csLevel;
			CASignature* pSignature;
	};
