class CACmdLnOptions
    {
	public:
	    CACmdLnOptions();
	    ~CACmdLnOptions();
	    int parse(int argc,const char** arg);
	    bool getDaemon();
	    int getServerPort();
	    int getSOCKSServerPort();
	    int getTargetPort();
	    int getTargetHost(char* host,int len);
	    int getSOCKSPort();
	    int getSOCKSHost(char* host,int len);
	    int getInfoServerPort();
	    int getInfoServerHost(char* host,int len);
			bool isLocalProxy();
			bool isFirstMix();
			bool isMiddleMix();
			bool isLastMix();
	protected:
	    bool bDaemon;
	    int iServerPort;
	    int iSOCKSServerPort;
	    int iTargetPort;
	    char* strTargetHost;
	    char* strSOCKSHost;
	    int iSOCKSPort;
	    char* strInfoServerHost;
	    int iInfoServerPort;
			bool bLocalProxy,bFirstMix,bMiddleMix,bLastMix;
    };
