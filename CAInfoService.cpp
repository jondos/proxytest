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
#ifndef ONLY_LOCAL_PROXY 
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
#include "CADynamicCascadeConfigurator.hpp"

extern CACmdLnOptions* pglobalOptions;

const char * STRINGS_REQUEST_TYPES[2]={"POST","GET"};
const char * STRINGS_REQUEST_COMMANDS[6]={"configure","helo","mixinfo/", "dynacascade", "cascade", "feedback"};

const UINT64 CAInfoService::MINUTE = 60;
const UINT64 CAInfoService::SEND_LOOP_SLEEP = 60;
const UINT64 CAInfoService::SEND_CASCADE_INFO_WAIT = MINUTE * 10;
const UINT64 CAInfoService::SEND_MIX_INFO_WAIT = MINUTE * 10;
const UINT64 CAInfoService::SEND_STATUS_INFO_WAIT = MINUTE;
const UINT32 CAInfoService::SEND_INFO_TIMEOUT_MS = 3000; // timeout for single send operations

THREAD_RETURN CAInfoService::InfoLoop(void *p)
	{
		INIT_STACK;
		BEGIN_STACK("CAInfoService::InfoLoop");
		
		CAMsg::printMsg(LOG_DEBUG, "CAInfoService - InfoLoop() started\n");
		CAInfoService* pInfoService=(CAInfoService*)p;
		bool bIsFirst=true; //send our own certifcate only the first time
		bool bOneUpdateDone = false;

		UINT32 currentTime;
		UINT32 lastCascadeUpdate;
		UINT32 lastStatusUpdate;
		UINT32 lastMixInfoUpdate;
		UINT32 nextUpdate;
		bool bPreventLoop = false;
		UINT32 temp;
#ifdef DYNAMIC_MIX		
		UINT32 loops = 4;
		UINT32 interval = 0;
#endif	
		lastCascadeUpdate=lastMixInfoUpdate=lastStatusUpdate=(UINT32)time(NULL); 
		// tell the algorithm it is time to update
		lastCascadeUpdate -= CAInfoService::SEND_CASCADE_INFO_WAIT; 
		lastMixInfoUpdate -= CAInfoService::SEND_MIX_INFO_WAIT;
		lastStatusUpdate -= CAInfoService::SEND_STATUS_INFO_WAIT;  
		
    while(pInfoService->isRunning())
		{
#ifdef DYNAMIC_MIX		
			/** Spliting the wait time into parts allows faster reconfiguration of dynamic mixes */
			if(loops < 4)
			{
				sSleep(interval);
				loops++;
				continue;
			}
#endif
			currentTime=time(NULL);
			if (currentTime >= (lastStatusUpdate + CAInfoService::SEND_STATUS_INFO_WAIT))
			{
				if(pInfoService->sendStatus(bIsFirst)==E_SUCCESS)
				//if(pInfoService->sendStatus(true)==E_SUCCESS)
				{
					lastStatusUpdate=time(NULL);
					bIsFirst=false;
					bOneUpdateDone = true;
					CAMsg::printMsg(LOG_DEBUG,"InfoService: Successfully sent Status information.\n");
				}
				else
				{
					CAMsg::printMsg(LOG_DEBUG,"InfoService: Could not send Status information.\n");
				}
					
			}
			
			// check every minute if configuring, every 10 minutes otherwise
			currentTime=time(NULL);
		    if (currentTime >= (lastCascadeUpdate + CAInfoService::SEND_CASCADE_INFO_WAIT) || pInfoService->isConfiguring())
			{				
				if (pglobalOptions->isFirstMix() || (pglobalOptions->isLastMix() && pInfoService->isConfiguring()))
				{
					if(pInfoService->sendCascadeHelo()!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_ERR,"InfoService: Could not send Cascade information.\n");
					}
					else
					{
						lastCascadeUpdate=time(NULL);
						bOneUpdateDone = true;
						CAMsg::printMsg(LOG_DEBUG,"InfoService: Successfully sent Cascade information.\n");
					}
				}
				currentTime=time(NULL);
				if (currentTime >= (lastMixInfoUpdate + CAInfoService::SEND_MIX_INFO_WAIT) || pInfoService->isConfiguring())
				{
					if (pInfoService->sendMixHelo() != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_ERR,"InfoService: Could not send MixInfo information.\n");
					}
					else
					{
						lastMixInfoUpdate=time(NULL);
						bOneUpdateDone = true;
						CAMsg::printMsg(LOG_DEBUG,"InfoService: Successfully sent MixInfo information.\n");
					}
				}
			}
