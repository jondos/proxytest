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
		struct _timeb timebuffer;
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
										sleep(5);
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
		return 0;
		return 0;
	}

SINT32 CARoundTripTime::stop()
	{
		m_bRun=false;
		return 0;
	}