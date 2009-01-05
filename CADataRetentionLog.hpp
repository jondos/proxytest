#include "CADataRetentionLogFile.hpp"

class CADataRetentionLog
	{
		public:
			CADataRetentionLog();
			~CADataRetentionLog();
			
			SINT32 setPublicEncryptionKey(CAASymCipher* pPublicKey);
			SINT32 setLogDir(UINT8* strLogDir);
			SINT32 log(t_dataretentionLogEntry*);
			SINT32 closeLog();

		private:
			SINT32 openLogFile(UINT32 time);

			CADataRetentionLogFile* m_pLogFile;
			CAASymCipher*						m_pPublicEncryptionKey;
			UINT8*									m_strLogDir;
			UINT32									m_max_t_out;
	};
