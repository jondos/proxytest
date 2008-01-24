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
#include "CALastMix.hpp"
#ifdef NEW_MIX_TYPE // TypeB mixes
  #include "TypeB/typedefsb.hpp"
#else // not TypeB mixes
  /* TypeB mixes doesn't use the default-implementation */
  #include "CALastMixChannelList.hpp"
#endif
#include "CASocketGroup.hpp"
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
	#include "CASocketAddrUnix.hpp"
#endif
#include "CACertStore.hpp"
#include "CABase64.hpp"
#include "CAPool.hpp"
#include "xml/DOM_Output.hpp"

extern CACmdLnOptions* pglobalOptions;

/*#ifdef LOG_CHANNEL
	#define MACRO_DO_LOG_CHANNEL\
		getcurrentTimeMillis(current_millis);\
		diff_time=diff64(current_millis,pChannelListEntry->timeCreated);\
		CAMsg::printMsg(LOG_DEBUG,"Channel %u closed - Start %Lu - End %Lu - Time [ms] - %u, Upload - %u, Download - %u, DataPacketsFromUser %u, DataPacketsToUser %u\n",\
			pChannelListEntry->channelIn,pChannelListEntry->timeCreated,\
			current_millis,diff_time,pChannelListEntry->trafficInFromUser,pChannelListEntry->trafficOutToUser,\
			pChannelListEntry->packetsDataInFromUser,pChannelListEntry->packetsDataOutToUser); 
#endif
*/
/*******************************************************************************/
// ----------START NEW VERSION -----------------------
//---------------------------------------------------------
/********************************************************************************/

SINT32 CALastMix::initOnce()
	{
		if(setTargets()!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not set Targets (proxies)!\n");
				return E_UNKNOWN;
			}

		m_pSignature=pglobalOptions->getSignKey();
		if(m_pSignature==NULL)
			return E_UNKNOWN;
		if(pglobalOptions->getListenerInterfaceCount()<1)
			{
				CAMsg::printMsg(LOG_CRIT,"No ListenerInterfaces specified!\n");
				return E_UNKNOWN;
			}
    if(pglobalOptions->getCascadeXML() != NULL)
    	initMixCascadeInfo(pglobalOptions->getCascadeXML());
		return E_SUCCESS;
	}

SINT32 CALastMix::init()
	{
		m_pRSA=new CAASymCipher();	
		if(m_pRSA->generateKeyPair(1024)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not generate a valid key pair\n");
				return E_UNKNOWN;
			}
		
		CAMsg::printMsg(LOG_INFO,"Waiting for Connection from previous Mix...\n");
		CAListenerInterface*  pListener=NULL;
		UINT32 interfaces=pglobalOptions->getListenerInterfaceCount();
		for(UINT32 i=1;i<=interfaces;i++)
			{
				pListener=pglobalOptions->getListenerInterface(i);
				if(!pListener->isVirtual())
					break;
				delete pListener;
				pListener=NULL;
			}
		if(pListener==NULL)
			{
				CAMsg::printMsg(LOG_CRIT," failed!\n");
				CAMsg::printMsg(LOG_CRIT,"Reason: no useable (non virtual) interface found!\n");
				return E_UNKNOWN;
			}
		const CASocketAddr* pAddr=NULL;
		pAddr=pListener->getAddr();
		delete pListener;
		m_pMuxIn=new CAMuxSocket();
		SINT32 ret=m_pMuxIn->accept(*pAddr);
		delete pAddr;
		if(ret!=E_SUCCESS)
		    {
					CAMsg::printMsg(LOG_CRIT," failed!\n");
					CAMsg::printMsg(LOG_CRIT,"Reason: call to accept() faild with error code: %i\n",ret); 
					return E_UNKNOWN;
		    }
		((CASocket*)*m_pMuxIn)->setRecvBuff(500*MIXPACKET_SIZE);
		((CASocket*)*m_pMuxIn)->setSendBuff(500*MIXPACKET_SIZE);
		if(((CASocket*)*m_pMuxIn)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)*m_pMuxIn)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}
		
		CAMsg::printMsg(LOG_INFO,"connected!\n");

