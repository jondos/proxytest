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
// proxytest.cpp : Definiert den Einsprungpunkt fuer die Konsolenanwendung.
//

#include "StdAfx.h"

#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"
#include "CALocalProxy.hpp"
#include "CAQueue.hpp"
#include "CAThreadList.hpp"
#include "CAStatusManager.hpp"
#include "CALibProxytest.hpp"

#ifdef _DEBUG //For FreeBSD memory checking functionality
	const char* _malloc_options="AX";
#endif

#ifdef REPLAY_DATABASE_PERFORMANCE_TEST
#include "CAReplayDatabase.hpp"
#endif

#ifndef ONLY_LOCAL_PROXY
	#include "xml/DOM_Output.hpp"
	#include "CAMix.hpp"
	#ifdef LOG_CRIME
		#include "tre/regex.h"
	#endif
	#ifdef NEW_MIX_TYPE
		/* use TypeB mixes */
		#include "TypeB/CAFirstMixB.hpp"
		#include "TypeB/CALastMixB.hpp"
	#else
		#include "TypeA/CAFirstMixA.hpp"
		#include "TypeA/CALastMixA.hpp"
	#endif
	#include "CAMiddleMix.hpp"
	#include "CALogPacketStats.hpp"
	#include "CATLSClientSocket.hpp"

#ifdef REPLAY_DATABASE_PERFORMANCE_TEST
  #include "CAReplayDatabase.hpp"
#endif
// The Mix....
CAMix* pMix=NULL;
#endif

bool bTriedTermination = false;


typedef struct
{
	unsigned short len;
	time_t time;
} log;

#ifndef _WIN32
	#ifdef _DEBUG
		void signal_broken_pipe( int sig)
			{
				CAMsg::printMsg(LOG_INFO,"Hm.. Broken Pipe.... Who cares!\n");
				signal(SIGPIPE,signal_broken_pipe);
			}
	#endif
#endif


/// Removes the stored PID (file)
void removePidFile()
	{
		if(CALibProxytest::getOptions()==NULL)
			{
				return;
			}
		UINT8 strPidFile[512];
		if(CALibProxytest::getOptions()->getPidFile(strPidFile,512)==E_SUCCESS)
			{
				if(::remove((char*)strPidFile)!=0)
					{
#ifndef _WIN32
						int old_uid=geteuid(); //old uid... stored if we have to switch to root
						seteuid(0);
						::remove((char*)strPidFile);
						seteuid(old_uid);
#endif
					}
			}
	}


/**do necessary initialisations of libraries etc.*/
void init()
	{
		CALibProxytest::init();
	}

/**do necessary cleanups of libraries etc.*/
void cleanup()
	{
#ifdef ENABLE_GPERFTOOLS_CPU_PROFILER
		ProfilerFlush();
		ProfilerStop();
#endif
//		delete pRTT;
#ifndef ONLY_LOCAL_PROXY
		if(pMix!=NULL)
			delete pMix;
		pMix=NULL;
#endif
		CAMsg::printMsg(LOG_CRIT,"Terminating Programm!\n");
		removePidFile();
		CALibProxytest::cleanup();
	}

///Remark: terminate() might be already defined by the c lib -- do not use this name...
void my_terminate(void)
{
	if(!bTriedTermination)
	{
		bTriedTermination = true;
#ifndef ONLY_LOCAL_PROXY
		if(pMix!=NULL)
		{
			pMix->shutDown();
			for (UINT32 i = 0; i < 20 && !(pMix->isShutDown()); i++)
			{
				msSleep(100);
			}
			delete pMix;
			pMix=NULL;
		}
#endif
		cleanup();
	}
}


void signal_segv( int )
{
	signal(SIGSEGV,SIG_DFL); //otherwise we might end up in endless loops...

	MONITORING_FIRE_SYS_EVENT(ev_sys_sigSegV);
	CAMsg::printMsg(LOG_CRIT,"Oops ... caught SIG_SEGV! Exiting ...\n");
#ifdef PRINT_THREAD_STACK_TRACE
	CAThread::METHOD_STACK* stack = CAThread::getCurrentStack();
	if (stack != NULL)
	{
		CAMsg::printMsg( LOG_CRIT, "Stack trace: %s, \"%s\"\n", stack->strMethodName, stack->strPosition);

	}
	else
	{
		CAMsg::printMsg( LOG_CRIT, "Stack trace: none available\n");
	}
#endif
	// my_terminate();  temporarily disabled
	exit(1);
}




