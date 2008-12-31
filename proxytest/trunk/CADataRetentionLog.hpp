#include "CADataRetentionLogFile.hpp"

class CADataRetentionLog
	{
		public:
			CADataRetentionLog();
			
			SINT32 setPublicEncryptionKey(CAASymCipher* pPublicKey);
			SINT32 setLogDir(UINT8* strLogDir);
			SINT32 log(t_dataretentionLogEntry*);

		private:
			CADataRetentionLogFile* m_pLogFile;
			CAASymCipher*						m_pPublicEncryptionKey;
	};