#ifdef LOG_CRIME
		m_nCrimeRegExpsURL=0;
		m_pCrimeRegExpsURL=pglobalOptions->getCrimeRegExpsURL(&m_nCrimeRegExpsURL);
		m_nCrimeRegExpsPayload = 0;
		m_pCrimeRegExpsPayload = pglobalOptions->getCrimeRegExpsPayload(&m_nCrimeRegExpsPayload);
#endif	
		ret=processKeyExchange();
		if(ret!=E_SUCCESS)
			return ret;
#ifdef REPLAY_DETECTION
		m_pReplayDB=new CADatabase();
		m_pReplayDB->start();
#endif		
		m_pQueueSendToMix=new CAQueue(sizeof(tQueueEntry));
		m_pQueueReadFromMix=new CAQueue(sizeof(tQueueEntry));

		m_pMuxInControlChannelDispatcher=new CAControlChannelDispatcher(m_pQueueSendToMix);
#ifdef REPLAY_DETECTION
		m_pReplayMsgProc=new CAReplayCtrlChannelMsgProc(this);
		m_pReplayMsgProc->startTimeStampPorpagation(REPLAY_TIMESTAMP_PROPAGATION_INTERVALL);
#endif

		m_bRestart=false;
		//Starting thread for Step 1a
		m_pthreadReadFromMix=new CAThread((UINT8*)"CALastMix - ReadFromMix");
		m_pthreadReadFromMix->setMainLoop(lm_loopReadFromMix);
		m_pthreadReadFromMix->start(this);
		
		//Starting thread for Step 4		
		m_pthreadSendToMix=new CAThread((UINT8*)"CALastMix - SendToMix");
		m_pthreadSendToMix->setMainLoop(lm_loopSendToMix);
		m_pthreadSendToMix->start(this);

		//Starting thread for logging
#ifdef LOG_PACKET_TIMES
		m_pLogPacketStats=new CALogPacketStats();
		m_pLogPacketStats->setLogIntervallInMinutes(LM_PACKET_STATS_LOG_INTERVALL);
		m_pLogPacketStats->start();
#endif		
    #ifndef NEW_MIX_TYPE // not TypeB mixes
      /* TypeB mixes are using an own implementation */
		m_pChannelList=new CALastMixChannelList;
    #endif
		return E_SUCCESS;
	}

/** Processes the startup communication with the preceeding mix.
	* \li Step 1: Mix \e n-1 open a TCP/IP Connection\n
	* \li Step 2: LastMix sends XML struct describing itself, 
	*					containing PubKey of LastMix, signed by LastMix 
	*					(see \ref XMLInterMixInitSendFromLast "XML structs")\n
	* \li Step 3: Mix \e n-1 sends XML struct containing encrpyted (with PubKey) 
	*					Symetric Key used for	interlink encryption between 
	*         Mix \e n-1 <---> LastMix, signed by Mix \e n-1
	*					(see \ref XMLInterMixInitAnswer "XML structs")\n
	*/
