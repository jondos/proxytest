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
#include "CAInfoService.hpp"
#include "CASocket.hpp"
#include "xml/xmloutput.h"
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"
#include "CASocketAddrINet.hpp"
#include "CAUtil.hpp"
extern CACmdLnOptions options;



THREAD_RETURN InfoLoop(void *p)
	{
		CAInfoService* pInfoService=(CAInfoService*)p;
		int helocount=10;

		while(pInfoService->getRun())
			{
				pInfoService->sendStatus();
				if(helocount==0)
					{
						pInfoService->sendHelo();
						helocount=10;
					}
				else 
				 helocount--;
				sSleep(60);
			}
		THREAD_RETURN_SUCCESS;
	}

CAInfoService::CAInfoService()
	{
		m_pFirstMix=NULL;
		m_bRun=false;
		m_pSignature=NULL;
	}

CAInfoService::CAInfoService(CAFirstMix* pFirstMix)
	{
		m_pFirstMix=pFirstMix;
		m_bRun=false;
		m_pSignature=NULL;
	}

CAInfoService::~CAInfoService()
	{
		stop();
	}

SINT32 CAInfoService::setSignature(CASignature* pSig)
	{
		m_pSignature=pSig;
		return E_SUCCESS;
	}

SINT32 CAInfoService::getLevel(SINT32* puser,SINT32* prisk,SINT32* ptraffic)
	{
		return m_pFirstMix->getLevel(puser,prisk,ptraffic);
	}

SINT32 CAInfoService::getMixedPackets(UINT32* ppackets)
	{
		if(m_pFirstMix!=NULL)
			return m_pFirstMix->getMixedPackets(ppackets);
		return E_UNKNOWN;
	}

SINT32 CAInfoService::start()
	{
		if(m_pSignature==NULL)
			return E_UNKNOWN;
		m_bRun=true;
		m_lastMixedPackets=0;
		m_minuts=1;
		m_threadRunLoop.setMainLoop(InfoLoop);
		return m_threadRunLoop.start(this);
	}

SINT32 CAInfoService::stop()
	{
		if(m_bRun)
			{
				m_bRun=false;
				m_threadRunLoop.join();
			}
		return E_SUCCESS;
	}

