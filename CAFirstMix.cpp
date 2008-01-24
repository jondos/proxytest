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
#include "CAASymCipher.hpp"
#include "CAInfoService.hpp"
#include "CASocketAddrINet.hpp"
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
#endif
extern CACmdLnOptions* pglobalOptions;
#include "CAReplayControlChannel.hpp"

const UINT32 CAFirstMix::MAX_CONCURRENT_NEW_CONNECTIONS = NUM_LOGIN_WORKER_TRHEADS * 2;

bool CAFirstMix::isShuttingDown()
{
	return m_bIsShuttingDown;
}

SINT32 CAFirstMix::initOnce()
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting FirstMix InitOnce\n");
		m_pSignature=pglobalOptions->getSignKey();
		if(m_pSignature==NULL)
			return E_UNKNOWN;
		//Try to find out how many (real) ListenerInterfaces are specified
		UINT32 tmpSocketsIn=pglobalOptions->getListenerInterfaceCount();
		m_nSocketsIn=0;
		for(UINT32 i=1;i<=tmpSocketsIn;i++)
			{
				CAListenerInterface* pListener=NULL;
				pListener=pglobalOptions->getListenerInterface(i);
				if(pListener==NULL)
					continue;
				if(!pListener->isVirtual())
					m_nSocketsIn++;
				delete pListener;
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
		UINT8 buff[255];
		m_nMixedPackets=0; //reset to zero after each restart (at the moment neccessary for infoservice)
		m_bRestart=false;
		//Establishing all Listeners
		m_arrSocketsIn=new CASocket[m_nSocketsIn];
		UINT32 i,aktSocket=0;
		for(i=1;i<=pglobalOptions->getListenerInterfaceCount();i++)
			{
				CAListenerInterface* pListener=NULL;
				pListener=pglobalOptions->getListenerInterface(i);
				if(pListener==NULL)
					{
            CAMsg::printMsg(LOG_CRIT,"Error: Listener interface invalid.\n");
						return E_UNKNOWN;
					}
				if(pListener->isVirtual())
					{
						delete pListener;
						continue;
					}
				m_arrSocketsIn[aktSocket].create();
				m_arrSocketsIn[aktSocket].setReuseAddr(true);
				CASocketAddr* pAddr=pListener->getAddr();
				pAddr->toString(buff,255);
				CAMsg::printMsg(LOG_DEBUG,"Listening on Interface: %s\n",buff);
				delete pListener;
#ifndef _WIN32
				//we have to be a temporaly superuser if port <1024...
				int old_uid=geteuid();
				if(pAddr->getType()==AF_INET&&((CASocketAddrINet*)pAddr)->getPort()<1024)
					{
						if(seteuid(0)==-1) //changing to root
							CAMsg::printMsg(LOG_CRIT,"Setuid failed! You must start the mix as root in order to use listener ports lower than 1024!\n");
					}
#endif
				SINT32 ret=m_arrSocketsIn[aktSocket].listen(*pAddr);
				delete pAddr;
#ifndef _WIN32
				seteuid(old_uid);
#endif
				if(ret!=E_SUCCESS)
					{
            CAMsg::printMsg(LOG_CRIT,"Socket error while listening on interface %d\n",i);
						return E_UNKNOWN;
					}
				aktSocket++;
			}
		CAMsg::printMsg(LOG_DEBUG,"FirstMix Init - listening on all interfaces\n");


		CASocketAddr* pAddrNext=NULL;
		for(i=0;i<pglobalOptions->getTargetInterfaceCount();i++)
			{
				TargetInterface oNextMix;
				pglobalOptions->getTargetInterface(oNextMix,i+1);
				if(oNextMix.target_type==TARGET_MIX)
					{
						pAddrNext=oNextMix.addr;
						break;
					}
				delete oNextMix.addr;
			}
		if(pAddrNext==NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"No next Mix specified!\n");
				return E_UNKNOWN;
			}
		m_pMuxOut=new CAMuxSocket();
		if(((CASocket*)(*m_pMuxOut))->create(pAddrNext->getType())!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot create SOCKET for connection to next Mix!\n");
				return E_UNKNOWN;
			}
		((CASocket*)(*m_pMuxOut))->setSendBuff(500*MIXPACKET_SIZE);
		((CASocket*)(*m_pMuxOut))->setRecvBuff(500*MIXPACKET_SIZE);
		//if(((CASocket*)(*m_pMuxOut))->setSendLowWat(MIXPACKET_SIZE)!=E_SUCCESS)
		//	CAMsg::printMsg(LOG_INFO,"SOCKET Option SENDLOWWAT not set!\n");
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET RecvBuffSize: %i\n",((CASocket*)(*m_pMuxOut))->getRecvBuff());
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET SendBuffSize: %i\n",((CASocket*)(*m_pMuxOut))->getSendBuff());
		//CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET SendLowWatSize: %i\n",((CASocket*)(*m_pMuxOut))->getSendLowWat());

		/** Connect to the next mix */
		if(connectToNextMix(pAddrNext) != E_SUCCESS)
			{
				delete pAddrNext;
			CAMsg::printMsg(LOG_DEBUG, "CAFirstMix::init - Unable to connect to next mix\n");
				return E_UNKNOWN;
			}
		delete pAddrNext;

		CAMsg::printMsg(LOG_INFO," connected!\n");
		if(((CASocket*)(*m_pMuxOut))->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)(*m_pMuxOut))->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}

    if(processKeyExchange()!=E_SUCCESS)
    {
        CAMsg::printMsg(LOG_CRIT,"Error in establishing secure communication with next Mix!\n");
        return E_UNKNOWN;
    }
		m_pIPList=new CAIPList();
