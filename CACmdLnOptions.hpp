class CACmdLnOptions
    {
	public:
	    CACmdLnOptions();
	    ~CACmdLnOptions();
	    int parse(int argc,const char** arg);
	    bool getDaemon();
	    int getServerPort();
	    int getTargetPort();
	    int getTargetHost(char* host,int len);
	protected:
	    bool bDaemon;
	    int iServerPort;
	    int iTargetPort;
	    char* strTargetHost;
    };