SINT32 CAInfoService::sendStatus()
	{
		if(!options.isFirstMix())
			return E_SUCCESS;
		CASocket oSocket;
		CASocketAddrINet oAddr;
		UINT8 hostname[255];
		UINT8 buffHeader[255];
		SINT32 tmpUser,tmpRisk,tmpTraffic;
		UINT32 tmpPackets;
		if(options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			return E_UNKNOWN;
		oAddr.setAddr(hostname,options.getInfoServerPort());
		if(oSocket.connect(oAddr)==E_SUCCESS)
			{
				BufferOutputStream oBufferStream(1024,1024);
				XMLOutput oxmlOut(oBufferStream);
				oBufferStream.reset();
				oxmlOut.BeginDocument("1.0","UTF-8",true);
				oxmlOut.BeginElementAttrs("MixCascadeStatus");
				UINT8 strMixId[255];
				options.getMixId(strMixId,255);
				oxmlOut.WriteAttr("id",(char*)strMixId);
				tmpUser=tmpPackets=tmpRisk=tmpTraffic=-1;
				getLevel(&tmpUser,&tmpRisk,&tmpTraffic);
				getMixedPackets((UINT32*)&tmpPackets);
				UINT32 avgTraffic=tmpPackets/m_minuts;
				m_minuts++;
				UINT32 diffTraffic=tmpPackets-m_lastMixedPackets;
				if(avgTraffic==0)
					{
						if(diffTraffic==0)
							tmpTraffic=0;
						else
							tmpTraffic=100;
					}
				else
					{
						double dTmp=(double)diffTraffic/(double)avgTraffic;
						tmpTraffic=min(SINT32(50.*dTmp),100);
					}
				m_lastMixedPackets=tmpPackets;
			
				oxmlOut.WriteAttr("nrOfActiveUsers",(int)tmpUser);
				oxmlOut.WriteAttr("currentRisk",(int)tmpRisk);
				oxmlOut.WriteAttr("trafficSituation",(int)tmpTraffic);
				oxmlOut.WriteAttr("mixedPackets",(int)tmpPackets);
				oxmlOut.EndAttrs();
				oxmlOut.EndElement();
				oxmlOut.EndDocument();
				UINT32 buffLen=1024;
				UINT8* buff=new UINT8[buffLen];
				m_pSignature->signXML(oBufferStream.getBuff(),oBufferStream.getBufferSize(),(UINT8*)buff,&buffLen);
				sprintf((char*)buffHeader,"POST /feedback HTTP/1.0\r\nContent-Length: %u\r\n\r\n",buffLen);
				oSocket.send(buffHeader,strlen((char*)buffHeader));
				oSocket.send(buff,buffLen);
				delete[] buff;
			}
		oSocket.close();	
		return E_SUCCESS;
	}

SINT32 CAInfoService::sendHelo()
	{
		CASocket oSocket;
		CASocketAddrINet oAddr;
		UINT8 hostname[255];
		UINT8 buffHeader[255];
		if(options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			return E_UNKNOWN;
		oAddr.setAddr(hostname,options.getInfoServerPort());
		if(oSocket.connect(oAddr)==E_SUCCESS)
			{
				UINT8* buff=new UINT8[2024];
				if(buff==NULL)
					return E_UNKNOWN;
				UINT32 buffLen;
				if(options.isFirstMix())
					{
					 //the following has to be moved to FirstMix...
						UINT8* tmpBuff=new UINT8[2024];
						UINT32 len=2024;
						if(m_pFirstMix->getMixCascadeInfo(tmpBuff,&len)!=E_SUCCESS)
							{
								delete [] tmpBuff;
								return E_UNKNOWN;
							}
						buffLen=2024;
						if(m_pSignature->signXML(tmpBuff,strlen((char*)tmpBuff),(UINT8*)buff,&buffLen)!=E_SUCCESS)
							{delete []buff;return E_UNKNOWN;}
					}
				else
					{
						buffLen=2024;
						UINT8* tmpBuff=new UINT8[2024];
						if(options.getMixXml(tmpBuff,2024)!=E_SUCCESS)
							{
								delete [] tmpBuff;
								return E_UNKNOWN;
							}
						if(m_pSignature->signXML(tmpBuff,strlen((char*)tmpBuff),(UINT8*)buff,&buffLen)!=E_SUCCESS)
							{delete []tmpBuff;delete []buff;return E_UNKNOWN;}
						delete[] tmpBuff;
					}
				sprintf((char*)buffHeader,"POST /helo HTTP/1.0\r\nContent-Length: %u\r\n\r\n",buffLen);
				oSocket.send(buffHeader,strlen((char*)buffHeader));
				oSocket.send(buff,buffLen);
				oSocket.close();
				delete []buff;		
				return E_SUCCESS;	
			}
		return E_UNKNOWN;
	}

SINT32 CAInfoService::test()
	{
		BufferOutputStream oBufferStream(1024,1024);
		XMLOutput oxmlOut(oBufferStream);
		CASignature oSignature;
		UINT8 fileBuff[2048];
		int handle=open("G:/proxytest/Debug/privkey.xml",O_BINARY|O_RDONLY);
		UINT32 len=read(handle,fileBuff,2048);
		close(handle);
		oSignature.setSignKey(fileBuff,len,SIGKEY_XML);
		UINT8 buff[1024];
		UINT32 buffLen;
#ifdef _WIN32
		_CrtMemState s1, s2, s3;
		_CrtMemCheckpoint( &s1 );
#endif
		for(int i=0;i<1000;i++)
			{
				oBufferStream.reset();
				oxmlOut.BeginDocument("1.0","UTF-8",true);
				oxmlOut.BeginElementAttrs("MixCascadeStatus");
				oxmlOut.WriteAttr("id","errwrzteutuztuit");
				int tmpUser,tmpPackets,tmpRisk,tmpTraffic;
			
				oxmlOut.WriteAttr("nrOfActiveUsers",(int)tmpUser);
				oxmlOut.WriteAttr("currentRisk",(int)tmpRisk);
				oxmlOut.WriteAttr("trafficSituation",(int)tmpTraffic);
				oxmlOut.WriteAttr("mixedPackets",(int)tmpPackets);
				oxmlOut.EndAttrs();
				oxmlOut.BeginElementAttrs("Mixes");
				oxmlOut.WriteAttr("count",(int)3);
				oxmlOut.EndAttrs();
				oxmlOut.EndElement();
				oxmlOut.EndElement();
				oxmlOut.EndDocument();
				oSignature.signXML(oBufferStream.getBuff(),oBufferStream.getBufferSize(),(UINT8*)buff,&buffLen);
			}
#ifdef _WIN32
		_CrtMemCheckpoint( &s2 );
		if ( _CrtMemDifference( &s3, &s1, &s2 ) )
      _CrtMemDumpStatistics( &s3 );
#endif
		return E_SUCCESS;
	}
