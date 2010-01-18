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
#include "CAFirstMix.hpp"
#include "CASocketGroup.hpp"
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CAFirstMixChannelList.hpp"
#include "CAListenerInterface.hpp"
#include "CAASymCipher.hpp"
#include "CAInfoService.hpp"
#include "CASocketAddrINet.hpp"
#include "CATempIPBlockList.hpp"
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
	#include "CASocketAddrUnix.hpp"
#endif
#include "CAThread.hpp"
#include "CAUtil.hpp"
#include "CASignature.hpp"
#include "CABase64.hpp"
#include "xml/DOM_Output.hpp"
#include "CAPool.hpp"
#ifdef WITH_CONTROL_CHANNELS_TEST
	#include "CAControlChannelTest.hpp"
#endif
#ifdef PAYMENT
	#include "CAAccountingControlChannel.hpp"
	#include "CAAccountingInstance.hpp"
	#include "CAAccountingDBInterface.hpp"
#endif
#include "CAReplayControlChannel.hpp"
#include "CAStatusManager.hpp"
#include "CALibProxytest.hpp"
const UINT32 CAFirstMix::MAX_CONCURRENT_NEW_CONNECTIONS = NUM_LOGIN_WORKER_TRHEADS * 2;

bool CAFirstMix::isShuttingDown()
{
	return m_bIsShuttingDown;
}

SINT32 CAFirstMix::initOnce()
	{
		SINT32 ret=CAMix::initOnce();
		if(ret!=E_SUCCESS)
			return ret;
		CAMsg::printMsg(LOG_DEBUG,"Starting FirstMix InitOnce\n");
		/*m_pSignature=CALibProxytest::getOptions()->getSignKey();
		if(m_pSignature==NULL)*/
		m_pMultiSignature = CALibProxytest::getOptions()->getMultiSigner();
		if(m_pMultiSignature == NULL)
			return E_UNKNOWN;
		//Try to find out how many (real) ListenerInterfaces are specified
		UINT32 tmpSocketsIn=CALibProxytest::getOptions()->getListenerInterfaceCount();
		m_nSocketsIn=0;
		for(UINT32 i=1;i<=tmpSocketsIn;i++)
			{
				CAListenerInterface* pListener=NULL;
				pListener=CALibProxytest::getOptions()->getListenerInterface(i);
				if(pListener==NULL)
					continue;
				if(!pListener->isVirtual())
					m_nSocketsIn++;
				delete pListener;
				pListener = NULL;
			}
		if(m_nSocketsIn<1)
			{
				CAMsg::printMsg(LOG_CRIT,"No useable ListenerInterfaces specified (maybe wrong values or all are 'virtual'!\n");
				return E_UNKNOWN;
			}

		CAMsg::printMsg(LOG_DEBUG,"Starting FirstMix InitOnce - finished\n");
		return E_SUCCESS;
	}



SINT32 CAFirstMix::init()
	{
		if (isShuttingDown())
		{
			return E_SHUTDOWN;
		}

#ifdef DYNAMIC_MIX
		m_bBreakNeeded = m_bReconfigured;
#endif

		CAMsg::printMsg(LOG_DEBUG,"Starting FirstMix Init\n");
		m_nMixedPackets=0; //reset to zero after each restart (at the moment neccessary for infoservice)
		m_bRestart=false;
		//Establishing all Listeners
		UINT32 i;
		m_arrSocketsIn=new CASocket*[m_nSocketsIn];
		for (i = 0; i < m_nSocketsIn; i++)
		{
			m_arrSocketsIn[i] = NULL;
		}
		//initiate ownerDocument for tc templates
		m_templatesOwner = createDOMDocument();

		SINT32 retSockets = CALibProxytest::getOptions()->createSockets(true, m_arrSocketsIn, m_nSocketsIn);


		if (retSockets != E_SUCCESS)
		{
			return retSockets;
		}

		CASocketAddr* pAddrNext=NULL;
		for(i=0;i<CALibProxytest::getOptions()->getTargetInterfaceCount();i++)
			{
				TargetInterface oNextMix;
				CALibProxytest::getOptions()->getTargetInterface(oNextMix,i+1);
				if(oNextMix.target_type==TARGET_MIX)
				{
					pAddrNext=oNextMix.addr;
					break;
				}
				delete oNextMix.addr;
				oNextMix.addr = NULL;
			}
		if(pAddrNext==NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"No next Mix specified!\n");
				return E_UNKNOWN;
			}
		m_pMuxOut=new CAMuxSocket();
		if(m_pMuxOut->getCASocket()->create(pAddrNext->getType())!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot create SOCKET for connection to next Mix!\n");
				return E_UNKNOWN;
			}
		m_pMuxOut->getCASocket()->setSendBuff(500*MIXPACKET_SIZE);
		m_pMuxOut->getCASocket()->setRecvBuff(500*MIXPACKET_SIZE);
		//if(((*m_pMuxOut))->setSendLowWat(MIXPACKET_SIZE)!=E_SUCCESS)
		//	CAMsg::printMsg(LOG_INFO,"SOCKET Option SENDLOWWAT not set!\n");
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET RecvBuffSize: %i\n",m_pMuxOut->getCASocket()->getRecvBuff());
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET SendBuffSize: %i\n",m_pMuxOut->getCASocket()->getSendBuff());
		//CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET SendLowWatSize: %i\n",((*m_pMuxOut))->getSendLowWat());

		/** Connect to the next mix */
		if((retSockets = connectToNextMix(pAddrNext)) != E_SUCCESS)
		{
			delete pAddrNext;
			pAddrNext = NULL;
			CAMsg::printMsg(LOG_DEBUG, "CAFirstMix::init - Unable to connect to next mix. Reason: %s (%i)\n", GET_NET_ERROR_STR(retSockets), retSockets);				
			return E_UNKNOWN;
		}
		delete pAddrNext;
		pAddrNext = NULL;
		MONITORING_FIRE_NET_EVENT(ev_net_nextConnected);
		CAMsg::printMsg(LOG_INFO," connected!\n");
		if(m_pMuxOut->getCASocket()->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(m_pMuxOut->getCASocket()->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}

    if(processKeyExchange()!=E_SUCCESS)
    {
    	MONITORING_FIRE_NET_EVENT(ev_net_nextConnectionClosed);
    	CAMsg::printMsg(LOG_CRIT,"Error in establishing secure communication with next Mix!\n");
        return E_UNKNOWN;
    }
		m_pIPList=new CAIPList();
		m_pIPBlockList = new CATempIPBlockList(1000 * 60 * 2); 
#ifdef COUNTRY_STATS
		char* db_host;
		char* db_user;
		char* db_passwd;
		CALibProxytest::getOptions()->getCountryStatsDBConnectionLoginData(&db_host,&db_user,&db_passwd);
		SINT32 retcountrydb=initCountryStats(db_host,db_user,db_passwd);
		delete[] db_host;
		db_host = NULL;
		delete[] db_user;
		db_user = NULL;
		delete[] db_passwd;
		db_passwd = NULL;
		if(retcountrydb!=E_SUCCESS)
			return E_UNKNOWN;
#endif
		m_pQueueSendToMix=new CAQueue(sizeof(tQueueEntry));
		m_pQueueReadFromMix=new CAQueue(sizeof(tQueueEntry));
		m_pChannelList=new CAFirstMixChannelList();
#ifdef HAVE_EPOLL
		m_psocketgroupUsersRead=new CASocketGroupEpoll(false);
		m_psocketgroupUsersWrite=new CASocketGroupEpoll(true);
#else
		m_psocketgroupUsersRead=new CASocketGroup(false);
		m_psocketgroupUsersWrite=new CASocketGroup(true);
#endif

		m_pMuxOutControlChannelDispatcher=new CAControlChannelDispatcher(m_pQueueSendToMix,NULL,NULL);
#ifdef REPLAY_DETECTION
		m_pReplayDB=new CADatabase();
		m_pReplayDB->start();
		m_pReplayMsgProc=new CAReplayCtrlChannelMsgProc(this);
		m_u64ReferenceTime=time(NULL);
		m_u64LastTimestampReceived=time(NULL);
#endif

#ifdef PAYMENT
		if(CAAccountingDBInterface::init() != E_SUCCESS)
		{
			exit(1);
		}
		CAAccountingInstance::init(this);
#endif

		m_pthreadsLogin=new CAThreadPool(NUM_LOGIN_WORKER_TRHEADS,MAX_LOGIN_QUEUE,false);

		//Starting thread for Step 1
		m_pthreadAcceptUsers=new CAThread((UINT8*)"CAFirstMix - AcceptUsers");
		m_pthreadAcceptUsers->setMainLoop(fm_loopAcceptUsers);
		m_pthreadAcceptUsers->start(this);

		//Starting thread for Step 3
		m_pthreadSendToMix=new CAThread((UINT8*)"CAFirstMix - SendToMix");
		m_pthreadSendToMix->setMainLoop(fm_loopSendToMix);
		m_pthreadSendToMix->start(this);

		//Startting thread for Step 4a
		m_pthreadReadFromMix=new CAThread((UINT8*)"CAFirstMix - ReadFromMix");
		m_pthreadReadFromMix->setMainLoop(fm_loopReadFromMix);
		m_pthreadReadFromMix->start(this);
#ifdef CH_LOG_STUDY
		nrOfChThread = new CAThread((UINT8*)"CAFirstMix - Channel open logging thread");
		nrOfChThread->setMainLoop(fm_loopLogChannelsOpened);
		nrOfChThread->start(this);
		currentOpenedChannels = 0;
#endif //CH_LOG_STUDY
		//Starting thread for logging
#ifdef LOG_PACKET_TIMES
		m_pLogPacketStats=new CALogPacketStats();
		m_pLogPacketStats->setLogIntervallInMinutes(FM_PACKET_STATS_LOG_INTERVALL);
		m_pLogPacketStats->start();
#endif
#ifdef REPLAY_DETECTION
//		sendReplayTimestampRequestsToAllMixes();
#endif
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix init() succeeded\n");
		MONITORING_FIRE_NET_EVENT(ev_net_keyExchangeNextSuccessful);
		return E_SUCCESS;
}

SINT32 CAFirstMix::connectToNextMix(CASocketAddr* a_pAddrNext)
{
	UINT8 buff[255];
	a_pAddrNext->toString(buff,255);
	CAMsg::printMsg(LOG_INFO,"Try to connect to next Mix on %s ...\n",buff);
	SINT32 err = E_UNKNOWN;
	SINT32 errLast = E_SUCCESS;
	
	for(UINT32 i=0; i < 100; i++)
	{
#ifdef DYNAMIC_MIX
		/** If someone reconfigured the mix, we need to break the loop as the next mix might have changed */
		if(m_bBreakNeeded != m_bReconfigured)
		{
			CAMsg::printMsg(LOG_DEBUG, "CAFirstMix::connectToNextMix - Broken the connect loop!\n");
			break;
		}
#endif
		err = m_pMuxOut->connect(*a_pAddrNext);
		if(err != E_SUCCESS)
		{
			err=GET_NET_ERROR;
			
			if(err!=ERR_INTERN_TIMEDOUT&&err!=ERR_INTERN_CONNREFUSED)
				break;
				
			if (errLast != err || i % 10 == 0)
			{
				CAMsg::printMsg(LOG_ERR, "Cannot connect to next Mix on %s. Reason: %s (%i). Retrying...\n",
					buff, GET_NET_ERROR_STR(err), err);
				errLast = err;
			}
			else
			{
#ifdef _DEBUG
				CAMsg::printMsg(LOG_DEBUG,"Cannot connect... retrying\n");
#endif
			}
			sSleep(10);
		}
		else
		{
			break;
		}
	}
	return err;
}