SINT32 CALastMix::processKeyExchange()
	{
		DOM_Document doc=DOM_Document::createDocument();
		DOM_Element elemMixes=doc.createElement("Mixes");
		setDOMElementAttribute(elemMixes,"count",1);
    //UINT8 cName[128];
    //pglobalOptions->getCascadeName(cName,128);
    //setDOMElementAttribute(elemMixes,"cascadeName",cName);
		doc.appendChild(elemMixes);
		
		addMixInfo(elemMixes, false);
		DOM_Element elemMix;
		getDOMChildByName(elemMixes, (UINT8*)"Mix", elemMix, false);

		//Inserting MixProtocol Version 
		// Version 0.3  - "normal", initial mix protocol
		// Version 0.4  - with new flow control
    // Version 0.5  - end-to-end 1:n channels (only between client and last mix)
		DOM_Element elemMixProtocolVersion=doc.createElement("MixProtocolVersion");
		elemMix.appendChild(elemMixProtocolVersion);
    #ifdef NEW_MIX_TYPE // TypeB mixes
      setDOMElementValue(elemMixProtocolVersion,(UINT8*)"0.5");
      DOM_Element elemDownstreamPackets = doc.createElement("DownstreamPackets");
      setDOMElementValue(elemDownstreamPackets, (UINT32)CHANNEL_DOWNSTREAM_PACKETS);
      elemMixProtocolVersion.appendChild(elemDownstreamPackets);
      DOM_Element elemChannelTimeout = doc.createElement("ChannelTimeout");
      /* let the client use our channel-timeout + 5 seconds */
      setDOMElementValue(elemChannelTimeout, (UINT32)(CHANNEL_TIMEOUT + 5));
      elemMixProtocolVersion.appendChild(elemChannelTimeout);
      DOM_Element elemChainTimeout = doc.createElement("ChainTimeout");
      /* let the client use our chain-timeout - 5 seconds */
      setDOMElementValue(elemChainTimeout, (UINT32)(CHAIN_TIMEOUT - 5));
      elemMixProtocolVersion.appendChild(elemChainTimeout);
    #else
      #ifdef NEW_FLOW_CONTROL
		setDOMElementValue(elemMixProtocolVersion,(UINT8*)"0.4");
      #else    
		setDOMElementValue(elemMixProtocolVersion,(UINT8*)"0.3");
      #endif
    #endif
		//Inserting RSA-Key
		DOM_DocumentFragment tmpDocFrag;
		m_pRSA->getPublicKeyAsDocumentFragment(tmpDocFrag);
		DOM_Node nodeRsaKey=doc.importNode(tmpDocFrag,true);
		elemMix.appendChild(nodeRsaKey);
		tmpDocFrag=0;
		//inserting Nonce
		DOM_Element elemNonce=doc.createElement("Nonce");
		UINT8 arNonce[16];
		getRandom(arNonce,16);
		UINT8 tmpBuff[50];
		UINT32 tmpLen=50;
		CABase64::encode(arNonce,16,tmpBuff,&tmpLen);
		tmpBuff[tmpLen]=0;
		setDOMElementValue(elemNonce,tmpBuff);
		elemMix.appendChild(elemNonce);
		
    
		
// Add Info about KeepAlive traffic
		DOM_Element elemKeepAlive;
		UINT32 u32KeepAliveSendInterval=pglobalOptions->getKeepAliveSendInterval();
		UINT32 u32KeepAliveRecvInterval=pglobalOptions->getKeepAliveRecvInterval();
		elemKeepAlive=doc.createElement("KeepAlive");
		DOM_Element elemKeepAliveSendInterval;
		DOM_Element elemKeepAliveRecvInterval;
		elemKeepAliveSendInterval=doc.createElement("SendInterval");
		elemKeepAliveRecvInterval=doc.createElement("ReceiveInterval");
		elemKeepAlive.appendChild(elemKeepAliveSendInterval);
		elemKeepAlive.appendChild(elemKeepAliveRecvInterval);
		setDOMElementValue(elemKeepAliveSendInterval,u32KeepAliveSendInterval);
		setDOMElementValue(elemKeepAliveRecvInterval,u32KeepAliveRecvInterval);
		elemMix.appendChild(elemKeepAlive);
		CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Offering -- SendInterval %u -- Receive Interval %u\n",u32KeepAliveSendInterval,u32KeepAliveRecvInterval);		
		
		// create signature
		if (signXML(elemMix) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo sent to users...\n");
		}

		UINT32 len=0;
		UINT8* messageBuff=DOM_Output::dumpToMem(doc,&len);
		UINT16 tmp=htons((UINT16)len);
		CAMsg::printMsg(LOG_INFO,"Sending Infos (chain length and RSA-Key, Message-Size %u)\n",len);
		
		if(	((CASocket*)*m_pMuxIn)->send((UINT8*)&tmp,2)!=2 ||
				((CASocket*)*m_pMuxIn)->send(messageBuff,len)!=(SINT32)len)
			{
				CAMsg::printMsg(LOG_ERR,"Error sending Key-Info!\n");
				delete []messageBuff;
				return E_UNKNOWN;
			}
		delete[] messageBuff;
		
		//Now receiving the symmetric key
		CAMsg::printMsg(LOG_INFO,"Waiting for len of Symmetric Key from previous Mix...\n");
		if(((CASocket*)*m_pMuxIn)->receiveFully((UINT8*)&tmp,2)!=E_SUCCESS)
			{
        CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info lenght!\n");
        return E_UNKNOWN;
			}
		len=ntohs(tmp);
		messageBuff=new UINT8[len+1]; //+1 for the closing Zero
		CAMsg::printMsg(LOG_INFO,"Got it - Len of Symmetric Key is: %i\n",len);
		CAMsg::printMsg(LOG_INFO,"Waiting for Symmetric Key from previous Mix...\n");
		if(((CASocket*)*m_pMuxIn)->receiveFully(messageBuff,len)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR,"Error receiving symetric key!\n");
				delete []messageBuff;
				return E_UNKNOWN;
			}
		messageBuff[len]=0;
		CAMsg::printMsg(LOG_INFO,"Symmetric Key Info received is:\n");
		CAMsg::printMsg(LOG_INFO,"%s\n",(char*)messageBuff);		
		//verify signature
		CASignature oSig;
		CACertificate* pCert=pglobalOptions->getPrevMixTestCertificate();
		oSig.setVerifyKey(pCert);
		delete pCert;
		if(oSig.verifyXML(messageBuff,len)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not verify the symetric key!\n");		
				delete []messageBuff;
				return E_UNKNOWN;
			}
		//Verifying nonce!
		MemBufInputSource oInput(messageBuff,len,"tmp");
		DOMParser oParser;
		oParser.parse(oInput);
		doc=oParser.getDocument();
		DOM_Element elemRoot=doc.getDocumentElement();
		elemNonce=NULL;
		getDOMChildByName(elemRoot,(UINT8*)"Nonce",elemNonce,false);
		tmpLen=50;
		memset(tmpBuff,0,tmpLen);
		if(elemNonce==NULL||getDOMElementValue(elemNonce,tmpBuff,&tmpLen)!=E_SUCCESS||
			CABase64::decode(tmpBuff,tmpLen,tmpBuff,&tmpLen)!=E_SUCCESS||
			tmpLen!=SHA_DIGEST_LENGTH ||
			memcmp(SHA1(arNonce,16,NULL),tmpBuff,SHA_DIGEST_LENGTH)!=0
			)
			{
				CAMsg::printMsg(LOG_CRIT,"Couldt not verify the Nonce!\n");		
				delete []messageBuff;
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_INFO,"Verified the symetric key!\n");		
		
		UINT8 key[150];
		UINT32 keySize=150;
		SINT32 ret=decodeXMLEncryptedKey(key,&keySize,messageBuff,len,m_pRSA);
		delete []messageBuff;
		if(ret!=E_SUCCESS||keySize!=64)
			{
				CAMsg::printMsg(LOG_CRIT,"Couldt not decrypt the symetric key!\n");		
				return E_UNKNOWN;
			}
		if(m_pMuxIn->setReceiveKey(key,32)!=E_SUCCESS||m_pMuxIn->setSendKey(key+32,32)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Couldt not set the symetric key to be used by the MuxSocket!\n");		
				return E_UNKNOWN;
			}
		m_pMuxIn->setCrypt(true);
		///Getting and calculating the KeepAlive Traffice...
		elemKeepAlive=NULL;
		elemKeepAliveSendInterval=NULL;
		elemKeepAliveRecvInterval=NULL;
		getDOMChildByName(elemRoot,(UINT8*)"KeepAlive",elemKeepAlive);
		getDOMChildByName(elemKeepAlive,(UINT8*)"SendInterval",elemKeepAliveSendInterval);
		getDOMChildByName(elemKeepAlive,(UINT8*)"ReceiveInterval",elemKeepAliveRecvInterval);
		UINT32 tmpSendInterval,tmpRecvInterval;
		getDOMElementValue(elemKeepAliveSendInterval,tmpSendInterval,0xFFFFFFFF); //if now send interval was given set it to "infinite"
		getDOMElementValue(elemKeepAliveRecvInterval,tmpRecvInterval,0xFFFFFFFF); //if no recv interval was given --> set it to "infinite"
		CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Getting offer -- SendInterval %u -- Receive Interval %u\n",tmpSendInterval,tmpRecvInterval);		
		m_u32KeepAliveSendInterval=max(u32KeepAliveSendInterval,tmpRecvInterval);
		if(m_u32KeepAliveSendInterval>10000)
			m_u32KeepAliveSendInterval-=10000; //make the send interval a little bit smaller than the related receive intervall
		m_u32KeepAliveRecvInterval=max(u32KeepAliveRecvInterval,tmpSendInterval);
		CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Calculated -- SendInterval %u -- Receive Interval %u\n",m_u32KeepAliveSendInterval,m_u32KeepAliveRecvInterval);		
		return E_SUCCESS;
	}