void signal_term( int )
	{
		MONITORING_FIRE_SYS_EVENT(ev_sys_sigTerm);
		CAMsg::printMsg(LOG_INFO,"Hm.. Signal SIG_TERM received... exiting!\n");
		my_terminate();
		exit(0);
	}

void signal_interrupt( int)
	{
		MONITORING_FIRE_SYS_EVENT(ev_sys_sigInt);
		CAMsg::printMsg(LOG_INFO,"Hm.. Strg+C pressed... exiting!\n");
#if defined _DEBUG && ! defined (ONLY_LOCAL_PROXY)
		CAMsg::printMsg(LOG_INFO,"%d threads listed.\n",CALibProxytest::getThreadList()->getSize());
		CALibProxytest::getThreadList()->showAll();
#endif
		my_terminate();
		exit(0);
	}

#ifndef ONLY_LOCAL_PROXY
void signal_hup(int)
	{
		CALibProxytest::getOptions()->reread(pMix);
	}
#endif


///Check what the sizes of base types are as expected -- if not kill the programm
void checkSizesOfBaseTypes()
	{
		#pragma warning( push )
		#pragma warning( disable : 4127 ) //Disable: Bedingter Ausdruck ist konstant
		if(sizeof(SINT8)!=1)
			{
				CAMsg::printMsg(LOG_CRIT,"sizeof(SINT8) != 1 --> maybe a compiler (optimization) problem!\n");
				exit(-1);
			}
		if(sizeof(UINT8)!=1)
			{
				CAMsg::printMsg(LOG_CRIT,"sizeof(UINT8) != 1 --> maybe a compiler (optimization) problem!\n");
				exit(-1);
			}
		if(sizeof(SINT16)!=2)
			{
				CAMsg::printMsg(LOG_CRIT,"sizeof(SINT16) != 2 --> maybe a compiler (optimization) problem!\n");
				exit(-1);
			}
		if(sizeof(UINT16)!=2)
			{
				CAMsg::printMsg(LOG_CRIT,"sizeof(UINT16) != 2 --> maybe a compiler (optimization) problem!\n");
				exit(-1);
			}
		if(sizeof(SINT32)!=4)
			{
				CAMsg::printMsg(LOG_CRIT,"sizeof(SINT32) != 4 --> maybe a compiler (optimization) problem!\n");
				exit(-1);
			}
		if(sizeof(UINT32)!=4)
			{
				CAMsg::printMsg(LOG_CRIT,"sizeof(UINT32) != 4 --> maybe a compiler (optimization) problem!\n");
				exit(-1);
			}
		#ifdef HAVE_NATIVE_UINT64
			if(sizeof(UINT64)!=8)
				{
					CAMsg::printMsg(LOG_CRIT,"sizeof(UINT64) != 8 --> maybe a compiler (optimization) problem!\n");
					exit(-1);
				}
		#endif
		#pragma warning( pop )
	}