/*
*@DOCME
*/
SINT32 CAFirstMix::processKeyExchange()
{
    UINT8* recvBuff=NULL;
    UINT32 len;
	CAMsg::printMsg(LOG_INFO, "Try to read the Key Info length from next Mix...\n");
    if(m_pMuxOut->receiveFully((UINT8*) &len, sizeof(len) ) != E_SUCCESS)
    {
        CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info length!\n");
        return E_UNKNOWN;
    }
    len=ntohl(len);
    CAMsg::printMsg(LOG_INFO, "Received Key Info length %u\n",len);
    recvBuff=new UINT8[len+1];

    if(m_pMuxOut->receiveFully(recvBuff, len) != E_SUCCESS)
    {
        CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info!\n");
        delete []recvBuff;
        recvBuff = NULL;
        return E_UNKNOWN;
    }
    recvBuff[len]=0;
    //get the Keys from the other mixes (and the Mix-Id's...!)
    CAMsg::printMsg(LOG_INFO,"Received Key Info...\n");
    CAMsg::printMsg(LOG_DEBUG,"%s\n",recvBuff);

    XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(recvBuff, len);
    delete []recvBuff;
    recvBuff = NULL;
    DOMElement* elemMixes=doc->getDocumentElement();
    if(elemMixes == NULL)
    {
		if(doc != NULL)
		{
			doc->release();
			doc = NULL;
		}
		CAMsg::printMsg(LOG_INFO,"before returning...\n");
		return E_UNKNOWN;
    }
		SINT32 count=0;
    if(getDOMElementAttribute(elemMixes,"count",&count)!=E_SUCCESS)
    {
    	if(doc != NULL)
		{
			doc->release();
			doc = NULL;
		}
        return E_UNKNOWN;
    }
 /*
		@todo Do not know why we do this here - probably it has something todo with the
		dynamic mix config, but makes not sense at all for me...

		getDOMElementAttribute(elemMixescascadeNaem
		char *cascadeName;
    cascadeName = elemMixes.getAttribute("cascadeName").transcode();
    if(cascadeName == NULL)
        return E_UNKNOWN;
    CALibProxytest::getOptions()->setCascadeName(cascadeName);
*/
    if(CALibProxytest::getOptions()->getTermsAndConditions() != NULL)
    {
    	appendTermsAndConditionsExtension(doc, elemMixes);
    }

    SINT32 extRet = handleKeyInfoExtensions(elemMixes);
    if(extRet != E_SUCCESS)
    {
    	if(doc != NULL)
		{
			doc->release();
			doc = NULL;
		}
		return E_UNKNOWN;
    }

    m_pRSA=new CAASymCipher;
    m_pRSA->generateKeyPair(1024);

    DOMNode* child=elemMixes->getLastChild();

    //tmp XML-Structure for constructing the XML which is sent to each user
    XERCES_CPP_NAMESPACE::DOMDocument* docXmlKeyInfo=createDOMDocument();
    DOMElement* elemRootKey=createDOMElement(docXmlKeyInfo,"MixCascade");
    setDOMElementAttribute(elemRootKey,"version",(UINT8*)"0.2"); //set the Version of the XML to 0.2
#ifdef LOG_DIALOG
    setDOMElementAttribute(elemRootKey,"study",(UINT8*)"true");
#endif
    docXmlKeyInfo->appendChild(elemRootKey);
    DOMElement* elemMixProtocolVersion=createDOMElement(docXmlKeyInfo,"MixProtocolVersion");
    setDOMElementValue(elemMixProtocolVersion,(UINT8*)MIX_CASCADE_PROTOCOL_VERSION);
    elemRootKey->appendChild(elemMixProtocolVersion);
    DOMNode* elemMixesKey=docXmlKeyInfo->importNode(elemMixes,true);
    elemRootKey->appendChild(elemMixesKey);

    //UINT32 tlen;
    /* //remove because it seems to be useless...
		while(child!=NULL)
    {
        if(child.getNodeName().equals("Mix"))
        {
            DOM_Node rsaKey=child.getFirstChild();
            CAASymCipher oRSA;
            oRSA.setPublicKeyAsDOMNode(rsaKey);
            tlen=256;
        }
        child=child.getPreviousSibling();
    }*/
    //tlen=256;

    //Inserting own Key in XML-Key struct
    DOMElement* elemKey=NULL;
    m_pRSA->getPublicKeyAsDOMElement(elemKey,docXmlKeyInfo);
    addMixInfo(elemMixesKey, true);
    DOMElement* elemOwnMix=NULL;
    getDOMChildByName(elemMixesKey, "Mix", elemOwnMix, false);
    elemOwnMix->appendChild(elemKey);
    CAMsg::printMsg(LOG_INFO,"before T&Cs1...\n");
    if(CALibProxytest::getOptions()->getTermsAndConditions() != NULL)
    {
    	elemOwnMix->appendChild(termsAndConditionsInfoNode(docXmlKeyInfo));
    }
    CAMsg::printMsg(LOG_INFO,"after T&Cs1...\n");
		elemOwnMix->appendChild(createDOMElement(docXmlKeyInfo,"SupportsEncrypedControlChannels"));
	CAMsg::printMsg(LOG_INFO,"after SupportEncChannels...\n");
	if (signXML(elemOwnMix) != E_SUCCESS)
	{
		CAMsg::printMsg(LOG_DEBUG,"Could not sign MixInfo sent to users...\n");
	}

	setDOMElementAttribute(elemMixesKey,"count",count+1);


	DOMNode* elemPayment=createDOMElement(docXmlKeyInfo,"Payment");
	elemRootKey->appendChild(elemPayment);
#ifdef PAYMENT
	setDOMElementAttribute(elemPayment,"required",(UINT8*)"true");
	setDOMElementAttribute(elemPayment,"version",(UINT8*)PAYMENT_VERSION);
	setDOMElementAttribute(elemPayment,"prepaidInterval", CALibProxytest::getOptions()->getPrepaidInterval());
	setDOMElementAttribute(elemPayment,"piid", CALibProxytest::getOptions()->getBI()->getID());

#else
	setDOMElementAttribute(elemPayment,"required",(UINT8*)"false");
#endif

	// create signature
	if (signXML(elemRootKey) != E_SUCCESS)
    {
		CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo sent to users...\n");
	}


	UINT32 tmp32Len = 0;
    UINT8* tmpB=DOM_Output::dumpToMem(docXmlKeyInfo, &tmp32Len);
    if (docXmlKeyInfo != NULL)
    {
    	docXmlKeyInfo->release();
    	docXmlKeyInfo = NULL;
    }

    if(tmp32Len > 0xFFFD) //too bytes are reserved for the length
    {
		CAMsg::printMsg(LOG_CRIT, "The key info size of %u bytes is too large for the clients. (maximum is %u)\n", 0xFFFD);
		if (doc != NULL)
		{
			doc->release();
			doc = NULL;
		}
		return E_UNKNOWN;
    }

    UINT16 tlen = (UINT16) tmp32Len;
    m_xmlKeyInfoBuff = new UINT8[tlen+sizeof(tlen)];
    memcpy(m_xmlKeyInfoBuff+sizeof(tlen), tmpB, tlen);
    UINT16 s = htons(tlen);
    memcpy(m_xmlKeyInfoBuff, &s, sizeof(s));
    m_xmlKeyInfoSize=tlen + sizeof(tlen);
    delete []tmpB;
    tmpB = NULL;

    //Sending symetric key...
    child=elemMixes->getFirstChild();
    while(child!=NULL)
    {
    	if(equals(child->getNodeName(),"Mix"))
        {
            //check Signature....
            CAMsg::printMsg(LOG_DEBUG,"Try to verify next mix signature...\n");
            //CASignature oSig;
            CACertificate* nextCert=CALibProxytest::getOptions()->getNextMixTestCertificate();
            /*oSig.setVerifyKey(nextCert);
            SINT32 ret=oSig.verifyXML(child,NULL);*/
            SINT32 result = CAMultiSignature::verifyXML(child, nextCert);
            delete nextCert;
            nextCert = NULL;
            if(result != E_SUCCESS)
            {
                CAMsg::printMsg(LOG_DEBUG,"Failed to verify XML signature of next mix!\n");
                if (doc != NULL)
                {
                	doc->release();
                	doc = NULL;
                }
                return E_UNKNOWN;
            }
            CAMsg::printMsg(LOG_DEBUG,"Successfully verified XML signature of next mix!\n");
            DOMNode* rsaKey=child->getFirstChild();
            CAASymCipher oRSA;
            oRSA.setPublicKeyAsDOMNode(rsaKey);
            DOMElement* elemNonce=NULL;
            getDOMChildByName(child,"Nonce",elemNonce,false);
            UINT8 arNonce[1024];
            if(elemNonce!=NULL)
            {
                UINT32 lenNonce=1024;
                UINT32 tmpLen=1024;
                getDOMElementValue(elemNonce,arNonce,&lenNonce);
                CABase64::decode(arNonce,lenNonce,arNonce,&tmpLen);
                lenNonce=tmpLen;
                tmpLen=1024;
                CABase64::encode(SHA1(arNonce,lenNonce,NULL),SHA_DIGEST_LENGTH,
                                 arNonce,&tmpLen);
                arNonce[tmpLen]=0;
            }
            UINT8 key[64];
            getRandom(key,64);
            //UINT8 buff[400];
            //UINT32 bufflen=400;
            XERCES_CPP_NAMESPACE::DOMDocument* docSymKey=createDOMDocument();
            DOMElement* elemRoot=NULL;
           	encodeXMLEncryptedKey(key,64,elemRoot,docSymKey,&oRSA);
           	docSymKey->appendChild(elemRoot);
            if(elemNonce!=NULL)
            {
                DOMElement* elemNonceHash=createDOMElement(docSymKey,"Nonce");
                setDOMElementValue(elemNonceHash,arNonce);
                elemRoot->appendChild(elemNonceHash);
            }
            UINT32 outlen=5000;
            UINT8* out=new UINT8[outlen];
			///Getting the KeepAlive Traffic...
			DOMElement* elemKeepAlive=NULL;
			DOMElement* elemKeepAliveSendInterval=NULL;
			DOMElement* elemKeepAliveRecvInterval=NULL;
			getDOMChildByName(child,"KeepAlive",elemKeepAlive,false);
			getDOMChildByName(elemKeepAlive,"SendInterval",elemKeepAliveSendInterval,false);
			getDOMChildByName(elemKeepAlive,"ReceiveInterval",elemKeepAliveRecvInterval,false);
			UINT32 tmpSendInterval,tmpRecvInterval;
			getDOMElementValue(elemKeepAliveSendInterval,tmpSendInterval,0xFFFFFFFF); //if now send interval was given set it to "infinite"
			getDOMElementValue(elemKeepAliveRecvInterval,tmpRecvInterval,0xFFFFFFFF); //if no recv interval was given --> set it to "infinite"
			CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Getting offer -- SendInterval %u -- ReceiveInterval %u\n",tmpSendInterval,tmpRecvInterval);
			// Add Info about KeepAlive traffic
			UINT32 u32KeepAliveSendInterval=CALibProxytest::getOptions()->getKeepAliveSendInterval();
			UINT32 u32KeepAliveRecvInterval=CALibProxytest::getOptions()->getKeepAliveRecvInterval();
			elemKeepAlive=createDOMElement(docSymKey,"KeepAlive");
			elemKeepAliveSendInterval=createDOMElement(docSymKey,"SendInterval");
			elemKeepAliveRecvInterval=createDOMElement(docSymKey,"ReceiveInterval");
			elemKeepAlive->appendChild(elemKeepAliveSendInterval);
			elemKeepAlive->appendChild(elemKeepAliveRecvInterval);
			setDOMElementValue(elemKeepAliveSendInterval,u32KeepAliveSendInterval);
			setDOMElementValue(elemKeepAliveRecvInterval,u32KeepAliveRecvInterval);
			elemRoot->appendChild(elemKeepAlive);
			CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Offering -- SendInterval %u -- Receive Interval %u\n",u32KeepAliveSendInterval,u32KeepAliveRecvInterval);
			m_u32KeepAliveSendInterval=max(u32KeepAliveSendInterval,tmpRecvInterval);
			if(m_u32KeepAliveSendInterval>10000)
			{
				m_u32KeepAliveSendInterval-=10000; //make the send interval a little bit smaller than the related receive intervall
			}
			m_u32KeepAliveRecvInterval=max(u32KeepAliveRecvInterval,tmpSendInterval);
			CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Calculated -- SendInterval %u -- Receive Interval %u\n",m_u32KeepAliveSendInterval,m_u32KeepAliveRecvInterval);

            //m_pSignature->signXML(elemRoot);
            m_pMultiSignature->signXML(elemRoot, false);
			DOM_Output::dumpToMem(docSymKey,out,&outlen);
            if (docSymKey != NULL)
            {
				docSymKey->release();
				docSymKey = NULL;
            }
			m_pMuxOut->setSendKey(key,32);
            m_pMuxOut->setReceiveKey(key+32,32);
            UINT32 size = htonl(outlen);
            CAMsg::printMsg(LOG_DEBUG,"Sending symmetric key to next Mix! Size: %i\n", outlen);
            m_pMuxOut->getCASocket()->send((UINT8*) &size, sizeof(size));
            m_pMuxOut->getCASocket()->send(out, outlen);
            m_pMuxOut->setCrypt(true);
            delete[] out;
            out = NULL;
            break;
        }
        child=child->getNextSibling();
    }
		///initialises MixParameters struct
	if(initMixParameters(elemMixes)!=E_SUCCESS)
	{
		if (doc != NULL)
		{
			doc->release();
			doc = NULL;
		}
		return E_UNKNOWN;
	}
    if(initMixCascadeInfo(elemMixes)!=E_SUCCESS)
    {
    	if (doc != NULL)
    	{
    		doc->release();
    		doc = NULL;
    	}
    	CAMsg::printMsg(LOG_CRIT,"Error initializing cascade info.\n");
        return E_UNKNOWN;
    }
/*    else
    {
        if(m_pInfoService != NULL)
            m_pInfoService->sendCascadeHelo();
    }*/
    CAMsg::printMsg(LOG_DEBUG,"Key exchange finished!\n");
    if (doc != NULL)
    {
    	doc->release();
    	doc = NULL;
    }
    return E_SUCCESS;
}


SINT32 CAFirstMix::setMixParameters(const tMixParameters& params)
	{
#ifdef REPLAY_DETECTION
		UINT32 diff=time(NULL)-m_u64LastTimestampReceived;
		for(UINT32 i=0;i<m_u32MixCount-1;i++)
			{
//@todo dangerous strcmp
				if(strcmp((char*)m_arMixParameters[i].m_strMixID,(char*)params.m_strMixID)==0)
					{
						m_arMixParameters[i].m_u32ReplayOffset=params.m_u32ReplayOffset;
						m_arMixParameters[i].m_u32ReplayBase=params.m_u32ReplayBase;
					}
				else{
						if (m_arMixParameters[i].m_u32ReplayOffset!=0) m_arMixParameters[i].m_u32ReplayOffset+=diff;
					}
			}
#endif
		return E_SUCCESS;
	}


SINT32 CAFirstMix::handleKeyInfoExtensions(DOMElement *root)
{
	if(root == NULL)
	{
		return E_UNKNOWN;
	}

	SINT32 ret = E_SUCCESS;
	DOMElement *extensionRoot = NULL;
	getDOMChildByName(root, KEYINFO_NODE_EXTENSIONS, extensionRoot);

	if(extensionRoot != NULL)
	{
		extensionRoot = (DOMElement *) root->removeChild(extensionRoot);
		ret = handleTermsAndConditionsExtension(extensionRoot);
		extensionRoot->release();
	}
	return ret;
}

