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
// proxytest.cpp : Definiert den Einsprungpunkt für die Konsolenanwendung.
//

#include "StdAfx.h"
#include "CASocket.hpp"
#include "CASocketGroup.hpp"
#include "CASocketAddrINet.hpp"
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

void signal_interrupt( int sig)
	{
		CAMsg::printMsg(LOG_INFO,"Hm.. Strg+C pressed... exiting!\n");
		exit(0);
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

Also a mix could only be part of one and only one cascade. If a mix is not part of a cascade, we call it a free mix.
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
*/
int main(int argc, const char* argv[])
	{
		CASocketAddrINet::init();

		
		if(argc>1)
			{
				CASocket oSocketServer;
				oSocketServer.listen(6789);
				CASocket oS;
				oSocketServer.accept(oS);
				sleep(1000000);
				exit(0);
			}
		else
			{
				CASocket oSocketClient;
				CASocketAddrINet serverAddr;
				serverAddr.setPort(6789);
				oSocketClient.connect(serverAddr);
				UINT32 u;
				SINT32 s;
				u=5000;
				s=oSocketClient.setSendBuff(5000);
				s=oSocketClient.getSendBuff();
				CAMsg::printMsg(LOG_DEBUG,"SendBuff now: %i\n",s);
				s=oSocketClient.getSendSpace();
				CAMsg::printMsg(LOG_DEBUG,"SendSpace now: %i\n",s);
				UINT8 buff[1000];
				for(int i=0;i<1000;i++)
					{
						oSocketClient.send(buff,1000,true);
						s=oSocketClient.getSendSpace();
						CAMsg::printMsg(LOG_DEBUG,"SendSpace now: %i\n",s);
					}
				exit(0);
			}		

		//some test....
		if(MIXPACKET_SIZE!=sizeof(MIXPACKET))
			{
				CAMsg::printMsg(LOG_CRIT,"MIXPACKET_SIZE != sizeof(MUXPACKET) --> maybe a compiler (optimization) problem!\n");
				exit(-1);
			}
		if(sizeof(UINT32)!=4)
			{
				CAMsg::printMsg(LOG_CRIT,"sizeof(UINT32) != 4 --> maybe a compiler (optimization) problem!\n");
				exit(-1);
			}

		if(CAQueue::test()!=E_SUCCESS)
			CAMsg::printMsg(LOG_CRIT,"CAQueue::test() NOT passed! Exiting\n");
		else
			CAMsg::printMsg(LOG_DEBUG,"CAQueue::test() passed!\n");
		
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
//					chdir("/");
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
		signal(SIGINT,signal_interrupt);
		CAMix* pMix=NULL;
		CARoundTripTime* pRTT=NULL;
		if(options.isLocalProxy())
			{
				pMix=new CALocalProxy();
			}
		else
			{
				pRTT=new CARoundTripTime();
				CAMsg::printMsg(LOG_INFO,"Starting RoundTripTime...\n");
				if(pRTT->start()!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"RoundTripTime Startup FAILED - Exiting!\n");
						goto EXIT;
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
			}
	  CAMsg::printMsg(LOG_INFO,"Starting MIX...\n");
		if(pMix->start()!=E_SUCCESS)
			CAMsg::printMsg(LOG_CRIT,"Error during MIX-Startup!\n");
EXIT:
		delete pRTT;
		delete pMix;
		CASocketAddrINet::destroy();
		#ifdef _WIN32		
			WSACleanup();
		#endif
		
		CAMsg::printMsg(LOG_CRIT,"Terminating Programm!\n");
		return 0;
	}
