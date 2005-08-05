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
#include "CAMix.hpp"
#include "CAMiddleMix.hpp"
#include "TypeA/CAFirstMixA.hpp"
#include "TypeA/CALastMixA.hpp"
#include "CALocalProxy.hpp"
#include "xml/DOM_Output.hpp"
#ifdef LOG_CRIME
#include "tre/regex.h"
#endif
#ifdef NEW_MIX_TYPE
	#include "TypeB/CAFirstMixB.hpp"
	#include "TypeB/CALastMixB.hpp"
#endif
#include "CALogPacketStats.hpp"
//#include "CAPayment.hpp"
//#ifdef _WIN32
//HANDLE hEventThreadEnde;
//#endif
CACmdLnOptions options;

//Global Locks required by OpenSSL-Library
CAMutex* pOpenSSLMutexes;

// The Mix....
CAMix* pMix=NULL;


typedef struct
{
	unsigned short len;
	time_t time;
} log;

#ifndef _WIN32
	#ifdef _DEBUG
		void signal_broken_pipe( int sig)
			{
				CAMsg::printMsg(LOG_WARNING,"Hm.. Broken Pipe.... How cares!\n");
				signal(SIGPIPE,signal_broken_pipe);
			}
	#endif
#endif

void removePidFile()
	{
		UINT8 strPidFile[512];
		if(options.getPidFile(strPidFile,512)==E_SUCCESS)
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

void signal_term( int )
	{
		CAMsg::printMsg(LOG_INFO,"Hm.. Signal SIG_TERM received... exiting!\n");
		removePidFile();
		exit(0);
	}

void signal_interrupt( int)
	{
		CAMsg::printMsg(LOG_INFO,"Hm.. Strg+C pressed... exiting!\n");
		removePidFile();
		exit(0);
	}

void signal_hup(int)
	{
		options.reread(pMix);
	}


///Callbackfunction for looking required by OpenSSL
void openssl_locking_callback(int mode, int type, char * /*file*/, int /*line*/)
	{
		if (mode & CRYPTO_LOCK)
			{
				pOpenSSLMutexes[type].lock();
			}
		else
			{
				pOpenSSLMutexes[type].unlock();
			}
	}

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

/**do necessary initialisations of libraries etc.*/
void init()
	{
		XMLPlatformUtils::Initialize();
		OpenSSL_add_all_algorithms();
		pOpenSSLMutexes=new CAMutex[CRYPTO_num_locks()];
		CRYPTO_set_locking_callback((void (*)(int,int,const char *,int))openssl_locking_callback);
		CAMsg::init();
		CASocketAddrINet::init();
		//startup
		#ifdef _WIN32
			int err=0;
			WSADATA wsadata;
			err=WSAStartup(0x0202,&wsadata);
		#endif
		initRandom();
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
The <Nonce/> is a random byte string of length=16 (chosen by Mix 3) used for replay detection.

\li Step 4: Mix 2 generates and sends a symmetric key to Mix 3, encrypted with the public key
of Mix 3, signed by Mix 2.
This key is used for inter-mix encryption and is actually a 64 byte octet stream.
See \ref XMLInterMixInitAnswer "[XML]" for a description of the XML struct send in this step.\n
The first 16 bytes a used as a key for
inter-mix encryption of packets send from Mix \e n-1 to Mix \e n (e.g. upstream direction).
The next 16 byte are used as IV for this cipher. The next 16 bytes are used as
key for downstream direction and the last 16 bytes are the IV for this cipher.
To detect replay attacks Mix 3 checks if the <Nonce/> element sent from Mix 2 is equal to SHA1(<Nonce/> chosen by Mix 3 in Step 3).

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
		pMix=NULL;
		int i;
		SINT32 maxFiles;
		init();
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
#endif

//Switch on debug infos
#ifdef CWDEBUG
Debug(libcw_do.on());
Debug(dc::malloc.on());
#endif
			//some test....
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
			testTre();
#endif

#ifdef _DEBUG
			UINT32 start;
#endif

		//temp
		/*for(;;){
		CAQueue* pQueue=new CAQueue(MIXPACKET_SIZE);
		UINT8 buff3[MIXPACKET_SIZE];
		UINT64 t1,t2;
		getcurrentTimeMicros(t1);
		int h=1000;
		for(int k=0;k<1000;k++)
		{
		for(int i=0;i<h;i++)
			pQueue->add(buff3,MIXPACKET_SIZE);
		UINT32 le=MIXPACKET_SIZE;
		for(int i=0;i<h;i++)
				pQueue->get(buff3,&le);
				}
		getcurrentTimeMicros(t2);
		printf("Queue Time: %u �s\n",diff64(t2,t1));
		delete pQueue;
		UINT8 buff23[MIXPACKET_SIZE*h];
		UINT32 aktIndex=0;
		getcurrentTimeMicros(t1);
		for(int k=0;k<1000;k++)
		{
		aktIndex=0;
		for(int i=0;i<h;i++)
		{
			memcpy(buff23+aktIndex,buff3,MIXPACKET_SIZE);
			aktIndex+=MIXPACKET_SIZE;
			}
		for(int i=0;i<h;i++)
		{
			aktIndex-=MIXPACKET_SIZE;
				memcpy(buff3,buff23+aktIndex,MIXPACKET_SIZE);
				}
				}
		getcurrentTimeMicros(t2);
		printf("Array Time: %u �s\n",diff64(t2,t1));
		}
		
		
		exit(0);*/		
		//end temp
		if(options.parse(argc,argv) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,"Error: Cannot parse configuration file!\n");
			goto EXIT;
		}

		if(!(options.isFirstMix()||options.isMiddleMix()||options.isLastMix()||options.isLocalProxy()))
			{
				CAMsg::printMsg(LOG_CRIT,"You must specifiy, which kind of Mix you want to run!\n");
				CAMsg::printMsg(LOG_CRIT,"Use -j or -c\n");
				CAMsg::printMsg(LOG_CRIT,"Or try --help for more options.\n");
				CAMsg::printMsg(LOG_CRIT,"Exiting...\n");
				goto EXIT;
			}



#ifdef _DEBUG
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
		for(i=0;i<10;i++)
			msSleep(200);
		start=time(NULL)-start;
		CAMsg::printMsg(LOG_DEBUG,"done! Takes %u seconds\n",start);
		//end Testin msSleep

#endif

#ifdef HAVE_CRTDBG
		_CrtMemCheckpoint( &s1 );
#endif
		UINT8 buff[255];

#ifndef WIN32
		maxFiles=options.getMaxOpenFiles();
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
		if(options.getUser(buff,255)==E_SUCCESS) //switching user
			{
				struct passwd* pwd=getpwnam((char*)buff);
				if(pwd==NULL||seteuid(pwd->pw_uid)==-1)
					CAMsg::printMsg(LOG_ERR,"Could not switch to effective user %s!\n",buff);
				else
					CAMsg::printMsg(LOG_INFO,"Switched to effective user %s!\n",buff);
			}
		if(geteuid()==0)
			CAMsg::printMsg(LOG_INFO,"Warning - Running as root!\n");
#endif
		if(options.getDaemon())
			{
				CAMsg::printMsg(LOG_DEBUG,"starting as daemon\n");
				#ifndef _WIN32
					if(options.getLogDir(buff,255)==E_SUCCESS)
						{
							if(options.getCompressLogs())
								CAMsg::setLogOptions(MSG_COMPRESSED_FILE);
							else
								CAMsg::setLogOptions(MSG_FILE);
						}
					else
						CAMsg::setLogOptions(MSG_LOG);
					SINT32 ret=CAMsg::openEncryptedLog();
					#ifdef LOG_CRIME
					if(ret!=E_SUCCESS)
							{
								if(options.isEncryptedLogEnabled())
									{
										CAMsg::printMsg(LOG_ERR,"Could not open encrypted log - exiting!\n");
										exit(EXIT_FAILURE);
									}
								else
									options.enableEncryptedLog(false);
							}
					#endif
					pid_t pid;
					CAMsg::printMsg(LOG_DEBUG,"daemon - before fork()\n");
					pid=fork();
					if(pid!=0)
						{
							CAMsg::printMsg(LOG_DEBUG,"Exiting parent!\n");
							exit(EXIT_SUCCESS);
						}		
					CAMsg::printMsg(LOG_DEBUG,"child after fork...\n");
					setsid();
					#ifndef DO_TRACE
					chdir("/");
					umask(0);
					#endif
				#endif
			}
		else
			{
				if(options.getLogDir((UINT8*)buff,255)==E_SUCCESS)
					{
						if(options.getCompressLogs())
							CAMsg::setLogOptions(MSG_COMPRESSED_FILE);
						else
							CAMsg::setLogOptions(MSG_FILE);
					}
				SINT32 ret=CAMsg::openEncryptedLog();
				#ifdef LOG_CRIME
					if(ret!=E_SUCCESS)
							{
								if(options.isEncryptedLogEnabled())
									{
										CAMsg::printMsg(LOG_ERR,"Could not open encrypted log - exiting!\n");
										exit(EXIT_FAILURE);
									}
								else
									options.enableEncryptedLog(false);
							}
#endif
			}
//			CAMsg::printMsg(LOG_ENCRYPTED,"Test: Anon proxy started!\n");
//			CAMsg::printMsg(LOG_ENCRYPTED,"Test2: Anon proxy started!\n");
//			CAMsg::printMsg(LOG_ENCRYPTED,"Test3: Anon proxy started!\n");

		CAMsg::printMsg(LOG_INFO,"Anon proxy started!\n");
		CAMsg::printMsg(LOG_INFO,MIX_VERSION_INFO);
#ifdef _DEBUG
		sockets=0;
#endif


#ifndef _WIN32
	#ifdef _DEBUG
			signal(SIGPIPE,signal_broken_pipe);
	#else
			signal(SIGPIPE,SIG_IGN);
	#endif
		struct sigaction newAction;
		newAction.sa_handler=signal_hup;
		newAction.sa_flags=0;
		sigaction(SIGHUP,&newAction,NULL);
		//signal(SIGHUP,signal_hup);
#endif
		signal(SIGINT,signal_interrupt);
		signal(SIGTERM,signal_term);

		//Try to write pidfile....
		UINT8 strPidFile[512];
		if(options.getPidFile(strPidFile,512)==E_SUCCESS)
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
						CAMsg::printMsg(LOG_CRIT,"Couldt not write pidfile - exiting!\n");
						exit(EXIT_FAILURE);
					}
				close(hFile);
