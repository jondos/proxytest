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
#include "CAFirstMix.hpp"
#include "CASocketGroup.hpp"
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CAFirstMixChannelList.hpp"
#include "CAASymCipher.hpp"
#include "CAInfoService.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocketAddrUnix.hpp"
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
extern CACmdLnOptions options;


SINT32 CAFirstMix::initOnce()
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting FirstMix InitOnce\n");
		m_pSignature=options.getSignKey();
		if(m_pSignature==NULL)
			return E_UNKNOWN;
		//Try to find out how many (real) ListenerInterfaces are specified
		UINT32 tmpSocketsIn=options.getListenerInterfaceCount();
		m_nSocketsIn=0;
		for(UINT32 i=1;i<=tmpSocketsIn;i++)
			{
				CAListenerInterface* pListener=NULL;
				pListener=options.getListenerInterface(i);
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
		return E_SUCCESS;
	}

SINT32 CAFirstMix::init()
	{
		m_nMixedPackets=0; //reset to zero after each restart (at the moment neccessary for infoservice)
		m_bRestart=false;
		//Establishing all Listeners
		m_arrSocketsIn=new CASocket[m_nSocketsIn];
		UINT32 i,aktSocket=0;
		for(i=1;i<=options.getListenerInterfaceCount();i++)
			{
				CAListenerInterface* pListener=NULL;
				pListener=options.getListenerInterface(i);
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
				delete pListener;
#ifndef _WIN32
				//we have to be a temporaly superuser if port <1024...
				int old_uid=geteuid();
				if(pAddr->getType()==AF_INET&&((CASocketAddrINet*)pAddr)->getPort()<1024)
					{
						if(seteuid(0)==-1) //changing to root
							CAMsg::printMsg(LOG_CRIT,"Setuid failed!\n");
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


		CASocketAddr* pAddrNext=NULL;
		for(i=0;i<options.getTargetInterfaceCount();i++)
			{
				TargetInterface oNextMix;
				options.getTargetInterface(oNextMix,i+1);
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
		if(((CASocket*)(*m_pMuxOut))->setSendLowWat(MIXPACKET_SIZE)!=E_SUCCESS)
			CAMsg::printMsg(LOG_INFO,"SOCKET Option SENDLOWWAT not set!\n");
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET RecvBuffSize: %i\n",((CASocket*)(*m_pMuxOut))->getRecvBuff());
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET SendBuffSize: %i\n",((CASocket*)(*m_pMuxOut))->getSendBuff());
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET SendLowWatSize: %i\n",((CASocket*)(*m_pMuxOut))->getSendLowWat());


		if(m_pMuxOut->connect(*pAddrNext,10,10)!=E_SUCCESS)
			{
				delete pAddrNext;
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix!\n");
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


    // moved to processKeyExchange()
    /*
		UINT16 len;
		if(((CASocket*)(*m_pMuxOut))->receiveFully((UINT8*)&len,2)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info lenght!\n");
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_CRIT,"Received Key Info lenght %u\n",ntohs(len));
		m_pRSA=new CAASymCipher;
		m_pRSA->generateKeyPair(1024);
		len=ntohs(len);
		UINT8* recvBuff=new UINT8[len+1];

		if(((CASocket*)(*m_pMuxOut))->receiveFully(recvBuff,len)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info!\n");
				delete []recvBuff;
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_CRIT,"Received Key Info...\n");
        recvBuff[len]=0; //get the Keys from the other mixes (and the Mix-Id's...!)
		if(initMixCascadeInfo(recvBuff,len+1)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error in establishing secure communication with next Mix!\n");
				delete []recvBuff;
				return E_UNKNOWN;
			}
    */

#ifdef PAYMENT
		m_pAccountingInstance = CAAccountingInstance::getInstance();
#endif

		m_pIPList=new CAIPList();
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

		m_pthreadsLogin=new CAThreadPool(NUM_LOGIN_WORKER_TRHEADS,MAX_LOGIN_QUEUE,false);

		//Starting thread for Step 1
		m_pthreadAcceptUsers=new CAThread();
		m_pthreadAcceptUsers->setMainLoop(fm_loopAcceptUsers);
		m_pthreadAcceptUsers->start(this);

		//Starting thread for Step 3
		m_pthreadSendToMix=new CAThread();
		m_pthreadSendToMix->setMainLoop(fm_loopSendToMix);
		m_pthreadSendToMix->start(this);

		//Startting thread for Step 4a
		m_pthreadReadFromMix=new CAThread();
		m_pthreadReadFromMix->setMainLoop(fm_loopReadFromMix);
		m_pthreadReadFromMix->start(this);

		//Starting InfoService
    /*    if(m_pInfoService == NULL)
        {
            m_pInfoService=new CAInfoService(this);
		CACertificate* tmp=options.getOwnCertificate();
		m_pInfoService->setSignature(m_pSignature,tmp);
		delete tmp;
        }
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix InfoService - Signature set\n");
		m_pInfoService->start();
		CAMsg::printMsg(LOG_DEBUG,"InfoService Loop started\n");
    */

		//Starting thread for logging
#ifdef LOG_PACKET_TIMES
		m_pLogPacketStats=new CALogPacketStats();
		m_pLogPacketStats->setLogIntervallInMinutes(FM_PACKET_STATS_LOG_INTERVALL);
		m_pLogPacketStats->start();
#endif
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix init() succeded\n");
		return E_SUCCESS;
}

/*
*@DOCME
*/
SINT32 CAFirstMix::processKeyExchange()
{
    UINT8* recvBuff=NULL;
    UINT16 len;


    if(((CASocket*)(*m_pMuxOut))->receiveFully((UINT8*)&len,2)!=E_SUCCESS)
    {
        CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info lenght!\n");
        return E_UNKNOWN;
    }
    len=ntohs(len);
    CAMsg::printMsg(LOG_CRIT,"Received Key Info lenght %u\n",len);
    recvBuff=new UINT8[len+1];

    if(((CASocket*)(*m_pMuxOut))->receiveFully(recvBuff,len)!=E_SUCCESS)
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
		int count=0;
    if(getDOMElementAttribute(elemMixes,"count",&count)!=E_SUCCESS)
        return E_UNKNOWN;
 /*
		@todo DOn not why we do this here - probablyy it has something todo with the
		dynamic mix config, but makes not sense at all for me...

		getDOMElementAttribute(elemMixescascadeNaem
		char *cascadeName;
    cascadeName = elemMixes.getAttribute("cascadeName").transcode();
    if(cascadeName == NULL)
        return E_UNKNOWN;
    options.setCascadeName(cascadeName);
*/
    m_pRSA=new CAASymCipher;
    m_pRSA->generateKeyPair(1024);

    DOM_Node child=elemMixes.getLastChild();

    //tmp XML-Structure for constructing the XML which is send to each user
    DOM_Document docXmlKeyInfo=DOM_Document::createDocument();
    DOM_Element elemRootKey=docXmlKeyInfo.createElement("MixCascade");
    setDOMElementAttribute(elemRootKey,"version",(UINT8*)"0.1"); //set the Version of the XML to 0.1
    docXmlKeyInfo.appendChild(elemRootKey);
    DOM_Element elemMixProtocolVersion=docXmlKeyInfo.createElement("MixProtocolVersion");
    setDOMElementValue(elemMixProtocolVersion,(UINT8*)MIX_CASCADE_PROTOCOL_VERSION);
    elemRootKey.appendChild(elemMixProtocolVersion);
    DOM_Node elemMixesKey=docXmlKeyInfo.importNode(elemMixes,true);
    elemRootKey.appendChild(elemMixesKey);

    UINT32 tlen;
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
    }
    tlen=256;

    //Inserting own Key in XML-Key struct
    DOM_DocumentFragment docfragKey;
    m_pRSA->getPublicKeyAsDocumentFragment(docfragKey);
    DOM_Element elemOwnMix=docXmlKeyInfo.createElement("Mix");
    UINT8 buffId[255];
    options.getMixId(buffId,255);
    elemOwnMix.setAttribute("id",DOMString((char*)buffId));
    elemOwnMix.appendChild(docXmlKeyInfo.importNode(docfragKey,true));
    elemMixesKey.insertBefore(elemOwnMix,elemMixesKey.getFirstChild());
    setDOMElementAttribute((DOM_Element&)elemMixesKey,"count",count+1);
    CACertificate* ownCert=options.getOwnCertificate();
    if(ownCert==NULL)
    {
        CAMsg::printMsg(LOG_DEBUG,"Own Test Cert is NULL -- so it could not be inserted into signed KeyInfo send to users...\n");
	}
    CACertStore* tmpCertStore=new CACertStore();
    tmpCertStore->add
    (ownCert);
    if(m_pSignature->signXML(elemRootKey,tmpCertStore)!=E_SUCCESS)
    {
        CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo send to users...\n");
    }
    delete ownCert;
    delete tmpCertStore;

    tlen=0;
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
            CACertificate* nextCert=options.getNextMixTestCertificate();
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

            m_pSignature->signXML(elemRoot);
            DOM_Output::dumpToMem(docSymKey,out,&outlen);
            m_pMuxOut->setSendKey(key,32);
            m_pMuxOut->setReceiveKey(key+32,32);
            UINT16 size=htons((UINT16)outlen);
            ((CASocket*)m_pMuxOut)->send((UINT8*)&size,2);
            ((CASocket*)m_pMuxOut)->send(out,outlen);
            m_pMuxOut->setCrypt(true);
            delete[] out;
            break;
        }
        child=child.getNextSibling();
    }

    if(initMixCascadeInfo(elemMixes)!=E_SUCCESS)
    {
        CAMsg::printMsg(LOG_CRIT,"Error initializing cascade info.\n");
        return E_UNKNOWN;
    }
    else
    {
        if(m_pInfoService != NULL)
            m_pInfoService->sendCascadeHelo();
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
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		CAQueue* pQueue=((CAFirstMix*)param)->m_pQueueSendToMix;
		CAMuxSocket* pMuxSocket=pFirstMix->m_pMuxOut;

		UINT32 len;
		SINT32 ret;
#ifndef USE_POOL
		tQueueEntry* pQueueEntry=new tQueueEntry;
		MIXPACKET* pMixPacket=&pQueueEntry->packet;
		while(!pFirstMix->m_bRestart)
			{
				len=sizeof(tQueueEntry);
				ret=pQueue->getOrWait((UINT8*)pQueueEntry,&len);
				if(ret!=E_SUCCESS||len!=sizeof(tQueueEntry))
					break;
				if(pMuxSocket->send(pMixPacket)!=MIXPACKET_SIZE)
					break;
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
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread SendToMix\n");
		THREAD_RETURN_SUCCESS;
	}

/* How to end this thread:
	0. set bRestart=true
*/
#define MAX_READ_FROM_NEXT_MIX_QUEUE_SIZE 10000000 //How many bytes could be in the incoming queue ??
THREAD_RETURN fm_loopReadFromMix(void* pParam)
	{
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

		while(!pFirstMix->m_bRestart)
			{
				if(pQueue->getSize()>MAX_READ_FROM_NEXT_MIX_QUEUE_SIZE)
					{
						msSleep(200);
						continue;
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
								break;
							}
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
			}
		delete pQueueEntry;
		#ifdef USE_POOL
			delete pPool;
		#endif
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
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		CASocket* socketsIn=pFirstMix->m_arrSocketsIn;
		CAIPList* pIPList=pFirstMix->m_pIPList;
		CAThreadPool* pthreadsLogin=pFirstMix->m_pthreadsLogin;
		UINT32 nSocketsIn=pFirstMix->m_nSocketsIn;
		CASocketGroup osocketgroupAccept(false);
		CAMuxSocket* pNewMuxSocket;
		UINT8* peerIP=new UINT8[4];
		UINT32 i=0;
		SINT32 countRead;
		SINT32 ret;
		for(i=0;i<nSocketsIn;i++)
			osocketgroupAccept.add(socketsIn[i]);
		while(!pFirstMix->getRestart())
			{
				countRead=osocketgroupAccept.select(10000);
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
						if(osocketgroupAccept.isSignaled(socketsIn[i]))
							{
								countRead--;
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"New direct Connection from Client!\n");
								#endif
								pNewMuxSocket=new CAMuxSocket;
								ret=socketsIn[i].accept(*(CASocket*)pNewMuxSocket);
								if(ret!=E_SUCCESS)
									{
										CAMsg::printMsg(LOG_ERR,"Accept Error %u - direct Connection from Client!\n",GET_NET_ERROR);
										delete pNewMuxSocket;
										if(ret==E_SOCKETCLOSED&&pFirstMix->getRestart()) //Hm, should we restart ??
											goto END_THREAD;
									}
								else
									{
										//Pruefen ob schon vorhanden..
										ret=((CASocket*)pNewMuxSocket)->getPeerIP(peerIP);
										#ifdef PAYMENT
											if(ret!=E_SUCCESS||pIPList->insertIP(peerIP)<0 ||
												pFirstMix->m_pAccountingInstance->isIPAddressBlocked(peerIP))
										#else
											if(ret!=E_SUCCESS||pIPList->insertIP(peerIP)<0)
										#endif
											{
												delete pNewMuxSocket;
											}
										else
											{
												t_UserLoginData* d=new t_UserLoginData;
												d->pNewUser=pNewMuxSocket;
												d->pMix=pFirstMix;
												memcpy(d->peerIP,peerIP,4);
												pthreadsLogin->addRequest(fm_loopDoUserLogin,d);
											}
									}
							}
						i++;
					}
			}
END_THREAD:
		delete []peerIP;
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread AcceptUser\n");
		THREAD_RETURN_SUCCESS;
	}

THREAD_RETURN fm_loopDoUserLogin(void* param)
	{
		t_UserLoginData* d=(t_UserLoginData*)param;
		d->pMix->doUserLogin(d->pNewUser,d->peerIP);
		delete d;
		THREAD_RETURN_SUCCESS;
	}

/** Sends and receives all data neccessary for a User to "login".
	* This means sending the public key of the Mixes and receiving the
	* sym keys of JAP. This is done in a thread on a per user basis
	* @todo Cleanup of runing thread if mix restarts...
***/
SINT32 CAFirstMix::doUserLogin(CAMuxSocket* pNewUser,UINT8 peerIP[4])
	{
		#ifdef _DEBUG
			int ret=((CASocket*)pNewUser)->setKeepAlive(true);
			if(ret!=E_SUCCESS)
				CAMsg::printMsg(LOG_DEBUG,"Error setting KeepAlive!");
		#else
			((CASocket*)pNewUser)->setKeepAlive(true);
		#endif
		/*
			ADDITIONAL PREREQUISITE:
			The timestamps in the messages require the user to sync his time
			with the time of the cascade. Hence, the current time needs to be
			added to the data that is sent to the user below.
			For the mixes that form the cascade, the synchronization can be
			left to an external protocol such as NTP. Unfortunately, this is
			not enforceable for all users.
		*/
		((CASocket*)pNewUser)->send(m_xmlKeyInfoBuff,m_xmlKeyInfoSize);  // send the mix-keys to JAP
		// es kann nicht blockieren unter der Annahme das der TCP-Sendbuffer > m_xmlKeyInfoSize ist....
		//wait for keys from user
#ifndef FIRST_MIX_SYMMETRIC
		MIXPACKET oMixPacket;
		((CASocket*)pNewUser)->setNonBlocking(true);	                    // stefan: sendet das send in der letzten zeile doch noch nicht? wenn doch, kann dann ein JAP nicht durch verweigern der annahme hier den mix blockieren? vermutlich nciht, aber andersherum faend ich das einleuchtender.
		if(pNewUser->receive(&oMixPacket,FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT)!=MIXPACKET_SIZE) //wait at most FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT
																																																	// milliseconds for user to send sym key
			{
				delete pNewUser;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		m_pRSA->decrypt(oMixPacket.data,oMixPacket.data);
		if(memcmp("KEYPACKET",oMixPacket.data,9)!=0)
			{
				m_pIPList->removeIP(peerIP);
				delete pNewUser;
				return E_UNKNOWN;
			}
		pNewUser->setKey(oMixPacket.data+9,32);
		pNewUser->setCrypt(true);
#else
		UINT16 xml_len;
		if(((CASocket*)pNewUser)->receiveFully((UINT8*)&xml_len,2,FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT)!=E_SUCCESS)
			{
				delete pNewUser;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		xml_len=ntohs(xml_len);
		UINT8* xml_buff=new UINT8[xml_len+2]; //+2 for size...
		if(((CASocket*)pNewUser)->receiveFully(xml_buff+2,xml_len,FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT)!=E_SUCCESS)
			{
				delete pNewUser;
				delete xml_buff;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		DOMParser oParser;
		MemBufInputSource oInput(xml_buff+2,xml_len,"tmp");
		oParser.parse(oInput);
		DOM_Document doc=oParser.getDocument();
		DOM_Element elemRoot;
		if(doc==NULL||(elemRoot=doc.getDocumentElement())==NULL||
			decryptXMLElement(elemRoot,m_pRSA)!=E_SUCCESS)
			{
				delete xml_buff;
				delete pNewUser;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		elemRoot=doc.getDocumentElement();
		if(!elemRoot.getNodeName().equals("JAPKeyExchange"))
			{
				delete xml_buff;
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
				delete xml_buff;
				delete pNewUser;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		//Sending Signature....
		xml_buff[0]=xml_len>>8;
		xml_buff[1]=xml_len&0xFF;
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
		xml_buff[0]=u32>>8;
		xml_buff[1]=u32&0xFF;
		((CASocket*)pNewUser)->send(xml_buff,u32+2);
		delete xml_buff;
		((CASocket*)pNewUser)->setNonBlocking(true);
#endif
		CAQueue* tmpQueue=new CAQueue(sizeof(tQueueEntry));
		if(m_pChannelList->add(pNewUser,peerIP,tmpQueue)!=E_SUCCESS)// adding user connection to mix->JAP channel list (stefan: sollte das nicht connection list sein? --> es handelt sich um eine Datenstruktu fŸr Connections/Channels ).
			{
				m_pIPList->removeIP(peerIP);
				delete tmpQueue;
				delete pNewUser;
				return E_UNKNOWN;
			}
#if defined(PAYMENT)||defined(FIRST_MIX_SYMMETRIC)||defined(WITH_CONTROL_CHANNELS_TEST)
		fmHashTableEntry* pHashEntry=m_pChannelList->get(pNewUser);
#endif
#ifdef PAYMENT
		// register AI control channel
		pHashEntry->pAccountingInfo->pControlChannel = new CAAccountingControlChannel(pHashEntry);
		pHashEntry->pControlChannelDispatcher->registerControlChannel(pHashEntry->pAccountingInfo->pControlChannel);
#endif
#ifdef FIRST_MIX_SYMMETRIC
		pHashEntry->pSymCipher=new CASymCipher();
		pHashEntry->pSymCipher->setKey(mixKey);
		pHashEntry->pSymCipher->setIVs(mixKey+16);
		pNewUser->setReceiveKey(linkKey,32);
		pNewUser->setSendKey(linkKey+32,32);
		pNewUser->setCrypt(true);
#endif
#ifdef WITH_CONTROL_CHANNELS_TEST
		pHashEntry->pControlChannelDispatcher->registerControlChannel(new CAControlChannelTest());
#endif
		incUsers();																	// increment the user counter by one
#ifdef HAVE_EPOLL
		m_psocketgroupUsersRead->add(*pNewUser,m_pChannelList->get(pNewUser)); // add user socket to the established ones that we read data from.
		#ifdef WITH_CONTROL_CHANNELS
			m_psocketgroupUsersWrite->add(*pNewUser,m_pChannelList->get(pNewUser));
		#endif
#else
		m_psocketgroupUsersRead->add(*pNewUser); // add user socket to the established ones that we read data from.
		#ifdef WITH_CONTROL_CHANNELS
			m_psocketgroupUsersWrite->add(*pNewUser);
		#endif
#endif
		return E_SUCCESS;
	}

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

		if(m_pthreadAcceptUsers!=NULL)
			delete m_pthreadAcceptUsers;
		m_pthreadAcceptUsers=NULL;
		if(m_pthreadSendToMix!=NULL)
			delete m_pthreadSendToMix;
		m_pthreadSendToMix=NULL;
		#ifdef LOG_PACKET_TIMES
		if(m_pLogPacketStats!=NULL)
			delete m_pLogPacketStats;
		m_pLogPacketStats=NULL;
		#endif
		if(m_arrSocketsIn!=NULL)
			delete[] m_arrSocketsIn;
		m_arrSocketsIn=NULL;
		if(m_pMuxOut!=NULL)
			{
				m_pMuxOut->close();
				delete m_pMuxOut;
			}
		m_pMuxOut=NULL;
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
			delete m_pChannelList;
		m_pChannelList=NULL;
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
			delete []m_xmlKeyInfoBuff;
		m_xmlKeyInfoBuff=NULL;
		m_docMixCascadeInfo=NULL;
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::clean() finished\n");
		#endif
		return E_SUCCESS;
	}

/**
 * @deprecated first part of this method moved to processKeyExchange(), rest moved to CAMix.cpp
 * This will initialize the XML Cascade Info send to the InfoService and
  * the Key struct which is send to each user which connects
* This function is called from init()
* @DOCME
*//*
SINT32 CAFirstMix::initMixCascadeInfo(UINT8* recvBuff, UINT16 len)
{
		//Parse the input, which is the XML send from the previos mix, containing keys and id's
		if(recvBuff==NULL||len==0)
			return E_UNKNOWN;

		CAMsg::printMsg(LOG_DEBUG,"Get KeyInfo (foolowing line)\n");
		CAMsg::printMsg(LOG_DEBUG,"%s\n",recvBuff);


		DOMParser oParser;
		MemBufInputSource oInput(recvBuff,len,"tmp");
		oParser.parse(oInput);
		DOM_Document doc=oParser.getDocument();
		DOM_Element elemMixes=doc.getDocumentElement();
		if(elemMixes==NULL)
			return E_UNKNOWN;
		int count=0;
		if(getDOMElementAttribute(elemMixes,"count",&count)!=E_SUCCESS)
			return E_UNKNOWN;

		DOM_Node child=elemMixes.getLastChild();

		//tmp XML-Structure for constructing the XML which is send to each user
		DOM_Document docXmlKeyInfo=DOM_Document::createDocument();
		DOM_Element elemRootKey=docXmlKeyInfo.createElement("MixCascade");
		setDOMElementAttribute(elemRootKey,"version",(UINT8*)"0.1"); //set the Version of the XML to 0.1
		docXmlKeyInfo.appendChild(elemRootKey);
		DOM_Element elemMixProtocolVersion=docXmlKeyInfo.createElement("MixProtocolVersion");
		setDOMElementValue(elemMixProtocolVersion,(UINT8*)MIX_CASCADE_PROTOCOL_VERSION);
		elemRootKey.appendChild(elemMixProtocolVersion);
		DOM_Node elemMixesKey=docXmlKeyInfo.importNode(elemMixes,true);
		elemRootKey.appendChild(elemMixesKey);

		UINT32 tlen;
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
			}
		tlen=256;


		//Inserting own Key in XML-Key struct
		DOM_DocumentFragment docfragKey;
		m_pRSA->getPublicKeyAsDocumentFragment(docfragKey);
		DOM_Element elemOwnMix=docXmlKeyInfo.createElement("Mix");
		UINT8 buffId[255];
		options.getMixId(buffId,255);
		elemOwnMix.setAttribute("id",DOMString((char*)buffId));
		elemOwnMix.appendChild(docXmlKeyInfo.importNode(docfragKey,true));
		elemMixesKey.insertBefore(elemOwnMix,elemMixesKey.getFirstChild());
		setDOMElementAttribute((DOM_Element&)elemMixesKey,"count",count+1);
		CACertificate* ownCert=options.getOwnCertificate();
		if(ownCert==NULL)
			{
				CAMsg::printMsg(LOG_DEBUG,"Own Test Cert is NULL -- so it could not be inserted into signed KeyInfo send to users...\n");
			}
		CACertStore* tmpCertStore=new CACertStore();
		tmpCertStore->add(ownCert);
		if(m_pSignature->signXML(elemRootKey,tmpCertStore)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo send to users...\n");
			}
		delete ownCert;
		delete tmpCertStore;

		tlen=0;
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
						CACertificate* nextCert=options.getNextMixTestCertificate();
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

						m_pSignature->signXML(elemRoot);
						DOM_Output::dumpToMem(docSymKey,out,&outlen);
						m_pMuxOut->setSendKey(key,32);
						m_pMuxOut->setReceiveKey(key+32,32);
						UINT16 size=htons((UINT16)outlen);
						((CASocket*)m_pMuxOut)->send((UINT8*)&size,2);
						((CASocket*)m_pMuxOut)->send(out,outlen);
						m_pMuxOut->setCrypt(true);
						delete[] out;
						break;
					}
				child=child.getNextSibling();
			}


	//CascadeInfo
		m_docMixCascadeInfo=DOM_Document::createDocument();
		DOM_Element elemRoot=m_docMixCascadeInfo.createElement("MixCascade");


		UINT8 id[50];
		options.getMixId(id,50);
		elemRoot.setAttribute(DOMString("id"),DOMString((char*)id));

		UINT8 name[255];
		if(options.getCascadeName(name,255)!=E_SUCCESS)
			{
				return E_UNKNOWN;
			}
		m_docMixCascadeInfo.appendChild(elemRoot);
		DOM_Element elem=m_docMixCascadeInfo.createElement("Name");
		DOM_Text text=m_docMixCascadeInfo.createTextNode(DOMString((char*)name));
		elem.appendChild(text);
		elemRoot.appendChild(elem);

		elem=m_docMixCascadeInfo.createElement("Network");
		elemRoot.appendChild(elem);
		DOM_Element elemListenerInterfaces=m_docMixCascadeInfo.createElement("ListenerInterfaces");
		elem.appendChild(elemListenerInterfaces);

		for(UINT32 i=1;i<=options.getListenerInterfaceCount();i++)
			{
				CAListenerInterface* pListener=options.getListenerInterface(i);
				if(pListener->isHidden()){//do nothing}
				else if(pListener->getType()==RAW_TCP)
					{
						DOM_DocumentFragment docFrag;
						pListener->toDOMFragment(docFrag,m_docMixCascadeInfo);
						elemListenerInterfaces.appendChild(docFrag);
					}
				delete pListener;
			}

		DOM_Element elemThisMix=m_docMixCascadeInfo.createElement("Mix");
		elemThisMix.setAttribute(DOMString("id"),DOMString((char*)id));
		DOM_Node elemMixesDocCascade=m_docMixCascadeInfo.createElement("Mixes");
		elemMixesDocCascade.appendChild(elemThisMix);
		count=1;
		elemRoot.appendChild(elemMixesDocCascade);

		DOM_Node node=elemMixes.getFirstChild();
		while(node!=NULL)
			{
				if(node.getNodeType()==DOM_Node::ELEMENT_NODE&&node.getNodeName().equals("Mix"))
					{
						elemMixesDocCascade.appendChild(m_docMixCascadeInfo.importNode(node,false));
						count++;
					}
				node=node.getNextSibling();
			}
		setDOMElementAttribute(elemMixesDocCascade,"count",count);
		return E_SUCCESS;
	}
*/