SINT32 CAFirstMix::handleTermsAndConditionsExtension(DOMElement *extensionsRoot)
{
	if(extensionsRoot == NULL)
	{
		return E_UNKNOWN;
	}

	DOMElement *tncDefs = NULL;
	DOMElement *tncTemplates = NULL;
	getDOMChildByName(extensionsRoot, KEYINFO_NODE_TNC_EXTENSION, tncDefs);
	if(tncDefs == NULL)
	{
		CAMsg::printMsg(LOG_CRIT,"No TNCs in TNC extension found.\n");
		return E_UNKNOWN;
	}
	getDOMChildByName(tncDefs, OPTIONS_NODE_TNCS_TEMPLATES, tncTemplates);

	UINT8 currentTnC_id[TMP_BUFF_SIZE];
	UINT32 currentTnC_id_len = TMP_BUFF_SIZE;

	UINT8 currentTnCEntry_templateRefid[TMP_BUFF_SIZE];
	UINT32 currentTnCEntry_templateRefid_len = TMP_BUFF_SIZE;

	UINT8 currentTnCEntry_locale[TMP_LOCALE_SIZE];
	UINT32 currentTnCEntry_locale_len = TMP_LOCALE_SIZE;

	DOMElement *currentTnCList = NULL;
	DOMElement *currentTnCEntry = NULL;
	memset(currentTnC_id, 0, TMP_BUFF_SIZE);
	memset(currentTnCEntry_templateRefid, 0, TMP_BUFF_SIZE);

	if(tncTemplates != NULL)
	{
		DOMNodeList *tncTemplateList = getElementsByTagName(tncDefs, TNC_TEMPLATE_ROOT_ELEMENT);
		m_nrOfTermsAndConditionsTemplates = tncTemplateList->getLength();
		CAMsg::printMsg(LOG_INFO, "Storing %u TC Templates.\n", m_nrOfTermsAndConditionsTemplates);
		if(m_nrOfTermsAndConditionsTemplates > 0)
		{
			m_tcTemplates = new DOMNode *[m_nrOfTermsAndConditionsTemplates];
			for (UINT32 i = 0; i < m_nrOfTermsAndConditionsTemplates; i++)
			{
				m_tcTemplates[i] = m_templatesOwner->importNode(tncTemplateList->item(i), true);
			}
		}
		else
		{
			CAMsg::printMsg(LOG_ERR, "Not a single TC Template specified! the Cascade is not properly configured.\n");
			return E_UNKNOWN;
		}
	}
	else
	{
		CAMsg::printMsg(LOG_ERR, "No TC Template node found! Cascade is not properly configured.\n");
		return E_UNKNOWN;
	}

	DOMNodeList *tncDefList = getElementsByTagName(tncDefs, OPTIONS_NODE_TNCS);
	if(tncDefList->getLength() == 0)
	{
		CAMsg::printMsg(LOG_CRIT, "No TNCs definitions found.\n");
		return E_UNKNOWN;
	}

	m_nrOfTermsAndConditionsDefs = tncDefList->getLength();
	m_tnCDefs = new TermsAndConditions *[m_nrOfTermsAndConditionsDefs];
	memset(m_tnCDefs, 0, (sizeof(TermsAndConditions*)*m_nrOfTermsAndConditionsDefs) );

	for (XMLSize_t i = 0; i < m_nrOfTermsAndConditionsDefs; i++)
	{
		currentTnCList = (DOMElement *) tncDefList->item(i);
		DOMNodeList *tncDefEntryList = getElementsByTagName(currentTnCList, OPTIONS_NODE_TNCS_TRANSLATION);
		getDOMElementAttribute(currentTnCList, OPTIONS_ATTRIBUTE_TNC_ID, currentTnC_id, &currentTnC_id_len);

		m_tnCDefs[i] = new TermsAndConditions(currentTnC_id, tncDefEntryList->getLength());

		for (XMLSize_t j = 0; j < tncDefEntryList->getLength(); j++)
		{
			currentTnCEntry = (DOMElement *) tncDefEntryList->item(j);

			getDOMElementAttribute(currentTnCEntry, OPTIONS_ATTRIBUTE_TNC_TEMPLATE_REFID,
					currentTnCEntry_templateRefid, &currentTnCEntry_templateRefid_len);
			getDOMElementAttribute(currentTnCEntry, OPTIONS_ATTRIBUTE_TNC_LOCALE,
					currentTnCEntry_locale, &currentTnCEntry_locale_len);

			DOMNode *templateNode = getTermsAndConditionsTemplate(currentTnCEntry_templateRefid);
			if(templateNode != NULL)
			{
				//m_tnCDefs[i]->synchLock->lock();
				m_tnCDefs[i]->addTranslation(currentTnCEntry_locale, currentTnCEntry, templateNode);
				//m_tnCDefs[i]->synchLock->unlock();
			}
			else
			{
				CAMsg::printMsg(LOG_CRIT,"Could not find template %s for T&C %s.\n", currentTnCEntry_templateRefid, currentTnC_id);
				return E_UNKNOWN;
			}
			memset(currentTnCEntry_templateRefid, 0, TMP_BUFF_SIZE);
			currentTnCEntry_templateRefid_len = TMP_BUFF_SIZE;
			memset(currentTnCEntry_locale, 0, TMP_LOCALE_SIZE);
			currentTnCEntry_locale_len = TMP_LOCALE_SIZE;
		}
		memset(currentTnC_id, 0, TMP_BUFF_SIZE);
		currentTnC_id_len = TMP_BUFF_SIZE;
	}
	return E_SUCCESS;
}

TermsAndConditions *CAFirstMix::getTermsAndConditions(const UINT8 *opSki)
{
	if( (m_tnCDefs == NULL) || (opSki == NULL) )
	{
		return NULL;
	}
	for(UINT32 i = 0; i < m_nrOfTermsAndConditionsDefs; i++)
	{
		if(m_tnCDefs[i] != NULL)
		{
			if(strncasecmp((char *) m_tnCDefs[i]->getID(),
							(const char *) opSki,
							strlen((char *) m_tnCDefs[i]->getID())) == 0)
			{
				return m_tnCDefs[i];
			}
		}
	}
	return NULL;
}

DOMNode *CAFirstMix::getTermsAndConditionsTemplate(UINT8 *templateRefID)
{
	UINT8 *currentRefId = NULL;
	bool match = false;

	for (UINT32 i = 0; i < m_nrOfTermsAndConditionsTemplates; i++)
	{
		currentRefId = getTermsAndConditionsTemplateRefId(m_tcTemplates[i]);
		match = (currentRefId != NULL) &&
				 (strncmp((char *)templateRefID, (char *)currentRefId, TEMPLATE_REFID_MAXLEN ) == 0);
		delete [] currentRefId;
		currentRefId = NULL;
		if(match)
		{
			return m_tcTemplates[i];
		}
	}
	return NULL;
}

/**How to end this thread:
0. set bRestart=true;
1. Close connection to next mix
2. put some bytes (len>MIX_PACKET_SIZE) in the Mix-Output-Queue
*/
THREAD_RETURN fm_loopSendToMix(void* param)
	{
		INIT_STACK;
		BEGIN_STACK("CAFirstMix::fm_loopSendToMix");

		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		CAQueue* pQueue=((CAFirstMix*)param)->m_pQueueSendToMix;
		CAMuxSocket* pMuxSocket=pFirstMix->m_pMuxOut;

		UINT32 len;
		SINT32 ret;

/*#ifdef DATA_RETENTION_LOG
		t_dataretentionLogEntry* pDataRetentionLogEntry=new t_dataretentionLogEntry;
#endif
*/
#ifndef USE_POOL
		tQueueEntry* pQueueEntry=new tQueueEntry;
		MIXPACKET* pMixPacket=&pQueueEntry->packet;
		UINT32 u32KeepAliveSendInterval=pFirstMix->m_u32KeepAliveSendInterval;
		while(!pFirstMix->m_bRestart)
			{
				len=sizeof(tQueueEntry);
				ret=pQueue->getOrWait((UINT8*)pQueueEntry,&len,u32KeepAliveSendInterval);
				if(ret==E_TIMEDOUT)
					{//send a dummy as keep-alvie-traffic
						pMixPacket->flags=CHANNEL_DUMMY;
						pMixPacket->channel=DUMMY_CHANNEL;
						getRandom(pMixPacket->data,DATA_SIZE);
					}
				else if(ret!=E_SUCCESS||len!=sizeof(tQueueEntry))
					{
						CAMsg::printMsg(LOG_ERR,"CAFirstMix::lm_loopSendToMix - Error in dequeueing MixPaket\n");
						CAMsg::printMsg(LOG_ERR,"ret=%i len=%i\n",ret,len);
						break;
					}

				if(pMuxSocket->send(pMixPacket)!=MIXPACKET_SIZE)
					{
						CAMsg::printMsg(LOG_ERR,"CAFirstMix::lm_loopSendToMix - Error in sending MixPaket\n");
						break;
					}
#if defined (LOG_PACKET_TIMES)
 				if(!isZero64(pQueueEntry->timestamp_proccessing_start))
					{
						getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
						pFirstMix->m_pLogPacketStats->addToTimeingStats(*pQueueEntry,pMixPacket->flags,true);
					}
#endif
#ifdef DATA_RETENTION_LOG
				if((pQueueEntry->packet.flags&CHANNEL_OPEN)!=0)
					{
						pQueueEntry->dataRetentionLogEntry.t_out=htonl(time(NULL));
						pFirstMix->m_pDataRetentionLog->log(&(pQueueEntry->dataRetentionLogEntry));
					}
#endif
			}
		delete pQueueEntry;
		pQueueEntry = NULL;
#else
		CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		tPoolEntry* pPoolEntry=new tPoolEntry;
		MIXPACKET* pMixPacket=&pPoolEntry->packet;
		while(!pFirstMix->m_bRestart)
			{
				len=sizeof(tQueueEntry);
				ret=pQueue->getOrWait((UINT8*)pPoolEntry,&len,MIX_POOL_TIMEOUT);
				if(ret==E_TIMEDOUT)
					{
						pMixPacket->flags=CHANNEL_DUMMY;
						pMixPacket->channel=DUMMY_CHANNEL;
						getRandom(pMixPacket->data,DATA_SIZE);
						#ifdef LOG_PACKET_TIMES
							setZero64(pPoolEntry->timestamp_proccessing_start);
						#endif
					}
				else if(ret!=E_SUCCESS||len!=sizeof(tQueueEntry))
					break;
				#ifdef LOG_PACKET_TIMES
					getcurrentTimeMicros(pPoolEntry->pool_timestamp_in);
				#endif
				pPool->pool(pPoolEntry);
				#ifdef LOG_PACKET_TIMES
					getcurrentTimeMicros(pPoolEntry->pool_timestamp_out);
				#endif
				if(pMuxSocket->send(pMixPacket)!=MIXPACKET_SIZE)
					break;
				#ifdef LOG_PACKET_TIMES
					if(!isZero64(pPoolEntry->timestamp_proccessing_start))
						{
							getcurrentTimeMicros(pPoolEntry->timestamp_proccessing_end);
							pFirstMix->m_pLogPacketStats->addToTimeingStats(*pPoolEntry,pMixPacket->flags,true);
						}
				#endif
			}
		delete pPoolEntry;
		pPoolEntry = NULL;
		delete pPool;
		pPool = NULL;
#endif

/*#ifdef DATA_RETENTION_LOG
		delete pDataRetentionLogEntry;
#endif
*/
		FINISH_STACK("CAFirstMix::fm_loopSendToMix");

		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread SendToMix\n");
		THREAD_RETURN_SUCCESS;
	}

/* How to end this thread:
	0. set bRestart=true
*/
THREAD_RETURN fm_loopReadFromMix(void* pParam)
	{
		INIT_STACK;
		BEGIN_STACK("CAFirstMix::fm_loopReadFromMix");

		CAFirstMix* pFirstMix=(CAFirstMix*)pParam;
		CAMuxSocket* pMuxSocket=pFirstMix->m_pMuxOut;
		CAQueue* pQueue=pFirstMix->m_pQueueReadFromMix;
		tQueueEntry* pQueueEntry=new tQueueEntry;
		MIXPACKET* pMixPacket=&pQueueEntry->packet;
		CASingleSocketGroup* pSocketGroup=new CASingleSocketGroup(false);
		pSocketGroup->add(*pMuxSocket);
		#ifdef USE_POOL
			CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		#endif
		UINT64 keepaliveNow,keepaliveLast;
		UINT32 u32KeepAliveRecvInterval=pFirstMix->m_u32KeepAliveRecvInterval;
		getcurrentTimeMillis(keepaliveLast);
		CAControlChannelDispatcher* pControlChannelDispatcher=pFirstMix->m_pMuxOutControlChannelDispatcher;
		while(!pFirstMix->m_bRestart)
			{
				if(pQueue->getSize()>MAX_READ_FROM_NEXT_MIX_QUEUE_SIZE)
					{
#ifdef DEBUG
						CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::Queue is full!\n");
#endif
						msSleep(200);
						getcurrentTimeMillis(keepaliveLast);
						continue;
					}
				//check if the connection is broken because we did not received a Keep_alive-Message
				getcurrentTimeMillis(keepaliveNow);
				UINT32 keepaliveDiff=diff64(keepaliveNow,keepaliveLast);
				if(keepaliveDiff>u32KeepAliveRecvInterval)
					{
						CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::loopReadFromMix() -- restart because of KeepAlive-Traffic Timeout!\n");
						pFirstMix->m_bRestart=true;
						MONITORING_FIRE_NET_EVENT(ev_net_nextConnectionClosed);
						break;
					}
				SINT32 ret=pSocketGroup->select(MIX_POOL_TIMEOUT);
				if(ret==E_TIMEDOUT)
					{
						#ifdef USE_POOL
							pMixPacket->flags=CHANNEL_DUMMY;
							pMixPacket->channel=DUMMY_CHANNEL;
							getRandom(pMixPacket->data,DATA_SIZE);
							#ifdef LOG_PACKET_TIMES
								setZero64(pQueueEntry->timestamp_proccessing_start);
							#endif
						#else
							continue;
						#endif
					}
				else if(ret>0)
					{
						ret=pMuxSocket->receive(pMixPacket);
						#ifdef LOG_PACKET_TIMES
							getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start);
						#endif
						if(ret!=MIXPACKET_SIZE)
							{
								pFirstMix->m_bRestart=true;
								CAMsg::printMsg(LOG_ERR,"CAFirstMix::lm_loopReadFromMix - received returned: %i -- restarting!\n",ret);
								MONITORING_FIRE_NET_EVENT(ev_net_nextConnectionClosed);
								break;
							}
					}
					if(pMixPacket->channel>0&&pMixPacket->channel<256)
						{
							#ifdef DEBUG
								CAMsg::printMsg(LOG_DEBUG,"CAFirstMix - sent a packet from the next mix to the ControlChanelDispatcher... \n");
							#endif
							pControlChannelDispatcher->proccessMixPacket(pMixPacket);
							getcurrentTimeMillis(keepaliveLast);
							continue;
						}
				#ifdef USE_POOL
					#ifdef LOG_PACKET_TIMES
						getcurrentTimeMicros(pQueueEntry->pool_timestamp_in);
					#endif
					pPool->pool((tPoolEntry*)pQueueEntry);
					#ifdef LOG_PACKET_TIMES
						getcurrentTimeMicros(pQueueEntry->pool_timestamp_out);
					#endif
				#endif
				pQueue->add(pQueueEntry, sizeof(tQueueEntry));
				getcurrentTimeMillis(keepaliveLast);
			}
		delete pQueueEntry;
		pQueueEntry = NULL;
		delete pSocketGroup;
		pSocketGroup = NULL;
		#ifdef USE_POOL
			delete pPool;
			pPool = NULL;
		#endif

		FINISH_STACK("CAFirstMix::fm_loopReadFromMix");
		THREAD_RETURN_SUCCESS;
	}