#ifdef NEW_MIX_TYPE // TypeB mixes
  void CALastMix::reconfigureMix() {
  }
#endif

SINT32 CALastMix::reconfigure()
	{
		CAMsg::printMsg(LOG_DEBUG,"Reconfiguring Last Mix\n");
		CAMsg::printMsg(LOG_DEBUG,"Re-read cache proxies\n");
		if(setTargets()!=E_SUCCESS)
			CAMsg::printMsg(LOG_DEBUG,"Could not set new cache proxies\n");
    #ifndef NEW_MIX_TYPE // not TypeB mixes
      #if defined (DELAY_CHANNELS)||defined (DELAY_CHANNELS_LATENCY)
		CAMsg::printMsg(LOG_DEBUG,"Set new ressources limitation parameters\n");
        if(m_pChannelList!=NULL) {
		#if defined (DELAY_CHANNELS)
			m_pChannelList->setDelayParameters(	pglobalOptions->getDelayChannelUnlimitTraffic(),
																					pglobalOptions->getDelayChannelBucketGrow(),
																					pglobalOptions->getDelayChannelBucketGrowIntervall());
		#endif
		#if defined (DELAY_CHANNELS_LATENCY)
			UINT32 utemp=pglobalOptions->getDelayChannelLatency();
			m_pChannelList->setDelayLatencyParameters(	utemp);
		#endif
		}
      #endif
    #else // TypeB mixes
      reconfigureMix();
    #endif
		return E_SUCCESS;
	}

