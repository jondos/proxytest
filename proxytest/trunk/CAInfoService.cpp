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
#include "xml/DOM_Output.hpp"
extern CACmdLnOptions options;



static THREAD_RETURN InfoLoop(void *p)
	{
		CAInfoService* pInfoService=(CAInfoService*)p;
		int helocount=0;

		while(pInfoService->getRun())
			{
				pInfoService->sendStatus();
				if(helocount==0)
					{
						pInfoService->sendMixHelo();
						pInfoService->sendCascadeHelo();
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
		m_pcertstoreOwnCerts=NULL;
		m_minuts=0;
		m_lastMixedPackets=0;
	}

CAInfoService::CAInfoService(CAFirstMix* pFirstMix)
	{
		m_pFirstMix=pFirstMix;
		m_bRun=false;
		m_pSignature=NULL;
		m_pcertstoreOwnCerts=NULL;
		m_minuts=0;
		m_lastMixedPackets=0;
	}

CAInfoService::~CAInfoService()
	{
		stop();
		delete m_pcertstoreOwnCerts;
	}
/** Sets the signature used to sign the messages send to Infoservice.
	* If pOwnCert!=NULL this Certifcate is included in the Signature
	*/
SINT32 CAInfoService::setSignature(CASignature* pSig,CACertificate* pOwnCert)
	{
		m_pSignature=pSig;
		if(m_pcertstoreOwnCerts!=NULL)
			delete m_pcertstoreOwnCerts;
		m_pcertstoreOwnCerts=NULL;
		if(pOwnCert!=NULL)
			{
				m_pcertstoreOwnCerts=new CACertStore();
				m_pcertstoreOwnCerts->add(pOwnCert);
			}
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

/** POSTs mix status to the InfoService. [only first mix does this at the moment]
	*/
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

//let the attributes in alphabetical order..
#define XML_MIX_CASCADE_STATUS "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<MixCascadeStatus LastUpdate=\"%s\" currentRisk=\"%i\" id=\"%s\" mixedPackets=\"%s\" nrOfActiveUsers=\"%i\" trafficSituation=\"%i\"\
></MixCascadeStatus>"
				
				UINT32 buffLen=4096;
				UINT8* buff=new UINT8[buffLen];
				UINT8 tmpBuff[1024];
				UINT8 buffMixedPackets[50];
				print64(buffMixedPackets,tmpPackets);
				UINT64 currentMillis;
				UINT8 tmpStrCurrentMillis[50];
				if(getcurrentTimeMillis(currentMillis)==E_SUCCESS)
					print64(tmpStrCurrentMillis,currentMillis);
				else
					tmpStrCurrentMillis[0]=0;
				sprintf((char*)tmpBuff,XML_MIX_CASCADE_STATUS,tmpStrCurrentMillis,tmpRisk,strMixId,buffMixedPackets,tmpUser,tmpTraffic);
				m_pSignature->signXML(tmpBuff,strlen((char*)tmpBuff),buff,&buffLen,m_pcertstoreOwnCerts);
				sprintf((char*)buffHeader,"POST /feedback HTTP/1.0\r\nContent-Length: %u\r\n\r\n",buffLen);
				oSocket.send(buffHeader,strlen((char*)buffHeader));
				oSocket.send(buff,buffLen);
				delete[] buff;
			}
		oSocket.close();	
		return E_SUCCESS;
	}

/** POSTs the HELO message for a mix to the InfoService.
	*/
SINT32 CAInfoService::sendMixHelo()
	{
		CASocket oSocket;
		CASocketAddrINet oAddr;
		UINT8 hostname[255];
		UINT8 buffHeader[255];
		UINT32 sendBuffLen;
		UINT8* sendBuff=NULL;
		if(options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			goto ERR;
		oAddr.setAddr(hostname,options.getInfoServerPort());
		if(oSocket.connect(oAddr)==E_SUCCESS)
			{
				DOM_Document docMixInfo;
				if(options.getMixXml(docMixInfo)!=E_SUCCESS)
					{
						goto ERR;
					}
				//insert (or update) the Timestamp
				DOM_Element elemRoot=docMixInfo.getDocumentElement();
				DOM_Element elemTimeStamp;
				if(getDOMChildByName(elemRoot,(UINT8*)"LastUpdate",elemTimeStamp,false)!=E_SUCCESS)
					{
						elemTimeStamp=docMixInfo.createElement("LastUpdate");
						elemRoot.appendChild(elemTimeStamp);
					}
				UINT64 currentMillis;
				getcurrentTimeMillis(currentMillis);
				UINT8 tmpStrCurrentMillis[50];
				print64(tmpStrCurrentMillis,currentMillis);
				setDOMElementValue(elemTimeStamp,tmpStrCurrentMillis);
				if(m_pSignature->signXML(docMixInfo,m_pcertstoreOwnCerts)!=E_SUCCESS)
					{
						goto ERR;
					}
				
				sendBuff=DOM_Output::dumpToMem(docMixInfo,&sendBuffLen);
				if(sendBuff==NULL)
					goto ERR;
				sprintf((char*)buffHeader,"POST /helo HTTP/1.0\r\nContent-Length: %u\r\n\r\n",sendBuffLen);
				oSocket.send(buffHeader,strlen((char*)buffHeader));
				oSocket.send(sendBuff,sendBuffLen);
				oSocket.close();
				delete []sendBuff;
				return E_SUCCESS;	
			}
ERR:
		if(sendBuff!=NULL)
			delete []sendBuff;
		return E_UNKNOWN;
	}

/** POSTs the HELO message for a whole cascade to the InfoService [only first mix does this].
	*/
SINT32 CAInfoService::sendCascadeHelo()
	{
		if(!options.isFirstMix())
			return E_SUCCESS;
		CASocket oSocket;
		CASocketAddrINet oAddr;
		UINT8 hostname[255];
		UINT8 buffHeader[255];
		UINT32 sendBuffLen;
		UINT8* sendBuff=NULL;
		if(options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			goto ERR;
		oAddr.setAddr(hostname,options.getInfoServerPort());
		if(oSocket.connect(oAddr)==E_SUCCESS)
			{
				DOM_Document docMixInfo;
				if(m_pFirstMix->getMixCascadeInfo(docMixInfo)!=E_SUCCESS)
					{
						goto ERR;
					}
				//insert (or update) the Timestamp
				DOM_Element elemRoot=docMixInfo.getDocumentElement();
				DOM_Element elemTimeStamp;
				if(getDOMChildByName(elemRoot,(UINT8*)"LastUpdate",elemTimeStamp,false)!=E_SUCCESS)
					{
						elemTimeStamp=docMixInfo.createElement("LastUpdate");
						elemRoot.appendChild(elemTimeStamp);
					}
				UINT64 currentMillis;
				getcurrentTimeMillis(currentMillis);
				UINT8 tmpStrCurrentMillis[50];
				print64(tmpStrCurrentMillis,currentMillis);
				setDOMElementValue(elemTimeStamp,tmpStrCurrentMillis);
				if(m_pSignature->signXML(docMixInfo,m_pcertstoreOwnCerts)!=E_SUCCESS)
					{
						goto ERR;
					}
				
				sendBuff=DOM_Output::dumpToMem(docMixInfo,&sendBuffLen);
				if(sendBuff==NULL)
					goto ERR;
				sprintf((char*)buffHeader,"POST /cascade HTTP/1.0\r\nContent-Length: %u\r\n\r\n",sendBuffLen);
				oSocket.send(buffHeader,strlen((char*)buffHeader));
				oSocket.send(sendBuff,sendBuffLen);
				oSocket.close();
				delete []sendBuff;
				return E_SUCCESS;	
			}
ERR:
		if(sendBuff!=NULL)
			delete []sendBuff;
		return E_UNKNOWN;
	}