/** \mainpage

\section docCommProto Description of the system and communication protocol

\subsection Basics

The whole system consists of JAP's, mix-servers, cache proxies and the InfoService. (see Figure 1)

\image html JAPArchitecture.gif "Figure 1: The Architecture of the Anonymous-Service"

The local proxy (JAP) and the mix servers communicate using TCP/IP-Internet connections.
 Each JAP has one and only one connection to a mix server. A mix server has one and only one connection
to one or two other mixes. If a mix receives data packets from JAP's and sends them to an other mix, we call
 them a \em first mix. So a first mix has exactly one connection to an other mix. If a mix receives packets from
 a mix and sends the data to the cache proxy, we call them the \em last mix. Consequently a last
 mix has only one connection to an other mix. Each mix with two connections to other mixes is called
 a \em middle mix. This type of mix will receive packets form one mix and forwards them to the other mix.

If mixes are connected in a meaningful way a chain is setup up, so what packets are transmitted from JAP's
to the cache-proxies and than to the Internet.
A chain of mixes is called a \em MixCascade (or \em cascade for short).
Many different cascades could exist at the same time, but JAP can select one and only one at the same time.

Also a mix could only be part of one and only one cascade. If a mix is not part of a cascade, we call it a \em free mix.
Free mixes are not useable for JAP, but could be connected to build a new cascade.

Beside JAPs and Mixes we have added a third component to our system: the \em InfoService.
The InfoService is more or less a database which stores information about
the system like available MixCascades, status of Mixes etc.
\subsection docMux Mulitplexing and Demultiplexing

JAP acts as a local proxy for the browser.
The browser opens many connections to the JAP (usually one per HTTP-Request).
All this connections are multiplexed over the one connection to the first mix JAP is connected to.
Every connection from the browser is called a \em channel (mix channel or \em AnonChannel).
A first mix sends the stream of packets (from multiple channels) from multiple users to the next mix (all over one TCP/IP connection). So JAP and the mixes have to multiplex/demultiplex the channels.
A channel can transport only fixed sized packets. These packets are called \em MixPackets.
So a first mix for instance gets many packets from different users in parallel
and sends them to the next mix in a serialized way. (see Figure 2)

\image html JAPMixPacketMux.gif "Figure 2: Mux/DeMux of fixed sized MixPackets"


Each MixPacket has a size of #MIXPACKET_SIZE (998) bytes (see Figure 3).
The header of each packet is as follows (see also: #MIXPACKET):
\li 4 bytes \c channel-ID
\li 2 bytes \c flags
\li #DATA_SIZE (992) bytes  \c data (transported bytes)

\image html MixPacketGeneral.gif "Figure 3: General format of a MixPacket


The data bytes have different meanings in different situations and mixes.
For instance the content itself is meaningful only at the last mix (because of multiple encryptions)
The format of the data (at the last mix) is:
\li 2 bytes \c len
\li 1 byte \c type
\li #PAYLOAD_SIZE (989) bytes \c payload

They payload are the bytes what should be transported from the browser to the Internet
or back via the anonymous channel.

The channel-ID changes in every mix. Also the content bytes changes in every mix,
because each mix will perform a single encryption/decryption.

\subsection docInterMixEncryption Inter-Mix Encryption
The stream of MixPackets between to mixes is encrypted using AES-128/128 in OFB-128 mode. Exactly only the
first part (16 bytes) of each MixPacket is encrypted. (see Figure 4)

\image html JAPInterMixEncryption.gif "Figure 4: Encryption between two mixes"

The encryption is done, so that an attacker could not see the channel-ID and flags of a MixPacket. OFB is chosen, so that
an attacker could not replay a MixPacket. If he replays a MixPacket, than at least the channel-ID
changes after decrypting. If this MixPacket was the first packet of a channel, than the cryptographic keys
for this channel will change to (because of the content format for the first packet of a channel, explained later).

For Upstream and Downstream different keys are used.
See \ref docCascadeInit "[Cascade setup]" for information about exchange of these keys.

\subsection docChannelSetup AnonChannel establishment
Before data could be sent through an AnonChannel one has to be established.
This is done by sending ChannelOpen messages through the mixes.
Figure 5 shows the format of such a message how it would arrive for example
at the last mix.

\image html MixPacketChannelOpen.gif "Figure 5: ChannelOpen packet (example for the last mix)"

\section docCascadeInit Initialisation of a Cascade

If a Mix startsup it reads its configuration information from a file. See \ref pageMixConfig "[MixConfig]" for more information.

\image html JAPCascadeInit.gif "Figure 6: Steps during Cascade initialisation (3 Mix example)

Figure 6 shows the steps which take place, than a Cascade starts. These steps are illustrated using a 3 Mix example.
The procedure is as follows:

\li Step 1: Each Mix starts, reads its configuration file (including own key
 for digital signature and test keys from previous and next mix) and generates a
 public key pair of the asymmetric crypto system used by the mix for encryption
 (currently this is RSA).

\li Step 2: Mix 2 establishes a TCP/IP connection with Mix 3.
No data is transmitted during this step.

\li Step 3: Mix 3 sends its public key  to Mix 2 (signed with its signature key).
See \ref XMLInterMixInitSendFromLast "[XML]" for a description of the XML struct send in this step.
The \<Nonce/\> is a random byte string of length=16 (chosen by Mix 3) used for replay detection.

\li Step 4: Mix 2 generates and sends a symmetric key to Mix 3, encrypted with the public key
of Mix 3, signed by Mix 2.
This key is used for inter-mix encryption and is actually a 64 byte octet stream.
See \ref XMLInterMixInitAnswer "[XML]" for a description of the XML struct send in this step.\n
The first 16 bytes a used as a key for
inter-mix encryption of packets send from Mix \e n-1 to Mix \e n (e.g. upstream direction).
The next 16 byte are used as IV for this cipher. The next 16 bytes are used as
key for downstream direction and the last 16 bytes are the IV for this cipher.
To detect replay attacks Mix 3 checks if the \<Nonce/\> element sent from Mix 2 is equal to SHA1(\<Nonce\/> chosen by Mix 3 in Step 3).

\li Step 5,6,7: This steps are equal to step 2,3,4. Mix 1 establishes a TCP/IP-connection
with Mix 2. Mix 2 send it publick key to Mix 1 and Mix 1 generates and sends
a symmetric key to Mix 2.

\section docMixJap Communication between Mix and JAP

\image html JAPMixJap.gif "Figure 7: Communication between Mix and JAP"

-# JAP opens a TCP/IP connection to a FirstMix
-# FirstMix sends information about the cascade (including public keys of the Mixes) to the JAP.
See \ref XMLMixKeyInfo "[XML]" for a description of the XML struct send.
-# JAP sends a special MixPacket, containing only 2 symmetric keys (IV is all '0') encrypted with the
public key of the FirstMix. This keys are used for link encryption between JAP and FirstMix.
Note: At the moment this is binary - but will use XML in the future.
The data part of this MixPacket is as follows:
	- the ASCII string: "KEYPACKET"
	- 16 random bytes used as symetric key for packets send from Mix to JAP
	- 16 random bytes used as symmetric key for packets send from JAP to Mix
	.

-# Normal MixPacket exchange according to the mix protocol.

\section docInfoService Communication with the InfoService
HTTP GET/POST-requests are used for communication with the InfoService.
The filename part of the URL specifies the command (action) to be executed and
additional information is transfered as XML struct in the body of the HTTP message.
Example:

\verbatim
POST /status HTTP/1.0
Content-Type: text/xml
Content-Length: 251

<?xml version="1.0" encoding="utf-8" ?>
<MixCascadeStatus id="testmix"
                  mixedPackets="567899"
                  nrOfActiveUsers="235"
                  trafficSituation="78"
                  LastUpdate="126940258075">
</MixCascadeStatus>
\endverbatim

\subsection docMixInfoService Communication between Mix and InfoService

\image html JAPMixInfoService.gif "Figure 8: Communication between Mix and InfoService"

\li 1. HELO-Messages send from each mix to the InfoService every 10 minutes to announce itself.
This is a POST request and the command name is: "helo".
See \ref XMLMixHELO "[XML]" for a description of the XML struct send.

\li 2. Status-Messages send from the FirstMix to the InfoService every minute to update the current
status of the MixCascade including Number of Users, Traffic situation etc.
This is a POST request and the command name is: "status".
See \ref XMLMixCascadeStatus "[XML]" for a description of the XML struct send.

*/