struct T_UserLoginData
	{
		CAMuxSocket* pNewUser;
		CAFirstMix* pMix;
		UINT8 peerIP[4];
	};

typedef struct T_UserLoginData t_UserLoginData;

SINT32 isAllowedToPassRestrictions(CASocket* pNewMuxSocket)
{
	UINT8* peerIP=new UINT8[4];
	SINT32 master = pNewMuxSocket->getPeerIP(peerIP);

	if(master == E_SUCCESS)
	{
		UINT32 size=0;
		UINT8 remoteIP[4];

		CAListenerInterface** intf = CALibProxytest::getOptions()->getInfoServices(size);

		master = E_UNKNOWN;

		for(UINT32 i = 0; i < size; i++)
		{
			if(intf[i]->getType() == HTTP_TCP || intf[i]->getType() == RAW_TCP)
			{
				CASocketAddrINet* addr = (CASocketAddrINet*) intf[i]->getAddr();
				if(addr == NULL)
				{
					continue;
				}

				addr->getIP(remoteIP);
				delete addr;
				addr = NULL;

				if(memcmp(peerIP, remoteIP, 4) == 0)
				{
					CAMsg::printMsg(LOG_DEBUG,"Got InfoService connection: You are allowed for login...\n");
					master = E_SUCCESS;
					break;
				}
			}
		}
	}
	delete[] peerIP;
	peerIP = NULL;

	return master;
}


/*How to end this thread
1. Set m_bRestart in firstMix to true
2. close all accept sockets
*/
THREAD_RETURN fm_loopAcceptUsers(void* param)
	{
		INIT_STACK;
		BEGIN_STACK("CAFirstMix::fm_loopAcceptUsers");

		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		CASocket** socketsIn=pFirstMix->m_arrSocketsIn;
		CAIPList* pIPList=pFirstMix->m_pIPList;
		CATempIPBlockList* pIPBlockList = pFirstMix->m_pIPBlockList;
		CAThreadPool* pthreadsLogin=pFirstMix->m_pthreadsLogin;
		UINT32 nSocketsIn=pFirstMix->m_nSocketsIn;
		CASocketGroup* psocketgroupAccept=new CASocketGroup(false);
		CAMuxSocket* pNewMuxSocket;
		UINT8* peerIP=new UINT8[4];
		UINT32 i=0;
		SINT32 countRead;
		SINT32 ret;
		SINT32 retPeerIP = E_SUCCESS;

		pFirstMix->m_newConnections = 0;

		// kick out users that already have connected
		for(i=0;i<nSocketsIn;i++)
		{
				while (socketsIn[i]->close() != E_SUCCESS)
				{
					sSleep(1);
				}
		}
		if (CALibProxytest::getOptions()->createSockets(false,pFirstMix-> m_arrSocketsIn, pFirstMix->m_nSocketsIn) != E_SUCCESS)
		{
			goto END_THREAD;
		}
		for(i=0;i<nSocketsIn;i++)
		{
			psocketgroupAccept->add(*socketsIn[i]);
		}
#ifdef REPLAY_DETECTION //before we can start to accept users we have to ensure that we received the replay timestamps form the over mixes
		CAMsg::printMsg(LOG_DEBUG,"Waiting for Replay Timestamp from next mixes\n");
		i=0;
		while(!pFirstMix->m_bRestart && i < pFirstMix->m_u32MixCount-1)
		{
			if(pFirstMix->m_arMixParameters[i].m_u32ReplayOffset==0)//not set yet
				{
					msSleep(100);//wait a little bit and try again
					continue;
				}
			i++;
		}
		CAMsg::printMsg(LOG_DEBUG,"All Replay Timestamp received\n");
#endif
		while(!pFirstMix->m_bRestart)
			{
				if (pIPBlockList->count()>40)
				{
					CAMsg::printMsg(LOG_DEBUG,"UserAcceptLoop: login timeout list counts %d. We have %d open sockets and %d new connections. Restarting server sockets...\n",pIPBlockList->count(), CASocket::countOpenSockets(), pFirstMix->m_newConnections);
					for(i=0;i<nSocketsIn;i++)
					{
						psocketgroupAccept->remove(*socketsIn[i]);
						while (socketsIn[i]->close() != E_SUCCESS)
						{
							sSleep(1);
						}
					}
	
					if (CALibProxytest::getOptions()->createSockets(false,pFirstMix-> m_arrSocketsIn, pFirstMix->m_nSocketsIn) != E_SUCCESS)
					{
						// could not listen
						goto END_THREAD;
					}
					for(i=0;i<nSocketsIn;i++)
					{
						psocketgroupAccept->add(*socketsIn[i]);
					}
					sSleep(1);
				}
				countRead=psocketgroupAccept->select(10000);
				if(countRead<0)
					{ //check for Error - are we restarting ?
						if(pFirstMix->m_bRestart ||countRead!=E_TIMEDOUT)
							goto END_THREAD;
					}
				i=0;
#ifdef _DEBUG
				CAMsg::printMsg(LOG_DEBUG,"UserAcceptLoop: countRead=%i\n",countRead);
#endif
				while(countRead>0&&i<nSocketsIn)
				{
					if(psocketgroupAccept->isSignaled(*socketsIn[i]))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New direct Connection from Client!\n");
						#endif
						pNewMuxSocket=new CAMuxSocket;
						ret=socketsIn[i]->accept(*(pNewMuxSocket->getCASocket()));
						pFirstMix->incNewConnections();

						if(ret!=E_SUCCESS)
						{
							// may return E_SOCKETCLOSED or E_SOCKET_LIMIT
							CAMsg::printMsg(LOG_ERR,"Accept Error %u - direct Connection from Client!\n",GET_NET_ERROR);
						}
						else if( (CALibProxytest::getOptions()->getMaxNrOfUsers() > 0 &&
								pFirstMix->getNrOfUsers() >= CALibProxytest::getOptions()->getMaxNrOfUsers())
								&& (isAllowedToPassRestrictions(pNewMuxSocket->getCASocket()) != E_SUCCESS)

								)
						{
							CAMsg::printMsg(LOG_DEBUG,"CAFirstMix User control: Too many users (Maximum:%d)! Rejecting user...\n", pFirstMix->getNrOfUsers(),CALibProxytest::getOptions()->getMaxNrOfUsers());
							ret = E_UNKNOWN;
						}
						else if ((pFirstMix->m_newConnections > CAFirstMix::MAX_CONCURRENT_NEW_CONNECTIONS)
								&& (isAllowedToPassRestrictions(pNewMuxSocket->getCASocket()) != E_SUCCESS)
								)

						{
							/* This should protect the mix from flooding attacks
							 * No more than MAX_CONCURRENT_NEW_CONNECTIONS are allowed.
							 */
#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"CAFirstMix Flooding protection: Too many concurrent new connections (Maximum:%d)! Rejecting user...\n", CAFirstMix::MAX_CONCURRENT_NEW_CONNECTIONS);
#endif
							ret = E_UNKNOWN;
						}
//#ifndef PAYMENT
						else if ((ret = pNewMuxSocket->getCASocket()->getPeerIP(peerIP)) != E_SUCCESS ||
								(retPeerIP = pIPList->insertIP(peerIP)) < 0) 
								// || (pIPBlockList->checkIP(peerIP) == E_UNKNOWN && isAllowedToPassRestrictions(pNewMuxSocket->getCASocket()) != E_SUCCESS))
						{
							if (ret != E_SUCCESS)
							{
								CAMsg::printMsg(LOG_DEBUG,"Could not insert IP address as IP could not be retrieved! We have %d login threads currently running.\n", pthreadsLogin->countRequests()); 
	
							}
							else if (retPeerIP < 0)
							{
								CAMsg::printMsg(LOG_DEBUG,"CAFirstMix Flooding protection: Could not insert IP address! We have %d login threads currently running.\n", pthreadsLogin->countRequests()); 
								pIPBlockList->insertIP(peerIP);
							}
							else if (pIPBlockList->checkIP(peerIP) == E_UNKNOWN)
							{
								CAMsg::printMsg(LOG_DEBUG, "Client IP address %u.%u.x.x is temporarily blocked! User login denied. We have %d open sockets and %d new connections.\n", peerIP[0],peerIP[1], CASocket::countOpenSockets(), pFirstMix->m_newConnections);
								pIPList->removeIP(peerIP);
							} 
							ret = E_UNKNOWN;
						}
//#endif
						else
						{
							t_UserLoginData* d=new t_UserLoginData;
							d->pNewUser=pNewMuxSocket;
							d->pMix=pFirstMix;
							memcpy(d->peerIP,peerIP,4);
#ifdef DEBUG
							CAMsg::printMsg(LOG_DEBUG,"%d concurrent client connections.\n", pFirstMix->m_newConnections);
#endif
							if(pthreadsLogin->addRequest(fm_loopDoUserLogin,d)!=E_SUCCESS)
							{
								CAMsg::printMsg(LOG_ERR,"Could not add an login request to the login thread pool!\n");
								ret=E_UNKNOWN;
							}
						}

						if (ret != E_SUCCESS)
						{
							delete pNewMuxSocket;
							pNewMuxSocket = NULL;
							pFirstMix->decNewConnections();
							if(ret==E_SOCKETCLOSED&&pFirstMix->m_bRestart) //Hm, should we restart ??
							{
								goto END_THREAD;
							}
							else //if(ret==E_SOCKET_LIMIT) // Hm no free sockets - wait some time to hope to get a free one...
							{
								msSleep(400);
							}
						}
					}
					i++;
				}
			}
END_THREAD:
		FINISH_STACK("CAFirstMix::fm_loopAcceptUsers");

		delete[] peerIP;
		peerIP = NULL;
		delete psocketgroupAccept;
		psocketgroupAccept = NULL;

		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread AcceptUser\n");
		THREAD_RETURN_SUCCESS;
	}


THREAD_RETURN fm_loopLog(void* param)
	{
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		pFirstMix->m_bRunLog=true;
		UINT32 countLog=0;
		while(pFirstMix->m_bRunLog)
			{
				if(countLog==0)
					{
						logMemoryUsage();
						countLog=10;
					}
				sSleep(30);
				countLog--;
			}
		THREAD_RETURN_SUCCESS;
	}

#ifdef CH_LOG_STUDY
THREAD_RETURN fm_loopLogChannelsOpened(void* param)
	{
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		pFirstMix->m_bRunLog=true;
		UINT32 countLog= 10;
		UINT32 value = 0;
		UINT32 total = 0;
		time_t newIvalTS = 0, ival = 0;

		pFirstMix->nrOfChOpMutex->lock();
		pFirstMix->lastLogTime = time(NULL);
		pFirstMix->nrOfChOpMutex->unlock();

		while(pFirstMix->m_bRunLog)
		{
			if(countLog == 0)
			{
				pFirstMix->nrOfChOpMutex->lock();
				value = pFirstMix->nrOfOpenedChannels;
				total = pFirstMix->currentOpenedChannels;
				pFirstMix->nrOfOpenedChannels = 0;
				newIvalTS = time(NULL);
				ival = newIvalTS - pFirstMix->lastLogTime;
				pFirstMix->lastLogTime = newIvalTS;
				pFirstMix->nrOfChOpMutex->unlock();
				CAMsg::printMsg(LOG_INFO,"Analyzing channels: Opened channels for %u JonDo connections in the last %u seconds, overall open channels: %u, users online: %u\n",
						value, ival, total, pFirstMix->getNrOfUsers() );
				countLog = 10;
			}
			sSleep(1);
			countLog--;
		}
		THREAD_RETURN_SUCCESS;
	}
#endif //CH_LOG_STUDY

THREAD_RETURN fm_loopDoUserLogin(void* param)
	{
		INIT_STACK;
		BEGIN_STACK("CAFirstMix::fm_loopDoUserLogin");
#ifdef COUNTRY_STATS
		my_thread_init();
#endif

		t_UserLoginData* d=(t_UserLoginData*)param;
		d->pMix->doUserLogin(d->pNewUser,d->peerIP);

		SAVE_STACK("CAFirstMix::fm_loopDoUserLogin", "after user login");
		d->pMix->decNewConnections();
		delete d;
		d = NULL;

#ifdef COUNTRY_STATS
		my_thread_end();
#endif
		FINISH_STACK("CAFirstMix::fm_loopDoUserLogin");

		THREAD_RETURN_SUCCESS;
	}

