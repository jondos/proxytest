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
#include "CASocket.hpp"
#include "CASocketGroup.hpp"
#include "CASocketAddrINet.hpp"
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"
#include "CAMuxSocket.hpp"
#include "CASocketList.hpp"
#include "CASymCipher.hpp"
#include "CAASymCipher.hpp"
#include "CAInfoService.hpp"
#include "CASignature.hpp"
#include "CADatagramSocket.hpp"
//#include "CARoundTripTime.hpp"
#include "CAUtil.hpp"
#include "CAMix.hpp"
#include "CAMiddleMix.hpp"
#include "CAFirstMix.hpp"
#include "CALastMix.hpp"
#include "CALocalProxy.hpp"
#include "CASymCipher.hpp"
#include "CABase64.hpp"
#include "CADatabase.hpp"
#include "CACertificate.hpp"
#include "CACertStore.hpp"
#include "CALastMixChannelList.hpp"
#include "xml/DOM_Output.hpp"
#ifdef LOG_CRIME
#include "tre/regex.h"
#endif
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

void signal_term( int sig)
	{
		CAMsg::printMsg(LOG_INFO,"Hm.. Signal SIG_TERM received... exiting!\n");
		exit(0);
	}

void signal_interrupt( int sig)
	{
		CAMsg::printMsg(LOG_INFO,"Hm.. Strg+C pressed... exiting!\n");
		exit(0);
	}

void signal_hup(int sig)
	{
		options.reread(pMix);
	}


//Callbackfunction for looking required by OpenSSL
void openssl_locking_callback(int mode, int type, char *file, int line)
	{
/*#if _DEBUG
		CAMsg::printMsg(LOG_DEBUG,"OpenSSL-Locking: thread=%4d mode=%s lock=%s %s:%d\n",
			CRYPTO_thread_id(),
			(mode&CRYPTO_LOCK)?"l":"u",
			(type&CRYPTO_READ)?"r":"w",file,line);
#endif*/
		if (mode & CRYPTO_LOCK)
			{
				pOpenSSLMutexes[type].lock();
			}
		else
			{
				pOpenSSLMutexes[type].unlock();
			}
	}