int main(int argc, const char* argv[])
	{
#ifndef ONLY_LOCAL_PROXY
		pMix=NULL;
#endif
		UINT32 lLogOpts = 0;
		SINT32 maxFiles,ret;
#if defined(HAVE_CRTDBG)
//			_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
//			_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
//			_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
//			_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
//			_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
//			_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );

		UINT32 tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
		tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;
		tmpDbgFlag |=_CRTDBG_LEAK_CHECK_DF;
		_CrtSetDbgFlag(tmpDbgFlag);
		_CrtMemState s1, s2, s3;
		_CrtMemCheckpoint( &s1 );
#endif
//Switch on debug infos
#ifdef CWDEBUG
		Debug(libcw_do.on());
		Debug(dc::malloc.on());
#endif
		init();
			//some test....
		//UINT8 buff1[500];
		//readPasswd(buff1,500);
		//printf("%s\n",buff1);
		//printf("Len: %i\n",strlen((char*)buff1));

		/*UINT32 size=0;
		UINT8* fg=readFile((UINT8*)"test.xml",&size);
		XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(fg,size);
		delete[] fg;
		doc->release();
		cleanup();
		exit(0);
		*/

		checkSizesOfBaseTypes();
#ifndef NEW_MIX_TYPE
		if(MIXPACKET_SIZE!=sizeof(MIXPACKET))
			{
				CAMsg::printMsg(LOG_CRIT,"MIXPACKET_SIZE [%u] != sizeof(MUXPACKET) [%u] --> maybe a compiler (optimization) problem!\n",MIXPACKET_SIZE,sizeof(MIXPACKET));
				CAMsg::printMsg(LOG_CRIT,"Offsets:\n");
				MIXPACKET oPacket;
				UINT8 *p=(UINT8 *)&oPacket;
				UINT32 soffsets[7]={0,4,6,6,6,8,9};
				UINT32 hoffsets[7];
				CAMsg::printMsg(LOG_CRIT,".channel %u (should be 0)\n",hoffsets[0]=(UINT8*)&(oPacket.channel)-p);
				CAMsg::printMsg(LOG_CRIT,".flags: %u (should be 4)\n",hoffsets[1]=(UINT8*)&oPacket.flags-p);
				CAMsg::printMsg(LOG_CRIT,".data: %u (should be 6)\n",hoffsets[2]=(UINT8*)&oPacket.data-p);
				CAMsg::printMsg(LOG_CRIT,".payload: %u (should be 6)\n",hoffsets[3]=(UINT8*)&oPacket.payload-p);
				CAMsg::printMsg(LOG_CRIT,".payload.len: %u (should be 6)\n",hoffsets[4]=(UINT8*)&oPacket.payload.len-p);
				CAMsg::printMsg(LOG_CRIT,".payload.type: %u (should be 8)\n",hoffsets[5]=(UINT8*)&oPacket.payload.type-p);
				CAMsg::printMsg(LOG_CRIT,".payload.data: %u (should be 9)\n",hoffsets[6]=(UINT8*)&oPacket.payload.data-p);
				for(int i=0;i<7;i++)
				 if(soffsets[i]!=hoffsets[i])
					exit(EXIT_FAILURE);
				CAMsg::printMsg(LOG_CRIT,"Hm, The Offsets seams to be ok - so we try to continue - hope that works...\n");
			}
#endif
#ifdef LOG_CRIME
//			testTre();
#endif

#ifdef REPLAY_DATABASE_PERFORMANCE_TEST
		CAReplayDatabase::measurePerformance((UINT8*)"dbperformace.log",1,10000001,500000,10,100000);
		exit(0);
#endif


#ifdef DATA_RETENTION_LOG
		if(sizeof(t_dataretentionLogEntry)!=18)
		{
				CAMsg::printMsg(LOG_CRIT,"sizeof(tDataRetentionLogEntry) [%u] != 18 --> maybe a compiler (optimization) problem!\n",sizeof(t_dataretentionLogEntry));
				exit(EXIT_FAILURE);
		}
#endif
//		CADataRetentionLogFile::doCheckAndPerformanceTest();
//		getch();
//		exit(0);
#ifdef _DEBUG
			UINT32 start;
#endif
	/*	CATLSClientSocket ssl;
		CASocketAddrINet addr;
		addr.setAddr((const UINT8*)"127.0.0.1",(UINT16)3456);
		UINT32 len1;
		UINT8* pt=readFile((UINT8*)"/Users/sk13/Documents/projects/jap/testkey.der",&len1);
		CACertificate* pCer=CACertificate::decode(pt,len1,CERT_DER);
		ssl.setServerCertificate(pCer);
		printf("try connect\n");
		ssl.connect(addr,1,0);
		ssl.receiveFully(pt,3);

		exit(0);
*/
//#ifdef INTEL_IPP_CRYPTO
//		CAASymCipher::testSpeed();
//		getch();
//		exit(0);
//#endif

		if(CALibProxytest::getOptions()->parse(argc,argv) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,"Error: Cannot parse configuration file!\n");
			goto EXIT;
		}
		if(!(	CALibProxytest::getOptions()->isFirstMix()||
					CALibProxytest::getOptions()->isMiddleMix()||
					CALibProxytest::getOptions()->isLastMix()||
					CALibProxytest::getOptions()->isLocalProxy()))
			{
				CAMsg::printMsg(LOG_CRIT,"You must specifiy, which kind of Mix you want to run!\n");
				CAMsg::printMsg(LOG_CRIT,"Use -j or -c\n");
				CAMsg::printMsg(LOG_CRIT,"Or try --help for more options.\n");
				CAMsg::printMsg(LOG_CRIT,"Exiting...\n");
				goto EXIT;
			}

		UINT8 buff[255];