#ifdef DYNAMIC_MIX
			// LERNGRUPPE
			// Check regulary for new cascade information
			/** @todo Implement a reasonable interval, as cascades are created only once a day at most.
				* This could be used for semi-dynamic cascades too.
				* Note that increasing the delay might cause problems in CAMix::start (MiddleMixes entering 
				*::init and so on
				*/
			if( pglobalOptions->isDynamic() )
			{
				pInfoService->dynamicCascadeConfiguration();
			}
#endif
			currentTime=time(NULL);
			//@TODO BUGGy -- because it assumes knowledge about update times, which are configurable in StdAfx.hpp
			// wait CAInfoService::SEND_LOOP_SLEEP seconds at most
			temp = (currentTime - lastStatusUpdate);
			if (bOneUpdateDone && temp > 0)
			{
					if (temp <= CAInfoService::SEND_LOOP_SLEEP)
					{
						bPreventLoop = false;
						nextUpdate = CAInfoService::SEND_LOOP_SLEEP - temp;
					}
					else if (bPreventLoop)
					{
						// prevent infinite loops
						bPreventLoop = false;
						nextUpdate = CAInfoService::SEND_LOOP_SLEEP;
					}
					else
					{
						bPreventLoop = true;
						nextUpdate = 0;
					}
			}
			else
			{ 
				bPreventLoop = false;
				nextUpdate = CAInfoService::SEND_LOOP_SLEEP;
			}
#ifdef DYNAMIC_MIX
			interval = nextUpdate / 4;
			CAMsg::printMsg(LOG_DEBUG,"InfoService: Next update in %i seconds...interval %i\n", nextUpdate, interval);
			loops = 0;
#else
			CAMsg::printMsg(LOG_DEBUG,"InfoService: Next update in %i seconds...\n", nextUpdate);
			
			/* We can interrupt this thread if the mix is shutting down */
			pInfoService->m_pLoopCV->lock();
			pInfoService->m_pLoopCV->wait(nextUpdate * 1000);
			pInfoService->m_pLoopCV->unlock();
			
			//sSleep(nextUpdate);
#endif
		}
		FINISH_STACK("CAInfoService::InfoLoop");
		THREAD_RETURN_SUCCESS;
	}
	

struct CAInfoService::InfoServiceHeloMsg
{
	UINT8* strXML;
	UINT32 len;
	CASocketAddrINet* addr;
	CAInfoService* is;
	SINT32 requestCommand;
	const UINT8* param;
};

THREAD_RETURN CAInfoService::TCascadeHelo(void *p)
{
	INIT_STACK;
	BEGIN_STACK("CAInfoService::TCascadeHelo");
	
	InfoServiceHeloMsg* message = (InfoServiceHeloMsg*)p;
	if (message->is->sendCascadeHelo(message->strXML, message->len, message->addr) == E_SUCCESS)
	{
		FINISH_STACK("CAInfoService::TCascadeHelo");
		THREAD_RETURN_SUCCESS;
	}
	FINISH_STACK("CAInfoService::TCascadeHelo");
	THREAD_RETURN_SUCCESS;
}	

THREAD_RETURN CAInfoService::TCascadeStatus(void *p)
{
	INIT_STACK;
	BEGIN_STACK("CAInfoService::TCascadeStatus");
	
	InfoServiceHeloMsg* message = (InfoServiceHeloMsg*)p;
	if (message->is->sendStatus(message->strXML, message->len, message->addr) == E_SUCCESS)
	{
		FINISH_STACK("CAInfoService::TCascadeStatus");
		THREAD_RETURN_SUCCESS;
	}
	FINISH_STACK("CAInfoService::TCascadeStatus");
	THREAD_RETURN_SUCCESS;
}	