#ifndef _WIN32
				seteuid(old_uid);
#endif
			}

//		CARoundTripTime* pRTT=NULL;
		if(options.isLocalProxy())
			{
				#ifndef NEW_MIX_TYPE
					pMix=new CALocalProxy();
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
				if(options.isFirstMix())
					{
						CAMsg::printMsg(LOG_INFO,"I am the First MIX..\n");
						#if !defined(NEW_MIX_TYPE)
							pMix=new CAFirstMixA();
						#else
							pMix=new CAFirstMixB();
						#endif
					}
				else if(options.isMiddleMix())
					{
						CAMsg::printMsg(LOG_INFO,"I am a Middle MIX..\n");
						pMix=new CAMiddleMix();
					}
				else
						#if !defined(NEW_MIX_TYPE)
							pMix=new CALastMixA();
						#else
							pMix=new CALastMixB();
						#endif
			}
	  CAMsg::printMsg(LOG_INFO,"Starting MIX...\n");
		if(pMix->start()!=E_SUCCESS)
			CAMsg::printMsg(LOG_CRIT,"Error during MIX-Startup!\n");
EXIT:
//		delete pRTT;
		if(pMix!=NULL)
			delete pMix;
//		CASocketAddrINet::destroy();
		#ifdef _WIN32
			WSACleanup();
		#endif
		removePidFile();
		options.clean();
//OpenSSL Cleanup
		CRYPTO_set_locking_callback(NULL);
		delete []pOpenSSLMutexes;
//XML Cleanup
		//Note: We have to destroy all XML Objects and all objects that uses XML Objects BEFORE
		//we terminate the XML lib!
		XMLPlatformUtils::Terminate();
		CAMsg::printMsg(LOG_CRIT,"Terminating Programm!\n");
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