#ifdef SERVER_MONITORING
		CAStatusManager::init();
#endif

/*#ifdef LOG_CRIME
		initHttpVerbLengths();
#endif
*/
#ifndef WIN32
		maxFiles=CALibProxytest::getOptions()->getMaxOpenFiles();

		struct rlimit coreLimit;
		coreLimit.rlim_cur = coreLimit.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &coreLimit) != 0)
		{
			CAMsg::printMsg(LOG_CRIT,"Could not set RLIMIT_CORE (max core file size) to unlimited size. -- Core dumps might not be generated!\n",maxFiles);
		}

		if(maxFiles>0)
			{
				struct rlimit lim;
				// Set the new MAX open files limit
				lim.rlim_cur = lim.rlim_max = maxFiles;
				if (setrlimit(RLIMIT_NOFILE, &lim) != 0)
				{
					CAMsg::printMsg(LOG_CRIT,"Could not set MAX open files to: %u -- Exiting!\n",maxFiles);
					exit(EXIT_FAILURE);
				}
			}
		if(CALibProxytest::getOptions()->getUser(buff,255)==E_SUCCESS) //switching user
			{
				struct passwd* pwd=getpwnam((char*)buff);
				if(pwd==NULL || (setegid(pwd->pw_gid)==-1) || (seteuid(pwd->pw_uid)==-1) )
					CAMsg::printMsg(LOG_ERR,"Could not switch to effective user %s!\n",buff);
				else
					CAMsg::printMsg(LOG_INFO,"Switched to effective user %s!\n",buff);
			}

		if(geteuid()==0)
			CAMsg::printMsg(LOG_INFO,"Warning - Running as root!\n");
