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
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"
#include "CASocketAddrINet.hpp"
#include "CAUtil.hpp"
//#include "xml/DOM_Output.hpp"
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

SINT32 CAInfoService::getMixedPackets(UINT64& ppackets)
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
		set64(m_lastMixedPackets,(UINT32)0);
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
		UINT64 tmpPackets;
		if(options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			return E_UNKNOWN;
		oAddr.setAddr(hostname,options.getInfoServerPort());
		if(oSocket.connect(oAddr)==E_SUCCESS)
			{
				UINT8 strMixId[255];
				options.getMixId(strMixId,255);
				
				tmpUser=tmpTraffic=tmpRisk=-1;
				set64(tmpPackets,(UINT32)-1);
				getLevel(&tmpUser,&tmpRisk,&tmpTraffic);
				getMixedPackets(tmpPackets);
				UINT32 avgTraffic=div64(tmpPackets,m_minuts);
				m_minuts++;
				UINT32 diffTraffic=diff64(tmpPackets,m_lastMixedPackets);
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
				set64(m_lastMixedPackets,tmpPackets);
			
#define XML_MIX_CASCADE_STATUS "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<MixCascadeStatus id=\"%s\" currentRisk=\"%i\" mixedPackets=\"%s\" nrOfActiveUsers=\"%i\" trafficSituation=\"%i\"\
 ></MixCascadeStatus>"
				
				UINT32 buffLen=1024;
				UINT8* buff=new UINT8[buffLen];
				UINT8 tmpBuff[1024];
				UINT8 buffMixedPackets[50];
				print64(buffMixedPackets,tmpPackets);
				sprintf((char*)tmpBuff,XML_MIX_CASCADE_STATUS,strMixId,tmpRisk,buffMixedPackets,tmpUser,tmpTraffic);
				m_pSignature->signXML(tmpBuff,strlen((char*)tmpBuff),buff,&buffLen);
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
		UINT32 xmlBuffLen=4096;
		UINT8* xmlBuff=new UINT8[xmlBuffLen];
		UINT32 sendBuffLen=4096;
		UINT8* sendBuff=new UINT8[sendBuffLen];
		if(xmlBuff==NULL||sendBuff==NULL||options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			goto ERR;
		oAddr.setAddr(hostname,options.getInfoServerPort());
		if(oSocket.connect(oAddr)==E_SUCCESS)
			{
				if(options.isFirstMix())
					{
						if(m_pFirstMix->getMixCascadeInfo(xmlBuff,&xmlBuffLen)!=E_SUCCESS)
							{
								goto ERR;
							}
					}
				else if(options.getMixXml(xmlBuff,&xmlBuffLen)!=E_SUCCESS)
					{
						goto ERR;
					}
				if(m_pSignature->signXML(xmlBuff,xmlBuffLen,sendBuff,&sendBuffLen)!=E_SUCCESS)
					{
						goto ERR;
					}
				sprintf((char*)buffHeader,"POST /helo HTTP/1.0\r\nContent-Length: %u\r\n\r\n",sendBuffLen);
				oSocket.send(buffHeader,strlen((char*)buffHeader));
				oSocket.send(sendBuff,sendBuffLen);
				oSocket.close();
				delete []sendBuff;
				delete []xmlBuff;
				return E_SUCCESS;	
			}
ERR:
		if(xmlBuff!=NULL)
			delete []xmlBuff;
		if(sendBuff!=NULL)
			delete []sendBuff;
		return E_UNKNOWN;
	}