THREAD_RETURN CAInfoService::TMixHelo(void *p)
{
	INIT_STACK;
	BEGIN_STACK("CAInfoService::TMixHelo");
	
	InfoServiceHeloMsg* message = (InfoServiceHeloMsg*)p;
	if (message->is->sendMixHelo(message->strXML, message->len, message->requestCommand, message->param, message->addr) == E_SUCCESS)
	{
		FINISH_STACK("CAInfoService::TMixHelo");
		THREAD_RETURN_SUCCESS;
	}
	FINISH_STACK("CAInfoService::TMixHelo");
	THREAD_RETURN_SUCCESS;
}	
	
	
	
#ifdef DYNAMIC_MIX

bool CAInfoService::newCascadeAvailable()
{
	CASocket socket;
	CASocketAddrINet *address = NULL;
	CAHttpClient httpClient;
	bool ret = false;

	UINT32 status, contentLength;
	UINT8 buff[255];
	UINT8 bufMixId[60];
	UINT32 mixIdLen = 60;
	pglobalOptions->getMixId( bufMixId, mixIdLen );
	sprintf((char*)buff, "/newcascadeinformationavailable/%s",bufMixId ); 
	if( pglobalOptions->getRandomInfoService(address) != E_SUCCESS)
	{
		CAMsg::printMsg( LOG_ERR, "Unable to get a random InfoService - This will cause problems! Check your configuration!\n");
		return false;
	}
	if(socket.connect(*address)!=E_SUCCESS)
	{
		goto EXIT;
	}
	//Send request
	httpClient.setSocket(&socket);
	httpClient.sendGetRequest(buff);
	httpClient.parseHTTPHeader(&contentLength, &status);
	if(status!=200)
	{
		socket.close();
		ret = false;
		goto EXIT;
	}
	ret = true;
EXIT:
	delete address;
	address = NULL;
	return ret;	
}

/**
  * LERNGRUPPE
  * Asks a random InfoService if there exist new cascade information for this mix.
  * If so, the reconfiguration is started
  * @retval E_UNKOWN if an error occurs 
  * @retval E_SUCCESS otherwise
  */
SINT32 CAInfoService::dynamicCascadeConfiguration()
{
	SINT32 ret = E_UNKNOWN;
	CADynamicCascadeConfigurator *configurator = NULL;
	
	if(!newCascadeAvailable())
		return E_SUCCESS;

	/** @todo maybe use a mutex here so that no second dynamicCascadeConfiguration can begin */
	if(m_bReconfig) return E_SUCCESS;		
	CAMsg::printMsg( LOG_DEBUG, "Starting dynamic reconfiguration...\n");
	m_bReconfig = true;
		
	configurator = new CADynamicCascadeConfigurator( m_pSignature, m_pMix );
	ret = configurator->configure();
	/** @todo Is this really safe? State of configuration is unknown here, reset should work though */
	if(ret != E_SUCCESS)
	{
		pglobalOptions->resetNextMix();
		pglobalOptions->resetPrevMix();
		pglobalOptions->setCascadeProposal(NULL, 0);
		m_pMix->dynaReconfigure(false);
	}	
	m_bReconfig = false;
	delete configurator;
	configurator = NULL;
	
	return ret;
}
#endif

CAInfoService::CAInfoService()
{
    m_pMix=NULL;
		m_bRun=false;
		m_pSignature=NULL;
		m_pcertstoreOwnCerts=NULL;
		m_minuts=0;
		m_lastMixedPackets=0;
    m_expectedMixRelPos = 0;
    	m_pLoopCV = new CAConditionVariable();
		m_pthreadRunLoop=new CAThread((UINT8*)"InfoServiceThread");
#ifdef DYNAMIC_MIX
		m_bReconfig = false;
#endif
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
    m_pLoopCV = new CAConditionVariable();
    m_pthreadRunLoop=new CAThread((UINT8*)"InfoServiceThread");
		m_pMix=pMix;
#ifdef DYNAMIC_MIX
		m_bReconfig = false;
#endif
	}

CAInfoService::~CAInfoService()
	{
		stop();
		delete m_pLoopCV;
		m_pLoopCV = NULL;
		delete m_pcertstoreOwnCerts;
		m_pcertstoreOwnCerts = NULL;
		delete m_pthreadRunLoop;
		m_pthreadRunLoop = NULL;
	}
