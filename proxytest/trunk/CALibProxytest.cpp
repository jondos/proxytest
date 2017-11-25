#include "StdAfx.h"
#include "CALibProxytest.hpp"
#include "CAMuxSocket.hpp"

#ifdef SERVER_MONITORING
	#include "CAStatusManager.hpp"
#endif
CACmdLnOptions* CALibProxytest::m_pglobalOptions;
CAMutex* CALibProxytest::m_pOpenSSLMutexes;
#if defined _DEBUG && ! defined (ONLY_LOCAL_PROXY)
	CAThreadList* CALibProxytest::m_pThreadList;
#endif

///Callbackfunction for locking required by OpenSSL <1.1
#if OPENSSL_VERSION_NUMBER < 0x10100000L 
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
#endif

/** Callback used by openssl to identify a thread*/
///TODO: Move this to CAThread !
unsigned long openssl_get_thread_id(void)
	{
#ifndef ONLY_LOCAL_PROXY
		return CAThread::getSelfID();
#else
		return 1;
#endif
	}


/**do necessary initialisations of libraries etc.*/
SINT32 CALibProxytest::init()
	{
#if !defined MXML_DOM 
		XMLPlatformUtils::Initialize();
#endif
		initDOMParser();
#if !defined ONLY_LOCAL_PROXY || defined INLUDE_MIDDLE_MIX
	#if OPENSSL_VERSION_NUMBER < 0x10100000L 
			SSL_library_init();
	#else
			OPENSSL_init_ssl(0, NULL);
	#endif
#endif
#if OPENSSL_VERSION_NUMBER < 0x10100000L 
		OpenSSL_add_all_algorithms();
		//It seems that only older versions of OpenSSL need the thred locking callbacks.
		//But the mor interesting question is: at which version did the change happen?
		m_pOpenSSLMutexes=new CAMutex[CRYPTO_num_locks()];
		CRYPTO_set_locking_callback((void (*)(int,int,const char *,int))openssl_locking_callback);
		CRYPTO_set_id_callback(openssl_get_thread_id);
#endif
#if defined _DEBUG && ! defined (ONLY_LOCAL_PROXY)
		m_pThreadList=new CAThreadList();
		CAThread::setThreadList(m_pThreadList);
#endif
		CAMsg::init();
		CASocketAddrINet::init();
		CASocket::init();
		CAMuxSocket::init();
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
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		CRYPTO_set_locking_callback(NULL);
		delete []m_pOpenSSLMutexes;
		m_pOpenSSLMutexes=NULL;
#endif
		//XML Cleanup
		//Note: We have to destroy all XML Objects and all objects that uses XML Objects BEFORE
		//we terminate the XML lib!
#ifdef SERVER_MONITORING
		CAStatusManager::cleanup();
#endif
		releaseDOMParser();
#if !defined MXML_DOM 
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
		CAMuxSocket::cleanup();
		CASocket::cleanup();
		CAMsg::cleanup();
		return E_SUCCESS;
	}

