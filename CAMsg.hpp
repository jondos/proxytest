// TODO: Sylog for NT!!

#define MSG_STDOUT 0x00
#define MSG_LOG 0x01
#define MSG_FILE 0x02

#ifdef _WIN32
	#define LOG_ERR		0
	#define LOG_CRIT	1
	#define LOG_INFO	2 
	#define LOG_DEBUG 3
#endif

class CAMsg
	{
		protected:
			CAMsg(); //Singleton!
			static CAMsg oMsg;
		public:
			~CAMsg();
			static SINT32 setOptions(UINT32 options);
			static SINT32 printMsg(UINT32 typ,char* format,...);
		protected:
			SINT32 openLog(UINT32 type);
			SINT32 closeLog();
			UINT32 uLogType;
			int hFileErr;
			int hFileInfo;
   };