/** Sets the signature used to sign the messages send to Infoservice.
	* If pOwnCert!=NULL this Certifcate is included in the Signature
	*/
SINT32 CAInfoService::setSignature(CASignature* pSig, CACertificate* pOwnCert,
								CACertificate** a_opCerts, UINT32 a_opCertsLength)
	{
		m_pSignature=pSig;
		
		delete m_pcertstoreOwnCerts;
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
    if(m_pMix!=NULL && pglobalOptions->isFirstMix())
        return ((CAFirstMix*)m_pMix)->getLevel(puser,prisk,ptraffic);
    return E_UNKNOWN;
}

SINT32 CAInfoService::getMixedPackets(UINT64& ppackets)
	{
		if(m_pMix!=NULL && pglobalOptions->isFirstMix())
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
				/* Interrupt this thread */
				m_pLoopCV->lock();
				m_pLoopCV->signal();
				m_pLoopCV->unlock();
				
				m_pthreadRunLoop->join();
			}
		return E_SUCCESS;
	}

SINT32 CAInfoService::sendStatus(bool bIncludeCerts)
{
	if(!pglobalOptions->isFirstMix())
	{
		return E_SUCCESS;
	}
	
	UINT32 len;
	SINT32 ret;
	UINT8* strStatusXML=getStatusXMLAsString(bIncludeCerts,len);
	
	if(strStatusXML==NULL)
	{
		return E_UNKNOWN;
	}
	
	ret = sendHelo(strStatusXML, len, TCascadeStatus, (UINT8*)"Status Thread", REQUEST_COMMAND_STATUS);
	delete[] strStatusXML;
	strStatusXML = NULL;
	return ret;
}



UINT8* CAInfoService::getStatusXMLAsString(bool bIncludeCerts,UINT32& len)
	{
		SINT32 tmpUser,tmpRisk,tmpTraffic;
		UINT64 tmpPackets;
			
		//httpClient.setSocket(&oSocket);
		UINT8 strMixId[255];
		pglobalOptions->getMixId(strMixId,255);
		
		tmpUser=tmpTraffic=tmpRisk=-1;
		set64(tmpPackets,(UINT32)-1);
		getLevel(&tmpUser,&tmpRisk,&tmpTraffic);
		SINT32 ret=getMixedPackets(tmpPackets);
		if(ret==E_SUCCESS)
			{
				UINT32 avgTraffic=div64(tmpPackets,m_minuts);
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
			}
		m_minuts++;
//let the attributes in alphabetical order..
#define XML_MIX_CASCADE_STATUS "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<MixCascadeStatus LastUpdate=\"%s\" currentRisk=\"%i\" id=\"%s\" mixedPackets=\"%s\" nrOfActiveUsers=\"%i\" trafficSituation=\"%i\"\
></MixCascadeStatus>"
				
		UINT32 buffLen=8192;
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
		{
			ptmpCertStore=NULL;
		}
		if(m_pSignature->signXML(tmpBuff,strlen((char*)tmpBuff),buff,&buffLen,ptmpCertStore)!=E_SUCCESS)
		{
			delete[] buff;
			buff = NULL;
		}
		else
		{
			len=buffLen;
		}
		return buff;
	}


/** POSTs mix status to the InfoService. [only first mix does this at the moment]
	* @retval E_UNKNOWN if something goes wrong
	* @retval E_SUCCESS otherwise
	*
	*/
	///todo use httpclient class
