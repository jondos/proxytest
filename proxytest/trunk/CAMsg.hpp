// TODO: Sylog for NT!!

#define MSG_LOG 0x01

#ifdef _WIN32
	#define LOG_ERR 0
	#define LOG_CRIT 0
	#define LOG_INFO 0 
	#define LOG_DEBUG 0
#endif
class CAMsg
    {
	protected:
	    CAMsg(); //Singleton!
//	    ~CAMsg();
	    static CAMsg oMsg;
	public:
	    ~CAMsg();
	    static int setOptions(int options);
	    static int printMsg(int typ,char* format,...);
	protected:
	    bool isLog;
    };