#endif

		ret = E_SUCCESS;
#ifndef ONLY_LOCAL_PROXY
		if(CALibProxytest::getOptions()->isSyslogEnabled())
		{
			ret = CAMsg::setLogOptions(MSG_LOG);
		}
#endif
		if(CALibProxytest::getOptions()->getLogDir((UINT8*)buff,255)==E_SUCCESS)
			{
				if(CALibProxytest::getOptions()->getCompressLogs())
					ret = CAMsg::setLogOptions(MSG_COMPRESSED_FILE);
				else
					ret = CAMsg::setLogOptions(MSG_FILE);
			}
#ifndef ONLY_LOCAL_PROXY
		if(CALibProxytest::getOptions()->isEncryptedLogEnabled())
		{
			if (CAMsg::openEncryptedLog() != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR,"Could not open encrypted log - exiting!\n");
				exit(EXIT_FAILURE);
			}
		}
#endif

		if(CALibProxytest::getOptions()->getDaemon()||CALibProxytest::getOptions()->getAutoRestart()) 
		{
				if (ret != E_SUCCESS)
				{
					CAMsg::printMsg(LOG_CRIT, "We need a log file in daemon mode in order to get any messages! Exiting...\n");
					exit(EXIT_FAILURE);
				}
		}

#ifndef _WIN32
		if(CALibProxytest::getOptions()->getDaemon()&&CALibProxytest::getOptions()->getAutoRestart()) //we need two forks...
			{
				pid_t pid;
				CAMsg::printMsg(LOG_DEBUG,"daemon - before fork()\n");
				pid=fork();
				if(pid!=0)
					{
						CAMsg::printMsg(LOG_DEBUG,"Exiting parent!\n");
						exit(EXIT_SUCCESS);
					}
				setsid();
				#ifndef DO_TRACE
					chdir("/");
					umask(0);
				#endif
			 // Close out the standard file descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
			}
		if(CALibProxytest::getOptions()->getDaemon()||CALibProxytest::getOptions()->getAutoRestart()) //if Autorestart is requested, when we fork a controlling process
			                              //which is only responsible for restarting the Mix if it dies
																		//unexpectly
			{
RESTART_MIX:
				CAMsg::printMsg(LOG_DEBUG,"starting as daemon\n");
				pid_t pid;
				CAMsg::printMsg(LOG_DEBUG,"daemon - before fork()\n");
				pid=fork();
				if(pid!=0)
					{
						if(!CALibProxytest::getOptions()->getAutoRestart())
							{
								CAMsg::printMsg(LOG_DEBUG,"Exiting parent!\n");
								exit(EXIT_SUCCESS);
							}
						int status=0;
						pid_t ret=waitpid(pid,&status,0); //wait for process termination
						if(ret==pid&&status!=0) //if unexpectly died --> restart
							goto RESTART_MIX;
						exit(EXIT_SUCCESS);
					}
				CAMsg::printMsg(LOG_DEBUG,"child after fork...\n");
				setsid();
				#ifndef DO_TRACE
					chdir("/");
					umask(0);
				#endif
			 // Close out the standard file descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
			}