THREAD_RETURN lm_loopLog(void* param)
	{
		CALastMix* pLastMix=(CALastMix*)param;
		pLastMix->m_bRunLog=true;
		UINT32 countLog=0;
		UINT8 buff[256];
		while(pLastMix->m_bRunLog)
			{
				if((countLog%10)==0)
					{
						logMemoryUsage();
					}
				if(countLog==0)
					{
						CAMsg::printMsg(LOG_DEBUG,"Uploaded  Packets: %u\n",pLastMix->m_logUploadedPackets);
						CAMsg::printMsg(LOG_DEBUG,"Downloaded Packets: %u\n",pLastMix->m_logDownloadedPackets);
						print64(buff,(UINT64&)pLastMix->m_logUploadedBytes);
						CAMsg::printMsg(LOG_DEBUG,"Uploaded  Bytes  : %s\n",buff);
						print64(buff,(UINT64&)pLastMix->m_logDownloadedBytes);
						CAMsg::printMsg(LOG_DEBUG,"Downloaded Bytes  : %s\n",buff);
						countLog=30;
					}
				sSleep(30);
				countLog--;
			}
		THREAD_RETURN_SUCCESS;
	}

/**How to end this thread:
0. set m_bRestart=true;
1. Close connection to next mix
2. put a byte in the Mix-Output-Queue
*/
THREAD_RETURN lm_loopSendToMix(void* param)
	{
		CALastMix* pLastMix=(CALastMix*)param;
		CAQueue* pQueue=pLastMix->m_pQueueSendToMix;
		CAMuxSocket* pMuxSocket=pLastMix->m_pMuxIn;
		SINT32 ret;
		UINT32 len;
		UINT32 u32KeepAliveSendInterval=pLastMix->m_u32KeepAliveSendInterval;
#ifndef USE_POOL
		tQueueEntry* pQueueEntry=new tQueueEntry;
		MIXPACKET* pMixPacket=&pQueueEntry->packet;

		while(!pLastMix->m_bRestart)
			{
				len=sizeof(tQueueEntry);
				ret=pQueue->getOrWait((UINT8*)pQueueEntry,&len,u32KeepAliveSendInterval);
				if(ret==E_TIMEDOUT)
					{
						pMixPacket->flags=CHANNEL_DUMMY;
						pMixPacket->channel=DUMMY_CHANNEL;
						getRandom(pMixPacket->data,DATA_SIZE);
					}
				else if(ret!=E_SUCCESS||len!=sizeof(tQueueEntry))
					{
						CAMsg::printMsg(LOG_ERR,"CALastMix::lm_loopSendToMix - Error in dequeueing MixPaket\n");
						CAMsg::printMsg(LOG_ERR,"ret=%i len=%i\n",ret,len);
						break;
					}
				if(pMuxSocket->send(pMixPacket)!=MIXPACKET_SIZE)
					{
						CAMsg::printMsg(LOG_ERR,"CALastMix::lm_loopSendToMix - Error in sending MixPaket\n");
						break;
					}
#ifdef LOG_PACKET_TIMES
 				if(!isZero64(pQueueEntry->timestamp_proccessing_start))
					{
						getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
						pLastMix->m_pLogPacketStats->addToTimeingStats(*pQueueEntry,CHANNEL_DATA,false);
					}
#endif					
			}
		delete pQueueEntry;
#else
		CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		tPoolEntry* pPoolEntry=new tPoolEntry;
		MIXPACKET* pMixPacket=&pPoolEntry->packet;
		while(!pLastMix->m_bRestart)
			{
				len=sizeof(tQueueEntry);
				SINT32 ret=pQueue->getOrWait((UINT8*)pPoolEntry,&len,MIX_POOL_TIMEOUT);
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
					{
						break;
					}
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
						pLastMix->m_pLogPacketStats->addToTimeingStats(*pPoolEntry,CHANNEL_DATA,false);
					}
#endif					
			}
		delete pPoolEntry;
		delete pPool;
