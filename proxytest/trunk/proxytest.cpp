// proxytest.cpp : Definiert den Einsprungpunkt für die Konsolenanwendung.
//

#include "StdAfx.h"
#include "CASocket.hpp"
#include "CASocketGroup.hpp"
#include "CASocketAddr.hpp"
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"
#include "CAMuxSocket.hpp"
#include "CASocketList.hpp"
#include "CAMuxChannelList.hpp"
#include "CASymCipher.hpp"
#include "CAASymCipher.hpp"
#include "CAInfoService.hpp"
#include "CASignature.hpp"
#include "CADatagramSocket.hpp"
#include "CARoundTripTime.hpp"
#include "CAUtil.hpp"
#include "CAMix.hpp"
#include "CAMiddleMix.hpp"
#include "CAFirstMix.hpp"
#include "CALastMix.hpp"
#include "CALocalProxy.hpp"
//#ifdef _WIN32
//HANDLE hEventThreadEnde;
//#endif
CACmdLnOptions options;

//CRITICAL_SECTION csClose;
typedef struct
{
	unsigned short len;
	time_t time;
} log;

//CASocketAddr socketAddrSquid;
#ifdef _DEBUG
int sockets;
#endif

#ifndef _WIN32
	#ifdef _DEBUG 
		void signal_broken_pipe( int sig)
			{
				CAMsg::printMsg(LOG_WARNING,"Hm.. Broken Pipe.... How cares!\n");
				signal(SIGPIPE,signal_broken_pipe);
			}
	#endif
#endif


int main(int argc, const char* argv[])
	{
		#ifdef _WIN32
			int err=0;
			WSADATA wsadata;
			err=WSAStartup(0x0202,&wsadata);
		#endif
// Util test..
BIGNUM bn;
getcurrentTimeMillis(&bn);
exit(0);

/* //Datagram test
	CADatagramSocket oSocket;
	oSocket.bind(5000);
	UINT8 buff[1000];
	CASocketAddr from;
	oSocket.receive(buff,1000,&from);
	oSocket.send(buff,100,&from);
	oSocket.close();
	exit(0);
 */
 /*		//low-water receive-test
		CASocket oSocket;
		oSocket.listen(9000);
		CASocket cSocket;
		oSocket.accept(cSocket);
		CASocketGroup oSocketGroup;
		printf("RecvLowWat returned: %i",cSocket.setRecvLowWat(1000));
		oSocketGroup.add(cSocket);
		oSocketGroup.select();
		printf("Can read: %i\n",cSocket.available());
		UINT8 buff[1];
	//	cSocket.receive(buff,1);
		sleep(2);
		oSocketGroup.select();
		printf("Can read: %i\n",cSocket.available());
		oSocket.close();
		exit(0);
*/
		//initalize Random..
		#if _WIN32
			RAND_screen();
		#else
			#ifndef __linux
				unsigned char randbuff[255];
				RAND_seed(randbuff,sizeof(randbuff));
			#endif
		#endif

		options.parse(argc,argv);
		if(options.getDaemon())
			{
				#ifndef _WIN32
					UINT8 buff[255];
					if(options.getLogDir(buff,255)==E_SUCCESS)
						CAMsg::setOptions(MSG_FILE);
					else
						CAMsg::setOptions(MSG_LOG);
					pid_t pid;
					pid=fork();
					if(pid!=0)
						exit(0);
					setsid();
					chdir("/");
					umask(0);
				#endif
			}
		else
			{
				UINT8 buff[255];
				if(options.getLogDir((UINT8*)buff,255)==E_SUCCESS)
					CAMsg::setOptions(MSG_FILE);
			}
	  CAMsg::printMsg(LOG_INFO,"Anon proxy started!\n");
	  CAMsg::printMsg(LOG_INFO,"Using: %s!\n",OPENSSL_VERSION_TEXT);
#ifdef _DEBUG
		sockets=0;
#endif

		
#ifndef _WIN32
	#ifdef _DEBUG
			signal(SIGPIPE,signal_broken_pipe);
	#else
			signal(SIGPIPE,SIG_IGN);
	#endif
#endif
		CARoundTripTime* pRTT=new CARoundTripTime();
		pRTT->start();
		CAMix* pMix=NULL;
		if(options.isLocalProxy())
			{
				pMix=new CALocalProxy();
			}
		else if(options.isFirstMix())
			{
				pMix=new CAFirstMix();
			}
		else if(options.isMiddleMix())
			{
				pMix=new CAMiddleMix();
			}
		else
			pMix=new CALastMix();
		pMix->start();
		delete pRTT;
		#ifdef _WIN32		
			WSACleanup();
		#endif
		return 0;
	}
