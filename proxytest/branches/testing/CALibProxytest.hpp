#include "CACmdLnOptions.hpp"
#include "CAMutex.hpp"
#if defined _DEBUG && ! defined (ONLY_LOCAL_PROXY)
	#include "CAThreadList.hpp"
#endif
class CALibProxytest
	{
		public:
			static SINT32 init();
			static SINT32 cleanup();
			static CACmdLnOptions* getOptions()
				{
					return m_pglobalOptions;
				}
		private:
			static void openssl_locking_callback(int mode, int type, char * /*file*/, int /*line*/);
			static CACmdLnOptions* m_pglobalOptions;
			static CAMutex* m_pOpenSSLMutexes;
#if defined _DEBUG && ! defined (ONLY_LOCAL_PROXY)
			static CAThreadList* m_pThreadList;
		public:
			static CAThreadList* getThreadList()
				{
					return m_pThreadList;
				}
#endif
	};
