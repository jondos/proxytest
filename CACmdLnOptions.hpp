class CACmdLnOptions
    {
	public:
	    CACmdLnOptions();
	    ~CACmdLnOptions();
	    int parse(int argc,const char** arg);
	    bool getDaemon();
	    UINT16 getServerPort();
	    UINT16 getSOCKSServerPort();
	    UINT16 getTargetPort();
	    SINT32 getTargetHost(UINT8* host,UINT32 len);
	    UINT16 getSOCKSPort();
	    SINT32 getSOCKSHost(UINT8* host,UINT32 len);
	    UINT16 getInfoServerPort();
	    SINT32 getInfoServerHost(UINT8* host,UINT32 len);
			SINT32 getKeyFileName(UINT8* filename,UINT32 len);
			SINT32 getCascadeName(UINT8* name,UINT32 len);
			SINT32 getLogDir(UINT8* name,UINT32 len);
			bool isLocalProxy();
			bool isFirstMix();
			bool isMiddleMix();
			bool isLastMix();
	protected:
	    bool bDaemon;
	    UINT16 iServerPort;
	    UINT16 iSOCKSServerPort;
	    UINT16 iTargetPort;
	    char* strTargetHost;
	    char* strSOCKSHost;
	    UINT16 iSOCKSPort;
	    char* strInfoServerHost;
	    UINT16 iInfoServerPort;
			char* strKeyFileName;
			bool bLocalProxy,bFirstMix,bMiddleMix,bLastMix;
			char* strCascadeName;  
			char* strLogDir;  
	};