termsAndConditionMixAnswer_t *CAFirstMix::handleTermsAndConditionsLogin(XERCES_CPP_NAMESPACE::DOMDocument* request)
{
	termsAndConditionMixAnswer_t *answer =
		new termsAndConditionMixAnswer_t;

	const XMLCh* reqName = request->getDocumentElement()->getTagName();

	//Client requests lacking T&C resources.
	if(XMLString::equals(reqName, TNC_REQUEST))
	{
		answer->result = TC_UNFINISHED;
		CAMsg::printMsg(LOG_DEBUG,"Handling TC request.\n");
		XERCES_CPP_NAMESPACE::DOMDocument *response = createDOMDocument();
		DOMElement *responseRoot = createDOMElement(response, TNC_RESPONSE);
		response->appendChild(responseRoot);
		//All elements with tag name 'Resources' (direct children of 'TermsAndConditionsRequest')
		DOMNodeList *requestedResources = getElementsByTagName(request->getDocumentElement(), TNC_RESOURCES);
		//All elements with tag name 'Translation' (direct children of 'Resources')
		DOMNodeList *requestedResourceItems = NULL;

		DOMNode *currentNode = NULL;
		DOMNode *currentAnswerNode = NULL;
		DOMNode *currentAnswerResourceNode = NULL;

		UINT32 idLen = TMP_BUFF_SIZE;
		UINT8 id[TMP_BUFF_SIZE];
		memset(id, 0, idLen);

		UINT32 localeLen = TMP_LOCALE_SIZE;
		UINT8 locale[TMP_LOCALE_SIZE];
		memset(locale, 0, localeLen);

		bool resourceError = false;

		if(requestedResources->getLength() == 0)
		{
			response->release();
			response = createDOMDocument();
			response->appendChild(createDOMElement(response, TNC_RESPONSE_INVALID_REQUEST));
			answer->result = TC_FAILED;
		}

		for (XMLSize_t i = 0; i < requestedResources->getLength(); i++)
		{
			idLen = TMP_BUFF_SIZE;
			localeLen = TMP_LOCALE_SIZE;
			bool validResource = false;

			currentNode = requestedResources->item(i);
			//check validity of the T&C resource request. attributes "id" and locale must be proper set.
			if( (getDOMElementAttribute(currentNode, OPTIONS_ATTRIBUTE_TNC_ID, id, &idLen) != E_SUCCESS) ||
				(strlen((char *)id) < 1)  )
			{
				CAMsg::printMsg(LOG_DEBUG,"Error: invalid id.\n");
				resourceError = true;
			}

			if(!currentNode->hasChildNodes())
			{
				//Empty Resource node is invalid!
				CAMsg::printMsg(LOG_DEBUG,"Error: No children.\n");
				resourceError = true;
			}

			if(!resourceError)
			{

				TermsAndConditions *requestedTnC = getTermsAndConditions(id);
				if(requestedTnC != NULL)
				{
					requestedResourceItems = getElementsByTagName((DOMElement *)currentNode, TNC_REQ_TRANSLATION);
					for(XMLSize_t j = 0; j < requestedResourceItems->getLength(); j++)
					{
						validResource = false;
						localeLen = TMP_LOCALE_SIZE;
						if( (getDOMElementAttribute(requestedResourceItems->item(j),
								OPTIONS_ATTRIBUTE_TNC_LOCALE,
								locale, &localeLen) != E_SUCCESS) ||
							(strlen((char *)locale) != ((TMP_LOCALE_SIZE) - 1) ) )
						{
							CAMsg::printMsg(LOG_DEBUG,"Error: Invalid locale for tnc %s\n", id);
							break;
						}

						const termsAndConditionsTranslation_t *requestedTranslation =
								requestedTnC->getTranslation(locale);
						//NOTE: No need to lock while working with the TC translation
						//because the translation containers are only modified during
						//inter-mix-keyexchange and cleanup.
						//Both must never run concurrently to this method
						if(requestedTranslation != NULL)
						{

							currentAnswerNode = response->importNode(currentNode, false);
							responseRoot->appendChild(currentAnswerNode);
							if( getDOMChildByName(requestedResourceItems->item(j), TNC_RESOURCE_TEMPLATE,
									currentAnswerResourceNode, false) == E_SUCCESS)
							{
								currentAnswerResourceNode = response->importNode(currentAnswerResourceNode, true);
								currentAnswerResourceNode->appendChild(
										response->importNode(requestedTranslation->tnc_template, true));
								currentAnswerNode->appendChild(currentAnswerResourceNode);
								validResource = true;
							}

							if( getDOMChildByName(requestedResourceItems->item(j), TNC_RESOURCE_CUSTOMIZED_SECT,
													currentAnswerResourceNode, false) == E_SUCCESS)
							{
								currentAnswerResourceNode = response->importNode(currentAnswerResourceNode, true);
								currentAnswerResourceNode->appendChild(
										response->importNode(requestedTranslation->tnc_customized, true));
								currentAnswerNode->appendChild(currentAnswerResourceNode);
								validResource = true;
							}
						}
						else
						{
							CAMsg::printMsg(LOG_DEBUG,"Error: no translation def found.\n");
							break;
						}
					}
				}
				else
				{
					CAMsg::printMsg(LOG_DEBUG,"Error: no tc def found.\n");
				}
			}

			if(!validResource)
			{
				//only a manipulated client sends an invalid T&C resource request.
				//Manipulation is interpreted as a rejection of the terms & conditions,
				//so abort login immediately.
				response->release();
				response = createDOMDocument();
				response->appendChild(createDOMElement(response, TNC_RESPONSE_INVALID_REQUEST));
				answer->result = TC_FAILED;
				break;
			}
		}
		answer->xmlAnswer = response;
	}
	//Client accepts/rejects the T&Cs.
	else if(XMLString::equals(reqName, TNC_CONFIRM))
	{
		CAMsg::printMsg(LOG_DEBUG,"handling TC confirm.\n");
		bool tcAccepted = false;
		getDOMElementAttribute(request->getDocumentElement(), "accepted", tcAccepted);
		CAMsg::printMsg(LOG_DEBUG,"Client has%s accepted the T&Cs.\n", (tcAccepted ? "" : " not")  );

		answer->xmlAnswer = NULL;
		answer->result = tcAccepted ? TC_CONFIRMED : TC_FAILED;
	}
	//stops the message exchange (if client needs time for showing the T & Cs, etc.)
	else if(XMLString::equals(reqName, TNC_INTERRUPT))
	{
		CAMsg::printMsg(LOG_DEBUG,"client requested interrupting the tc login.\n");
		answer->xmlAnswer = NULL;
		answer->result = TC_FAILED;
	}
	//Client sends an invalid message
	else
	{
		answer->xmlAnswer = NULL; //TODO: set errorMessage here
		answer->result = TC_FAILED;
	}
	return answer;
}

SINT32 CAFirstMix::doUserLogin(CAMuxSocket* pNewUser,UINT8 peerIP[4])
{
	INIT_STACK;
	SINT32 ret = doUserLogin_internal(pNewUser, peerIP);
	FINISH_STACK("CAFirstMix::doUserLogin");
	return ret;
}

