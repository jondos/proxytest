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
#include "CACertificate.hpp"
#include "CAXMLBI.hpp"

extern CACmdLnOptions options;

const char * STRINGS_REQUEST_TYPES[2]={"POST","GET"};
const char * STRINGS_REQUEST_COMMANDS[3]={"configure","helo","mixinfo/"};

static THREAD_RETURN InfoLoop(void *p)
	{
		CAMsg::printMsg(LOG_DEBUG, "CAInfoService - InfoLoop() started\n");
		CAInfoService* pInfoService=(CAInfoService*)p;
		SINT32 helocount=0;
		bool errorSendingCascade = false;
		bool bIsFirst=true; //send our own certifcate only the first time
    	while(pInfoService->isRunning())
		{
			if(pInfoService->sendStatus(bIsFirst)==E_SUCCESS)
			{
				bIsFirst=false;
			}
	// check every minute if configuring, every 10 minutes otherwise
        	if(helocount==0 || pInfoService->isConfiguring() || errorSendingCascade)
			{
            	if(options.isFirstMix() || (options.isLastMix() && pInfoService->isConfiguring()))
				{
					if(pInfoService->sendCascadeHelo()!=E_SUCCESS)
					{
						errorSendingCascade = true;
                    	CAMsg::printMsg(LOG_ERR,"InfoService: Error: Could not send Cascade Information.\n");
					}
					else
					{
						errorSendingCascade = false;
                   		CAMsg::printMsg(LOG_DEBUG,"InfoService: Successful sent Cascade Information.\n");
					}
				}
				if(helocount==0 || pInfoService->isConfiguring())
				{
            		pInfoService->sendMixHelo();
				}
				helocount=9;
			}
			else
			{ 
				helocount--;
			}
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
		m_pthreadRunLoop=new CAThread((UINT8*)"InfoServiceThread");
}

CAInfoService::CAInfoService(CAMix* pMix)
	{
    m_pMix=NULL;
		m_bRun=false;
		m_pSignature=NULL;
		m_pcertstoreOwnCerts=NULL;
		m_minuts=0;
		m_lastMixedPackets=0;
    m_expectedMixRelPos = 0;
		m_pthreadRunLoop=new CAThread((UINT8*)"InfoServiceThread");
		m_pMix=pMix;
	}

CAInfoService::~CAInfoService()
	{
		stop();
		delete m_pcertstoreOwnCerts;
		delete m_pthreadRunLoop;
	}
/** Sets the signature used to sign the messages send to Infoservice.
	* If pOwnCert!=NULL this Certifcate is included in the Signature
	*/
SINT32 CAInfoService::setSignature(CASignature* pSig, CACertificate* pOwnCert,
								CACertificate** a_opCerts, UINT32 a_opCertsLength)
	{
		m_pSignature=pSig;
		if(m_pcertstoreOwnCerts!=NULL)
		{
			delete m_pcertstoreOwnCerts;
		}
		m_pcertstoreOwnCerts=NULL;
		m_pcertstoreOwnCerts=new CACertStore();

		if (a_opCerts != NULL && a_opCertsLength > 0)
		{
			for (SINT32 i = a_opCertsLength - 1; i >= 0; i--)
			{
				m_pcertstoreOwnCerts->add(a_opCerts[i]);
			}
		}
		if(pOwnCert!=NULL)
			{
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
		m_pthreadRunLoop->setMainLoop(InfoLoop);
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAInfoService::start() - starting InfoService thread\n");
		#endif
		
		return m_pthreadRunLoop->start(this);
	}

SINT32 CAInfoService::stop()
	{
		if(m_bRun)
			{
				m_bRun=false;
				m_pthreadRunLoop->join();
			}
		return E_SUCCESS;
	}

SINT32 CAInfoService::sendStatus(bool bIncludeCerts)
{
	if(!options.isFirstMix())
		return E_SUCCESS;
	SINT32 returnValue = E_UNKNOWN;
	SINT32 currentValue;
	UINT32 nrAddresses;
	UINT32 len;
	UINT8* strStatusXML=getStatusXMLAsString(bIncludeCerts,len);
	if(strStatusXML==NULL)
		return E_UNKNOWN;
	CAListenerInterface** ppSocketAddresses = options.getInfoServices(nrAddresses);
	for (UINT32 i = 0; i < nrAddresses; i++)
	{
		CASocketAddrINet* pAddr=(CASocketAddrINet*)ppSocketAddresses[i]->getAddr();
		currentValue = sendStatus(strStatusXML,len,pAddr);
		if (currentValue == E_SUCCESS)
		{
			returnValue = currentValue;
		}
		delete pAddr;
	}
	delete[] strStatusXML;
	return returnValue;	
}



UINT8* CAInfoService::getStatusXMLAsString(bool bIncludeCerts,UINT32& len)
	{
		SINT32 tmpUser,tmpRisk,tmpTraffic;
		UINT64 tmpPackets;
			
		//httpClient.setSocket(&oSocket);
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
				return NULL;
			}
		len=buffLen;
		return buff;
	}


/** POSTs mix status to the InfoService. [only first mix does this at the moment]
	* @retval E_UNKNOWN if something goes wrong
	* @retval E_SUCCESS otherwise
	*
	*/
	///todo use httpclient class
SINT32 CAInfoService::sendStatus(UINT8* a_strStatusXML,UINT32 a_len, CASocketAddrINet* a_pSocketAddress)
	{
		UINT8 buffHeader[512];
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAInfoService::sendStatus()\n");
		#endif
		if(!options.isFirstMix())
			return E_SUCCESS;
		CASocket oSocket;
		
		if (a_pSocketAddress == NULL)
		{
			return E_UNKNOWN;
		}
		
		if(oSocket.connect(*a_pSocketAddress)!=E_SUCCESS)
			return E_UNKNOWN;
		#ifdef DEBUG
			UINT8 hostname[255];
			UINT32 len=255;
			a_pSocketAddress->getIPAsStr(hostname,len);
			CAMsg::printMsg(LOG_DEBUG, "CAInfoService::sendStatus() - connected to InfoService %s:%d.\n", hostname, a_pSocketAddress->getPort());
		#endif
			
		sprintf((char*)buffHeader,"POST /feedback HTTP/1.0\r\nContent-Length: %u\r\n\r\n",a_len);
		oSocket.sendFully(buffHeader,strlen((char*)buffHeader));
		SINT32 ret=oSocket.sendFully(a_strStatusXML,a_len);
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAInfoService::sendStatus() - Msg sent\n");
		#endif

		oSocket.close();	
		if(ret==E_SUCCESS)
			return E_SUCCESS;
		return E_UNKNOWN;
	}


/** POSTs the MIXINFO message for a mix to the InfoService.
	*/
/*SINT32 CAInfoService::sendMixInfo(const UINT8* pMixID)
{
	return sendMixHelo(REQUEST_COMMAND_MIXINFO,pMixID, NULL);
}
*/
SINT32 CAInfoService::sendMixHelo(SINT32 requestCommand,const UINT8* param)
{
	UINT32 len;
	UINT8* strMixHeloXML=getMixHeloXMLAsString(len);
	if(strMixHeloXML==NULL)
		return E_UNKNOWN;
	SINT32 returnValue = E_UNKNOWN;
	SINT32 currentValue;
	UINT32 nrAddresses;
	CAListenerInterface** socketAddresses = options.getInfoServices(nrAddresses);
	for (UINT32 i = 0; i < nrAddresses; i++)
	{
		CASocketAddrINet* pAddr=(CASocketAddrINet*)socketAddresses[i]->getAddr();
		currentValue = sendMixHelo(strMixHeloXML,len,requestCommand, param, pAddr);
		if (currentValue == E_SUCCESS)
		{
			returnValue = currentValue;
		}
		delete pAddr;
	}
	delete[] strMixHeloXML;
	return returnValue;		
}


UINT8* CAInfoService::getMixHeloXMLAsString(UINT32& a_len)
	{
    a_len = 0;

		UINT32 sendBuffLen;
		UINT8* sendBuff=NULL;
			DOM_Element elemTimeStamp;
	
DOM_Element elemRoot;


				DOM_Document docMixInfo;
				if(options.getMixXml(docMixInfo)!=E_SUCCESS)
					{
						goto ERR;
					}
				//insert (or update) the Timestamp
				elemRoot=docMixInfo.getDocumentElement();
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
				a_len=sendBuffLen;
				return sendBuff;	
			
ERR:
		if(sendBuff!=NULL)
			delete []sendBuff;
		return NULL;
	}


/** POSTs the HELO message for a mix to the InfoService.
	*/
SINT32 CAInfoService::sendMixHelo(UINT8* a_strMixHeloXML,UINT32 a_len,SINT32 requestCommand,const UINT8* param,
									CASocketAddrINet* a_pSocketAddress)
{
    UINT8* recvBuff = NULL;
    SINT32 ret = E_SUCCESS;
    UINT32 len = 0;

		CASocket oSocket;
		UINT8 hostname[255];
		UINT8 buffHeader[255];
		CAHttpClient httpClient;

    UINT32 requestType = REQUEST_TYPE_POST;
    bool receiveAnswer = false;

		if (a_pSocketAddress == NULL)
			{
				return E_UNKNOWN;
			}

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


		if(a_pSocketAddress->getIPAsStr(hostname, 255)!=E_SUCCESS)
		{
			goto ERR;
		}
		
    oSocket.setRecvBuff(255);
		if(oSocket.connect(*a_pSocketAddress)==E_SUCCESS)
			{
				httpClient.setSocket(&oSocket);

				const char* strRequestCommand=STRINGS_REQUEST_COMMANDS[requestCommand];
				const char* strRequestType=STRINGS_REQUEST_TYPES[requestType];

        CAMsg::printMsg(LOG_DEBUG,"InfoService: Sending [%s] %s to InfoService %s:%d.\r\n", strRequestType,strRequestCommand, hostname, a_pSocketAddress->getPort());

        if(requestCommand==REQUEST_COMMAND_MIXINFO)
					sprintf((char*)buffHeader,"%s /%s%s HTTP/1.0\r\nContent-Length: %u\r\n\r\n", strRequestType, strRequestCommand, param,a_len);
				else
					sprintf((char*)buffHeader,"%s /%s HTTP/1.0\r\nContent-Length: %u\r\n\r\n", strRequestType, strRequestCommand, a_len);
 				if(oSocket.sendFully(buffHeader,strlen((char*)buffHeader))!=E_SUCCESS||
						oSocket.sendFully(a_strMixHeloXML,a_len)!=E_SUCCESS)
					goto ERR;

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
										delete []recvBuff;
                    recvBuff=NULL;
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
            recvBuff=NULL;
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
		return E_UNKNOWN;
	}

SINT32 CAInfoService::sendCascadeHelo()
{
  if(options.isMiddleMix())
		return E_SUCCESS;
	UINT32 len;
	UINT8* strCascadeHeloXML=getCascadeHeloXMLAsString(len);
	if(strCascadeHeloXML==NULL)
		return E_UNKNOWN;
	SINT32 returnValue = E_UNKNOWN;
	SINT32 currentValue;
	UINT32 nrAddresses;
	CAListenerInterface** socketAddresses = options.getInfoServices(nrAddresses);
	for (UINT32 i = 0; i < nrAddresses; i++)
	{
		CASocketAddrINet* pAddr=(CASocketAddrINet*)socketAddresses[i]->getAddr();
		currentValue = sendCascadeHelo(strCascadeHeloXML,len,pAddr);
		if (currentValue == E_SUCCESS)
		{
			returnValue = currentValue;
		}
		delete pAddr;
	}
	delete[] strCascadeHeloXML;
	return returnValue;
}

UINT8* CAInfoService::getCascadeHeloXMLAsString(UINT32& a_len)
	{	
		a_len=0;
 		UINT32 sendBuffLen;
		UINT8* sendBuff=NULL;
		DOM_Document docMixInfo;
		DOM_Element elemTimeStamp;
		DOM_Element elemRoot;
    if(m_pMix->getMixCascadeInfo(docMixInfo)!=E_SUCCESS)
			{
        CAMsg::printMsg(LOG_INFO,"InfoService: Error: Cascade not yet configured.\r\n");
				goto ERR;
			}
		//insert (or update) the Timestamp
		elemRoot=docMixInfo.getDocumentElement();
		
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

		a_len=sendBuffLen;
		return sendBuff;		
ERR:
		if(sendBuff!=NULL)
			delete []sendBuff;
		return NULL;
	}

/** POSTs the HELO message for a whole cascade to the InfoService.
 * If the running mix is a last mix, this method is used to inform the
 * InfoService that it wants to create a new cascade. The InfoService
 * then tells all involved mixes to join this cascade.
 * If the current mix is a middle mix, this method does nothing.
 * @param E_SUCCESS on success
 * @param E_UNKNOWN on any error
	*/
SINT32 CAInfoService::sendCascadeHelo(UINT8* a_strCascadeHeloXML,UINT32 a_len,CASocketAddrINet* a_pSocketAddress)
{	
    if(options.isMiddleMix())
			return E_SUCCESS;
		CASocket oSocket;
		UINT8 hostname[255];
		UINT8 buffHeader[255];
		CAHttpClient httpClient;

		if (a_pSocketAddress == NULL)
		{
			return E_UNKNOWN;
		}
		
		
		if(a_pSocketAddress->getIPAsStr(hostname, 255)!=E_SUCCESS)
		{
			goto ERR;
		}
		if(oSocket.connect(*a_pSocketAddress)==E_SUCCESS)
			{
				httpClient.setSocket(&oSocket);
        if(options.isFirstMix())
        {
            CAMsg::printMsg(LOG_DEBUG,"InfoService: Sending cascade helo to InfoService %s:%d.\r\n", hostname, a_pSocketAddress->getPort());
        }
        else
        {
            CAMsg::printMsg(LOG_DEBUG,"InfoService: Sending cascade configuration request to InfoService %s:%d.\r\n", hostname, a_pSocketAddress->getPort());
        }

				sprintf((char*)buffHeader,"POST /cascade HTTP/1.0\r\nContent-Length: %u\r\n\r\n",a_len);
				if(	oSocket.sendFully(buffHeader,strlen((char*)buffHeader))!=E_SUCCESS||
						oSocket.sendFully(a_strCascadeHeloXML,a_len)!=E_SUCCESS)
						goto ERR;
				//Receive answer --> 200 Ok or failure
				//HTTP/1.1 200 Ok
				if(oSocket.receiveFullyT(buffHeader,12,MIX_TO_INFOSERVICE_TIMEOUT)!=E_SUCCESS)
					goto ERR;
				if(memcmp(buffHeader+9,"200",3)!=0)
					goto ERR;
				oSocket.close();
				return E_SUCCESS;	
			}
ERR:
		return E_UNKNOWN;
}

SINT32 CAInfoService::handleConfigEvent(DOM_Document& doc)
{
	///*** Nedd redesigning!*/
/*    CAMsg::printMsg(LOG_INFO,"InfoService: Cascade info received from InfoService\n");

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
		*/
    return E_SUCCESS;
}

#ifdef PAYMENT
SINT32 CAInfoService::getPaymentInstance(const UINT8* a_pstrPIID,CAXMLBI** a_pXMLBI)
{
	SINT32 returnValue = E_UNKNOWN;
	SINT32 currentValue;
	UINT32 nrAddresses;
	CAListenerInterface** socketAddresses = options.getInfoServices(nrAddresses);
	for (UINT32 i = 0; i < nrAddresses; i++)
	{
		currentValue = getPaymentInstance(a_pstrPIID, a_pXMLBI,
						(CASocketAddrINet*)socketAddresses[i]->getAddr());
		if (currentValue == E_SUCCESS)
		{
			returnValue = currentValue;
		}
	}
	return returnValue;	
}


/** Gets a payment instance from the InfoService.
	@param a_pstrPIID id of the payment instacne for which the information is requested
	@retval E_SUCCESS if succesful
	@retval E_UNKNOWN if an error occured
	@return a_pXMLBI will point to a pointer to the newly created CAXMLBI object or to NULL
*/
SINT32 CAInfoService::getPaymentInstance(const UINT8* a_pstrPIID,CAXMLBI** a_pXMLBI,
										 CASocketAddrINet* a_socketAddress)
	{
		CASocket socket;
		CASocketAddrINet address;
		UINT8 hostname[255];
		UINT8 request[255];
		CAHttpClient httpClient;
		UINT32 status, contentLength;
	
		//Connect to InfoService
		if(a_socketAddress->getIPAsStr(hostname, 255)!=E_SUCCESS)
		{
			return E_UNKNOWN;
		}
	
		address.setAddr(hostname,a_socketAddress->getPort());
	
		if(socket.connect(address)!=E_SUCCESS)
			return E_UNKNOWN;
	
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAInfoService::getPaymentInstance() - connected to InfoService %s:%d\n",hostname, a_socketAddress->getPort());
		#endif
	
		//Send request
		httpClient.setSocket(&socket);
		sprintf((char*) request, "/paymentinstance/%s", (char*) a_pstrPIID);
		httpClient.sendGetRequest(request);
		httpClient.parseHTTPHeader(&contentLength, &status);
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAInfoService::getPaymentInstance() - Request sent, HTTP status: %i, content length: %i\n", status, contentLength);
		#endif	
		if(status!=200||contentLength>0x00FFFF)
			{
				return E_UNKNOWN;
			}
			
		UINT8* content=new UINT8[contentLength+1];
		if(httpClient.getContent(content, &contentLength)!=E_SUCCESS)
			{
				delete []content;
				return E_UNKNOWN;
			}
		socket.close();
		//Parse XML
		MemBufInputSource oInput( content, contentLength, "PaymentInstance" );
		DOMParser oParser;
		oParser.parse( oInput );
		delete []content;
		DOM_Document doc = oParser.getDocument();
		if(doc==NULL)
			return E_UNKNOWN;
		DOM_Element elemRoot=doc.getDocumentElement();

		*a_pXMLBI = CAXMLBI::getInstance(elemRoot);
		if (*a_pXMLBI != NULL)
			{
				return E_SUCCESS;
			}
		else
			{
				return E_UNKNOWN;
			}
	}
#endif