#ifdef COUNTRY_STATS
		initCountryStats();
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

		m_pMuxOutControlChannelDispatcher=new CAControlChannelDispatcher(m_pQueueSendToMix);
#ifdef REPLAY_DETECTION
		m_pReplayMsgProc=new CAReplayCtrlChannelMsgProc(this);
#endif

#ifdef PAYMENT
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

		//Starting thread for logging
#ifdef LOG_PACKET_TIMES
		m_pLogPacketStats=new CALogPacketStats();
		m_pLogPacketStats->setLogIntervallInMinutes(FM_PACKET_STATS_LOG_INTERVALL);
		m_pLogPacketStats->start();
#endif
#ifdef REPLAY_DETECTION
		sendReplayTimestampRequestsToAllMixes();
#endif
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix init() succeded\n");
		return E_SUCCESS;
}

SINT32 CAFirstMix::connectToNextMix(CASocketAddr* a_pAddrNext)
{
	UINT8 buff[255];
	a_pAddrNext->toString(buff,255);
	CAMsg::printMsg(LOG_INFO,"Try to connect to next Mix on %s ...\n",buff);
	SINT32 err = E_UNKNOWN;
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
#ifdef _DEBUG
		 	CAMsg::printMsg(LOG_DEBUG,"Con-Error: %i\n",err);
#endif
			if(err!=ERR_INTERN_TIMEDOUT&&err!=ERR_INTERN_CONNREFUSED)
				break;
#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"Cannot connect... retrying\n");
#endif				
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
    UINT16 len;
	CAMsg::printMsg(LOG_CRIT,"Try to read the Key Info length from next Mix...\n");
    if(m_pMuxOut->receiveFully((UINT8*)&len,2)!=E_SUCCESS)
    {
        CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info length!\n");
        return E_UNKNOWN;
    }
    len=ntohs(len);
    CAMsg::printMsg(LOG_CRIT,"Received Key Info length %u\n",len);
    recvBuff=new UINT8[len+1];

    if(m_pMuxOut->receiveFully(recvBuff,len)!=E_SUCCESS)
    {
        CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info!\n");
        delete []recvBuff;
        return E_UNKNOWN;
    }
    recvBuff[len]=0;
    //get the Keys from the other mixes (and the Mix-Id's...!)
    CAMsg::printMsg(LOG_INFO,"Received Key Info...\n");
    CAMsg::printMsg(LOG_DEBUG,"%s\n",recvBuff);

    MemBufInputSource oInput(recvBuff,len,"tmp");
    DOMParser oParser;
    oParser.parse(oInput);
    delete []recvBuff;

		DOM_Document doc=oParser.getDocument();
    DOM_Element elemMixes=doc.getDocumentElement();
    if(elemMixes==NULL)
		return E_UNKNOWN;
		SINT32 count=0;
    if(getDOMElementAttribute(elemMixes,"count",&count)!=E_SUCCESS)
        return E_UNKNOWN;
 /*
		@todo Do not know why we do this here - probably it has something todo with the
		dynamic mix config, but makes not sense at all for me...

		getDOMElementAttribute(elemMixescascadeNaem
		char *cascadeName;
    cascadeName = elemMixes.getAttribute("cascadeName").transcode();
    if(cascadeName == NULL)
        return E_UNKNOWN;
    pglobalOptions->setCascadeName(cascadeName);
*/
    m_pRSA=new CAASymCipher;
    m_pRSA->generateKeyPair(1024);

    DOM_Node child=elemMixes.getLastChild();

    //tmp XML-Structure for constructing the XML which is sent to each user
    DOM_Document docXmlKeyInfo=DOM_Document::createDocument();
    DOM_Element elemRootKey=docXmlKeyInfo.createElement("MixCascade");
    setDOMElementAttribute(elemRootKey,"version",(UINT8*)"0.2"); //set the Version of the XML to 0.2
    docXmlKeyInfo.appendChild(elemRootKey);
    DOM_Element elemMixProtocolVersion=docXmlKeyInfo.createElement("MixProtocolVersion");
    setDOMElementValue(elemMixProtocolVersion,(UINT8*)MIX_CASCADE_PROTOCOL_VERSION);
    elemRootKey.appendChild(elemMixProtocolVersion);
    DOM_Node elemMixesKey=docXmlKeyInfo.importNode(elemMixes,true);
    elemRootKey.appendChild(elemMixesKey);

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
    DOM_DocumentFragment docfragKey;
    m_pRSA->getPublicKeyAsDocumentFragment(docfragKey);
    addMixInfo(elemMixesKey, true);
    DOM_Element elemOwnMix;
    	getDOMChildByName(elemMixesKey, (UINT8*)"Mix", elemOwnMix, false);
		elemOwnMix.appendChild(docXmlKeyInfo.importNode(docfragKey,true));
	if (signXML(elemOwnMix) != E_SUCCESS)
	{
		CAMsg::printMsg(LOG_DEBUG,"Could not sign MixInfo sent to users...\n");
	}
	
    setDOMElementAttribute((DOM_Element&)elemMixesKey,"count",count+1);
    
    
	  DOM_Node elemPayment=docXmlKeyInfo.createElement("Payment");
		elemRootKey.appendChild(elemPayment);
		#ifdef PAYMENT
			setDOMElementAttribute(elemPayment,"required",(UINT8*)"true");
			setDOMElementAttribute(elemPayment,"version",(UINT8*)PAYMENT_VERSION);
			
			UINT32 prepaidInterval;
			pglobalOptions->getPrepaidInterval(&prepaidInterval);
			setDOMElementAttribute(elemPayment,"prepaidInterval", prepaidInterval);
			setDOMElementAttribute(elemPayment,"piid", pglobalOptions->getBI()->getID());
			
		#else
			setDOMElementAttribute(elemPayment,"required",(UINT8*)"false");
		#endif

	// create signature
	if (signXML(elemRootKey) != E_SUCCESS)
    {
		CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo sent to users...\n");
		}

    
    UINT32 tlen=0;
    UINT8* tmpB=DOM_Output::dumpToMem(docXmlKeyInfo,&tlen);
    m_xmlKeyInfoBuff=new UINT8[tlen+2];
    memcpy(m_xmlKeyInfoBuff+2,tmpB,tlen);
    UINT16 s=htons((UINT16)tlen);
    memcpy(	m_xmlKeyInfoBuff,&s,2);
    m_xmlKeyInfoSize=(UINT16)tlen+2;
    delete []tmpB;

    //Sending symetric key...
    child=elemMixes.getFirstChild();
    while(child!=NULL)
    {
        if(child.getNodeName().equals("Mix"))
        {
            //check Signature....
            CAMsg::printMsg(LOG_DEBUG,"Try to verify next mix signature...\n");
            CASignature oSig;
            CACertificate* nextCert=pglobalOptions->getNextMixTestCertificate();
            oSig.setVerifyKey(nextCert);
            SINT32 ret=oSig.verifyXML(child,NULL);
            delete nextCert;
            if(ret!=E_SUCCESS)
            {
                CAMsg::printMsg(LOG_DEBUG,"failed!\n");
                return E_UNKNOWN;
            }
            CAMsg::printMsg(LOG_DEBUG,"success!\n");
            DOM_Node rsaKey=child.getFirstChild();
            CAASymCipher oRSA;
            oRSA.setPublicKeyAsDOMNode(rsaKey);
            DOM_Element elemNonce;
            getDOMChildByName(child,(UINT8*)"Nonce",elemNonce,false);
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
            DOM_DocumentFragment docfragSymKey;
            encodeXMLEncryptedKey(key,64,docfragSymKey,&oRSA);
            DOM_Document docSymKey=DOM_Document::createDocument();
            docSymKey.appendChild(docSymKey.importNode(docfragSymKey,true));
            DOM_Element elemRoot=docSymKey.getDocumentElement();
            if(elemNonce!=NULL)
            {
                DOM_Element elemNonceHash=docSymKey.createElement("Nonce");
                setDOMElementValue(elemNonceHash,arNonce);
                elemRoot.appendChild(elemNonceHash);
            }
            UINT32 outlen=5000;
            UINT8* out=new UINT8[outlen];
						///Getting the KeepAlive Traffice...
						DOM_Element elemKeepAlive;
						DOM_Element elemKeepAliveSendInterval;
						DOM_Element elemKeepAliveRecvInterval;
						getDOMChildByName(child,(UINT8*)"KeepAlive",elemKeepAlive,false);
						getDOMChildByName(elemKeepAlive,(UINT8*)"SendInterval",elemKeepAliveSendInterval,false);
						getDOMChildByName(elemKeepAlive,(UINT8*)"ReceiveInterval",elemKeepAliveRecvInterval,false);
						UINT32 tmpSendInterval,tmpRecvInterval;
						getDOMElementValue(elemKeepAliveSendInterval,tmpSendInterval,0xFFFFFFFF); //if now send interval was given set it to "infinite"
						getDOMElementValue(elemKeepAliveRecvInterval,tmpRecvInterval,0xFFFFFFFF); //if no recv interval was given --> set it to "infinite"
						CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Getting offer -- SendInterval %u -- ReceiveInterval %u\n",tmpSendInterval,tmpRecvInterval);
						// Add Info about KeepAlive traffic
						UINT32 u32KeepAliveSendInterval=pglobalOptions->getKeepAliveSendInterval();
						UINT32 u32KeepAliveRecvInterval=pglobalOptions->getKeepAliveRecvInterval();
						elemKeepAlive=docSymKey.createElement("KeepAlive");
						elemKeepAliveSendInterval=docSymKey.createElement("SendInterval");
						elemKeepAliveRecvInterval=docSymKey.createElement("ReceiveInterval");
						elemKeepAlive.appendChild(elemKeepAliveSendInterval);
						elemKeepAlive.appendChild(elemKeepAliveRecvInterval);
						setDOMElementValue(elemKeepAliveSendInterval,u32KeepAliveSendInterval);
						setDOMElementValue(elemKeepAliveRecvInterval,u32KeepAliveRecvInterval);
						elemRoot.appendChild(elemKeepAlive);
						CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Offering -- SendInterval %u -- Receive Interval %u\n",u32KeepAliveSendInterval,u32KeepAliveRecvInterval);		
						m_u32KeepAliveSendInterval=max(u32KeepAliveSendInterval,tmpRecvInterval);
						if(m_u32KeepAliveSendInterval>10000)
							m_u32KeepAliveSendInterval-=10000; //make the send interval a little bit smaller than the related receive intervall
						m_u32KeepAliveRecvInterval=max(u32KeepAliveRecvInterval,tmpSendInterval);
						CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Calculated -- SendInterval %u -- Receive Interval %u\n",m_u32KeepAliveSendInterval,m_u32KeepAliveRecvInterval);		

            m_pSignature->signXML(elemRoot);
            DOM_Output::dumpToMem(docSymKey,out,&outlen);
						m_pMuxOut->setSendKey(key,32);
            m_pMuxOut->setReceiveKey(key+32,32);
            UINT16 size=htons((UINT16)outlen);
						CAMsg::printMsg(LOG_DEBUG,"Sending symmetric key to next Mix! Size: %i\n",outlen);
            ((CASocket*)m_pMuxOut)->send((UINT8*)&size,2);
            ((CASocket*)m_pMuxOut)->send(out,outlen);
            m_pMuxOut->setCrypt(true);
            delete[] out;
            break;
        }
        child=child.getNextSibling();
    }
		///initialises MixParameters struct
		if(initMixParameters(elemMixes)!=E_SUCCESS)
			return E_UNKNOWN;
    if(initMixCascadeInfo(elemMixes)!=E_SUCCESS)
    {
        CAMsg::printMsg(LOG_CRIT,"Error initializing cascade info.\n");
        return E_UNKNOWN;
    }
/*    else
    {
        if(m_pInfoService != NULL)
            m_pInfoService->sendCascadeHelo();
    }*/
		CAMsg::printMsg(LOG_DEBUG,"Keyexchange finished!\n");
    return E_SUCCESS;
}