/** \mainpage 
 
\section docCommProto Description of the system and communication protocol

\subsection Basics

The whole system consist of JAP's, mix-servers, cache proxies and the InfoService. (see Figure 1)

\image html JAPArchitecture.gif "Figure 1: The Architecture of the Anonymous-Service"
	
The local proxy (JAP) and the mix servers communicate using TCP/IP-Internet connections.
 Each JAP has one and only one connection to a mix server. A mix server has one and only one connection
to one or two other mixes. If a mix receives data packets from JAP's and sends them to an other mix, we call
 them a \em first mix. So a first mix has exactly one connection to an other mix. If a mix receives packets from
 a mix and sends the data to the cache proxy, we call them the \em last mix. Consequently a last
 mix has only one connection to an other mix. Each mix with two connections to other mixes is called
 a \em middle mix. This type of mix will receive packets form one mix and forwards them to the other mix.

If mixes are connected in a meanigful way a chain is setup up, so what packets are transmitted from JAP's 
to the cache-proxies and than to the Internet.
A chain of mixes is called a \em mix-cascade (or \em cascade for short).
Many different cascades could exist at the same time, but JAP can select one and only one at the same time.

Also a mix could only be part of one and only one cascade. If a mix is not part of a cascade, we call it a \em free mix.
Free mixes are not useable for JAP, but could be connected to build a new cascade.
  
\subsection docMux Mulitplexing and Demultiplexing

JAP acts as a local proxy for the browser. 
The browser opens many connections to the JAP (usually one per HTTP-Request).
All this connections are multiplexed over the one connection to the first mix JAP is connected to.
Every connection from the browser is called a \em channel (mix channel or anonymous channel). 
A first mix sends the stream of packets (from multiple channels) from multiple users to the next mix.
All over one TCP/IP connection. So JAP and the mixes have to multiplex/demultiplex the channels.
A channel can transport only fixed sized packets. These packets are called \em mix-packets. 
So a first mix for instance gets many packets from different users in parallel 
and sends them to the next mix in a serialized way. (see Figure 2)

\image html JAPMixPacketMux.gif "Figure 2: Mux/DeMux of fixed sized mix-packets"


Each mix-packet has a size of #MIXPACKET_SIZE (998) bytes. 
The header of each packet is as follows (see also: #MIXPACKET):
\li 4 bytes \c channel-id
\li 2 bytes \c flags
\li #DATA_SIZE (992) bytes  \c content (transported bytes)

The content bytes have different meanings in different situations and mixes.
For instance the content itself is meaningful only at the last mix (because of multiple encryptions)
The format of the content (at the last mix) is:
\li 2 bytes \c len
\li 1 byte \c type
\li #PAYLOAD_SIZE (989) bytes \c payload

They payload are the bytes what should be transported from the browser to the Internet 
or back via the anonymous channel.

The channel-id is changed in every mix. Also the content bytes changes in every mix, 
because each mix will perform a single encryption/decryption.

\subsection docInterMixEncryption Inter-Mix Encryption
The stream of mix-packets between to mixes is encrypted using AES-128/128 in OFB-128 mode. Exactly only the
first part of each mix-packet is encrpyted. (see Figure 3)

\image html JAPInterMixEncryption.gif "Figure 3: Encryption between two mixes"
	
The encryption is done, so that an attacker could not see the channel-id and flags of a mix-packet. OFB is chosen, so that
an attacker could not replay a mix-packed. If he replays a mix-packed, than at least the channel-id 
changes after decrypting. If this mix-packed was the first packed of a channel, than the cryptographic keys
for this channel will change to (because of the content format for the first packed of a channel, explained later). 

For Upstream and Downstream different keys are used.

\section docCascadeInit Initialisation of a Cascade

\image html JAPCascadeInit.gif "Figure 4: Steps during Cascade initialisation (3 Mix example)

Fiugre 4 shows the steps which take place, than a Cascade starts. These steps are illustrated using a 3 Mix example.
The procedure is as follows:

\li Step 1: Each Mix starts, reads its configuration file (including own key
 for digital signature and test keys from previous and next mix) and generates a 
 public key pair of the asymmetric crpyto system used by the mix for encryption 
 (currently this is RSA).

\li Step 2: Mix 2 establishes a TCP/IP connection with Mix 3. 
No data is transmitted during this step.

\li Step 3: Mix 3 sends its public key  to Mix 2 (signed with its signature key).
See \ref XMLInterMixInitSendFromLast "[XML]" for a description of the XML struct send in this step. 

\li Step 4: Mix 2 generates and sends a symmetric key to Mix 3, encrypted with the public key 
of Mix 3, signed by Mix 2. 
This key is used for inter-mix encryption and is actually a 64 byte octet stream.
See \ref XMLInterMixInitAnswer "[XML]" for a description of the XML struct send in this step.\n  
The first 16 bytes a used as a key for 
inter-mix encryption of packets send from Mix \e n-1 to Mix \e n (e.g. upstream direction).
The next 16 byte are used as IV for this cipher. The next 16 bytes are used as
key for downstream direction and the last 16 bytes are the IV for this cipher.

\li Step 5,6,7: This steps are equal to step 2,3,4. Mix 1 establishes a TCP/IP-connection
with Mix 2. Mix 2 send it publick key to Mix 1 and Mix 1 generates and sends 
a symmetric key to Mix 2.

\section docMixInfoService Communication between Mix and InfoService

\image html JAPMixInfoService.gif "Figure 5: Communication between Mix and InfoService"

\li 1. HELO-Message send from each mix to the InfoService every 10 minutes to announce itself.  
See \ref XMLMixHELO "[XML]" for a description of the XML struct send. 

\li 2. Status-Message send from the FirstMix to the InfoService every minute to update the current
status of the Mix cascade including Number of Users, Traffic situation etc.
See \ref XMLMixCascadeStatus "[XML]" for a description of the XML struct send. 

\section docMixJap Communication between Mix and JAP

\image html JAPMixJap.gif "Figure 6: Communication between Mix and JAP"

\li 1. JAP opens a TCP/IP connection to a FirstMix

\li 2. FirstMix sends information about the cascade (including public keys of the Mixes) to the JAP.
See \ref XMLMixKeyInfo "[XML]" for a description of the XML struct send. 

\li 3. JAP sends a special MixPacket, containing only 2 symmetric keys encrpyted with the
public key of the FirstMix. This keys are used for link encryption between JAP and FirstMix.
Note: At the moment this is binary - but will use XML in the future.

\li 4. Normal MixPacket exchange according to the mix protocol.
*/