#endif
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread SendToMix\n");
		THREAD_RETURN_SUCCESS;
	}

#define MAX_READ_FROM_PREV_MIX_QUEUE_SIZE	10000000

/* How to end this thread:
 * 1. set m_brestart=true
 */  	
THREAD_RETURN lm_loopReadFromMix(void *pParam)
	{
		CALastMix* pLastMix=(CALastMix*)pParam;
		CAMuxSocket* pMuxSocket=pLastMix->m_pMuxIn;
		CAQueue* pQueue=pLastMix->m_pQueueReadFromMix;
		tQueueEntry* pQueueEntry=new tQueueEntry;
		MIXPACKET* pMixPacket=&pQueueEntry->packet;
		CASingleSocketGroup* pSocketGroup=new CASingleSocketGroup(false);
		pSocketGroup->add(*pMuxSocket);
		#ifdef USE_POOL
			CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		#endif
		UINT64 keepaliveNow,keepaliveLast;
		UINT32 u32KeepAliveRecvInterval=pLastMix->m_u32KeepAliveRecvInterval;
		getcurrentTimeMillis(keepaliveLast);
		while(!pLastMix->m_bRestart)
			{
				if(pQueue->getSize()>MAX_READ_FROM_PREV_MIX_QUEUE_SIZE)
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
						pLastMix->m_bRestart=true;
						break;
					}
				SINT32 ret=pSocketGroup->select(MIX_POOL_TIMEOUT);	
        if(ret < 0) 
					{
						if (ret == E_TIMEDOUT) 
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
						else 
							{
								/* another error occured (happens sometimes while debugging because
								 * of interruption, if a breakpoint is reached -> poll() returns
								 * errorcode EINTR)
								 * Note: Any Error on select() does not mean, that the underliny connections have some error state, because
								 * in this case select() returns the socket and than this socket returns the error
								 */
								continue;
							}
					}
				else if(ret>0)
					{
						ret=pMuxSocket->receive(pMixPacket); //receives a whole MixPacket
						#ifdef LOG_PACKET_TIMES
							getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start);
						#endif
						if(ret!=MIXPACKET_SIZE)
							{//something goes wrong...
								CAMsg::printMsg(LOG_ERR,"CALastMix::lm_loopReadFromMix - received returned: %i\n",ret);
								pLastMix->m_bRestart=true;
								break;
							}
					}
				#ifdef USE_POOL
					#ifdef LOG_PACKET_TIMES
						getcurrentTimeMicros(pQueueEntry->pool_timestamp_in);
					#endif
					pPool->pool((tPoolEntry*) pQueueEntry);
					#ifdef LOG_PACKET_TIMES
						getcurrentTimeMicros(pQueueEntry->pool_timestamp_out);
					#endif
				#endif		
				pQueue->add(pQueueEntry,sizeof(tQueueEntry));
				getcurrentTimeMillis(keepaliveLast);
			}
		delete pQueueEntry;
		delete pSocketGroup;
		#ifdef USE_POOL
			delete pPool;
		#endif			
		THREAD_RETURN_SUCCESS;
	}

