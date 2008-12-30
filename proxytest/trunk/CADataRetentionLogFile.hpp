#ifndef __CA_DATA_RETENTION_LOG_FILE
#define __CA_DATA_RETENTION_LOG_FILE


struct  __t__data_retention_log_file_header
	{
		UINT8 version;
		UINT8 reserved[3];

		UINT8  day;
		UINT8  month;
		UINT16 year;

		UINT8 entity;
		UINT8 loggedFields;
		UINT8 entriesPerBlock;
		UINT8 keys;
	};

typedef struct __t__data_retention_log_file_header t_dataretentionLogFileHeader;

const UINT8 DATARETENTION_ENTITY_FIRST_MIX=1;
const UINT8 DATARETENTION_ENTITY_MIDDLE_MIX=2;
const UINT8 DATARETENTION_ENTITY_LAST_MIX=3;

class CADataRetentionLogFile
	{
		public:
			CADataRetentionLogFile();
			SINT32 openLog(UINT8* strLogDir,UINT32 date);
			SINT32 closeLog();
			SINT32 log(t_dataretentionLogEntry*);
		
		private:
			SINT32 writeHeader();
			SINT32 writeFooter();

			int    m_hLogFile;
			UINT8  m_Day;
			UINT8  m_Month;
			UINT16 m_Year;
			UINT8* m_arOneBlock;
			UINT32 m_nLogEntriesPerBlock;
			UINT32 m_nBytesPerLogEntry;
			UINT32 m_nCurrentLogEntriesInBlock;
	};

#endif //__CA_DATA_RETENTION_LOG_FILE