SINT32 CAInfoService::sendStatus(const UINT8* a_strStatusXML,UINT32 a_len, const CASocketAddrINet* a_pSocketAddress) const
	{
		UINT8 buffHeader[512];
		SINT32 ret = E_UNKNOWN;
		UINT8 hostname[255];
		UINT64 currentTimeout = MIX_TO_INFOSERVICE_TIMEOUT;
		UINT64 startupTime, currentMillis;
		
		
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAInfoService::sendStatus()\n");
		#endif
		if(!pglobalOptions->isFirstMix())
		{
			return E_SUCCESS;
		}
		CASocket oSocket(true);
		if (a_pSocketAddress == NULL)
		{
			return E_UNKNOWN;
		}
		
		if(oSocket.connect(*a_pSocketAddress, MIX_TO_INFOSERVICE_TIMEOUT)!=E_SUCCESS)
		{
			return E_UNKNOWN;
		}
		
		if(a_pSocketAddress->getIPAsStr(hostname, 255) == E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG, "InfoService: Sending current status to InfoService %s:%d.\n", hostname, a_pSocketAddress->getPort());
			sprintf((char*)buffHeader,"POST /feedback HTTP/1.0\r\nContent-Length: %u\r\n\r\n",a_len);
			
			getcurrentTimeMillis(startupTime);			
			oSocket.sendFullyTimeOut(buffHeader,strlen((char*)buffHeader), currentTimeout, SEND_INFO_TIMEOUT_MS);
			
			getcurrentTimeMillis(currentMillis);	
			currentTimeout -= (currentMillis - startupTime);
			ret=oSocket.sendFullyTimeOut(a_strStatusXML, a_len, currentTimeout, SEND_INFO_TIMEOUT_MS);
		}

		oSocket.close();	
		if(ret==E_SUCCESS)
		{
			return E_SUCCESS;
		}
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
	SINT32 ret;
	UINT8* strMixHeloXML=getMixHeloXMLAsString(len);
	
	if(strMixHeloXML==NULL)
	{
		CAMsg::printMsg(LOG_DEBUG,"InfoService:sendMixHelo() -- Error: getMixHeloXMLAsString() returned NULL!\n");
		return E_UNKNOWN;
	}
	
	ret = sendHelo(strMixHeloXML, len, TMixHelo, (UINT8*)"Mix Helo Thread", requestCommand, param);
	delete[] strMixHeloXML;
	strMixHeloXML = NULL;
	return ret;
}


UINT8* CAInfoService::getMixHeloXMLAsString(UINT32& a_len)
	{
    a_len = 0;

		UINT32 sendBuffLen;
		UINT8* sendBuff=NULL;
		XERCES_CPP_NAMESPACE::DOMDocument* docMixInfo=NULL;
		if(pglobalOptions->getMixXml(docMixInfo)!=E_SUCCESS)
			{
				goto ERR;
			}
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
		delete []sendBuff;
		sendBuff = NULL;
		return NULL;
	}


/** POSTs the HELO message for a mix to the InfoService.
	*/