#ifdef LOG_CRIME
	bool CALastMix::checkCrime(const UINT8* payLoad,UINT32 payLen)
	{ 
		//Lots of TODO!!!!
		//DNS Lookup may block if Host does not exists!!!!!
		//so we use regexp....

		UINT8 *startOfUrl, *endOfUrl;
		UINT32 strLen;
		if(payLen<3)
		{
			return false;
		}

		if (m_nCrimeRegExpsURL > 0)
		{
			startOfUrl = (UINT8*)memchr(payLoad,32,payLen-1); //search for first space...
			if(startOfUrl==NULL)
			{
				return false;
			}
			startOfUrl++;
			//search for first space after start of URL 
			endOfUrl = (UINT8*)memchr(startOfUrl, 32 , payLen - (startOfUrl - payLoad)); 
			if(endOfUrl==NULL)
			{
				return false;
			}
			strLen = endOfUrl-startOfUrl;
	
			for(UINT32 i = 0; i < m_nCrimeRegExpsURL; i++)
			{
				if(regnexec(&m_pCrimeRegExpsURL[i],(char*)startOfUrl,strLen,0,NULL,0)==0)
				{
					return true;
				}
			}
		}

		if (m_nCrimeRegExpsPayload == 0)
			{
				// there are no regular expressions for Payload
				return false;
			}
		
		for(UINT32 i = 0; i < m_nCrimeRegExpsPayload; i++)
		{
			if (regnexec(&m_pCrimeRegExpsPayload[i],(const char*)payLoad ,payLen,0,NULL,0)==0)
			{
				return true;
			}
		}
		return false;			
	}
#endif

/** Reads the configured proxies from \c options.
	* @retval E_UNKNOWN if no proxies are specified
	* @retval E_SUCCESS if successfully configured the proxies
	*/