#endif



#if defined (_DEBUG) &&!defined(ONLY_LOCAL_PROXY)
		//		CADatabase::test();
		if(CAQueue::test()!=E_SUCCESS)
			CAMsg::printMsg(LOG_CRIT,"CAQueue::test() NOT passed! Exiting\n");
		else
			CAMsg::printMsg(LOG_DEBUG,"CAQueue::test() passed!\n");

		//CALastMixChannelList::test();
		//exit(0);
		//Testing msSleep
		CAMsg::printMsg(LOG_DEBUG,"Should sleep now for aprox 2 seconds....\n");
		start=time(NULL);
		for(SINT32 i=0;i<10;i++)
			msSleep(200);
		start=time(NULL)-start;
		CAMsg::printMsg(LOG_DEBUG,"done! Takes %u seconds\n",start);
		//end Testin msSleep
#endif


//			CAMsg::printMsg(LOG_ENCRYPTED,"Test: Anon proxy started!\n");
//			CAMsg::printMsg(LOG_ENCRYPTED,"Test2: Anon proxy started!\n");
//			CAMsg::printMsg(LOG_ENCRYPTED,"Test3: Anon proxy started!\n");

		CAMsg::printMsg(LOG_INFO,"Anon proxy started!\n");
		CAMsg::printMsg(LOG_INFO,MIX_VERSION_INFO);

#ifdef ENABLE_GPERFTOOLS_CPU_PROFILER
		ProfilerStart("gperf.cpuprofiler.data");
#endif

#ifndef _WIN32
	#ifdef _DEBUG
			signal(SIGPIPE,signal_broken_pipe);
	#else
			signal(SIGPIPE,SIG_IGN);
	#endif
	#ifndef ONLY_LOCAL_PROXY
		struct sigaction newAction;
		memset(&newAction,0,sizeof(newAction));
		newAction.sa_handler=signal_hup;
		newAction.sa_flags=0;
		sigaction(SIGHUP,&newAction,NULL);
	#endif
#endif
		signal(SIGINT,signal_interrupt);
		signal(SIGTERM,signal_term);
#if !defined (_DEBUG) && !defined(NO_SIGSEV_CATCH)
		signal(SIGSEGV,signal_segv);
#endif
		//Try to write pidfile....
		UINT8 strPidFile[512];
		if(CALibProxytest::getOptions()->getPidFile(strPidFile,512)==E_SUCCESS)
			{
				#ifndef _WIN32
					int old_uid=geteuid(); //old uid... stored if we have to switch to root
				#endif
				pid_t pid=getpid();
				UINT8 thePid[10];
				sprintf((char*)thePid,"%i",pid);
				int len=strlen((char*)thePid);
				int hFile=open((char*)strPidFile,O_TRUNC|O_CREAT|O_WRONLY,S_IREAD|S_IWRITE);
#ifndef _WIN32
				if(hFile==-1&&seteuid(0)!=-1) //probably we do not have enough rights (because we have already switch to an other uid --> try to go back temporaly..
					{
						hFile=open((char*)strPidFile,O_TRUNC|O_CREAT|O_WRONLY,S_IREAD|S_IWRITE);
					}
#endif
				if(hFile==-1||len!=write(hFile,thePid,len))
					{
						#ifndef _WIN32
										seteuid(old_uid);
						#endif
						CAMsg::printMsg(LOG_CRIT,"Could not write pidfile - exiting!\n");
						exit(EXIT_FAILURE);
					}
				close(hFile);
#ifndef _WIN32
				seteuid(old_uid);
#endif
			}