int main(int argc, const char* argv[])
	{		
	/*	CAQueue oQ;
		UINT32 l,b;
		l=4;
		SINT32 r=oQ.getOrWait((UINT8*)&b,&l,5000);
		oQ.add(&b,4);
		r=oQ.getOrWait((UINT8*)&b,&l,5000);
		r=oQ.getOrWait((UINT8*)&b,&l,5000);
		*/
//		UINT16 timestamp;
//		currentTimestamp((UINT8*)&timestamp);
		pMix=NULL;

		int i;
		UINT32 start;
		SINT32 maxFiles;
		//Setup Routines
		XMLPlatformUtils::Initialize();	
		OpenSSL_add_all_algorithms();
		pOpenSSLMutexes=new CAMutex[CRYPTO_num_locks()];
		CRYPTO_set_locking_callback((void (*)(int,int,const char *,int))openssl_locking_callback);	

	/*		UINT8 buffz[162];
			memset(buffz,65,162);
			buffz[76]=10;
			buffz[153]=10;
			UINT8 out[180];
			UINT32 outlen=180;
			CABase64::decode(buffz,162,out,&outlen);
			int z=3;
	*//*		CAPayment oPayment;
			oPayment.init((UINT8*)"dud14.inf.tu-dresden.de",3306,(UINT8*)"payment",(UINT8*)"payment");
			UINT32 accessUntil;
			SINT32 ret=oPayment.checkAccess((UINT8*)"payer1",5,&accessUntil);
			ret=oPayment.checkAccess((UINT8*)"payer2",5,&accessUntil);
			exit(0);
	*/		
			/*			CAASymCipher oRsa;
			oRsa.generateKeyPair(1024);
			UINT8 buff1[1024];
			UINT32 len=1024;
			oRsa.getPublicKeyAsXML(buff1,&len);
	*/		/*	
		if(argc>1)
			{
				CASocket oSocketServer;
				oSocketServer.create(AF_INET);
				//oSocketServer.setRecvBuff(15000);
				//SINT32 j=oSocketServer.getRecvBuff();
				//CAMsg::printMsg(LOG_DEBUG,"Recv-Buff %i\n",j);
				oSocketServer.listen(6789);
				CASocket oS;
				
				oSocketServer.accept(oS);
				//oS.setRecvBuff(1000);
				//j=oS.getRecvBuff();
				//CAMsg::printMsg(LOG_DEBUG,"Recv-Buff %i\n",j);
				oS.setNonBlocking(true);
				for(;;)
					{
						UINT8 buff[21];
						int ret=oS.receive(buff,20);
						if(ret!=E_AGAIN)
							{
								buff[ret]=0;
								printf((char*)buff);
							}
						else
							printf("E_AGAIN\n!");
						sleep(1);
					}
				exit(0);
			}
		else
			{
				CASocket oSocketClient;
				oSocketClient.create(AF_INET);
				SINT32 s=oSocketClient.setSendBuff(15000);

				CASocketAddrINet serverAddr;
				serverAddr.setPort(6789);
				oSocketClient.connect(serverAddr);
				sleep(20);
				UINT8 buff[20];
				memset(buff,'A',20);
				oSocketClient.send(buff,20);
				sleep(20);
				memset(buff,'B',20);
				oSocketClient.send(buff,20);
*//*				UINT32 u;
				u=5000;
			//	s=oSocketClient.setSendBuff(5000);
				s=oSocketClient.getSendBuff();
				CAMsg::printMsg(LOG_DEBUG,"SendBuff now: %i\n",s);
				s=oSocketClient.getSendSpace();
				CAMsg::printMsg(LOG_DEBUG,"SendSpace now: %i\n",s);
				UINT8 buff[1000];
				for(int i=0;i<1000;i++)
					{
						s=oSocketClient.send(buff,1000,true);
						CAMsg::printMsg(LOG_DEBUG,"Has Send: %i\n",s);
						s=oSocketClient.getSendSpace();
						CAMsg::printMsg(LOG_DEBUG,"SendSpace now: %i\n",s);
					}

				sleep(100000);
				exit(0);
			}		
*/
#if defined(_WIN32) && defined(_DEBUG)
/*			_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
			_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
			_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
			_CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT );
			_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
			_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDOUT );
*/
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
					exit(-1);
				CAMsg::printMsg(LOG_CRIT,"Hm, The Offsets seams to be ok - so we try to continue - hope that works...\n");
			}
		if(sizeof(UINT32)!=4)
			{
				CAMsg::printMsg(LOG_CRIT,"sizeof(UINT32) != 4 --> maybe a compiler (optimization) problem!\n");
				exit(-1);
			}
				
#ifdef LOG_CRIME
			testTre();
#endif
		//startup
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
		if(!(options.isFirstMix()||options.isMiddleMix()||options.isLastMix()||options.isLocalProxy()))
			{
				CAMsg::printMsg(LOG_CRIT,"You must specifiy, which kind of Mix you want to run!\n");
				CAMsg::printMsg(LOG_CRIT,"Use -j or -c\n");
				CAMsg::printMsg(LOG_CRIT,"Or try --help for more options.\n");
				CAMsg::printMsg(LOG_CRIT,"Exiting...\n");
				goto EXIT;
			}
			