SINT32 CALastMix::setTargets()
	{
		UINT32 cntTargets=pglobalOptions->getTargetInterfaceCount();
		if(cntTargets==0)
			{
				CAMsg::printMsg(LOG_CRIT,"No Targets (proxies) specified!\n");
				return E_UNKNOWN;
			}
		m_pCacheLB->clean();
		m_pSocksLB->clean();
		UINT32 i;
		for(i=1;i<=cntTargets;i++)
			{
				TargetInterface oTargetInterface;
				pglobalOptions->getTargetInterface(oTargetInterface,i);
				if(oTargetInterface.target_type==TARGET_HTTP_PROXY)
					m_pCacheLB->add(oTargetInterface.addr);
				else if(oTargetInterface.target_type==TARGET_SOCKS_PROXY)
					m_pSocksLB->add(oTargetInterface.addr);
				delete oTargetInterface.addr;
			}
		CAMsg::printMsg(LOG_DEBUG,"This mix will use the following proxies:\n");
		for(i=0;i<m_pCacheLB->getElementCount();i++)
			{
				CASocketAddrINet* pAddr=m_pCacheLB->get();
				UINT8 ip[4];
				pAddr->getIP(ip);
				UINT32 port=pAddr->getPort();
				CAMsg::printMsg(LOG_DEBUG,"%u. HTTP Proxy's Address: %u.%u.%u.%u:%u\n",i+1,ip[0],ip[1],ip[2],ip[3],port);
			}
		for(i=0;i<m_pSocksLB->getElementCount();i++)
			{
				CASocketAddrINet* pAddr=m_pSocksLB->get();
				UINT8 ip[4];
				pAddr->getIP(ip);
				UINT32 port=pAddr->getPort();
				CAMsg::printMsg(LOG_DEBUG,"%u. SOCKS Proxy's Address: %u.%u.%u.%u:%u\n",i+1,ip[0],ip[1],ip[2],ip[3],port);
			}
		return E_SUCCESS;
	}			

SINT32 CALastMix::clean()
{
		m_bRestart=true;
		m_bRunLog=false;
#ifdef REPLAY_DETECTION
		if(m_pReplayMsgProc!=NULL)
			{
				delete m_pReplayMsgProc;
			}
		m_pReplayMsgProc=NULL;
#endif

		if(m_pMuxInControlChannelDispatcher!=NULL)
			{
				delete m_pMuxInControlChannelDispatcher;
			}
		m_pMuxInControlChannelDispatcher=NULL;

		if(m_pMuxIn!=NULL)
			{
				m_pMuxIn->close();
			}
		//writng some bytes to the queue...
		if(m_pQueueSendToMix!=NULL)
			{
				UINT8 b[sizeof(tQueueEntry)+1];
				m_pQueueSendToMix->add(b,sizeof(tQueueEntry)+1);
			}

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
	
#ifdef LOG_PACKET_TIMES
			CAMsg::printMsg(LOG_CRIT,"Wait for LoopLogPacketStats to terminate!\n");
			if(m_pLogPacketStats!=NULL)
				{
					m_pLogPacketStats->stop();
					delete m_pLogPacketStats;
				}
			m_pLogPacketStats=NULL;
#endif	
		if(m_pChannelList!=NULL)
			{
				lmChannelListEntry* pChannelListEntry=m_pChannelList->getFirstSocket();
				while(pChannelListEntry!=NULL)
					{
						delete pChannelListEntry->pCipher;
						delete pChannelListEntry->pQueueSend;
						pChannelListEntry->pSocket->close();
						delete pChannelListEntry->pSocket;
						pChannelListEntry=m_pChannelList->getNextSocket();
					}
			}	
		if(m_pQueueReadFromMix!=NULL)
			delete m_pQueueReadFromMix;
		m_pQueueReadFromMix=NULL;	
		if(m_pQueueSendToMix!=NULL)
			delete m_pQueueSendToMix;
		m_pQueueSendToMix=NULL;	
    #ifndef NEW_MIX_TYPE // not TypeB mixes
      /* TypeB mixes are using an own implementation */
		if(m_pChannelList!=NULL)
			delete m_pChannelList;
		m_pChannelList=NULL;
		if(m_pMuxIn!=NULL)
			{
				m_pMuxIn->close();
				delete m_pMuxIn;
			}
		m_pMuxIn=NULL;
		if(m_pRSA!=NULL)
			delete m_pRSA;
		m_pRSA=NULL;
    #endif
		return E_SUCCESS;
	}

SINT32 CALastMix::initMixCascadeInfo(DOM_Element& mixes)
{
    SINT32 r = CAMix::initMixCascadeInfo(mixes);
    DOM_Element cascade = m_docMixCascadeInfo.getDocumentElement();
    setDOMElementAttribute(cascade,"create",(UINT8*)"true");
    return r;
}
#endif //ONLY_LOCAL_PROXY