/** Sends and receives all data neccessary for a User to "login".
	* This means sending the public key of the Mixes and receiving the
	* sym keys of JAP. This is done in a thread on a per user basis
	* @todo Cleanup of runing thread if mix restarts...
***/
SINT32 CAFirstMix::doUserLogin_internal(CAMuxSocket* pNewUser,UINT8 peerIP[4])
	{
		INIT_STACK;
		BEGIN_STACK("CAFirstMix::doUserLogin");
			int ret=pNewUser->getCASocket()->setKeepAlive(true);
			if(ret!=E_SUCCESS)
				CAMsg::printMsg(LOG_DEBUG,"Error setting KeepAlive for user login connection!");

		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: start\n");
		#endif

		SAVE_STACK("CAFirstMix::doUserLogin", "after setting keep alive");

		// send the mix-keys to JAP
		if (pNewUser->getCASocket()->sendFullyTimeOut(m_xmlKeyInfoBuff,m_xmlKeyInfoSize, 40000, 10000) != E_SUCCESS)
		{
// #ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: Sending login data to client %u.%u.x.x has been interrupted!\n", peerIP[0],peerIP[1]);
// #endif
			m_pIPBlockList->insertIP(peerIP);

			delete pNewUser;
			pNewUser = NULL;
			m_pIPList->removeIP(peerIP);
			return E_UNKNOWN;
		}
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: login data sent\n");
		#endif
		SAVE_STACK("CAFirstMix::doUserLogin", "after sending login data");

		//(pNewUser)->send(m_xmlKeyInfoBuff,m_xmlKeyInfoSize);
		// es kann nicht blockieren unter der Annahme das der TCP-Sendbuffer > m_xmlKeyInfoSize ist....

		//wait for keys from user
		UINT16 xml_len;
		if(pNewUser->getCASocket()->isClosed())
		{
			CAMsg::printMsg(LOG_DEBUG,"User login: Socket was closed while waiting for first symmetric key from client!\n");
		}
		else if (pNewUser->getCASocket()->receiveFullyT((UINT8*)&xml_len,2,FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT)!=E_SUCCESS)
		{
//			#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG,"User login: timed out while waiting for first symmetric key from client %u.%u.x.x!\n", peerIP[0], peerIP[1]);
//			#endif
			m_pIPBlockList->insertIP(peerIP);
			
			delete pNewUser;
			pNewUser = NULL;
			m_pIPList->removeIP(peerIP);
			return E_UNKNOWN;
		}
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: received first symmetric key from client\n");
		#endif
		SAVE_STACK("CAFirstMix::doUserLogin", "received first symmetric key");

		xml_len=ntohs(xml_len);
		UINT8* xml_buff=new UINT8[xml_len+2]; //+2 for size...
		if(pNewUser->getCASocket()->isClosed() ||
		   pNewUser->getCASocket()->receiveFullyT(xml_buff+2,xml_len,FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT)!=E_SUCCESS)
		{
#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: timed out while waiting for second symmetric key from client!\n");
#endif
			m_pIPBlockList->insertIP(peerIP);

			delete pNewUser;
			pNewUser = NULL;
			delete[] xml_buff;
			xml_buff = NULL;
			m_pIPList->removeIP(peerIP);
			return E_UNKNOWN;
		}

		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: received second symmetric key from client\n");
		#endif
		SAVE_STACK("CAFirstMix::doUserLogin", "received second symmetric key");

		XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(xml_buff+2,xml_len);
		DOMElement* elemRoot=NULL;
		if(doc==NULL||(elemRoot=doc->getDocumentElement())==NULL||
			decryptXMLElement(elemRoot,m_pRSA)!=E_SUCCESS)
			{
				delete[] xml_buff;
				xml_buff = NULL;
				delete pNewUser;
				pNewUser = NULL;
				m_pIPList->removeIP(peerIP);
				if(doc!=NULL)
				{
					doc->release();
					doc = NULL;
				}
				return E_UNKNOWN;
			}
		elemRoot=doc->getDocumentElement();
		if(!equals(elemRoot->getNodeName(),"JAPKeyExchange"))
			{
				if (doc != NULL)
				{
					doc->release();
					doc = NULL;
				}
				delete[] xml_buff;
				xml_buff = NULL;
				delete pNewUser;
				pNewUser = NULL;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		DOMElement* elemLinkEnc=NULL;
		DOMElement* elemMixEnc=NULL;
		getDOMChildByName(elemRoot,"LinkEncryption",elemLinkEnc,false);
		getDOMChildByName(elemRoot,"MixEncryption",elemMixEnc,false);
		UINT8 linkKey[255],mixKey[255];
		UINT32 linkKeyLen=255,mixKeyLen=255;
		if(	getDOMElementValue(elemLinkEnc,linkKey,&linkKeyLen)!=E_SUCCESS||
				getDOMElementValue(elemMixEnc,mixKey,&mixKeyLen)!=E_SUCCESS||
				CABase64::decode(linkKey,linkKeyLen,linkKey,&linkKeyLen)!=E_SUCCESS||
				CABase64::decode(mixKey,mixKeyLen,mixKey,&mixKeyLen)!=E_SUCCESS||
				linkKeyLen!=64||mixKeyLen!=32)
			{
				if (doc != NULL)
				{
					doc->release();
					doc = NULL;
				}
				delete[] xml_buff;
				xml_buff = NULL;
				delete pNewUser;
				pNewUser = NULL;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		//Getting control channel keys if available
		///TODO: move to the if-statement above
		DOMElement* elemControlChannelEnc=NULL;
		getDOMChildByName(elemRoot,"ControlChannelEncryption",elemControlChannelEnc,false);
		UINT8 controlchannelKey[255];
		UINT32 controlchannelKeyLen=255;
		UINT8* controlchannelRecvKey=NULL;
		UINT8* controlchannelSentKey=NULL;
		if(	getDOMElementValue(elemControlChannelEnc,controlchannelKey,&controlchannelKeyLen)==E_SUCCESS&&
				CABase64::decode(controlchannelKey,controlchannelKeyLen,controlchannelKey,&controlchannelKeyLen)==E_SUCCESS&&
				controlchannelKeyLen==32)
			{
				controlchannelRecvKey=controlchannelKey;
				controlchannelSentKey=controlchannelKey+16;
			}

		//Sending Signature....
		xml_buff[0]=(UINT8)(xml_len>>8);
		xml_buff[1]=(UINT8)(xml_len&0xFF);
		UINT8* sig=new UINT8[255];
		UINT32 siglen=255;
		//TODO change me to fully support MultiSig
		//m_pSignature->sign(xml_buff,xml_len+2,sig,&siglen);
		m_pMultiSignature->sign(xml_buff,xml_len+2,sig,&siglen);
		XERCES_CPP_NAMESPACE::DOMDocument* docSig=createDOMDocument();

		DOMElement *elemSig=NULL;

#ifdef LOG_DIALOG
		DOMElement* elemDialog=NULL;
		getDOMChildByName(elemRoot,"Dialog",elemDialog,false);
		UINT8 strDialog[255];
		memset(strDialog,0,255);
		UINT32 dialogLen=255;
		getDOMElementValue(elemDialog,strDialog,&dialogLen);
#endif
#ifdef REPLAY_DETECTION
		//checking if Replay-Detection is enabled
		DOMElement *elemReplay=NULL;
		UINT8 replay[6];
		UINT32 replay_len=5;
		if(	(getDOMChildByName(elemRoot,"ReplayDetection",elemReplay,false)==E_SUCCESS)&&
			(getDOMElementValue(elemReplay,replay,&replay_len)==E_SUCCESS)&&
			(strncmp((char*)replay,"true",5)==0)){
				elemRoot=createDOMElement(docSig,"MixExchange");
				elemReplay=createDOMElement(docSig,"Replay");
				docSig->appendChild(elemRoot);
				elemRoot->appendChild(elemReplay);

				UINT32 diff=(UINT32)(time(NULL)-m_u64LastTimestampReceived);
				for(SINT32 i=0;i<getMixCount()-1;i++)
				{
					DOMElement* elemMix=createDOMElement(docSig,"Mix");
					setDOMElementAttribute(elemMix,"id",m_arMixParameters[i].m_strMixID);
					DOMElement* elemReplayOffset=createDOMElement(docSig,"ReplayOffset");
					setDOMElementValue(elemReplayOffset,(UINT32) (m_arMixParameters[i].m_u32ReplayOffset+diff));
					elemMix->appendChild(elemReplayOffset);
					DOMElement* elemReplayBase=createDOMElement(docSig,"ReplayBase");
					setDOMElementValue(elemReplayBase,(UINT32) (m_arMixParameters[i].m_u32ReplayBase));
					elemMix->appendChild(elemReplayBase);
					elemReplay->appendChild(elemMix);
				}

				DOMElement* elemMix=createDOMElement(docSig,"Mix");
				UINT8 buff[255];
				CALibProxytest::getOptions()->getMixId(buff,255);
				setDOMElementAttribute(elemMix,"id",buff);
				DOMElement* elemReplayOffset=createDOMElement(docSig,"ReplayOffset");
				setDOMElementValue(elemReplayOffset,(UINT32) (time(NULL)-m_u64ReferenceTime));
				elemMix->appendChild(elemReplayOffset);
				DOMElement* elemReplayBase=createDOMElement(docSig,"ReplayBase");
				setDOMElementValue(elemReplayBase,(UINT32) (REPLAY_BASE));
				elemMix->appendChild(elemReplayOffset);
				elemReplay->appendChild(elemMix);

				elemSig=createDOMElement(docSig,"Signature");
				elemRoot->appendChild(elemSig);

				CAMsg::printMsg(LOG_DEBUG,"Replay Detection requested\n");
				}
		else {
				elemSig=createDOMElement(docSig,"Signature");
				docSig->appendChild(elemSig);
			}
#endif
#ifndef REPLAY_DETECTION
		elemSig=createDOMElement(docSig,"Signature");
		docSig->appendChild(elemSig);
#endif
		DOMElement* elemSigValue=createDOMElement(docSig,"SignatureValue");
		elemSig->appendChild(elemSigValue);
		UINT32 u32=siglen;
		CABase64::encode(sig,u32,sig,&siglen);
		sig[siglen]=0;
		setDOMElementValue(elemSigValue,sig);
		delete[] sig;
		u32=xml_len;
		DOM_Output::dumpToMem(docSig,xml_buff+2,&u32);
		if (docSig != NULL)
		{
			docSig->release();
			docSig = NULL;
		}
		xml_buff[0]=(UINT8)(u32>>8);
		xml_buff[1]=(UINT8)(u32&0xFF);

		if (pNewUser->getCASocket()->isClosed() ||
		    pNewUser->getCASocket()->sendFullyTimeOut(xml_buff,u32+2, 30000, 10000) != E_SUCCESS)
		{
			if (doc != NULL)
			{
				doc->release();
				doc = NULL;
			}
			CAMsg::printMsg(LOG_DEBUG,"User login: Sending key exchange signature has been interrupted!\n");
			m_pIPBlockList->insertIP(peerIP);
			
			delete[] xml_buff;
			xml_buff = NULL;
			delete pNewUser;
			pNewUser = NULL;
			m_pIPList->removeIP(peerIP);
			return E_UNKNOWN;
		}
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: key exchange signature sent\n");
		#endif
		delete[] xml_buff;
		xml_buff = NULL;
#if 0 //terms and conditions sending disabled.
		/* handle Terms And Conditions */
		bool loginFailed = false;
		bool tcProcedureFinished = false;

		while( !(tcProcedureFinished || loginFailed) )
		{
			UINT16 tcRequestDataLen = 0;
			if (pNewUser->getCASocket()->isClosed() ||
				pNewUser->getCASocket()->receiveFullyT((UINT8*)&tcRequestDataLen, 2, 10000) != E_SUCCESS)
			{
				loginFailed = true;
				CAMsg::printMsg(LOG_ERR,"User login: Receiving t&c protocol data size has been interrupted!\n");
				break;
			}

			tcRequestDataLen = ntohs(tcRequestDataLen);
			//CAMsg::printMsg(LOG_DEBUG,"User login: expecting %u bytes!\n", tcRequestDataLen);
			UINT8 tcDataBuf[tcRequestDataLen+1];
			tcDataBuf[tcRequestDataLen] = 0;
			if (pNewUser->getCASocket()->isClosed() ||
				pNewUser->getCASocket()->receiveFullyT(tcDataBuf, tcRequestDataLen, 10000) != E_SUCCESS)
			{
				loginFailed = true;
				CAMsg::printMsg(LOG_ERR,"User login: Receiving t&c protocol data has been interrupted!\n");
				break;
			}

			XERCES_CPP_NAMESPACE::DOMDocument *tcData = parseDOMDocument(tcDataBuf, tcRequestDataLen);
			if( (tcData == NULL) || (tcData->getDocumentElement() == NULL) )
			{
				CAMsg::printMsg(LOG_ERR,"Could not parse t&c data!\n");
				loginFailed = true;
				break;
			}

			termsAndConditionMixAnswer_t *answer = handleTermsAndConditionsLogin(tcData);
			if(answer == NULL)
			{
				loginFailed = true;
				tcData->release();
				tcData = NULL;
				break;
			}

			if( (answer->xmlAnswer != NULL) )
			{
				UINT32 size = 0;
				UINT8 *answerBuff = DOM_Output::dumpToMem(answer->xmlAnswer, &size);
				if(answerBuff != NULL)
				{
					//CAMsg::printMsg(LOG_DEBUG,"User login: answer %s\n", answerBuff);

					UINT32 netSize = htonl(size);

					if (pNewUser->getCASocket()->isClosed() ||
						pNewUser->getCASocket()->sendFullyTimeOut((UINT8 *) &netSize, 4, 30000, 10000) != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_DEBUG,"User login: sending t&c protocol data size has been interrupted!\n");
						loginFailed = true;
					}
					else if (pNewUser->getCASocket()->isClosed() ||
						pNewUser->getCASocket()->sendFullyTimeOut(answerBuff, size, 30000, 10000) != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_DEBUG,"User login: Sending t&c protocol data has been interrupted!\n");
						loginFailed = true;
					}
					delete [] answerBuff;
				}
			}
			tcProcedureFinished = (answer->result != TC_UNFINISHED);
			loginFailed = (answer->result == TC_FAILED);
			cleanupTnCMixAnswer(answer);
			delete answer;
			answer = NULL;
			tcData->release();
			tcData = NULL;
		}

		//Only if a positive confirm message was received, the client may pass!

		if(loginFailed)
		{
			if (doc != NULL)
			{
				doc->release();
				doc = NULL;
			}
			CAMsg::printMsg(LOG_DEBUG,"User login: failed!\n");
			delete[] xml_buff;
			xml_buff = NULL;
			delete pNewUser;
			pNewUser = NULL;
			m_pIPList->removeIP(peerIP);
			return E_UNKNOWN;
		}
		/* end Terms And Conditions negotiation */
#endif
		SAVE_STACK("CAFirstMix::doUserLogin", "sent key exchange signature");

		pNewUser->getCASocket()->setNonBlocking(true);

		SAVE_STACK("CAFirstMix::doUserLogin", "Creating CAQueue...");
		CAQueue* tmpQueue=new CAQueue(sizeof(tQueueEntry));

		SAVE_STACK("CAFirstMix::doUserLogin", "Adding user to connection list...");
#ifdef LOG_DIALOG
		fmHashTableEntry* pHashEntry=m_pChannelList->add(pNewUser,peerIP,tmpQueue,strDialog);
#else
		fmHashTableEntry* pHashEntry=m_pChannelList->add(pNewUser,peerIP,tmpQueue,controlchannelSentKey,controlchannelRecvKey);
#endif
		if( (pHashEntry == NULL) ||
			(pHashEntry->pControlMessageQueue == NULL) )// adding user connection to mix->JAP channel list (stefan: sollte das nicht connection list sein? --> es handelt sich um eine Datenstruktu fr Connections/Channels ).
		{
			if (doc != NULL)
			{
				doc->release();
				doc = NULL;
			}
			CAMsg::printMsg(LOG_ERR,"User login: Could not add new socket to connection list!\n");
			if(pHashEntry!=NULL)
				m_pChannelList->remove(pNewUser);
			m_pIPList->removeIP(peerIP);
			delete tmpQueue;
			tmpQueue = NULL;
			delete pNewUser;
			pNewUser = NULL;
			return E_UNKNOWN;
		}

		SAVE_STACK("CAFirstMix::doUserLogin", "socket added to connection list");
#ifdef PAYMENT
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: registering payment control channel\n");
		#endif
		pHashEntry->pControlChannelDispatcher->registerControlChannel(new CAAccountingControlChannel(pHashEntry));
		SAVE_STACK("CAFirstMix::doUserLogin", "payment registered");
#endif
		pHashEntry->pSymCipher=new CASymCipher();
		pHashEntry->pSymCipher->setKey(mixKey);
		pHashEntry->pSymCipher->setIVs(mixKey+16);
		pNewUser->setReceiveKey(linkKey,32);
		pNewUser->setSendKey(linkKey+32,32);
		pNewUser->setCrypt(true);

		if (doc != NULL)
		{
			doc->release();
			doc = NULL;
		}
#ifdef PAYMENT

		SAVE_STACK("CAFirstMix::doUserLogin", "Starting AI login procedure");
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG,"Starting AI login procedure for owner %x \n", pHashEntry);
#endif
		SINT32 ai_ret;
		MIXPACKET *paymentLoginPacket = new MIXPACKET;
		tQueueEntry *aiAnswerQueueEntry=new tQueueEntry;
		CAQueue *controlMessages = pHashEntry->pControlMessageQueue;
		UINT32 qlen=sizeof(tQueueEntry);
		SINT32 aiLoginStatus = 0;
		aiLoginStatus = CAAccountingInstance::loginProcessStatus(pHashEntry);
		while(aiLoginStatus & AUTH_LOGIN_NOT_FINISHED)
		{
			if(pNewUser->receive(paymentLoginPacket, AI_LOGIN_SO_TIMEOUT) != MIXPACKET_SIZE)
			{
				CAMsg::printMsg(LOG_NOTICE,"AI login: client receive timeout.\n");
				aiLoginStatus = AUTH_LOGIN_FAILED;
				break;
			}
			if(paymentLoginPacket->channel > 0 && paymentLoginPacket->channel < 256)
			{
				if(!pHashEntry->pControlChannelDispatcher->proccessMixPacket(paymentLoginPacket))
				{
					aiLoginStatus = AUTH_LOGIN_FAILED;
					break;
				}

				while(controlMessages->getSize()>0)
				{
					controlMessages->get((UINT8*)aiAnswerQueueEntry,&qlen);
					pNewUser->prepareForSend(&(aiAnswerQueueEntry->packet));
					ai_ret = pNewUser->getCASocket()->
						sendFullyTimeOut(((UINT8*)&(aiAnswerQueueEntry->packet)), MIXPACKET_SIZE, 3*(AI_LOGIN_SO_TIMEOUT), AI_LOGIN_SO_TIMEOUT);
					if (ai_ret != E_SUCCESS)
					{
						if(ai_ret == E_TIMEDOUT )
						{
							CAMsg::printMsg(LOG_NOTICE,"timeout occurred during AI login.");
						}
						aiLoginStatus = AUTH_LOGIN_FAILED;
						goto loop_break;
					}
				}
				if(aiLoginStatus == AUTH_LOGIN_FAILED)
				{
					break;
				}
			}
			aiLoginStatus = CAAccountingInstance::loginProcessStatus(pHashEntry);
		}
loop_break:
		SAVE_STACK("CAFirstMix::doUserLogin", "AI login packages exchanged.");
		/* We have exchanged all AI login packets:
		 * 1. AccountCert
		 * 2. ChallengeResponse
		 * 3. Cost confirmation.
		 * Now start settlement to ensure that the clients account is balanced
		 */
		if(!(aiLoginStatus & AUTH_LOGIN_FAILED))
		{
			if(!(aiLoginStatus & (AUTH_LOGIN_SKIP_SETTLEMENT )) || (aiLoginStatus & (AUTH_WAITING_FOR_FIRST_SETTLED_CC)) )
			{
//#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG,"AI login messages successfully exchanged: now starting settlement for user account balancing check\n");
//#endif
				if(CAAccountingInstance::newSettlementTransaction() != E_SUCCESS)
				{
					aiLoginStatus |= AUTH_LOGIN_FAILED;
				}
			}
//#ifdef DEBUG
			else
			{
				CAMsg::printMsg(LOG_DEBUG,"AI login messages successfully exchanged: skipping settlement, user has valid prepaid amount\n");
			}
//#endif
		}

		if(!(aiLoginStatus & AUTH_LOGIN_FAILED))
		{
			aiLoginStatus = CAAccountingInstance::finishLoginProcess(pHashEntry);
			if(pNewUser != NULL)
			{
				while(controlMessages->getSize()>0)
				{
					controlMessages->get((UINT8*)aiAnswerQueueEntry,&qlen);
					pNewUser->prepareForSend(&(aiAnswerQueueEntry->packet));

					//not really elegant but works: if the client closed the socket during the settlement
					//the first send will succeed but the second one will fail.
					ai_ret = pNewUser->getCASocket()->
							sendFullyTimeOut(((UINT8*)&(aiAnswerQueueEntry->packet)), 499, 3*(AI_LOGIN_SO_TIMEOUT), AI_LOGIN_SO_TIMEOUT);

					ai_ret =  pNewUser->getCASocket()->
							sendFullyTimeOut(((UINT8*)&(aiAnswerQueueEntry->packet)+499), 499, 3*(AI_LOGIN_SO_TIMEOUT), AI_LOGIN_SO_TIMEOUT);
					if (ai_ret != E_SUCCESS)
					{
						int errnum = errno;
						CAMsg::printMsg(LOG_INFO,"AI login: net error occured after settling: %s\n", strerror(errnum));
						aiLoginStatus |= AUTH_LOGIN_FAILED;
						break;
					}
				}
			}
			else
			{
				CAMsg::printMsg(LOG_DEBUG,"Socket was disposed.\n");
				aiLoginStatus |= AUTH_LOGIN_FAILED;
			}
		}

		delete paymentLoginPacket;
		paymentLoginPacket = NULL;
		delete aiAnswerQueueEntry;
		aiAnswerQueueEntry = NULL;

		SAVE_STACK("CAFirstMix::doUserLogin", "AI login procedure finished.");

		if((aiLoginStatus & AUTH_LOGIN_FAILED))
		{
#ifdef DEBUG
			CAMsg::printMsg(LOG_INFO,"User AI login failed: deleting socket %x\n", pHashEntry);
#endif
			CAAccountingInstance::unlockLogin(pHashEntry);
			m_pChannelList->remove(pNewUser);
			delete pNewUser;
			pNewUser = NULL;
			m_pIPList->removeIP(peerIP);
			return E_UNKNOWN;
		}
		/* Hot fix: push timeout entry only if login was succesful, otherwise
		 * socket may be deleted due to timeout, login fails and the socket will be deleted
		 * for second time causing a segfault.
		 */
		m_pChannelList->pushTimeoutEntry(pHashEntry);
		CAAccountingInstance::unlockLogin(pHashEntry);
