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
#include "StdAfx.h"
#include "CARoundTripTime.hpp"
#include "CASocketAddr.hpp"
#include "CACmdLnOptions.hpp"
#include "CAUtil.hpp"
extern CACmdLnOptions options;

THREAD_RETURN RoundTripTimeLoop(void *p)
	{
		CARoundTripTime* pRTT=(CARoundTripTime*)p;
		CASocketAddr addrNextMix;
		SINT32 tmpPort=options.getTargetRTTPort();
		if(tmpPort==E_UNSPECIFIED)
			tmpPort=options.getTargetPort()+1;
		UINT8* buff=new UINT8[4096];
		options.getTargetHost(buff,4096);
		addrNextMix.setAddr((char*)buff,tmpPort);
		tmpPort=options.getServerRTTPort();
		if(tmpPort==E_UNSPECIFIED)
			tmpPort=options.getServerPort()+1;
		CADatagramSocket oSocket;
		oSocket.bind(tmpPort);
		UINT8 localPortAndIP[6];
		CASocketAddr::getLocalHostIP(localPortAndIP);
		tmpPort=htons(tmpPort);
		memcpy(localPortAndIP+4,&tmpPort,2);
		CASocketAddr to;
		to.sin_family=AF_INET;
		BIGNUM* bnTmp=BN_new();
		BIGNUM* bnTmp2=BN_new();
		memset(buff,0,4);
		while(pRTT->getRun())
			{
				SINT32 len=oSocket.receive(buff+8,4096-8,NULL);
				if(len!=SOCKET_ERROR)
					{
						if(!options.isLastMix()) //what to do if not last Mix....
							{
								getcurrentTimeMillis(bnTmp);
								if((buff[8]&0x80)!=0) // Hinweg...
									{
										int bnSize=BN_num_bytes(bnTmp);
										memset(buff+8+len,0,4); //clearing first '0' bits....
										BN_bn2bin(bnTmp,buff+8+len+8-bnSize); //inserting Timestamp....
										len+=8;
										memcpy(buff+8+len,localPortAndIP,6); //inserting (return) Port and IP
										len+=6;
										oSocket.send(buff+8,len,&addrNextMix);
									}
								else //Rueckweg
									{
										BN_bin2bn(buff+8+len-8,8,bnTmp2);
										BN_sub(bnTmp,bnTmp,bnTmp2);
										int bnSize=BN_num_bytes(bnTmp);
										memset(buff+4,0,8); //clearing first '0' bits....
										BN_bn2bin(bnTmp,buff+4+8-bnSize); //inserting Timestamp....										
										memcpy(&to.sin_addr,buff+len-6,4);
										memcpy(&to.sin_port,buff+len-2,2);
										len-=6; // Removed return IP/Port
										oSocket.send(buff,len,&to);
									}
							}
						else //what to do if last mix...
							{
								memcpy(&to.sin_addr,buff+8+len-6,4);
								memcpy(&to.sin_port,buff+8+len-2,2);
								len-=6; // Removed return IP/Port
								buff[8]=0; // Cleared Header...
								oSocket.send(buff+8,len,&to);
							}
					}
			}
		BN_free(bnTmp);
		BN_free(bnTmp2);
		delete buff;
		THREAD_RETURN_SUCCESS;
	}

SINT32 CARoundTripTime::start()
	{
		m_bRun=true;
		#ifdef _WIN32
		 _beginthread(RoundTripTimeLoop,0,this);
		#else
		 pthread_t othread;
		 pthread_create(&othread,NULL,RoundTripTimeLoop,this);
		#endif
		return E_SUCCESS;
	}

SINT32 CARoundTripTime::stop()
	{
		m_bRun=false;
		return E_SUCCESS;
	}