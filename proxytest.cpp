/*
Copyright (c) 2000, The JAP-Team 
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.

	- Redistributions in binary form must reproduce the above copyright notice, 
	  this list of conditions and the following disclaimer in the documentation and/or 
		other materials provided with the distribution.

	- Neither the name of the University of Technology Dresden, Germany nor the names of its contributors 
	  may be used to endorse or promote products derived from this software without specific 
		prior written permission. 

	
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS 
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/
// proxytest.cpp : Definiert den Einsprungpunkt f�r die Konsolenanwendung.
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
		CASocketAddr::init();
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
		CAMix* pMix=NULL;
	  CAMsg::printMsg(LOG_INFO,"Starting RoundTripTime...\n");
		if(pRTT->start()!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"RoundTripTime Startup FAILED - Exiting!\n");
				goto EXIT;
			}
		if(options.isLocalProxy())
			{
				pMix=new CALocalProxy();
			}
		else if(options.isFirstMix())
			{
				CAMsg::printMsg(LOG_INFO,"I am the First MIX..\n");
				pMix=new CAFirstMix();
			}
		else if(options.isMiddleMix())
			{
				CAMsg::printMsg(LOG_INFO,"I am a Middle MIX..\n");
				pMix=new CAMiddleMix();
			}
		else
			pMix=new CALastMix();
	  CAMsg::printMsg(LOG_INFO,"Starting MIX...\n");
		if(pMix->start()!=E_SUCCESS)
			CAMsg::printMsg(LOG_CRIT,"Error during MIX-Startup!\n");
EXIT:
		delete pRTT;
		delete pMix;
		CASocketAddr::destroy();
		#ifdef _WIN32		
			WSACleanup();
		#endif
		
		CAMsg::printMsg(LOG_CRIT,"Terminating Programm!\n");
		return 0;
	}