#ifdef DEBUG
		CAMsg::printMsg(LOG_INFO,"User AI login successful for owner %x\n", pHashEntry);
#endif
#endif

#ifdef WITH_CONTROL_CHANNELS_TEST
		pHashEntry->pControlChannelDispatcher->registerControlChannel(new CAControlChannelTest());
#endif
#ifdef REPLAY_DETECTION
		pHashEntry->pControlChannelDispatcher->registerControlChannel(new CAReplayControlChannel(m_pReplayMsgProc));
#endif
#ifdef COUNTRY_STATS
		incUsers(pHashEntry);
#else
		incUsers();
#endif
#ifdef HAVE_EPOLL
		m_psocketgroupUsersRead->add(*pNewUser,m_pChannelList->get(pNewUser)); // add user socket to the established ones that we read data from.
		m_psocketgroupUsersWrite->add(*pNewUser,m_pChannelList->get(pNewUser));
#else
		if(m_psocketgroupUsersRead->add(*pNewUser)!=E_SUCCESS)// add user socket to the established ones that we read data from.
		{
			CAMsg::printMsg(LOG_DEBUG,"User login: Adding to m_psocketgroupUsersRead failed!\n");
		}
		if(	m_psocketgroupUsersWrite->add(*pNewUser)!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG,"User login: Adding to m_psocketgroupUsersWrite failed!\n");
		}
#endif

#ifndef LOG_DIALOG
		CAMsg::printMsg(LOG_DEBUG,"User login: finished\n");
#else
		CAMsg::printMsg(LOG_DEBUG,"User login: finished -- connection-ID: %Lu -- country-id: %u -- dialog: %s\n",pHashEntry->id,pHashEntry->countryID,pHashEntry->strDialog);
#endif
		return E_SUCCESS;
	}

#ifdef PAYMENT
bool CAFirstMix::forceKickout(fmHashTableEntry* pHashTableEntry, const XERCES_CPP_NAMESPACE::DOMDocument *pErrDoc)
{
	return m_pChannelList->forceKickout(pHashTableEntry, pErrDoc);
}
#endif
//NEVER EVER DELETE THIS!
/*
THREAD_RETURN loopReadFromUsers(void* param)
	{
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		CAFirstMixChannelList* pChannelList=pFirstMix->m_pChannelList;
		CASocketGroup* psocketgroupUsersRead=pFirstMix->m_psocketgroupUsersRead;
		CASocketGroup* psocketgroupUsersWrite=pFirstMix->m_psocketgroupUsersWrite;
		CAQueue* pQueueSendToMix=pFirstMix->m_pQueueSendToMix;
//		CAInfoService* pInfoService=pFirstMix->m_pInfoService;
		CAASymCipher* pRSA=pFirstMix->m_pRSA;
		CAIPList* pIPList=pFirstMix->m_pIPList;
		CAMuxSocket* pNextMix=pFirstMix->m_pMuxOut;

		CAMuxSocket* pMuxSocket;

		SINT32 countRead;
		SINT32 ret;
		UINT8* ip=new UINT8[4];
		UINT8* tmpBuff=new UINT8[MIXPACKET_SIZE];
		MIXPACKET* pMixPacket=new MIXPACKET;
		UINT8* rsaBuff=new UINT8[RSA_SIZE];

		fmHashTableEntry* pHashEntry;
		fmChannelListEntry* pEntry;
		CASymCipher* pCipher=NULL;

		for(;;)
			{
				countRead=psocketgroupUsersRead->select(false,1000); //if we sleep here forever, we will not notice new sockets...
				if(countRead<0)
					{ //check for error
						if(pFirstMix->m_bRestart ||countRead!=E_TIMEDOUT)
							goto END_THREAD;
					}
				pHashEntry=pChannelList->getFirst();
				while(pHashEntry!=NULL&&countRead>0)
					{
						pMuxSocket=pHashEntry->pMuxSocket;
						if(psocketgroupUsersRead->isSignaled(*pMuxSocket))
							{
								countRead--;
								ret=pMuxSocket->receive(pMixPacket,0);
								if(ret==SOCKET_ERROR)
									{
										pMuxSocket->getPeerIP(ip);
										pIPList->removeIP(ip);
										psocketgroupUsersRead->remove(pMuxSocket);
										psocketgroupUsersWrite->remove(pMuxSocket);
										pEntry=pChannelList->getFirstChannelForSocket(pMuxSocket);
										while(pEntry!=NULL)
											{
												pNextMix->close(pEntry->channelOut,tmpBuff);
												pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
												delete pEntry->pCipher;
												pEntry=pChannelList->getNextChannel(pEntry);
											}
										ASSERT(pHashEntry->pQueueSend!=NULL,"Send queue is NULL");
										delete pHashEntry->pQueueSend;
										pChannelList->remove(pMuxSocket);
										pMuxSocket->close();
										delete pMuxSocket;
										pFirstMix->decUsers();
									}
								else if(ret==MIXPACKET_SIZE)
									{
										if(pMixPacket->flags==CHANNEL_CLOSE)
											{
												pEntry=pChannelList->get(pMuxSocket,pMixPacket->channel);
												if(pEntry!=NULL)
													{
														pNextMix->close(pEntry->channelOut,tmpBuff);
														pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
														delete pEntry->pCipher;
														pChannelList->removeChannel(pMuxSocket,pMixPacket->channel);
													}
												else
													{
														#if defined(_DEBUG) && ! defined(__MIX_TEST)
															CAMsg::printMsg(LOG_DEBUG,"Invalid ID to close from Browser!\n");
														#endif
													}
											}
										else
											{
												pEntry=pChannelList->get(pMuxSocket,pMixPacket->channel);
												if(pEntry!=NULL&&pMixPacket->flags==CHANNEL_DATA)
													{
														pMixPacket->channel=pEntry->channelOut;
														pCipher=pEntry->pCipher;
														pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														pNextMix->send(pMixPacket,tmpBuff);
														pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
														pFirstMix->incMixedPackets();
													}
												else if(pEntry==NULL&&(pMixPacket->flags==CHANNEL_OPEN_OLD||pMixPacket->flags==CHANNEL_OPEN_NEW))
													{
														pCipher= new CASymCipher();
														pRSA->decrypt(pMixPacket->data,rsaBuff);
														pCipher->setKeyAES(rsaBuff);
														pCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																						 pMixPacket->data+RSA_SIZE-KEY_SIZE,
																						 DATA_SIZE-RSA_SIZE);
														memcpy(pMixPacket->data,rsaBuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);

														if(pChannelList->addChannel(pMuxSocket,pMixPacket->channel,pCipher,&pMixPacket->channel)!=E_SUCCESS)
															{//todo --> maybe move up to not make decryption!!
																delete pCipher;
															}
														else
															{
																#if defined(_DEBUG) && !defined(__MIX_TEST)
																	CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",pMixPacket->channel);
																#endif
																pNextMix->send(pMixPacket,tmpBuff);
																pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
																pFirstMix->incMixedPackets();
															}
													}
											}
									}
							}
						pHashEntry=pChannelList->getNext();
					}
			}
END_THREAD:
		delete ip;
		delete tmpBuff;
		delete pMixPacket;
		delete rsaBuff;
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread ReadFromUser\n");
		THREAD_RETURN_SUCCESS;
	}
*/

SINT32 CAFirstMix::clean()
	{
		//#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::clean() start\n");
		//#endif
		m_bRunLog=false;
		m_bRestart=true;
		if(m_pMuxOut!=NULL)
			{
				m_pMuxOut->close();
			}

		//writing some bytes to the queue...
		if(m_pQueueSendToMix!=NULL)
			{
				UINT8 b[sizeof(tQueueEntry)+1];
				m_pQueueSendToMix->add(b,sizeof(tQueueEntry)+1);
			}

		if(m_pthreadAcceptUsers!=NULL)
				{
					CAMsg::printMsg(LOG_CRIT,"Wait for LoopAcceptUsers!\n");
					m_pthreadAcceptUsers->join();
					delete m_pthreadAcceptUsers;
				}
		m_pthreadAcceptUsers=NULL;
		if(m_pthreadsLogin!=NULL)
			delete m_pthreadsLogin;
		m_pthreadsLogin=NULL;
    //     if(m_pInfoService!=NULL)
    //     {
    //         CAMsg::printMsg(LOG_CRIT,"Stopping InfoService....\n");
    //         CAMsg::printMsg	(LOG_CRIT,"Memory usage before: %u\n",getMemoryUsage());
    //         m_pInfoService->stop();
    //         CAMsg::printMsg	(LOG_CRIT,"Memory usage after: %u\n",getMemoryUsage());
    //         CAMsg::printMsg(LOG_CRIT,"Stopped InfoService!\n");
    //         delete m_pInfoService;
    //     }
    //     m_pInfoService=NULL;

	if(m_pthreadSendToMix!=NULL)
		{
			CAMsg::printMsg(LOG_CRIT,"Wait for LoopSendToMix!\n");
			m_pthreadSendToMix->join();
			delete m_pthreadSendToMix;
		}
	m_pthreadSendToMix=NULL;
	if(m_pthreadReadFromMix!=NULL)
		{
			CAMsg::printMsg(LOG_CRIT,"Wait for LoopReadFromMix!\n");
			m_pthreadReadFromMix->join();
			delete m_pthreadReadFromMix;
		}
	m_pthreadReadFromMix=NULL;

#ifdef CH_LOG_STUDY
	if(nrOfChThread != NULL)
	{
		nrOfChThread->join();
		delete nrOfChThread;
		nrOfChThread = NULL;
	}
#endif

#ifdef LOG_PACKET_TIMES
		if(m_pLogPacketStats!=NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"Wait for LoopLogPacketStats to terminate!\n");
				m_pLogPacketStats->stop();
				delete m_pLogPacketStats;
			}
		m_pLogPacketStats=NULL;
#endif
		if(m_arrSocketsIn!=NULL)
			{
				for(UINT32 i=0;i<m_nSocketsIn;i++)
				{
					if (m_arrSocketsIn[i] != NULL)
					{
						m_arrSocketsIn[i]->close();
						delete m_arrSocketsIn[i];
						m_arrSocketsIn[i] = NULL;
					}
				}
				delete[] m_arrSocketsIn;
			}
		m_arrSocketsIn=NULL;
#ifdef REPLAY_DETECTION
		if(m_pReplayMsgProc!=NULL)
			{
				delete m_pReplayMsgProc;
			}
		m_pReplayMsgProc=NULL;
#endif

		if(m_pMuxOutControlChannelDispatcher!=NULL)
		{
			delete m_pMuxOutControlChannelDispatcher;
		}
		m_pMuxOutControlChannelDispatcher=NULL;

		if(m_pMuxOut!=NULL)
			{
				m_pMuxOut->close();
				delete m_pMuxOut;
			}
		m_pMuxOut=NULL;
#ifdef COUNTRY_STATS
		deleteCountryStats();
#endif
		if(m_pIPList!=NULL)
			delete m_pIPList;
		m_pIPList=NULL;
		if(m_pQueueSendToMix!=NULL)
			delete m_pQueueSendToMix;
		m_pQueueSendToMix=NULL;
		if(m_pQueueReadFromMix!=NULL)
			delete m_pQueueReadFromMix;
		m_pQueueReadFromMix=NULL;

		if(m_pChannelList!=NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"Before deleting CAFirstMixChannelList()!\n");
				CAMsg::printMsg	(LOG_CRIT,"Memory usage before: %u\n",getMemoryUsage());
				fmHashTableEntry* pHashEntry=m_pChannelList->getFirst();
				while(pHashEntry!=NULL)
					{
						CAMuxSocket * pMuxSocket=pHashEntry->pMuxSocket;
						delete pHashEntry->pQueueSend;
						pHashEntry->pQueueSend = NULL;
						delete pHashEntry->pSymCipher;
						pHashEntry->pSymCipher = NULL;

						fmChannelListEntry* pEntry=m_pChannelList->getFirstChannelForSocket(pHashEntry->pMuxSocket);
						while(pEntry!=NULL)
							{
								delete pEntry->pCipher;
								pEntry->pCipher = NULL;
								pEntry=m_pChannelList->getNextChannel(pEntry);
							}
						m_pChannelList->remove(pHashEntry->pMuxSocket);
						pMuxSocket->close();
						delete pMuxSocket;
						pMuxSocket = NULL;
						pHashEntry=m_pChannelList->getNext();
					}
			}

		if(m_pChannelList!=NULL)
			delete m_pChannelList;
		m_pChannelList=NULL;
		CAMsg::printMsg	(LOG_CRIT,"Memory usage after: %u\n",getMemoryUsage());
