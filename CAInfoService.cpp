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
#include "CASingleSocketGroup.hpp"
#include "CALastMix.hpp"
#include "CAHttpClient.hpp"

extern CACmdLnOptions options;

const char * STRINGS_REQUEST_TYPES[2]={"POST","GET"};
const char * STRINGS_REQUEST_COMMANDS[3]={"configure","helo","mixinfo/"};

static THREAD_RETURN InfoLoop(void *p)
	{
		CAInfoService* pInfoService=(CAInfoService*)p;
		int helocount=0;
		bool bIsFirst=true; //send our own certifcate only the first time
    while(pInfoService->isRunning())
			{
				if(pInfoService->sendStatus(bIsFirst)==E_SUCCESS)
					bIsFirst=false;
	// check every minute if configuring, every 10 minutes otherwise
        if(helocount==0 || pInfoService->isConfiguring())
					{
            if(options.isFirstMix() || (options.isLastMix() && pInfoService->isConfiguring()))
							{
								if(pInfoService->sendCascadeHelo()!=E_SUCCESS)
									{
                    CAMsg::printMsg(LOG_ERR,"InfoService: Error: Could not send Cascade Information.\n");
									}
								else
									{
                    CAMsg::printMsg(LOG_DEBUG,"InfoService: Successfull send Cascade Information.\n");
									}
							}
            pInfoService->sendMixHelo();
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
    m_pMix=NULL;
		m_bRun=false;
		m_pSignature=NULL;
		m_pcertstoreOwnCerts=NULL;
		m_minuts=0;
		m_lastMixedPackets=0;
    m_expectedMixRelPos = 0;
}

CAInfoService::CAInfoService(CAMix* pMix)
	{
		m_pMix=pMix;
		m_bRun=false;
		m_pSignature=NULL;
		m_pcertstoreOwnCerts=NULL;
		m_minuts=0;
		m_lastMixedPackets=0;
    m_expectedMixRelPos = 0;
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
    if(m_pMix!=NULL && options.isFirstMix())
        return ((CAFirstMix*)m_pMix)->getLevel(puser,prisk,ptraffic);
    return E_UNKNOWN;
}

SINT32 CAInfoService::getMixedPackets(UINT64& ppackets)
	{
    if(m_pMix!=NULL && options.isFirstMix())
        return ((CAFirstMix*)m_pMix)->getMixedPackets(ppackets);
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
	* @retval E_UNKNOWN if something goes wrong
	* @retval E_SUCCESS otherwise
	*/
SINT32 CAInfoService::sendStatus(bool bIncludeCerts)
	{
		if(!options.isFirstMix())
			return E_SUCCESS;
		CASocket oSocket;
		CASocketAddrINet oAddr;
		UINT8 hostname[255];
		UINT8 buffHeader[255];
		SINT32 tmpUser,tmpRisk,tmpTraffic;
		UINT64 tmpPackets;
		CAHttpClient httpClient;
		
		if(options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			return E_UNKNOWN;
		oAddr.setAddr(hostname,options.getInfoServerPort());
		if(oSocket.connect(oAddr)!=E_SUCCESS)
			return E_UNKNOWN;
		httpClient.setSocket(&oSocket);
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
		CACertStore* ptmpCertStore=m_pcertstoreOwnCerts;
		if(!bIncludeCerts)
			ptmpCertStore=NULL;
		if(m_pSignature->signXML(tmpBuff,strlen((char*)tmpBuff),buff,&buffLen,ptmpCertStore)!=E_SUCCESS)
			{
				delete[] buff;
				return E_UNKNOWN;
			}
		sprintf((char*)buffHeader,"POST /feedback HTTP/1.0\r\nContent-Length: %u\r\n\r\n",buffLen);
		oSocket.sendFully(buffHeader,strlen((char*)buffHeader));
		SINT32 ret=oSocket.sendFully(buff,buffLen);
		delete[] buff;
		oSocket.close();	
		if(ret==E_SUCCESS)
			return E_SUCCESS;
		return E_UNKNOWN;
	}


/** POSTs the MIXINFO message for a mix to the InfoService.
	*/
SINT32 CAInfoService::sendMixInfo(const UINT8* pMixID)
{
	return sendMixHelo(REQUEST_COMMAND_MIXINFO,pMixID);
}

/** POSTs the HELO message for a mix to the InfoService.
	*/
SINT32 CAInfoService::sendMixHelo(SINT32 requestCommand,const UINT8* param)
{
    UINT8* recvBuff = NULL;
    SINT32 ret = E_SUCCESS;
    UINT32 len = 0;

		CASocket oSocket;
		CASocketAddrINet oAddr;
		UINT8 hostname[255];
		UINT8 buffHeader[255];
		UINT32 sendBuffLen;
		UINT8* sendBuff=NULL;
		CAHttpClient httpClient;

    UINT32 requestType = REQUEST_TYPE_POST;
    bool receiveAnswer = false;

    if(requestCommand<0)
			{
        if(m_bConfiguring)
					{
            requestCommand = REQUEST_COMMAND_CONFIGURE;
            receiveAnswer = true;
					}
        else
					{
            requestCommand = REQUEST_COMMAND_HELO;
            receiveAnswer = true;
					}
			}
		else
			{
        if(requestCommand==REQUEST_COMMAND_MIXINFO)
					{
            requestType=REQUEST_TYPE_GET;
            receiveAnswer = true;
					}
			}


		if(options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			goto ERR;
		oAddr.setAddr(hostname,options.getInfoServerPort());

    oSocket.setRecvBuff(255);
		if(oSocket.connect(oAddr)==E_SUCCESS)
			{
				httpClient.setSocket(&oSocket);
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

				const char* strRequestCommand=STRINGS_REQUEST_COMMANDS[requestCommand];
				const char* strRequestType=STRINGS_REQUEST_TYPES[requestType];

        CAMsg::printMsg(LOG_DEBUG,"InfoService: Sending [%s] %s to InfoService.\r\n", strRequestType,strRequestCommand);

        if(requestCommand==REQUEST_COMMAND_MIXINFO)
					sprintf((char*)buffHeader,"%s /%s%s HTTP/1.0\r\nContent-Length: %u\r\n\r\n", strRequestType, strRequestCommand, param,sendBuffLen);
				else
					sprintf((char*)buffHeader,"%s /%s HTTP/1.0\r\nContent-Length: %u\r\n\r\n", strRequestType, strRequestCommand, sendBuffLen);
 				if(oSocket.sendFully(buffHeader,strlen((char*)buffHeader))!=E_SUCCESS||
						oSocket.sendFully(sendBuff,sendBuffLen)!=E_SUCCESS)
					goto ERR;

				delete []sendBuff;
				sendBuff=NULL;
        if(receiveAnswer)
        {
            ret = httpClient.parseHTTPHeader(&len);
						if(ret!=E_SUCCESS)
							goto ERR;
            if(len > 0)
            {
                recvBuff = new UINT8[len+1];
                ret = oSocket.receiveFully(recvBuff, len);
                if(ret!=E_SUCCESS)
                {
										delete recvBuff;
                    oSocket.close();
                    goto ERR;
                }
								recvBuff[len]=0;
								CAMsg::printMsg(LOG_DEBUG,"Received from Infoservice:\n");
								CAMsg::printMsg(LOG_DEBUG,(char*)recvBuff);
								CAMsg::printMsg(LOG_DEBUG,"\n");
            }
        }

        oSocket.close();

        if(recvBuff != NULL)
        {
            MemBufInputSource oInput(recvBuff,len,"tmpID");
            DOMParser oParser;
            oParser.parse(oInput);
            DOM_Document doc=oParser.getDocument();
            delete[] recvBuff;
            DOM_Element root;
            if(!doc.isNull() && (root = doc.getDocumentElement()) != NULL)
            {
                if(root.getNodeName().equals("MixCascade"))
                {
                    ret = handleConfigEvent(doc);
                    if(ret == E_SUCCESS)
                        m_bConfiguring = false;
                    else
                        return ret;
                }
                else if(root.getNodeName().equals("Mix"))
                {
                    if(m_expectedMixRelPos < 0)
                    {
                        CAMsg::printMsg(LOG_DEBUG,"InfoService: Setting new previous mix: %s\n",root.getAttribute("id").transcode());

                        options.setPrevMix(doc);
                    }
                    else if(m_expectedMixRelPos > 0)
                    {
                        CAMsg::printMsg(LOG_DEBUG,"InfoService: Setting new next mix: %s\n",root.getAttribute("id").transcode());

                        options.setNextMix(doc);
                    }
                }
            }
            else
            {
                CAMsg::printMsg(LOG_CRIT,"InfoService: Error parsing answer from InfoService!\n");
            }
        }

				return E_SUCCESS;	
			}
ERR:
		if(sendBuff!=NULL)
			delete []sendBuff;
		return E_UNKNOWN;
	}

/** POSTs the HELO message for a whole cascade to the InfoService.
 * If the running mix is a last mix, this method is used to inform the
 * InfoService that it wants to create a new cascade. The InfoService
 * then tells all involved mixes to join this cascade.
 * If the current mix is a middle mix, this method does nothing.
 * @param E_SUCCESS on success
 * @param E_UNKNOWN on any error
	*/
SINT32 CAInfoService::sendCascadeHelo()
{
    if(options.isMiddleMix())
			return E_SUCCESS;
		CASocket oSocket;
		CASocketAddrINet oAddr;
		UINT8 hostname[255];
		UINT8 buffHeader[255];
		UINT32 sendBuffLen;
		UINT8* sendBuff=NULL;
		CAHttpClient httpClient;
		
		if(options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			goto ERR;
		oAddr.setAddr(hostname,options.getInfoServerPort());
		if(oSocket.connect(oAddr)==E_SUCCESS)
			{
				httpClient.setSocket(&oSocket);
				DOM_Document docMixInfo;
        if(m_pMix->getMixCascadeInfo(docMixInfo)!=E_SUCCESS)
					{
            CAMsg::printMsg(LOG_INFO,"InfoService: Error: Cascade not yet configured.\r\n");
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

        if(options.isFirstMix())
        {
            CAMsg::printMsg(LOG_DEBUG,"InfoService: Sending cascade helo to InfoService.\r\n");
        }
        else
        {
            CAMsg::printMsg(LOG_DEBUG,"InfoService: Sending cascade configuration request to InfoService.\r\n");
        }

				sprintf((char*)buffHeader,"POST /cascade HTTP/1.0\r\nContent-Length: %u\r\n\r\n",sendBuffLen);
				if(	oSocket.sendFully(buffHeader,strlen((char*)buffHeader))!=E_SUCCESS||
						oSocket.sendFully(sendBuff,sendBuffLen)!=E_SUCCESS)
						goto ERR;
				//Receive answer --> 200 Ok or failure
				//HTTP/1.1 200 Ok
				if(oSocket.receiveFully(buffHeader,12,MIX_TO_INFOSERVICE_TIMEOUT)!=E_SUCCESS)
					goto ERR;
				if(memcmp(buffHeader+9,"200",3)!=0)
					goto ERR;
				oSocket.close();
				delete []sendBuff;
				return E_SUCCESS;	
			}
ERR:
		if(sendBuff!=NULL)
			delete []sendBuff;
		return E_UNKNOWN;
}

SINT32 CAInfoService::handleConfigEvent(DOM_Document& doc)
{
    CAMsg::printMsg(LOG_INFO,"InfoService: Cascade info received from InfoService\n");

    DOM_Element root=doc.getDocumentElement();
    DOM_Node mixesElement;
    getDOMChildByName(root,(UINT8*)"Mixes", mixesElement, false);
    char* mixId;
    char* prevMixId = NULL;
    char* myNextMixId = NULL;
    char* myPrevMixId = NULL;
    bool bFoundMix = false;
    UINT8 myMixId[64];
    options.getMixId(myMixId,64);
    DOM_Node child = mixesElement.getFirstChild();
    while(child!=NULL)
    {
        if(child.getNodeName().equals("Mix") && child.getNodeType() == DOM_Node::ELEMENT_NODE)
        {
            mixId = static_cast<const DOM_Element&>(child).getAttribute("id").transcode();
            if(strcmp(mixId,(char*)myMixId) == 0)
            {
                bFoundMix = true;
                myPrevMixId = prevMixId;
            }
            else if(bFoundMix == true)
            {
                myNextMixId = mixId;
                break;
            }
            prevMixId = mixId;
        }
        child = child.getNextSibling();
    }

    SINT32 ret = 0;
    //char* c;

    if(myPrevMixId != NULL)
    {
        CAMsg::printMsg(LOG_INFO,"InfoService: Asking InfoService about previous mix ...\n");

        m_expectedMixRelPos = -1;
        //c = new char[8+strlen(myPrevMixId)+1];
        //strcpy(c,"mixinfo/");
        //strcat(c,myPrevMixId);
        ret = sendMixInfo((UINT8*)myPrevMixId);
        //delete[] c;
        if(ret != E_SUCCESS)
        {
            CAMsg::printMsg(LOG_CRIT,"InfoService: Error retrieving mix info from InfoService!\n");
            return ret;
        }
    }

    if(myNextMixId != NULL)
    {
        CAMsg::printMsg(LOG_INFO,"InfoService: Asking InfoService about next mix ...\n");

        m_expectedMixRelPos = 1;
        //c = new char[8+strlen(myNextMixId)+1];
        //strcpy(c,"mixinfo/");
        //strcat(c,myNextMixId);
        ret = sendMixInfo((UINT8*)myNextMixId);
        //delete[] c;
        if(ret != E_SUCCESS)
        {
            CAMsg::printMsg(LOG_CRIT,"InfoService: Error retrieving mix info from InfoService!\n");
            return ret;
        }
    }
    return E_SUCCESS;
}

