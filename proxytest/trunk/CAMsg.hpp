#define MSG_LOG 0x01

class CAMsg
    {
	protected:
	    CAMsg(); //Singleton!
	    ~CAMsg();
	    static CAMsg oMsg;
	public:
	    static int setOptions(int options);
	    static int printMsg(int typ,char* format,...);
	protected:
	    bool isLog;
    };