SINT32 CAFirstMix::setMixParameters(const tMixParameters& params)
	{
		for(UINT32 i=0;i<m_u32MixCount-1;i++)
			{
				if(strcmp((char*)m_arMixParameters[i].m_strMixID,(char*)params.m_strMixID)==0)
					{
						m_arMixParameters[i].m_u32ReplayRefTime=params.m_u32ReplayRefTime;
						return E_SUCCESS;
					}
			}
		return E_SUCCESS;
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
#ifdef LOG_PACKET_TIMES
 				if(!isZero64(pQueueEntry->timestamp_proccessing_start))
					{
						getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
						pFirstMix->m_pLogPacketStats->addToTimeingStats(*pQueueEntry,pMixPacket->flags,true);
					}
#endif
			}
		delete pQueueEntry;
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
		delete pPool;
#endif
		FINISH_STACK("CAFirstMix::fm_loopSendToMix");

		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread SendToMix\n");
		THREAD_RETURN_SUCCESS;
	}

/* How to end this thread:
	0. set bRestart=true
*/
#define MAX_READ_FROM_NEXT_MIX_QUEUE_SIZE 10000000 //How many bytes could be in the incoming queue ??
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
								break;
							}
					}
					if(pMixPacket->channel>0&&pMixPacket->channel<256)
						{
							#ifdef DEBUG
								CAMsg::printMsg(LOG_DEBUG,"CAFirstMix - sent a packet from the next mix to the ControlChanelDispatcher... \n");
							#endif
							pControlChannelDispatcher->proccessMixPacket(pMixPacket);
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
				pQueue->add(pMixPacket,sizeof(tQueueEntry));
				getcurrentTimeMillis(keepaliveLast);
			}
		delete pQueueEntry;
		delete pSocketGroup;
		#ifdef USE_POOL
			delete pPool;
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

