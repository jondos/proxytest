#include "CASignature.hpp"
class CAInfoService
	{
		public:
			CAInfoService();
			~CAInfoService();
			int sendHelo();
			int start();
			int stop();
			int setLevel(int user,int risk,int traffic);
			int getLevel(int* puser,int* prisk,int* ptraffic);
			bool getRun(){return bRun;}
			int setSignature(CASignature* pSignature);
			CASignature* getSignature(){return pSignature;}
		private:
			int nUser;
			int nRisk;
			int nTraffic; 
			bool bRun;
			CRITICAL_SECTION csLevel;
			CASignature* pSignature;
	};
