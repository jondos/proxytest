class CAInfoService
	{
		public:
			CAInfoService();
			~CAInfoService();
			int start();
			int stop();
			int setLevel(int user,int risk,int traffic);
			int getLevel(int* puser,int* prisk,int* ptraffic);
			bool getRun(){return bRun;}
		private:
			int nUser;
			int nRisk;
			int nTraffic; 
			bool bRun;
			CRITICAL_SECTION csLevel;
	};