//		CARoundTripTime* pRTT=NULL;
		if(CALibProxytest::getOptions()->isLocalProxy())
			{
				#ifndef NEW_MIX_TYPE
					CALocalProxy* pProxy=new CALocalProxy();
					CAMsg::printMsg(LOG_INFO,"Starting LocalProxy...\n");
					if(pProxy->start()!=E_SUCCESS)
						CAMsg::printMsg(LOG_CRIT,"Error during MIX-Startup!\n");
					delete pProxy;
					pProxy = NULL;
				#else
					CAMsg::printMsg(LOG_CRIT,"Compiled without LocalProxy support!\n");
					goto EXIT;
				#endif
			}
		else
			{
				//pRTT=new CARoundTripTime();
				//CAMsg::printMsg(LOG_INFO,"Starting RoundTripTime...\n");
				//if(pRTT->start()!=E_SUCCESS)
				//	{
				//		CAMsg::printMsg(LOG_CRIT,"RoundTripTime Startup FAILED - Exiting!\n");
				//		goto EXIT;
				//	}
				//else
#ifndef ONLY_LOCAL_PROXY
			SINT32 s32MaxSockets=CASocket::getMaxOpenSockets();
			CAMsg::printMsg(LOG_CRIT,"Max Number of sockets we can open: %i\n",s32MaxSockets);
			if(s32MaxSockets>100&&s32MaxSockets<10000)
				{
				CASocket::setMaxNormalSockets(s32MaxSockets-10);
				}
				MONITORING_FIRE_SYS_EVENT(ev_sys_start);
				if(CALibProxytest::getOptions()->isFirstMix())
				{
					CAMsg::printMsg(LOG_INFO,"I am the First MIX...\n");
					#if !defined(NEW_MIX_TYPE)
						pMix=new CAFirstMixA();
					#else
						pMix=new CAFirstMixB();
					#endif
					MONITORING_FIRE_NET_EVENT(ev_net_firstMixInited);
				}
				else if(CALibProxytest::getOptions()->isMiddleMix())
				{
					CAMsg::printMsg(LOG_INFO,"I am a Middle MIX...\n");
					pMix=new CAMiddleMix();
					MONITORING_FIRE_NET_EVENT(ev_net_middleMixInited);
				}
				else
				{
					CAMsg::printMsg(LOG_INFO,"I am the Last MIX...\n");
					#if !defined(NEW_MIX_TYPE)
						pMix=new CALastMixA();
					#else
						pMix=new CALastMixB();
					#endif
					MONITORING_FIRE_NET_EVENT(ev_net_lastMixInited);
				}
#else
				CAMsg::printMsg(LOG_ERR,"this Mix is compile to work only as local proxy!\n");
				goto EXIT;
#endif
			}
#ifndef ONLY_LOCAL_PROXY
#ifndef DYNAMIC_MIX
	  CAMsg::printMsg(LOG_INFO,"Starting MIX...\n");
		if(pMix->start()!=E_SUCCESS)
			CAMsg::printMsg(LOG_CRIT,"Error during MIX-Startup!\n");
#else
    /* LERNGRUPPE */
while(true)
{
	CAMsg::printMsg(LOG_INFO,"Starting MIX...\n");
	if(pMix->start()!=E_SUCCESS)
	{
		/** @todo Hmm, maybe we could remain running, but that may well result in an endless running loop eating the cpu */
		CAMsg::printMsg(LOG_CRIT,"Error during MIX-Startup!\n");
		goto EXIT;
	}

	/* If we got here, the mix should already be reconfigured, so we only need a new instance */
	delete pMix;
	pMix = NULL;

	if(CALibProxytest::getOptions()->isFirstMix())
	{
		CAMsg::printMsg(LOG_INFO,"I am now the First MIX..\n");
#if !defined(NEW_MIX_TYPE)
            pMix=new CAFirstMixA();
#else
            pMix=new CAFirstMixB();
#endif
	}
	else if(CALibProxytest::getOptions()->isMiddleMix())
	{
		CAMsg::printMsg(LOG_INFO,"I am now a Middle MIX..\n");
		pMix=new CAMiddleMix();
	}
	else
	{
		/* Reconfiguration of a last mix?! Not really...*/
		CAMsg::printMsg( LOG_ERR, "Tried to reconfigure a former first/middle-Mix to a LastMix -> impossible!\n");
		goto EXIT;
	}
}
#endif //DYNAMIC_MIX
#endif //ONLY_LOCAL_PROXY
EXIT:
		cleanup();
#if defined(HAVE_CRTDBG)
		_CrtMemCheckpoint( &s2 );
		if ( _CrtMemDifference( &s3, &s1, &s2 ) )
      _CrtMemDumpStatistics( &s3 );
#endif
#ifdef CWDEBUG
		Debug(list_allocations_on(libcw_do));
#endif
		return 0;
	}