#ifdef PAYMENT
	CAAccountingInstance::clean();
	CAAccountingDBInterface::cleanup();
#endif
		if(m_psocketgroupUsersRead!=NULL)
			delete m_psocketgroupUsersRead;
		m_psocketgroupUsersRead=NULL;
		if(m_psocketgroupUsersWrite!=NULL)
			delete m_psocketgroupUsersWrite;
		m_psocketgroupUsersWrite=NULL;
		if(m_pRSA!=NULL)
			delete m_pRSA;
		m_pRSA=NULL;
		if(m_xmlKeyInfoBuff!=NULL)
			delete[] m_xmlKeyInfoBuff;
		m_xmlKeyInfoBuff=NULL;
		m_docMixCascadeInfo=NULL;
		if(m_arMixParameters!=NULL)
		{
			for(UINT32 i=0;i<m_u32MixCount-1;i++)
				{
					delete[] m_arMixParameters[i].m_strMixID;
				}
			delete[] m_arMixParameters;
		}
		m_arMixParameters=NULL;
		m_u32MixCount=0;
		m_nMixedPackets=0; //reset to zero after each restart (at the moment neccessary for infoservice)
		m_nUser=0;


		delete m_pIPBlockList;
		m_pIPBlockList = NULL;

		CAMsg::printMsg	(LOG_CRIT,"before tc cleanup\n");
		for (UINT32 i = 0; i < m_nrOfTermsAndConditionsDefs; i++)
		{
			delete m_tnCDefs[i];
			m_tnCDefs[i] = NULL;
		}
		delete [] m_tnCDefs;
		m_tnCDefs = NULL;
		m_nrOfTermsAndConditionsDefs = 0;
		CAMsg::printMsg	(LOG_CRIT,"before template cleanup\n");
		for(UINT32 i = 0; i < m_nrOfTermsAndConditionsTemplates; i++)
		{
			m_tcTemplates[i]->release();
			m_tcTemplates[i] = NULL;
		}
		delete [] m_tcTemplates;
		m_tcTemplates = NULL;
		CAMsg::printMsg	(LOG_CRIT,"before template owner release\n");
		if(m_templatesOwner != NULL)
		{
			m_templatesOwner->release();
			m_templatesOwner = NULL;
		}
		CAMsg::printMsg	(LOG_CRIT,"after template owner release\n");
		m_nrOfTermsAndConditionsTemplates = 0;
		//#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::clean() finished\n");
		//#endif

		return E_SUCCESS;
	}

#ifdef DELAY_USERS
SINT32 CAFirstMix::reconfigure()
	{
		CAMsg::printMsg(LOG_DEBUG,"Reconfiguring First Mix\n");
#ifdef DELAY_USERS
		CAMsg::printMsg(LOG_DEBUG,"Set new ressources limitation parameters\n");
		if(m_pChannelList!=NULL)
			m_pChannelList->setDelayParameters(	CALibProxytest::getOptions()->getDelayChannelUnlimitTraffic(),
																					CALibProxytest::getOptions()->getDelayChannelBucketGrow(),
																					CALibProxytest::getOptions()->getDelayChannelBucketGrowIntervall());
#endif
		return E_SUCCESS;
	}
#endif

/** set u32MixCount and m_arMixParameters from the <Mixes> element received from the second mix.*/
SINT32 CAFirstMix::initMixParameters(DOMElement*  elemMixes)
	{
		DOMNodeList* nl=getElementsByTagName(elemMixes,"Mix");
		m_u32MixCount=nl->getLength();
		m_arMixParameters=new tMixParameters[m_u32MixCount];
		memset(m_arMixParameters,0,sizeof(tMixParameters)*m_u32MixCount);
		UINT8 buff[255];
		CALibProxytest::getOptions()->getMixId(buff,255);
		UINT32 len=strlen((char*)buff)+1;
		UINT32 aktMix=0;
		for(UINT32 i=0;i<nl->getLength();i++)
		{
			DOMNode* child=nl->item(i);
			len=255;
			getDOMElementAttribute(child,"id",buff,&len);
			m_arMixParameters[aktMix].m_strMixID=new UINT8[len+1];
			memcpy(m_arMixParameters[aktMix].m_strMixID,buff,len);
			m_arMixParameters[aktMix].m_strMixID[len]=0;
			aktMix++;
		}
		m_u32MixCount++;
		return E_SUCCESS;
	}

UINT32 CAFirstMix::getNrOfUsers()
{
	#ifdef PAYMENT
	return CAAccountingInstance::getNrOfUsers();
	#else
	return m_nUser;
	#endif
}

SINT32 CAFirstMix::getMixedPackets(UINT64& ppackets)
{
	set64(ppackets,m_nMixedPackets);
	return E_SUCCESS;
}

SINT32 CAFirstMix::getLevel(SINT32* puser,SINT32* prisk,SINT32* ptraffic)
{
	*puser=(SINT32)getNrOfUsers();
	*prisk=-1;
	*ptraffic=-1;
	return E_SUCCESS;
}

#ifdef REPLAY_DETECTION
SINT32 CAFirstMix::sendReplayTimestampRequestsToAllMixes()
	{
		for(UINT32 i=0;i<m_u32MixCount-1;i++)
			{
				m_pReplayMsgProc->sendGetTimestamp(m_arMixParameters[i].m_strMixID);
			}
		return E_SUCCESS;
	}
#endif //REPLAY_DETECTION

#ifdef COUNTRY_STATS
#define COUNTRY_STATS_DB "CountryStats"
#define NR_OF_COUNTRIES 254

/** Escape a string so tha it could be used as table anme. This is: exchange the chars '.' ; '/' ; '\' ; ':' with '_'
*/
void mysqlEscapeTableName(UINT8* str)
	{
		UINT32 i=0;
		while(str[i]!=0)
			{
				if(str[i]=='.'||str[i]==':'||str[i]=='/'||str[i]=='\\')
					str[i]='_';
				i++;
			}
	}

SINT32 CAFirstMix::initCountryStats(char* db_host,char* db_user,char* db_passwd)
	{
		m_CountryStats=NULL;
		m_mysqlCon=mysql_init(NULL);
#ifdef HAVE_MYSQL_OPT_RECONNECT
		my_bool thetrue=1;
		mysql_options(m_mysqlCon,MYSQL_OPT_RECONNECT,&thetrue);
#endif
		MYSQL* tmp=NULL;
		tmp=mysql_real_connect(m_mysqlCon,db_host,db_user,db_passwd,COUNTRY_STATS_DB,0,NULL,0);
		if(tmp==NULL)
			{
				CAMsg::printMsg(LOG_DEBUG,"Could not connect to CountryStats DB!\n");
				mysql_thread_end();
				mysql_close(m_mysqlCon);
				m_mysqlCon=NULL;
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_DEBUG,"Connected to CountryStats DB!\n");
		char query[1024];
		UINT8 buff[255];
		CALibProxytest::getOptions()->getCascadeName(buff,255);
		mysqlEscapeTableName(buff);
		sprintf(query,"CREATE TABLE IF NOT EXISTS `stats_%s` (date timestamp,id int,count int,packets_in int,packets_out int)",buff);
		SINT32 ret=mysql_query(m_mysqlCon,query);
		if(ret!=0)
		{
				CAMsg::printMsg(LOG_INFO,"CountryStats DB - create table for %s failed!\n",buff);
		}
		m_CountryStats=new UINT32[NR_OF_COUNTRIES+1];
		memset((void*)m_CountryStats,0,sizeof(UINT32)*(NR_OF_COUNTRIES+1));
		m_PacketsPerCountryIN=new tUINT32withLock[NR_OF_COUNTRIES+1];
		//memset((void*)m_PacketsPerCountryIN,0,sizeof(UINT32)*(NR_OF_COUNTRIES+1));
		m_PacketsPerCountryOUT=new tUINT32withLock[NR_OF_COUNTRIES+1];
		//memset((void*)m_PacketsPerCountryOUT,0,sizeof(UINT32)*(NR_OF_COUNTRIES+1));
		m_threadLogLoop=new CAThread((UINT8*)"Country Logger Thread");
		m_threadLogLoop->setMainLoop(iplist_loopDoLogCountries);
		m_bRunLogCountries=true;
		m_threadLogLoop->start(this,true);
		return E_SUCCESS;
	}

SINT32 CAFirstMix::deleteCountryStats()
	{
		m_bRunLogCountries=false;
		if(m_threadLogLoop!=NULL)
			{
				m_threadLogLoop->join();
				delete m_threadLogLoop;
				m_threadLogLoop=NULL;
			}
		if(m_mysqlCon!=NULL)
			{
				mysql_thread_end();
				mysql_close(m_mysqlCon);
				m_mysqlCon=NULL;
			}

		delete[] m_CountryStats;
		m_CountryStats=NULL;

		delete[] m_PacketsPerCountryIN;
		m_PacketsPerCountryIN=NULL;

		delete[] m_PacketsPerCountryOUT;
		m_PacketsPerCountryOUT=NULL;
		return E_SUCCESS;
	}

/** Update the statisitics of the countries users come from. The dependency between the argumenst is as follow:
	* @param bRemove if true the number of users of a given country is decreased, if false it is increased
	* @param a_countryID the country the user comes from. Must be set if bRemove==true. If bRemove==false and ip==NULL, than
	*        if also must be set to the country the user comes from. In case ip!=NULL if holdes the default country id, if no country for the ip could be found
	* @param ip the ip the user comes from. this ip is looked up in the databse to find the corresponding country. it is only used if bRemove==false. If no country for
	*         that ip could be found a_countryID is used as default value
  * @return the countryID which was asigned  to the user. This may be the default value a_countryID, if no country could be found.
**/
SINT32 CAFirstMix::updateCountryStats(const UINT8 ip[4],UINT32 a_countryID,bool bRemove)
	{
		if(!bRemove)
			{
				UINT32 countryID=a_countryID;
				if(ip!=NULL)
					{
						UINT32 u32ip=ip[0]<<24|ip[1]<<16|ip[2]<<8|ip[3];
						char query[1024];
						sprintf(query,"SELECT id FROM ip2c WHERE ip_lo<=\"%u\" and ip_hi>=\"%u\" LIMIT 1",u32ip,u32ip);
						int ret=mysql_query(m_mysqlCon,query);
						if(ret!=0)
							{
								CAMsg::printMsg(LOG_INFO,"CountryStatsDB - updateCountryStats - error (%i) in finding countryid for ip %u\n",ret,u32ip);
								goto RET;
							}
						MYSQL_RES* result=mysql_store_result(m_mysqlCon);
						if(result==NULL)
							{
								CAMsg::printMsg(LOG_INFO,"CountryStatsDB - updateCountryStats - error in retriving results of the query\n");
								goto RET;
							}
						MYSQL_ROW row=mysql_fetch_row(result);
						SINT32 t;
						if(row!=NULL&&(t=atoi(row[0]))>0&&t<=NR_OF_COUNTRIES)
							{
								countryID=t;
								#ifdef DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Country ID for ip %u is %u\n",u32ip,countryID);
								#endif
								//temp hack
								if(countryID==99)
								{
									CAMsg::printMsg(LOG_DEBUG,"Country ID for ip %u.%u.%u.%u is %u\n",ip[0],ip[1],ip[2],ip[3],countryID);
								}
							}
						else
							{
								CAMsg::printMsg(LOG_DEBUG,"DO country stats query result no result for ip %u)\n",u32ip);
							}
						mysql_free_result(result);
					}
RET:
				m_CountryStats[countryID]++;
				return countryID;
			}
		else//bRemove
			{
				m_CountryStats[a_countryID]--;
			}
		return a_countryID;
	}

THREAD_RETURN iplist_loopDoLogCountries(void* param)
	{
		mysql_thread_init();
		CAMsg::printMsg(LOG_DEBUG,"Starting iplist_loopDoLogCountries\n");
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		UINT32 s=0;
		UINT8 buff[255];
		memset(buff,0,255);
		CALibProxytest::getOptions()->getCascadeName(buff,255);
		mysqlEscapeTableName(buff);
		while(pFirstMix->m_bRunLogCountries)
			{
				if(s==LOG_COUNTRIES_INTERVALL)
					{
						UINT8 aktDate[255];
						time_t aktTime=time(NULL);
						strftime((char*)aktDate,255,"%Y%m%d%H%M%S",gmtime(&aktTime));
						char query[1024];
						sprintf(query,"INSERT into `stats_%s` (date,id,count,packets_in,packets_out) VALUES (\"%s\",\"%%u\",\"%%u\",\"%%u\",\"%%u\")",buff,aktDate);
						pFirstMix->m_pmutexUser->lock();
						for(UINT32 i=0;i<NR_OF_COUNTRIES+1;i++)
							{
								if(pFirstMix->m_CountryStats[i]>0)
									{
										char aktQuery[1024];
										sprintf(aktQuery,query,i,pFirstMix->m_CountryStats[i],pFirstMix->m_PacketsPerCountryIN[i].getAndzero(),pFirstMix->m_PacketsPerCountryOUT[i].getAndzero());
										SINT32 ret=mysql_query(pFirstMix->m_mysqlCon,aktQuery);
										if(ret!=0)
										{
											CAMsg::printMsg(LOG_INFO,"CountryStats DB - failed to update CountryStats DB with new values - error %i\n",ret);
										}
									}
							}
						pFirstMix->m_pmutexUser->unlock();
						s=0;
					}
				sSleep(10);
				s++;
			}
		CAMsg::printMsg(LOG_DEBUG,"Exiting iplist_loopDoLogCountries\n");
		mysql_thread_end();
		THREAD_RETURN_SUCCESS;
	}
#endif
#endif //ONLY_LOCAL_PROXY