/*How to end this thread
1. Set m_bRestart in firstMix to true
2. close all accept sockets
*/
THREAD_RETURN fm_loopAcceptUsers(void* param)
	{
		INIT_STACK;
		BEGIN_STACK("CAFirstMix::fm_loopAcceptUsers");
		
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		CASocket* socketsIn=pFirstMix->m_arrSocketsIn;
		CAIPList* pIPList=pFirstMix->m_pIPList;
		CAThreadPool* pthreadsLogin=pFirstMix->m_pthreadsLogin;
		UINT32 nSocketsIn=pFirstMix->m_nSocketsIn;
		CASocketGroup* psocketgroupAccept=new CASocketGroup(false);
		CAMuxSocket* pNewMuxSocket;
		UINT8* peerIP=new UINT8[4];
		UINT32 i=0;
		SINT32 countRead;
		SINT32 ret;
		
		pFirstMix->m_newConnections = 0;
		
		for(i=0;i<nSocketsIn;i++)
		{
			psocketgroupAccept->add(socketsIn[i]);
		}
#ifdef REPLAY_DETECTION //before we can start to accept users we have to ensure that we received the replay timestamps form the over mixes
		CAMsg::printMsg(LOG_DEBUG,"Waiting for Replay Timestamp from next mixes\n");
		i=0;
		while(!pFirstMix->getRestart()&&i<pFirstMix->m_u32MixCount-1)
		{
			if(pFirstMix->m_arMixParameters[i].m_u32ReplayRefTime==0)//not set yet
				{
					msSleep(100);//wait a little bit and try again
					continue;
				}
			i++;
		}
		CAMsg::printMsg(LOG_DEBUG,"All Replay Timestamp received\n");
#endif
		while(!pFirstMix->getRestart())
			{
				countRead=psocketgroupAccept->select(10000);
				if(countRead<0)
					{ //check for Error - are we restarting ?
						if(pFirstMix->getRestart()||countRead!=E_TIMEDOUT)
							goto END_THREAD;
					}
				i=0;
#ifdef _DEBUG
				CAMsg::printMsg(LOG_DEBUG,"UserAcceptLoop: countRead=%i\n",countRead);
#endif
				while(countRead>0&&i<nSocketsIn)
				{
					if(psocketgroupAccept->isSignaled(socketsIn[i]))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New direct Connection from Client!\n");
						#endif
						pNewMuxSocket=new CAMuxSocket;
						ret=socketsIn[i].accept(*(CASocket*)pNewMuxSocket);
						pFirstMix->m_newConnections++;							 
						if(ret!=E_SUCCESS)
						{
							// may return E_SOCKETCLOSED or E_SOCKET_LIMIT
							CAMsg::printMsg(LOG_ERR,"Accept Error %u - direct Connection from Client!\n",GET_NET_ERROR);														
						}
						else if (pglobalOptions->getMaxNrOfUsers() > 0 && pFirstMix->getNrOfUsers() >= pglobalOptions->getMaxNrOfUsers())
						{
							CAMsg::printMsg(LOG_DEBUG,"CAFirstMix User control: Too many users (Maximum:%d)! Rejecting user...\n", pFirstMix->getNrOfUsers(), pglobalOptions->getMaxNrOfUsers());
							ret = E_UNKNOWN;
						}
						else if (pFirstMix->m_newConnections > CAFirstMix::MAX_CONCURRENT_NEW_CONNECTIONS)
						{
							/* This should protect the mix from flooding attacks
							 * No more than MAX_CONCURRENT_NEW_CONNECTIONS are allowed.
							 */
#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"CAFirstMix Flooding protection: Too many concurrent new connections (Maximum:%d)! Rejecting user...\n", CAFirstMix::MAX_CONCURRENT_NEW_CONNECTIONS);
#endif
							ret = E_UNKNOWN;
						}
#ifndef PAYMENT					
						else if ((ret = ((CASocket*)pNewMuxSocket)->getPeerIP(peerIP)) != E_SUCCESS ||
								pIPList->insertIP(peerIP)<0) 
						{
							if (ret != E_SUCCESS)
							{
								CAMsg::printMsg(LOG_DEBUG,"Could not insert IP address as IP could not be retrieved!\n");
							}
							else
							{
								ret = E_UNKNOWN;
								CAMsg::printMsg(LOG_DEBUG,"CAFirstMix Flooding protection: Could not insert IP address!\n");	
							}							
						}
#endif						
						else
						{
							t_UserLoginData* d=new t_UserLoginData;
							d->pNewUser=pNewMuxSocket;
							d->pMix=pFirstMix;
							memcpy(d->peerIP,peerIP,4);
							if(pthreadsLogin->addRequest(fm_loopDoUserLogin,d)!=E_SUCCESS)
							{
								CAMsg::printMsg(LOG_ERR,"Could not add an login request to the login thread pool!\n");
							}
						}
											
						if (ret != E_SUCCESS)
						{
							delete pNewMuxSocket;
							pFirstMix->m_newConnections--;
							if(ret==E_SOCKETCLOSED&&pFirstMix->getRestart()) //Hm, should we restart ??
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
		delete psocketgroupAccept;
		
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


THREAD_RETURN fm_loopDoUserLogin(void* param)
	{
		INIT_STACK;
		BEGIN_STACK("CAFirstMix::fm_loopDoUserLogin");
		

		t_UserLoginData* d=(t_UserLoginData*)param;
		d->pMix->doUserLogin(d->pNewUser,d->peerIP);
		
		SAVE_STACK("CAFirstMix::fm_loopDoUserLogin", "after user login");
		d->pMix->m_newConnections--;
		delete d;
		
		FINISH_STACK("CAFirstMix::fm_loopDoUserLogin");
		
		THREAD_RETURN_SUCCESS;
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
		
		#ifdef _DEBUG
			int ret=((CASocket*)pNewUser)->setKeepAlive(true);
			if(ret!=E_SUCCESS)
				CAMsg::printMsg(LOG_DEBUG,"Error setting KeepAlive!");
		#else
			((CASocket*)pNewUser)->setKeepAlive(true);
		#endif
		
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: start\n");
		#endif	
		
		SAVE_STACK("CAFirstMix::doUserLogin", "after setting keep alive");
		
		// send the mix-keys to JAP
		if (((CASocket*)pNewUser)->sendFullyTimeOut(m_xmlKeyInfoBuff,m_xmlKeyInfoSize, 40000, 10000) != E_SUCCESS)
		{
#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: Sending login data has been interrupted!\n");
#endif
			delete pNewUser;
			m_pIPList->removeIP(peerIP);
			return E_UNKNOWN;
		}
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: login data sent\n");
		#endif
		SAVE_STACK("CAFirstMix::doUserLogin", "after sinding login data");
		
		//((CASocket*)pNewUser)->send(m_xmlKeyInfoBuff,m_xmlKeyInfoSize);
		// es kann nicht blockieren unter der Annahme das der TCP-Sendbuffer > m_xmlKeyInfoSize ist....
		
		//wait for keys from user		
		UINT16 xml_len;
		if(((CASocket*)pNewUser)->isClosed() || 
		   ((CASocket*)pNewUser)->receiveFullyT((UINT8*)&xml_len,2,FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT)!=E_SUCCESS)
		{
			#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG,"User login: timed out while waiting for first symmetric key from client!\n");
			#endif
			delete pNewUser;
			m_pIPList->removeIP(peerIP);
			return E_UNKNOWN;
		}
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: received first symmetric key from client\n");
		#endif
		SAVE_STACK("CAFirstMix::doUserLogin", "received first symmetric key");
		
		xml_len=ntohs(xml_len);
		UINT8* xml_buff=new UINT8[xml_len+2]; //+2 for size...
		if(((CASocket*)pNewUser)->isClosed() ||
		   ((CASocket*)pNewUser)->receiveFullyT(xml_buff+2,xml_len,FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT)!=E_SUCCESS)
		{
#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: timed out while waiting for second symmetric key from client!\n");
#endif
			delete pNewUser;
			delete[] xml_buff;
			m_pIPList->removeIP(peerIP);
			return E_UNKNOWN;
		}
		
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: received second symmetric key from client\n");
		#endif
		SAVE_STACK("CAFirstMix::doUserLogin", "received second symmetric key");
		
		DOMParser oParser;
		MemBufInputSource oInput(xml_buff+2,xml_len,"tmp");
		oParser.parse(oInput);
		DOM_Document doc=oParser.getDocument();
		DOM_Element elemRoot;
		if(doc==NULL||(elemRoot=doc.getDocumentElement())==NULL||
			decryptXMLElement(elemRoot,m_pRSA)!=E_SUCCESS)
			{
				delete[] xml_buff;
				delete pNewUser;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		elemRoot=doc.getDocumentElement();
		if(!elemRoot.getNodeName().equals("JAPKeyExchange"))
			{
				delete[] xml_buff;
				delete pNewUser;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		DOM_Element elemLinkEnc,elemMixEnc;
		getDOMChildByName(elemRoot,(UINT8*)"LinkEncryption",elemLinkEnc,false);
		getDOMChildByName(elemRoot,(UINT8*)"MixEncryption",elemMixEnc,false);
		UINT8 linkKey[255],mixKey[255];
		UINT32 linkKeyLen=255,mixKeyLen=255;
		if(	getDOMElementValue(elemLinkEnc,linkKey,&linkKeyLen)!=E_SUCCESS||
				getDOMElementValue(elemMixEnc,mixKey,&mixKeyLen)!=E_SUCCESS||
				CABase64::decode(linkKey,linkKeyLen,linkKey,&linkKeyLen)!=E_SUCCESS||
				CABase64::decode(mixKey,mixKeyLen,mixKey,&mixKeyLen)!=E_SUCCESS||
				linkKeyLen!=64||mixKeyLen!=32)
			{
				delete[] xml_buff;
				delete pNewUser;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		//Sending Signature....
		xml_buff[0]=(UINT8)(xml_len>>8);
		xml_buff[1]=(UINT8)(xml_len&0xFF);
		UINT8 sig[255];
		UINT32 siglen=255;
		m_pSignature->sign(xml_buff,xml_len+2,sig,&siglen);
		DOM_Document docSig=DOM_Document::createDocument();
		elemRoot=docSig.createElement("Signature");
		DOM_Element elemSigValue=docSig.createElement("SignatureValue");
		docSig.appendChild(elemRoot);
		elemRoot.appendChild(elemSigValue);
		UINT32 u32=siglen;
		CABase64::encode(sig,u32,sig,&siglen);
		sig[siglen]=0;
		setDOMElementValue(elemSigValue,sig);
		u32=xml_len;
		DOM_Output::dumpToMem(docSig,xml_buff+2,&u32);
		xml_buff[0]=(UINT8)(u32>>8);
		xml_buff[1]=(UINT8)(u32&0xFF);
		
		if (((CASocket*)pNewUser)->isClosed() ||
		    ((CASocket*)pNewUser)->sendFullyTimeOut(xml_buff,u32+2, 30000, 10000) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG,"User login: Sending key exchange signature has been interrupted!\n");
			delete[] xml_buff;
			delete pNewUser;
			m_pIPList->removeIP(peerIP);
			return E_UNKNOWN;
		}
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"User login: key exchange signature sent\n");
		#endif
		delete[] xml_buff;
		
		SAVE_STACK("CAFirstMix::doUserLogin", "sent key exchange signature");
		
		((CASocket*)pNewUser)->setNonBlocking(true);
		
		SAVE_STACK("CAFirstMix::doUserLogin", "Creating CAQueue...");
		CAQueue* tmpQueue=new CAQueue(sizeof(tQueueEntry));
		
		SAVE_STACK("CAFirstMix::doUserLogin", "Adding user to connection list...");
		fmHashTableEntry* pHashEntry=m_pChannelList->add(pNewUser,peerIP,tmpQueue);
		if(pHashEntry==NULL)// adding user connection to mix->JAP channel list (stefan: sollte das nicht connection list sein? --> es handelt sich um eine Datenstruktu fr Connections/Channels ).
		{
			CAMsg::printMsg(LOG_ERR,"User login: Could not add new socket to connection list!\n");
			m_pIPList->removeIP(peerIP);
			delete tmpQueue;
			delete pNewUser;
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
		m_psocketgroupUsersRead->add(*pNewUser); // add user socket to the established ones that we read data from.
		m_psocketgroupUsersWrite->add(*pNewUser);
#endif
		CAMsg::printMsg(LOG_DEBUG,"User login: finished\n");

		return E_SUCCESS;
	}
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
						if(pFirstMix->getRestart()||countRead!=E_TIMEDOUT)
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
										((CASocket*)pMuxSocket)->getPeerIP(ip);
										pIPList->removeIP(ip);
										psocketgroupUsersRead->remove(*(CASocket*)pMuxSocket);
										psocketgroupUsersWrite->remove(*(CASocket*)pMuxSocket);
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
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::clean() start\n");
		#endif
		m_bRunLog=false;
		m_bRestart=true;
		if(m_pMuxOut!=NULL)
			{
				m_pMuxOut->close();
			}
		if(m_arrSocketsIn!=NULL)
			{
				for(UINT32 i=0;i<m_nSocketsIn;i++)
					m_arrSocketsIn[i].close();
			}
		//writng some bytes to the queue...
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

#ifdef PAYMENT
	CAAccountingInstance::clean();
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
			delete[] m_arrSocketsIn;
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
						delete pHashEntry->pSymCipher; 

						fmChannelListEntry* pEntry=m_pChannelList->getFirstChannelForSocket(pHashEntry->pMuxSocket);
						while(pEntry!=NULL)
							{
								delete pEntry->pCipher;
			
								pEntry=m_pChannelList->getNextChannel(pEntry);
							}
						m_pChannelList->remove(pHashEntry->pMuxSocket);
						pMuxSocket->close();
						delete pMuxSocket;
						pHashEntry=m_pChannelList->getNext();
					}
			}
		if(m_pChannelList!=NULL)
			delete m_pChannelList;
		m_pChannelList=NULL;
		CAMsg::printMsg	(LOG_CRIT,"Memory usage after: %u\n",getMemoryUsage());	

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
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::clean() finished\n");
		#endif
		return E_SUCCESS;
	}

#ifdef DELAY_USERS
SINT32 CAFirstMix::reconfigure()
	{
		CAMsg::printMsg(LOG_DEBUG,"Reconfiguring First Mix\n");
#ifdef DELAY_USERS
		CAMsg::printMsg(LOG_DEBUG,"Set new ressources limitation parameters\n");
		if(m_pChannelList!=NULL)
			m_pChannelList->setDelayParameters(	pglobalOptions->getDelayChannelUnlimitTraffic(),
																					pglobalOptions->getDelayChannelBucketGrow(),
																					pglobalOptions->getDelayChannelBucketGrowIntervall());	
#endif		
		return E_SUCCESS;
	}
#endif

/** set u32MixCount and m_arMixParameters from the <Mixes> element received from the second mix.*/
SINT32 CAFirstMix::initMixParameters(DOM_Element& elemMixes)
	{
		DOM_NodeList nl=elemMixes.getElementsByTagName("Mix");
		m_u32MixCount=nl.getLength();
		m_arMixParameters=new tMixParameters[m_u32MixCount];
		memset(m_arMixParameters,0,sizeof(tMixParameters)*m_u32MixCount);
		UINT8 buff[255];
		pglobalOptions->getMixId(buff,255);
		UINT32 len=strlen((char*)buff)+1;
		UINT32 aktMix=0;
		for(UINT32 i=0;i<nl.getLength();i++)
			{
				DOM_Node child=nl.item(i);
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
#define NR_OF_COUNTRIES 250

SINT32 CAFirstMix::initCountryStats()
	{
		m_CountryStats=NULL;
		m_mysqlCon=mysql_init(NULL);
		MYSQL* tmp=NULL;
		tmp=mysql_real_connect(m_mysqlCon,NULL,"root",NULL,COUNTRY_STATS_DB,0,NULL,0);
		if(tmp==NULL)
			{
				CAMsg::printMsg(LOG_DEBUG,"Could not connet to CountryStats DB!\n");
				my_thread_end();
				mysql_close(m_mysqlCon);
				m_mysqlCon=NULL;
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_DEBUG,"Connected to CountryStats DB!\n");
		char query[1024];
		UINT8 buff[255];
		pglobalOptions->getCascadeName(buff,255);
		sprintf(query,"CREATE TABLE IF NOT EXISTS `stats_%s` (date timestamp,id int,count int,packets_in int,packets_out int)",buff);
		SINT32 ret=mysql_query(m_mysqlCon,query);
		if(ret!=0)
		{
				CAMsg::printMsg(LOG_INFO,"CountryStats DB - create table for %s failed!\n",buff);
		}
		m_CountryStats=new UINT32[NR_OF_COUNTRIES+1];
		memset((void*)m_CountryStats,0,sizeof(UINT32)*(NR_OF_COUNTRIES+1));
		m_PacketsPerCountryIN=new UINT32[NR_OF_COUNTRIES+1];
		memset((void*)m_PacketsPerCountryIN,0,sizeof(UINT32)*(NR_OF_COUNTRIES+1));
		m_PacketsPerCountryOUT=new UINT32[NR_OF_COUNTRIES+1];
		memset((void*)m_PacketsPerCountryOUT,0,sizeof(UINT32)*(NR_OF_COUNTRIES+1));
		m_threadLogLoop=new CAThread();
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
				my_thread_end();
				mysql_close(m_mysqlCon);
				m_mysqlCon=NULL;
			}
		if(m_CountryStats!=NULL)
			delete[] m_CountryStats;
		m_CountryStats=NULL;
		if(m_PacketsPerCountryIN!=NULL)
			delete[] m_PacketsPerCountryIN;
		m_PacketsPerCountryIN=NULL;
		if(m_PacketsPerCountryOUT!=NULL)
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
		CAMsg::printMsg(LOG_DEBUG,"Starting iplist_loopDoLogCountries\n");														
		CAFirstMix* pIPList=(CAFirstMix*)param;
		UINT32 s=0;
		UINT8 buff[255];
		pglobalOptions->getCascadeName(buff,255);
		mysql_thread_init();
		while(pIPList->m_bRunLogCountries)
			{
				if(s==LOG_COUNTRIES_INTERVALL)
					{
						UINT8 aktDate[255];
						time_t aktTime=time(NULL);
						strftime((char*)aktDate,255,"%Y%m%d%H%M%S",gmtime(&aktTime));
						char query[1024];
						sprintf(query,"INSERT into `stats_%s` (date,id,count,packets_in,packets_out) VALUES (\"%s\",\"%%u\",\"%%u\",\"%%u\",\"%%u\")",buff,aktDate);
						pIPList->m_pmutexUser->lock();
						for(UINT32 i=0;i<NR_OF_COUNTRIES+1;i++)
							{
								if(pIPList->m_CountryStats[i]>0)
									{
										char aktQuery[1024];
										sprintf(aktQuery,query,i,pIPList->m_CountryStats[i],pIPList->m_PacketsPerCountryIN[i],pIPList->m_PacketsPerCountryOUT[i]);
										pIPList->m_PacketsPerCountryIN[i]=pIPList->m_PacketsPerCountryOUT[i]=0;
										SINT32 ret=mysql_query(pIPList->m_mysqlCon,aktQuery);
										if(ret!=0)
										{
											CAMsg::printMsg(LOG_INFO,"CountryStats DB - failed to update CountryStats DB with new values - error %i\n",ret);
										}
									}
							}
						pIPList->m_pmutexUser->unlock();
						s=0;
					}
				sSleep(10);
				s++;
			}
		mysql_thread_end();
		CAMsg::printMsg(LOG_DEBUG,"Exiting iplist_loopDoLogCountries\n");														
		THREAD_RETURN_SUCCESS;	
	}
#endif
#endif //ONLY_LOCAL_PROXY