SINT32 CAInfoService::sendMixHelo(const UINT8* a_strMixHeloXML,UINT32 a_len,SINT32 requestCommand,const UINT8* param,
									const CASocketAddrINet* a_pSocketAddress)
	{
    UINT8* recvBuff = NULL;
    SINT32 ret = E_SUCCESS;
    UINT32 len = 0;

		CASocket oSocket(true);
		UINT8 hostname[255];
		UINT8 buffHeader[255];
		CAHttpClient httpClient;

		UINT64 currentTimeout = MIX_TO_INFOSERVICE_TIMEOUT;
		UINT64 startupTime, currentMillis;

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
		else if(requestCommand==REQUEST_COMMAND_MIXINFO)
			{
				requestType=REQUEST_TYPE_GET;
				receiveAnswer = true;
			}
	
		if(a_pSocketAddress->getIPAsStr(hostname, 255)!=E_SUCCESS)
			{
				goto ERR;
			}
		
    oSocket.setRecvBuff(255);
		if(oSocket.connect(*a_pSocketAddress, MIX_TO_INFOSERVICE_TIMEOUT)==E_SUCCESS)
		{
			httpClient.setSocket(&oSocket);
			const char* strRequestCommand=STRINGS_REQUEST_COMMANDS[requestCommand];
			const char* strRequestType=STRINGS_REQUEST_TYPES[requestType];
			CAMsg::printMsg(LOG_DEBUG,"InfoService: Sending [%s] %s to InfoService %s:%d.\r\n", strRequestType,strRequestCommand, hostname, a_pSocketAddress->getPort());
			if(requestCommand==REQUEST_COMMAND_MIXINFO)
   			{
				sprintf((char*)buffHeader,"%s /%s%s HTTP/1.0\r\nContent-Length: %u\r\n\r\n", strRequestType, strRequestCommand, param,a_len);
   			}
			else
			{
				sprintf((char*)buffHeader,"%s /%s HTTP/1.0\r\nContent-Length: %u\r\n\r\n", strRequestType, strRequestCommand, a_len);
			}
			
		getcurrentTimeMillis(startupTime);
		if (oSocket.sendFullyTimeOut(buffHeader,strlen((char*)buffHeader), currentTimeout, SEND_INFO_TIMEOUT_MS)!=E_SUCCESS)
		{
			goto ERR;
		}

		getcurrentTimeMillis(currentMillis);
		currentTimeout -= (currentMillis - startupTime);
		if (currentTimeout <= 0 ||
			oSocket.sendFullyTimeOut(a_strMixHeloXML, a_len, currentTimeout, SEND_INFO_TIMEOUT_MS)!=E_SUCCESS)
		{
			goto ERR;
		}

    if(receiveAnswer)
      {
				getcurrentTimeMillis(currentMillis);	
				currentTimeout -= (currentMillis - startupTime);
				if(currentTimeout <= 0 || httpClient.parseHTTPHeader(&len) != E_SUCCESS)
				{
					goto ERR;
				}
				
	      if(len > 0)
            {
            	getcurrentTimeMillis(currentMillis);	
				currentTimeout -= (currentMillis - startupTime);
				
                recvBuff = new UINT8[len+1];                
                if (currentTimeout <= 0 || 
					oSocket.receiveFullyT(recvBuff, len, currentTimeout) != E_SUCCESS)
                {
					delete []recvBuff;
                    recvBuff=NULL;
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
            XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(recvBuff,len);
            delete[] recvBuff;
            recvBuff=NULL;
            DOMElement* root=NULL;
            if(doc!=NULL && (root = doc->getDocumentElement()) != NULL)
            {
                if(equals(root->getNodeName(),"MixCascade"))
                {
                    ret = handleConfigEvent(doc);
                    if(ret == E_SUCCESS)
                        m_bConfiguring = false;
                    else
                        return ret;
                }
                else if(equals(root->getNodeName(),"Mix"))
                {
                    if(m_expectedMixRelPos < 0)
											{
												XMLCh* id=XMLString::transcode("id");
												char* tmpStr=XMLString::transcode(root->getAttribute(id));
                        CAMsg::printMsg(LOG_DEBUG,"InfoService: Setting new previous mix: %s\n",tmpStr);
												XMLString::release(&tmpStr);
												XMLString::release(&id);
                        pglobalOptions->setPrevMix(doc);
											}
                    else if(m_expectedMixRelPos > 0)
											{
												XMLCh* id=XMLString::transcode("id");
												char* tmpStr=XMLString::transcode(root->getAttribute(id));
                        CAMsg::printMsg(LOG_DEBUG,"InfoService: Setting new next mix: %s\n",tmpStr);
												XMLString::release(&id);
												XMLString::release(&tmpStr);
                        pglobalOptions->setNextMix(doc);
											}
                }
                CAMsg::printMsg(LOG_DEBUG,"InfoService::sendMixHelo(): XML infoservice doc 0x%x not needed anymore.\n", 
                		doc);
            }
            else
            {
                CAMsg::printMsg(LOG_CRIT,"InfoService: Error parsing answer from InfoService!\n");
            }
        }
		return E_SUCCESS;	
	}
	else
	{
    	CAMsg::printMsg(LOG_DEBUG,"InfoService: sendMixHelo() connecting to InfoService %s:%d failed!\n", hostname, a_pSocketAddress->getPort());
	}
ERR:
	oSocket.close();
	return E_UNKNOWN;
}


SINT32 CAInfoService::sendHelo(UINT8* a_strXML, UINT32 a_len, THREAD_RETURN (*a_thread)(void *), UINT8* a_strThreadName, SINT32 requestCommand, const UINT8* param)
{
	SINT32 returnValue = E_UNKNOWN;
	UINT32 nrAddresses;
	CAListenerInterface** socketAddresses = pglobalOptions->getInfoServices(nrAddresses);
	CAThread** threads = new CAThread*[nrAddresses];
	InfoServiceHeloMsg** messages = new InfoServiceHeloMsg*[nrAddresses];
	
	for (UINT32 i = 0; i < nrAddresses; i++)
	{
		messages[i] = new InfoServiceHeloMsg();
		messages[i]->addr = (CASocketAddrINet*)socketAddresses[i]->getAddr();
		messages[i]->len = a_len;
		messages[i]->strXML = a_strXML;
		messages[i]->is = this;
		messages[i]->requestCommand = requestCommand;
		messages[i]->param = param;
		threads[i] = new CAThread(a_strThreadName);
		threads[i]->setMainLoop((THREAD_RETURN (*)(void *))a_thread);
		threads[i]->start((void*)(messages[i]), false, true);
	}
	
	for (UINT32 i = 0; i < nrAddresses; i++)
	{
		if (threads[i]->join() == E_SUCCESS)
		{
			returnValue = E_SUCCESS;
		}
		delete messages[i]->addr;
		messages[i]->addr = NULL;
		delete messages[i];
		messages[i] = NULL;
		delete threads[i];
		threads[i] = NULL;
	}
	
	delete[] messages;
	messages = NULL;
	delete[] threads;
	threads = NULL;
	
	return returnValue;
}



SINT32 CAInfoService::sendCascadeHelo()
{	
	if(pglobalOptions->isMiddleMix())
  	{
		return E_SUCCESS;
  	}
  	
	UINT32 len;
	SINT32 ret;
	UINT8* strCascadeHeloXML=getCascadeHeloXMLAsString(len);
	if(strCascadeHeloXML==NULL)
	{
		return E_UNKNOWN;
	}
	
	ret = sendHelo(strCascadeHeloXML, len, TCascadeHelo, (UINT8*)"Cascade Helo Thread", REQUEST_COMMAND_CASCADE);
	delete[] strCascadeHeloXML;
	strCascadeHeloXML = NULL;
	return ret;
}

UINT8* CAInfoService::getCascadeHeloXMLAsString(UINT32& a_len)
	{	
		a_len=0;
 		UINT32 sendBuffLen;
		UINT8* sendBuff=NULL;
		XERCES_CPP_NAMESPACE::DOMDocument* docMixInfo=NULL;
		DOMElement* elemTimeStamp=NULL;
		DOMElement* elemRoot=NULL;
		
	  if(m_pMix->getMixCascadeInfo(docMixInfo)!=E_SUCCESS)
		{
	    CAMsg::printMsg(LOG_INFO,"InfoService: Cascade not yet configured.\n");
			goto ERR;
		}
		//insert (or update) the Timestamp
		elemRoot=docMixInfo->getDocumentElement();
		
		if(getDOMChildByName(elemRoot,"LastUpdate",elemTimeStamp,false)!=E_SUCCESS)
			{
				elemTimeStamp=createDOMElement(docMixInfo,"LastUpdate");
				elemRoot->appendChild(elemTimeStamp);
			}
		UINT64 currentMillis;
		getcurrentTimeMillis(currentMillis);
		UINT8 tmpStrCurrentMillis[50];
		print64(tmpStrCurrentMillis,currentMillis);
		setDOMElementValue(elemTimeStamp,tmpStrCurrentMillis);
		
		if (m_serial != 0)
		{
			print64(tmpStrCurrentMillis, m_serial);
			setDOMElementAttribute(elemRoot, "serial", tmpStrCurrentMillis);
		}
		
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
		delete []sendBuff;
		sendBuff = NULL;
		return NULL;
	}

/** POSTs the HELO message for a whole cascade to the InfoService.
 * If the running mix is a last mix, this method is used to inform the
 * InfoService that it wants to create a new cascade. The InfoService
 * then tells all involved mixes to join this cascade.
 * If the current mix is a middle mix, this method does nothing.
 * @retval E_SUCCESS on success
 * @retval E_UNKNOWN on any error
	*/
SINT32 CAInfoService::sendCascadeHelo(const UINT8* a_strCascadeHeloXML,UINT32 a_len,const CASocketAddrINet* a_pSocketAddress) const
{	
	if(pglobalOptions->isMiddleMix() || (pglobalOptions->isLastMix() && !m_bConfiguring))
	{
		return E_SUCCESS;
	}
	CASocket oSocket(true);
	UINT8 hostname[255];
	UINT8 buffHeader[255];
	UINT64 currentTimeout = MIX_TO_INFOSERVICE_TIMEOUT;
	UINT64 startupTime, currentMillis;
	
	if (a_pSocketAddress == NULL)
	{
		return E_UNKNOWN;
	}
	
	if(a_pSocketAddress->getIPAsStr(hostname, 255)!=E_SUCCESS)
	{
		goto ERR;
	}
	if(oSocket.connect(*a_pSocketAddress, MIX_TO_INFOSERVICE_TIMEOUT)==E_SUCCESS)
	{
	    if(pglobalOptions->isFirstMix())
		{
            CAMsg::printMsg(LOG_DEBUG,"InfoService: Sending cascade helo to InfoService %s:%d.\r\n", hostname, a_pSocketAddress->getPort());
		}
	    else
		{
            CAMsg::printMsg(LOG_DEBUG,"InfoService: Sending cascade configuration request to InfoService %s:%d.\r\n", hostname, a_pSocketAddress->getPort());
		}
        // LERNGRUPPE
        // Semi-dynamic cascades are temporary cascades, not yet established! InfoService can cope with them now
        // using the /dynacascade command
	    if( pglobalOptions->isLastMix() && m_bConfiguring )
		{
			sprintf((char*)buffHeader,"POST /dynacascade HTTP/1.0\r\nContent-Length: %u\r\n\r\n",a_len);
		}
	    else
		{
			sprintf((char*)buffHeader,"POST /cascade HTTP/1.0\r\nContent-Length: %u\r\n\r\n",a_len);
		}
		
		getcurrentTimeMillis(startupTime);
		if(	oSocket.sendFullyTimeOut(buffHeader, strlen((char*)buffHeader), currentTimeout, SEND_INFO_TIMEOUT_MS)!=E_SUCCESS)
		{
			goto ERR;
		}
		getcurrentTimeMillis(currentMillis);	
		currentTimeout -= (currentMillis - startupTime);
		if (currentTimeout <= 0 ||
		    oSocket.sendFullyTimeOut(a_strCascadeHeloXML, a_len, currentTimeout, SEND_INFO_TIMEOUT_MS)!=E_SUCCESS)
		{
			goto ERR;
		}
		//Receive answer --> 200 Ok or failure
		//HTTP/1.1 200 Ok
		getcurrentTimeMillis(currentMillis);	
		currentTimeout -= (currentMillis - startupTime);
		if(currentTimeout <= 0 ||
		   oSocket.receiveFullyT(buffHeader, 12, currentTimeout)!=E_SUCCESS)
		{
			goto ERR;
		}
		if(memcmp(buffHeader+9,"200",3)!=0)
		{
			goto ERR;
		}
		oSocket.close();
		return E_SUCCESS;	
	}
ERR:
	oSocket.close();
	return E_UNKNOWN;
}

SINT32 CAInfoService::handleConfigEvent(XERCES_CPP_NAMESPACE::DOMDocument* doc) const
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
    pglobalOptions->getMixId(myMixId,64);
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
	CAListenerInterface** socketAddresses = pglobalOptions->getInfoServices(nrAddresses);
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
	@param a_pstrPIID id of the payment instance for which the information is requested
	@param a_pXMLBI a pointer to a pointer which on a successful return will point to a newly created CAXMLBI object
	@param a_socketAddress adress of the InfoService from which the information is to be requested
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
		*a_pXMLBI=NULL;
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
				content = NULL;
				return E_UNKNOWN;
			}
		socket.close();
		//Parse XML
		XERCES_CPP_NAMESPACE::DOMDocument* doc = parseDOMDocument(content,contentLength);
		delete []content;
		content = NULL;
		if(doc==NULL)
			return E_UNKNOWN;
		DOMElement* elemRoot=doc->getDocumentElement();

		*a_pXMLBI = CAXMLBI::getInstance(elemRoot);
		CAMsg::printMsg(LOG_DEBUG, "CAInfoService::getPaymentInstance(): XML doc not needed anymore\n");
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
#endif //ONLY_LOCAL_PROXY
