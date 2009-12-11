#include "StdAfx.h"
#include "CALibProxytest.hpp"
#ifdef SERVER_MONITORING
	#include "CAStatusManager.hpp"
#endif
CACmdLnOptions* CALibProxytest::m_pglobalOptions;
CAMutex* CALibProxytest::m_pOpenSSLMutexes;
#if defined _DEBUG && ! defined (ONLY_LOCAL_PROXY)
	CAThreadList* CALibProxytest::m_pThreadList;
#endif

///Callbackfunction for locking required by OpenSSL
void CALibProxytest::openssl_locking_callback(int mode, int type, char * /*file*/, int /*line*/)
	{
		if (mode & CRYPTO_LOCK)
			{
				m_pOpenSSLMutexes[type].lock();
			}
		else
			{
				m_pOpenSSLMutexes[type].unlock();
			}
	}

/** Callback used by openssl to identify a thread*/
///TODO: Move this to CAThread !
unsigned long openssl_get_thread_id(void)
	{
		return CAThread::getSelfID();
	}


/**do necessary initialisations of libraries etc.*/
SINT32 CALibProxytest::init()
	{
#ifndef ONLY_LOCAL_PROXY
		XMLPlatformUtils::Initialize();
		initDOMParser();
#endif
#ifndef ONLY_LOCAL_PROXY
		SSL_library_init();
#endif
		OpenSSL_add_all_algorithms();
		m_pOpenSSLMutexes=new CAMutex[CRYPTO_num_locks()];
		CRYPTO_set_locking_callback((void (*)(int,int,const char *,int))openssl_locking_callback);
		CRYPTO_set_id_callback(openssl_get_thread_id);
#if defined _DEBUG && ! defined (ONLY_LOCAL_PROXY)
		m_pThreadList=new CAThreadList();
		CAThread::setThreadList(m_pThreadList);
#endif
		CAMsg::init();
		CASocketAddrINet::init();
		//startup
		#ifdef _WIN32
			int err=0;
			WSADATA wsadata;
			err=WSAStartup(0x0202,&wsadata);
		#endif
		initRandom();
		m_pglobalOptions=new CACmdLnOptions();
		return E_SUCCESS;
}

/**do necessary cleanups of libraries etc.*/
SINT32 CALibProxytest::cleanup()
	{
		CASocketAddrINet::cleanup();
		#ifdef _WIN32
			WSACleanup();
		#endif
		delete m_pglobalOptions;
		m_pglobalOptions=NULL;

	//OpenSSL Cleanup
		CRYPTO_set_locking_callback(NULL);
		delete []m_pOpenSSLMutexes;
		m_pOpenSSLMutexes=NULL;
		//XML Cleanup
		//Note: We have to destroy all XML Objects and all objects that uses XML Objects BEFORE
		//we terminate the XML lib!
#ifdef SERVER_MONITORING
		CAStatusManager::cleanup();
#endif
		releaseDOMParser();
#ifndef ONLY_LOCAL_PROXY
		XMLPlatformUtils::Terminate();
#endif

#if defined _DEBUG && ! defined (ONLY_LOCAL_PROXY)
			if(m_pThreadList != NULL)
			{
				int nrOfThreads = m_pThreadList->getSize();
				CAMsg::printMsg(LOG_INFO,"After cleanup %d threads listed.\n", nrOfThreads);
				if(nrOfThreads > 0)
				{
					m_pThreadList->showAll();
				}
				delete m_pThreadList;
				m_pThreadList = NULL;
			}
#endif
		CAMsg::cleanup();
		return E_SUCCESS;
	}