/*		UINT8 buff1[1024];
		UINT32 len=1024;
		oRsa.getPublicKeyAsXML(buff1,&len);
		CASignature oSignature;
		oSignature.generateSignKey(1024);
		UINT8 out[2048];
		UINT32 outlen=2048;
		oSignature.signXML(buff1,len,out,&outlen);
		SINT32 r=oSignature.verifyXML(out,outlen);
		r=r;
	*/
	/*	int handle=open("jap.pfx",O_BINARY|O_RDONLY);
		UINT32 size=filelength(handle);
		UINT8* bg=new UINT8[2048];
		read(handle,bg,size);
		close(handle);
		CASignature oSignature;
		oSignature.setSignKey(bg,size,SIGKEY_PKCS12,"pass");
		CACertificate* cert=CACertificate::decode(bg,size,CERT_PKCS12,"pass");
		*//*
		int handle=open("jap.cer",O_BINARY|O_RDONLY);
		UINT32 size=filelength(handle);
		UINT8* bg=new UINT8[2048];
		read(handle,bg,size);
		close(handle);
		CACertificate* cert=CACertificate::decode(bg,size,DER);
		CACertStore oCerts;
		oCerts.add(cert);
		delete cert;
		handle=open("mix.cer",O_BINARY|O_RDONLY);
		size=filelength(handle);
		read(handle,bg,size);
		close(handle);
		cert=CACertificate::decode(bg,size,DER);
		oCerts.add(cert);
		CASignature oSignature;
		oSignature.generateSignKey(1024);
		UINT8 in[255];
		strcpy((char*)in,"<Test><Value>1</Value></Test>");
		UINT8 out[20480];
		UINT32 outlen=20480;
		oSignature.signXML(in,strlen((char*)in),out,&outlen,&oCerts);
		handle=open("text.xml",O_CREAT|O_BINARY|O_RDWR,_S_IWRITE);
		write(handle,"<?xml version=\"1.0\"?>",21);
		write(handle,out,outlen);
		close(handle);
*/
#ifdef _DEBUG
		//		CADatabase::test();
//		if(CAQueue::test()!=E_SUCCESS)
//			CAMsg::printMsg(LOG_CRIT,"CAQueue::test() NOT passed! Exiting\n");
//		else
//			CAMsg::printMsg(LOG_DEBUG,"CAQueue::test() passed!\n");

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
#ifdef _WIN32
		_CrtMemCheckpoint( &s1 );
#endif
		UINT8 buff[255];

#ifndef WIN32
		maxFiles=options.getMaxOpenFiles();
		if(maxFiles>0)
			{
				struct rlimit lim;
				/* Set the new MAX open files limit */
				lim.rlim_cur = lim.rlim_max = maxFiles;
				if (setrlimit(RLIMIT_NOFILE, &lim) != 0) 
					{
						CAMsg::printMsg(LOG_CRIT,"Could not set MAX open files to: %u -- Exiting!\n",maxFiles);
						exit(1);
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
					CAMsg::openSpecialLog();
					pid_t pid;
					pid=fork();
					if(pid!=0)
						exit(0);
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
				CAMsg::openSpecialLog();
			}
		CAMsg::printMsg(LOG_INFO,"Anon proxy started!\n");
		CAMsg::printMsg(LOG_INFO,"Version: %s\n",MIX_VERSION);
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
			signal(SIGHUP,signal_hup);
#endif
		signal(SIGINT,signal_interrupt);
		signal(SIGTERM,signal_term);
//		CARoundTripTime* pRTT=NULL;
		if(options.isLocalProxy())
			{
				pMix=new CALocalProxy();
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
						pMix=new CAFirstMix();
					}
				else if(options.isMiddleMix())
					{
						CAMsg::printMsg(LOG_INFO,"I am a Middle MIX..\n");
						pMix=new CAMiddleMix();
					}
				else
					pMix=new CALastMix();
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
//OpenSSL Cleanup
		CRYPTO_set_locking_callback(NULL);	
		delete []pOpenSSLMutexes;
//XML Cleanup
		XMLPlatformUtils::Terminate();
		CAMsg::printMsg(LOG_CRIT,"Terminating Programm!\n");
#if defined(_WIN32) && defined(_DEBUG)
		_CrtMemCheckpoint( &s2 );
		if ( _CrtMemDifference( &s3, &s1, &s2 ) )
      _CrtMemDumpStatistics( &s3 );
#endif
#ifdef CWDEBUG
Debug(list_allocations_on(libcw_do));
#endif
		return 0;
	}
