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
#include "CAMiddleMix.hpp"
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CASocketAddrINet.hpp"
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
	#include "CASocketAddrUnix.hpp"
#endif
#include "CAThread.hpp"
#include "CAInfoService.hpp"
#include "CAUtil.hpp"
#include "CABase64.hpp"
#include "CAPool.hpp"
#include "xml/DOM_Output.hpp"
#include "CAStatusManager.hpp"
#include "CALibProxytest.hpp"

SINT32 CAMiddleMix::initOnce()
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting MiddleMix InitOnce\n");
		//m_pSignature=CALibProxytest::getOptions()->getSignKey();
		m_pMultiSignature=CALibProxytest::getOptions()->getMultiSigner();
		if(m_pMultiSignature==NULL)
			{
				return E_UNKNOWN;
			}
		if(CALibProxytest::getOptions()->getListenerInterfaceCount()<1)
			{
				CAMsg::printMsg(LOG_CRIT,"No ListenerInterfaces specified!\n");
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

/** Processes key exchange with Mix \e n+1 and Mix \e n-1.
	* \li Step 1: Opens TCP/IP-Connection to Mix \e n+1. \n
	* \li Step 2: Receives info about Mix \e n+1 .. LastMix as XML struct
	*         (see \ref  XMLInterMixInitSendFromLast "XML struct") \n
  * \li Step 3: Verfies signature, generates symetric Keys used for link encryption
	*         with Mix \n+1. \n
	* \li Step 4: Sends symetric Key to Mix \e n+1, encrypted with PubKey of Mix \e n+1
	*         (see \ref XMLInterMixInitAnswer "XML struct") \n
	* \li Step 5: Sends info about Mix \e n .. LastMix as XML struct
	*         (see \ref  XMLInterMixInitSendFromLast "XML struct")to Mix \e n-1 \n
	* \li Step 6: Recevies symetric Key used for link encrpytion with Mix \e n-1
	*					(see \ref XMLInterMixInitAnswer "XML struct") \n
	*
	* @retval E_SUCCESS if KeyExchange with Mix \e n+1 and Mix \e n-1 was succesful
	*	@retval E_UNKNOWN otherwise
	*/
SINT32 CAMiddleMix::processKeyExchange()
	{
		UINT8* recvBuff=NULL;
		UINT32 len;
		SINT32 ret;

		if(m_pMuxOut->getCASocket()->receiveFully((UINT8*)&len, sizeof(len))!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Error receiving Key Info length from Mix n+1!\n");
				MONITORING_FIRE_NET_EVENT(ev_net_keyExchangeNextFailed);
				return E_UNKNOWN;
			}
		len=ntohl(len);
		CAMsg::printMsg(LOG_INFO,"Received Key Info length %u\n",len);
		recvBuff=new UINT8[len+1]; //for the \0 at the end
		if(recvBuff==NULL)
			return E_UNKNOWN;
		if(m_pMuxOut->getCASocket()->receiveFully(recvBuff,len)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Error receiving Key Info from Mix n+1!\n");
				MONITORING_FIRE_NET_EVENT(ev_net_keyExchangeNextFailed);
				delete []recvBuff;
				recvBuff = NULL;
				return E_UNKNOWN;
			}
		recvBuff[len]=0; //make a string
		CAMsg::printMsg(LOG_INFO,"Received Key Info...\n");
		CAMsg::printMsg(LOG_INFO,"%s\n",recvBuff);

		//Parsing KeyInfo received from Mix n+1
		XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(recvBuff,len);
		delete []recvBuff;
		recvBuff = NULL;
		if(doc==NULL)
			{
				CAMsg::printMsg(LOG_INFO,"Error parsing Key Info from Mix n+1!\n");
				MONITORING_FIRE_NET_EVENT(ev_net_keyExchangeNextFailed);
				return E_UNKNOWN;
			}

		DOMElement* root=doc->getDocumentElement();

		//Finding first <Mix> entry and sending symetric key...
		bool bFoundNextMix=false;
		DOMNode* child=root->getFirstChild();
		SINT32 result;
		while(child!=NULL)
			{
				if(equals(child->getNodeName(),"Mix"))
					{
						//check Signature....
						//CASignature oSig;
						CACertificate* nextCert=CALibProxytest::getOptions()->getNextMixTestCertificate();
						//oSig.setVerifyKey(nextCert);
						ret = CAMultiSignature::verifyXML(child, nextCert);
						//ret=oSig.verifyXML(child,NULL);
						delete nextCert;
						nextCert = NULL;
						if(ret!=E_SUCCESS)
							{
								MONITORING_FIRE_NET_EVENT(ev_net_keyExchangeNextFailed);
								CAMsg::printMsg(LOG_INFO,"Could not verify Key Info from next Mix!\n");
								return E_UNKNOWN;
							}

						 if ((result = checkCompatibility(child, "next")) != E_SUCCESS)
						{
							if (doc != NULL)
							{
								doc->release();
								doc = NULL;
							}
							return result;
						}

						//extracting Nonce and computing Hash of them
						DOMElement* elemNonce;
						getDOMChildByName(child,"Nonce",elemNonce,false);
						UINT32 lenNonce=1024;
						UINT8 arNonce[1024];
						UINT32 tmpLen=1024;
						if(elemNonce==NULL)
							{
								MONITORING_FIRE_NET_EVENT(ev_net_keyExchangeNextFailed);
								CAMsg::printMsg(LOG_INFO,"No nonce found in Key Info from next Mix!\n");
								if (doc != NULL)
								{
									doc->release();
									doc = NULL;
								}
								return E_UNKNOWN;
							}
						getDOMElementValue(elemNonce,arNonce,&lenNonce);
						CABase64::decode(arNonce,lenNonce,arNonce,&tmpLen);
						lenNonce=tmpLen;
						tmpLen=1024;
						CABase64::encode(SHA1(arNonce,lenNonce,NULL),SHA_DIGEST_LENGTH,
																	arNonce,&tmpLen);
						arNonce[tmpLen]=0;

						//Extracting PubKey of Mix n+1, generating SymKey for link encryption
						//with Mix n+1, encrypt and send them
						DOMNode* rsaKey=child->getFirstChild();
						CAASymCipher oRSA;
						oRSA.setPublicKeyAsDOMNode(rsaKey);
						UINT8 key[64];
						getRandom(key,64);
						XERCES_CPP_NAMESPACE::DOMDocument* docSymKey=createDOMDocument();
						DOMElement* elemRoot=NULL;
						encodeXMLEncryptedKey(key,64,elemRoot,docSymKey,&oRSA);
						docSymKey->appendChild(elemRoot);

						appendCompatibilityInfo(elemRoot);

						DOMElement* elemNonceHash=createDOMElement(docSymKey,"Nonce");
						setDOMElementValue(elemNonceHash,arNonce);
						elemRoot->appendChild(elemNonceHash);

						///Getting the KeepAlive Traffice...
						DOMElement *elemKeepAlive;
						DOMElement *elemKeepAliveSendInterval;
						DOMElement *elemKeepAliveRecvInterval;
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
						m_u32KeepAliveSendInterval2=max(u32KeepAliveSendInterval,tmpRecvInterval);
						if(m_u32KeepAliveSendInterval>10000)
							m_u32KeepAliveSendInterval2-=10000; //make the send interval a little bit smaller than the related receive intervall
						m_u32KeepAliveRecvInterval2=max(u32KeepAliveRecvInterval,tmpSendInterval);
						CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Calculated -- SendInterval %u -- Receive Interval %u\n",m_u32KeepAliveSendInterval2,m_u32KeepAliveRecvInterval2);

						//m_pSignature->signXML(elemRoot);
						m_pMultiSignature->signXML(elemRoot, true);
						m_pMuxOut->setSendKey(key,32);
						m_pMuxOut->setReceiveKey(key+32,32);
						UINT32 outlen=0;
						UINT8* out=DOM_Output::dumpToMem(docSymKey,&outlen);
						CAMsg::printMsg(LOG_DEBUG,"send length %u\n", outlen);
						UINT32 size=htonl(outlen);
						m_pMuxOut->getCASocket()->send((UINT8*)&size, sizeof(size));
						m_pMuxOut->getCASocket()->send(out, outlen);
						if (docSymKey != NULL)
						{
							docSymKey->release();
							docSymKey = NULL;
						}
						delete[] out;
						out = NULL;
						bFoundNextMix=true;
						break;
					}
				child=child->getNextSibling();
			}
		if(!bFoundNextMix)
			{
				CAMsg::printMsg(LOG_INFO,"Error -- no Key Info from Mix n+1 found!\n");
				MONITORING_FIRE_NET_EVENT(ev_net_keyExchangeNextFailed);
				if (doc != NULL)
				{
					doc->release();
					doc = NULL;
				}
				return E_UNKNOWN;
			}
		// -----------------------------------------
		// ---- Start exchange with Mix n-1 --------
		// -----------------------------------------
		//Inserting own (key) info
		UINT32 count=0;
		if(getDOMElementAttribute(root,"count",count)!=E_SUCCESS)
			{
				MONITORING_FIRE_NET_EVENT(ev_net_keyExchangeNextFailed);
				if (doc != NULL)
				{
					doc->release();
					doc = NULL;
				}
				return E_UNKNOWN;
			}
		count++;
		setDOMElementAttribute(root,"count",count);

		addMixInfo(root, true);
		DOMElement* mixNode;
		getDOMChildByName(root, "Mix", mixNode, false);


		UINT8 tmpBuff[50];
		CALibProxytest::getOptions()->getMixId(tmpBuff,50); //the mix id...
		setDOMElementAttribute(mixNode,"id",tmpBuff);
		//Supported Mix Protocol -->currently "0.3"
		DOMElement* elemMixProtocolVersion=createDOMElement(doc,"MixProtocolVersion");
		mixNode->appendChild(elemMixProtocolVersion);
		setDOMElementValue(elemMixProtocolVersion,(UINT8*)"0.3");

		DOMElement* elemKey=NULL;
		m_pRSA->getPublicKeyAsDOMElement(elemKey,doc); //the key
		mixNode->appendChild(elemKey);

		appendCompatibilityInfo(mixNode);

		//inserting Nonce
		DOMElement* elemNonce=createDOMElement(doc,"Nonce");
		UINT8 arNonce[16];
		getRandom(arNonce,16);
		UINT32 tmpLen=50;
		CABase64::encode(arNonce,16,tmpBuff,&tmpLen);
		tmpBuff[tmpLen]=0;
		setDOMElementValue(elemNonce,tmpBuff);
		mixNode->appendChild(elemNonce);

// Add Info about KeepAlive traffic
		DOMElement* elemKeepAlive;
		UINT32 u32KeepAliveSendInterval=CALibProxytest::getOptions()->getKeepAliveSendInterval();
		UINT32 u32KeepAliveRecvInterval=CALibProxytest::getOptions()->getKeepAliveRecvInterval();
		elemKeepAlive=createDOMElement(doc,"KeepAlive");
		DOMElement* elemKeepAliveSendInterval;
		DOMElement* elemKeepAliveRecvInterval;
		elemKeepAliveSendInterval=createDOMElement(doc,"SendInterval");
		elemKeepAliveRecvInterval=createDOMElement(doc,"ReceiveInterval");
		elemKeepAlive->appendChild(elemKeepAliveSendInterval);
		elemKeepAlive->appendChild(elemKeepAliveRecvInterval);
		setDOMElementValue(elemKeepAliveSendInterval,u32KeepAliveSendInterval);
		setDOMElementValue(elemKeepAliveRecvInterval,u32KeepAliveRecvInterval);
		mixNode->appendChild(elemKeepAlive);
		CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Offering -- SendInterval %u -- Receive Interval %u\n",u32KeepAliveSendInterval,u32KeepAliveRecvInterval);

		/* append the terms and conditions, if there are any, to the KeyInfo
		 * Extensions, (nodes that can be removed from the KeyInfo without
		 * destroying the signature of the "Mix"-node).
		 */
		if(CALibProxytest::getOptions()->getTermsAndConditions() != NULL)
		{
			appendTermsAndConditionsExtension(doc, root);
			mixNode->appendChild(termsAndConditionsInfoNode(doc));
		}

		// create signature
		if(signXML(mixNode)!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo send to users...\n");
		}



		//UINT8* out=new UINT8[0xFFFF];
		//memset(out, 0, (sizeof(UINT8)*0xFFFF));
		UINT32 outlen = 0;
		UINT8* out = DOM_Output::dumpToMem(doc, &outlen);
#ifdef _DEBUG
		CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n", outlen);
#endif
		len = htonl(outlen);
		//memcpy(out,&len, sizeof(len));

		m_pMuxIn->getCASocket()->send((UINT8*) &len, sizeof(len));

		ret=m_pMuxIn->getCASocket()->send(out, outlen);
		delete[] out;
		out = NULL;
		if( (ret < 0) || (ret != outlen) )
		{
			CAMsg::printMsg(LOG_DEBUG,"Error sending new New Key Info\n");
			MONITORING_FIRE_NET_EVENT(ev_net_keyExchangeNextFailed);
			if (doc != NULL)
			{
				doc->release();
				doc = NULL;
			}
			return E_UNKNOWN;
		}
		MONITORING_FIRE_NET_EVENT(ev_net_keyExchangeNextSuccessful);
		CAMsg::printMsg(LOG_DEBUG,"Sending new key info succeeded\n");

		//Now receiving the symmetric key form Mix n-1
		m_pMuxIn->getCASocket()->receive((UINT8*) &len, sizeof(len));
		len = ntohl(len);
		recvBuff = new UINT8[len+1]; //for \0 at the end
		if(m_pMuxIn->getCASocket()->receive(recvBuff, len) != len)
		{
			MONITORING_FIRE_NET_EVENT(ev_net_keyExchangePrevFailed);
			CAMsg::printMsg(LOG_ERR,"Error receiving symmetric key from Mix n-1!\n");
			delete []recvBuff;
			recvBuff = NULL;
			if (doc != NULL)
			{
				doc->release();
				doc = NULL;
			}
			return E_UNKNOWN;
		}
		recvBuff[len]=0;
		CAMsg::printMsg(LOG_INFO,"Symmetric Key Info received is:\n");
		CAMsg::printMsg(LOG_INFO,"%s\n",(char*)recvBuff);
		//Parsing doc received
		doc=parseDOMDocument(recvBuff, len);
		delete[] recvBuff;
		recvBuff = NULL;
		if(doc==NULL)
		{
			MONITORING_FIRE_NET_EVENT(ev_net_keyExchangePrevFailed);
			CAMsg::printMsg(LOG_INFO,"Error parsing Key Info from previous Mix!\n");
			return E_UNKNOWN;
		}
		DOMElement* elemRoot=doc->getDocumentElement();
		//verify signature
		//CASignature oSig;
		CACertificate* pCert=CALibProxytest::getOptions()->getPrevMixTestCertificate();
		//oSig.setVerifyKey(pCert);
		result = CAMultiSignature::verifyXML(elemRoot, pCert);
		delete pCert;
		pCert = NULL;
		//if(oSig.verifyXML(elemRoot)!=E_SUCCESS)
		if(result != E_SUCCESS)
		{
			MONITORING_FIRE_NET_EVENT(ev_net_keyExchangePrevFailed);
			CAMsg::printMsg(LOG_CRIT,"Could not verify the symmetric key from previous Mix!\n");
			if (doc != NULL)
			{
				doc->release();
				doc = NULL;
			}
			return E_UNKNOWN;
		}

		 if ((result = checkCompatibility(elemRoot, "previous")) != E_SUCCESS)
			{
				if (doc != NULL)
				{
					doc->release();
					doc = NULL;
				}
				return result;
			}

		//Verifying nonce
		elemNonce=NULL;
		getDOMChildByName(elemRoot,"Nonce",elemNonce,false);
		tmpLen=50;
		memset(tmpBuff,0,tmpLen);
		if(elemNonce==NULL||getDOMElementValue(elemNonce,tmpBuff,&tmpLen)!=E_SUCCESS||
			CABase64::decode(tmpBuff,tmpLen,tmpBuff,&tmpLen)!=E_SUCCESS||
			tmpLen!=SHA_DIGEST_LENGTH ||
			memcmp(SHA1(arNonce,16,NULL),tmpBuff,SHA_DIGEST_LENGTH)!=0
			)
		{
			MONITORING_FIRE_NET_EVENT(ev_net_keyExchangePrevFailed);
			CAMsg::printMsg(LOG_CRIT,"Could not verify the Nonce!\n");
			if (doc != NULL)
			{
				doc->release();
				doc = NULL;
			}
			return E_UNKNOWN;
		}
		CAMsg::printMsg(LOG_INFO,"Verified the symmetric key!\n");

		UINT8 key[150];
		UINT32 keySize=150;

		ret=decodeXMLEncryptedKey(key,&keySize,elemRoot,m_pRSA);
		if(ret!=E_SUCCESS||keySize!=64)
		{
			MONITORING_FIRE_NET_EVENT(ev_net_keyExchangePrevFailed);
			CAMsg::printMsg(LOG_CRIT,"Could not set the symmetric key to be used by the MuxSocket!\n");
			if (doc != NULL)
			{
				doc->release();
				doc = NULL;
			}
			return E_UNKNOWN;
		}
		MONITORING_FIRE_NET_EVENT(ev_net_keyExchangePrevSuccessful);
		m_pMuxIn->setReceiveKey(key,32);
		m_pMuxIn->setSendKey(key+32,32);

		///Getting and calculating the KeepAlive Traffice...
		elemKeepAlive=NULL;
		elemKeepAliveSendInterval=NULL;
		elemKeepAliveRecvInterval=NULL;
		getDOMChildByName(elemRoot,"KeepAlive",elemKeepAlive);
		getDOMChildByName(elemKeepAlive,"SendInterval",elemKeepAliveSendInterval);
		getDOMChildByName(elemKeepAlive,"ReceiveInterval",elemKeepAliveRecvInterval);
		UINT32 tmpSendInterval,tmpRecvInterval;
		getDOMElementValue(elemKeepAliveSendInterval,tmpSendInterval,0xFFFFFFFF); //if now send interval was given set it to "infinite"
		getDOMElementValue(elemKeepAliveRecvInterval,tmpRecvInterval,0xFFFFFFFF); //if no recv interval was given --> set it to "infinite"
		CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Getting offer -- SendInterval %u -- Receive Interval %u\n",tmpSendInterval,tmpRecvInterval);
		m_u32KeepAliveSendInterval=max(u32KeepAliveSendInterval,tmpRecvInterval);
		if(m_u32KeepAliveSendInterval>10000)
			m_u32KeepAliveSendInterval-=10000; //make the send interval a little bit smaller than the related receive intervall
		m_u32KeepAliveRecvInterval=max(u32KeepAliveRecvInterval,tmpSendInterval);
		CAMsg::printMsg(LOG_DEBUG,"KeepAlive-Traffic: Calculated -- SendInterval %u -- Receive Interval %u\n",m_u32KeepAliveSendInterval,m_u32KeepAliveRecvInterval);

		if (doc != NULL)
		{
			doc->release();
			doc = NULL;
		}

		return E_SUCCESS;
	}

SINT32 CAMiddleMix::init()
	{
#ifdef DYNAMIC_MIX
		m_bBreakNeeded = m_bReconfigured;
#endif

		CAMsg::printMsg(LOG_INFO,"Creating Key...\n");
		m_pRSA=new CAASymCipher();
		if(m_pRSA->generateKeyPair(1024)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Init: Error generating Key-Pair...\n");
				return E_UNKNOWN;
			}

    // connect to next mix
		CASocketAddr* pAddrNext=NULL;
		for(UINT32 i=0;i<CALibProxytest::getOptions()->getTargetInterfaceCount();i++)
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
				CAMsg::printMsg(LOG_CRIT,"Init: Cannot create SOCKET for outgoing connection...\n");
				return E_UNKNOWN;
			}
		m_pMuxOut->getCASocket()->setRecvBuff(50*MIXPACKET_SIZE);
		m_pMuxOut->getCASocket()->setSendBuff(50*MIXPACKET_SIZE);


    CAMsg::printMsg(LOG_INFO,"Waiting for Connection from previous Mix...\n");
		CAListenerInterface* pListener=NULL;
		UINT32 interfaces=CALibProxytest::getOptions()->getListenerInterfaceCount();
		for(UINT32 i=1;i<=interfaces;i++)
			{
				pListener=CALibProxytest::getOptions()->getListenerInterface(i);
				if(!pListener->isVirtual())
					break;
				delete pListener;
				pListener=NULL;
			}
		if(pListener==NULL)
			{
				CAMsg::printMsg(LOG_CRIT," failed!\n");
				CAMsg::printMsg(LOG_CRIT,"Reason: no usable (non virtual) interface found!\n");
				return E_UNKNOWN;
			}
		const CASocketAddr* pAddr=NULL;
		pAddr=pListener->getAddr();
		delete pListener;
		pListener = NULL;
		m_pMuxIn=new CAMuxSocket();
#ifdef DYNAMIC_MIX
		// LERNGRUPPE Do not block if we are currently reconfiguring
		if(m_bBreakNeeded != m_bReconfigured)
		{
			return E_UNKNOWN;
		}
#endif
		SINT32 ret=m_pMuxIn->accept(*pAddr);
		delete pAddr;
		pAddr = NULL;
		if(ret!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error waiting for previous Mix... -- Exiting!\n");
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_INFO," connected!\n");
		MONITORING_FIRE_NET_EVENT(ev_net_prevConnected);

		m_pMuxIn->getCASocket()->setRecvBuff(50*MIXPACKET_SIZE);
		m_pMuxIn->getCASocket()->setSendBuff(50*MIXPACKET_SIZE);
		m_pMuxIn->getCASocket()->setKeepAlive((UINT32)1800);


		/** Connect to next mix */
		if(connectToNextMix(pAddrNext) != E_SUCCESS)
		{
			delete pAddrNext;
			pAddrNext = NULL;
			MONITORING_FIRE_NET_EVENT(ev_net_prevConnectionClosed);
			CAMsg::printMsg(LOG_DEBUG, "CAMiddleMix::init - Unable to connect to next mix\n");
			return E_UNKNOWN;
		}
		delete pAddrNext;
		pAddrNext = NULL;

//		mSocketGroup.add(muxOut);
		MONITORING_FIRE_NET_EVENT(ev_net_nextConnected);
		CAMsg::printMsg(LOG_INFO," connected!\n");

		m_pMuxOut->getCASocket()->setKeepAlive((UINT32)1800);


	    if((ret = processKeyExchange())!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,"Error in proccessKeyExchange()!\n");
				return ret;
		}

		m_pQueueSendToMixBefore=new CAQueue(sizeof(tQueueEntry));
		m_pQueueSendToMixAfter=new CAQueue(sizeof(tQueueEntry));

		m_pMuxInControlChannelDispatcher=new CAControlChannelDispatcher(m_pQueueSendToMixBefore,NULL,0);

#ifdef REPLAY_DETECTION
		m_pReplayDB=new CADatabase();
		m_pReplayDB->start();
		m_pReplayMsgProc=new CAReplayCtrlChannelMsgProc(this);
		m_pReplayMsgProc->startTimeStampPorpagation(REPLAY_TIMESTAMP_PROPAGATION_INTERVALL);
		m_u64ReferenceTime=time(NULL);
#endif
		m_pMiddleMixChannelList=new CAMiddleMixChannelList();

		return E_SUCCESS;
	}

/** UPSTREAM (to WEB) Take the packets from the Queue and write them to the Socket **/
THREAD_RETURN mm_loopSendToMixAfter(void* param)
	{
		INIT_STACK;
		BEGIN_STACK("CAFirstMix::fm_loopSendToMixAfter");

		CAMiddleMix* pMiddleMix=(CAMiddleMix*)param;
		CAQueue* pQueue=((CAMiddleMix*)param)->m_pQueueSendToMixAfter;
		CAMuxSocket* pMuxSocket=pMiddleMix->m_pMuxOut;

		UINT32 len;
		SINT32 ret;
		tPoolEntry* pPoolEntry=new tPoolEntry;
		MIXPACKET* pMixPacket=&pPoolEntry->packet;
		UINT32 u32KeepAliveSendInterval=pMiddleMix->m_u32KeepAliveSendInterval2;
		while(pMiddleMix->m_bRun)
			{
				len=sizeof(tPoolEntry);
				ret=pQueue->getOrWait((UINT8*)pPoolEntry,&len,u32KeepAliveSendInterval);
				if(!(pMiddleMix->m_bRun))
				{
					CAMsg::printMsg(LOG_INFO,"SendToMixAfter thread: was interrupted.\n");
					MONITORING_FIRE_NET_EVENT(ev_net_nextConnectionClosed);
					break;
				}
				if(ret==E_TIMEDOUT)
					{//send a dummy as keep-alive-traffic
						pMixPacket->flags=CHANNEL_DUMMY;
						pMixPacket->channel=DUMMY_CHANNEL;
						getRandom(pMixPacket->data,DATA_SIZE);
					}
				else if(ret!=E_SUCCESS||len!=sizeof(tQueueEntry))
					{
						MONITORING_FIRE_NET_EVENT(ev_net_nextConnectionClosed);
						CAMsg::printMsg(LOG_ERR,"CAFirstMix::lm_loopSendToMixAfter - Error in dequeueing MixPaket\n");
						CAMsg::printMsg(LOG_ERR,"ret=%i len=%i\n",ret,len);
						break;
					}
				if((pMuxSocket->send(pMixPacket)!=MIXPACKET_SIZE))
					{
						MONITORING_FIRE_NET_EVENT(ev_net_nextConnectionClosed);
						CAMsg::printMsg(LOG_ERR,"CAFirstMix::lm_loopSendToMixAfter - Error in sending MixPaket\n");
						break;
					}
#ifdef LOG_PACKET_TIMES
 				if(!isZero64(pPoolEntry->timestamp_proccessing_start))
					{
						getcurrentTimeMicros(pPoolEntry->timestamp_proccessing_end);
						pFirstMix->m_pLogPacketStats->addToTimeingStats(*pPoolEntry,pMixPacket->flags,true);
					}
#endif
			}
		pMiddleMix->m_bRun = false;
		delete pPoolEntry;
		pPoolEntry = NULL;
		FINISH_STACK("CAFirstMix::fm_loopSendToMixAfter");

		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread SendToMixAfter\n");
		THREAD_RETURN_SUCCESS;
	}

/** DOWNSTREAM (to Client) Take the packets from the Queue and write them to the Socket **/
THREAD_RETURN mm_loopSendToMixBefore(void* param)
	{
		INIT_STACK;
		BEGIN_STACK("CAFirstMix::fm_loopSendToMixBefore");

		CAMiddleMix* pMiddleMix=(CAMiddleMix*)param;
		CAQueue* pQueue=((CAMiddleMix*)param)->m_pQueueSendToMixBefore;
		CAMuxSocket* pMuxSocket=pMiddleMix->m_pMuxIn;

		UINT32 len;
		SINT32 ret;
		tPoolEntry* pPoolEntry=new tPoolEntry;
		MIXPACKET* pMixPacket=&pPoolEntry->packet;
		UINT32 u32KeepAliveSendInterval=pMiddleMix->m_u32KeepAliveSendInterval;
		while(pMiddleMix->m_bRun)
			{
				len=sizeof(tPoolEntry);
				ret=pQueue->getOrWait((UINT8*)pPoolEntry,&len,u32KeepAliveSendInterval);
				if(!(pMiddleMix->m_bRun))
				{
					CAMsg::printMsg(LOG_INFO,"SendToMixBefore thread: was interrupted.\n");
					break;
				}
				if(ret==E_TIMEDOUT)
					{//send a dummy as keep-alive-traffic
						pMixPacket->flags=CHANNEL_DUMMY;
						pMixPacket->channel=DUMMY_CHANNEL;
						getRandom(pMixPacket->data,DATA_SIZE);
					}
				else if(ret!=E_SUCCESS||len!=sizeof(tQueueEntry))
					{
						MONITORING_FIRE_NET_EVENT(ev_net_prevConnectionClosed);
						CAMsg::printMsg(LOG_ERR,"CAFirstMix::lm_loopSendToMixBefore - Error in dequeueing MixPaket\n");
						CAMsg::printMsg(LOG_ERR,"ret=%i len=%i\n",ret,len);
						break;
					}
				if((pMuxSocket->send(pMixPacket)!=MIXPACKET_SIZE))
					{
						MONITORING_FIRE_NET_EVENT(ev_net_prevConnectionClosed);
						CAMsg::printMsg(LOG_ERR,"CAFirstMix::lm_loopSendToMixBefore - Error in sending MixPaket\n");
						break;
					}
#ifdef LOG_PACKET_TIMES
 				if(!isZero64(pPoolEntry->timestamp_proccessing_start))
					{
						getcurrentTimeMicros(pPoolEntry->timestamp_proccessing_end);
						pFirstMix->m_pLogPacketStats->addToTimeingStats(*pPoolEntry,pMixPacket->flags,true);
					}
#endif
			}
		pMiddleMix->m_bRun = false;
		delete pPoolEntry;
		pPoolEntry = NULL;
		FINISH_STACK("CAFirstMix::fm_loopSendToMixBefore");

		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread SendToMixBefore\n");
		THREAD_RETURN_SUCCESS;
	}

#ifdef NEW_CHANNEL_ENCRYPTION
	#define MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS 2*KEY_SIZE
	#define MIDDLE_MIX_ASYM_PADDING_SIZE 42
#else
	#define MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS KEY_SIZE
	#define MIDDLE_MIX_ASYM_PADDING_SIZE 0
#endif

THREAD_RETURN mm_loopReadFromMixBefore(void* param)
	{
		CAMiddleMix* pMix=(CAMiddleMix*)param;
		HCHANNEL channelOut = 0, channelIn = 0;
		tPoolEntry* pPoolEntry=new tPoolEntry;
		MIXPACKET* pMixPacket=&pPoolEntry->packet;

		CAQueue* pQueue=pMix->m_pQueueSendToMixAfter;

		CASymCipher* pCipher;
		SINT32 ret;
		UINT8* tmpRSABuff=new UINT8[RSA_SIZE];
		UINT32 rsaOutLen=RSA_SIZE;
		CASingleSocketGroup oSocketGroup(false);
		oSocketGroup.add(*(pMix->m_pMuxIn));

		#ifdef USE_POOL
			CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		#endif

		while(pMix->m_bRun)
			{
				if(pQueue->getSize()>MAX_READ_FROM_PREV_MIX_QUEUE_SIZE)
				{
#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::Queue prev is full!\n");
#endif
					msSleep(200);
					continue;
				}
				#ifndef USE_POOL
					ret=oSocketGroup.select(1000);
				#else
					ret=oSocketGroup.select(MIX_POOL_TIMEOUT);
				#endif
				if(ret!=1)
					{
						if(ret==E_TIMEDOUT)
							{
								#ifndef USE_POOL
									continue;
								#else
									pMixPacket->channel=DUMMY_CHANNEL;
									pMixPacket->flags=CHANNEL_DUMMY;
									getRandom(pMixPacket->data,DATA_SIZE);
									pPool->pool(pPoolEntry);
									if(m_pMuxOut->send(pMixPacket)==SOCKET_ERROR)
										pMix->m_bRun=false;
								#endif
							}
						else
							{
								CAMsg::printMsg(LOG_CRIT,"loopUpStream -- Fehler bei select() -- goto ERR!\n");
								pMix->m_bRun=false;
								MONITORING_FIRE_NET_EVENT(ev_net_prevConnectionClosed);
							}
					}
				else
					{
						ret=pMix->m_pMuxIn->receive(pMixPacket);

						if ((ret!=SOCKET_ERROR)&&(pMixPacket->flags & ~CHANNEL_ALLOWED_FLAGS))
						{
							CAMsg::printMsg(LOG_INFO,"loopUpStream received a packet with invalid flags: %0X .  Removing them.\n",(pMixPacket->flags & ~CHANNEL_ALLOWED_FLAGS));
							pMixPacket->flags&=CHANNEL_ALLOWED_FLAGS;
						}

						if(ret==SOCKET_ERROR)
						{

							CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
							pMix->m_bRun=false;
							MONITORING_FIRE_NET_EVENT(ev_net_prevConnectionClosed);
						}
						#ifdef USE_POOL
						else if(pMixPacket->channel==DUMMY_CHANNEL)
						{
							pMixPacket->flags=CHANNEL_DUMMY;
							getRandom(pMixPacket->data,DATA_SIZE);
							pPool->pool(pPoolEntry);
							if(pMix->m_pMuxOut->send(pMixPacket)==SOCKET_ERROR)
								pMix->m_bRun=false;
						}
						#endif

						else //receive successful
						{
							channelIn = pMixPacket->channel;
							if(pMix->m_pMiddleMixChannelList->getInToOut(pMixPacket->channel,&channelOut,&pCipher)!=E_SUCCESS)
							{//new connection ?
								if(pMixPacket->flags & CHANNEL_OPEN) //if not channel-open flag set -->drop this packet
								{
									#ifdef _DEBUG
										CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
									#endif
									#ifdef NEW_CHANNEL_ENCRYPTION
										pMix->m_pRSA->decryptOAEP(pMixPacket->data,tmpRSABuff,&rsaOutLen);
									#else
										pMix->m_pRSA->decrypt(pMixPacket->data,tmpRSABuff);
									#endif
									#ifdef REPLAY_DETECTION
										// replace time(NULL) with the real timestamp ()
										// packet-timestamp + m_u64ReferenceTime
										UINT32 stamp=((UINT32)(tmpRSABuff[13]<<16)+(UINT32)(tmpRSABuff[14]<<8)+(UINT32)(tmpRSABuff[15]))*REPLAY_BASE;
										if(pMix->m_pReplayDB->insert(tmpRSABuff,stamp+pMix->m_u64ReferenceTime)!=E_SUCCESS)
//											if(pMix->m_pReplayDB->insert(tmpRSABuff,time(NULL))!=E_SUCCESS)
											{
												CAMsg::printMsg(LOG_INFO,"Replay: Duplicate packet ignored.\n");
												continue;
											}
									#endif

									pCipher=new CASymCipher();
									pCipher->setKeys(tmpRSABuff,MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS);
									pCipher->crypt1(pMixPacket->data+RSA_SIZE,
												pMixPacket->data+rsaOutLen-MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS,
												DATA_SIZE-RSA_SIZE);
									memcpy(pMixPacket->data,tmpRSABuff+MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS,rsaOutLen-MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS);
									getRandom(pMixPacket->data+DATA_SIZE-MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS-MIDDLE_MIX_ASYM_PADDING_SIZE,MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS+MIDDLE_MIX_ASYM_PADDING_SIZE);
									pMix->m_pMiddleMixChannelList->add(pMixPacket->channel,pCipher,&channelOut);
									pMixPacket->channel=channelOut;
									#ifdef USE_POOL
										pPool->pool(pPoolEntry);
									#endif
									#ifdef LOG_CRIME
									crimeSurveillanceUpstream(pMixPacket, channelIn);
									#endif
									pQueue->add(pPoolEntry,sizeof(tPoolEntry));
								}
							}
							else
							{//established connection
								pCipher->crypt1(pMixPacket->data,pMixPacket->data,DATA_SIZE);
								pCipher->unlock();
								#ifdef USE_POOL
									pPool->pool(pPoolEntry);
								#endif
								if((pMixPacket->flags&CHANNEL_CLOSE)!=0)
								{//Channel close received -->remove channel form channellist
									pMix->m_pMiddleMixChannelList->remove(pMixPacket->channel);
								}
								pMixPacket->channel=channelOut;
								#ifdef LOG_CRIME
								crimeSurveillanceUpstream(pMixPacket, channelIn);
								#endif
								pQueue->add(pPoolEntry,sizeof(tPoolEntry));
							}
						}
					}
			}

		CAMsg::printMsg(LOG_CRIT,"loopReadFromMixBefore -- Exiting clean ups...\n");
		pMix->m_bRun=false;
		UINT8 b[sizeof(tQueueEntry)+1];
		/* write bytes to the send queues to accelerate loop()-joins for the send threads*/
		if(pMix->m_pQueueSendToMixBefore!=NULL)
		{
			pMix->m_pQueueSendToMixBefore->add(b,sizeof(tQueueEntry)+1);
		}
		if(pMix->m_pQueueSendToMixAfter!=NULL)
		{
			pMix->m_pQueueSendToMixAfter->add(b,sizeof(tQueueEntry)+1);
		}
		delete tmpRSABuff;
		tmpRSABuff = NULL;
		delete pPoolEntry;
		pPoolEntry = NULL;
		#ifdef USE_POOL
			delete pPool;
			pPool = NULL;
		#endif
		CAMsg::printMsg(LOG_CRIT,"loopReadFromMixBefore -- Now Exiting!\n");
		THREAD_RETURN_SUCCESS;
	}

THREAD_RETURN mm_loopReadFromMixAfter(void* param)
	{
		CAMiddleMix* pMix=(CAMiddleMix*)param;
		HCHANNEL channelIn;
		CASymCipher* pCipher;

		tPoolEntry* pPoolEntry=new tPoolEntry;
		MIXPACKET* pMixPacket=&pPoolEntry->packet;

		SINT32 ret;
		CASingleSocketGroup oSocketGroup(false);
		oSocketGroup.add(*(pMix->m_pMuxOut));

		CAQueue* pQueue=pMix->m_pQueueSendToMixBefore;

#ifdef USE_POOL
		CAPool* pPool=new CAPool(MIX_POOL_SIZE);
#endif
		while(pMix->m_bRun)
			{
				if(pQueue->getSize()>MAX_READ_FROM_NEXT_MIX_QUEUE_SIZE)
				{
#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::Queue next is full!\n");
#endif
					msSleep(200);
					continue;
				}
				#ifndef USE_POOL
					ret=oSocketGroup.select(1000);
				#else
					ret=oSocketGroup.select(MIX_POOL_TIMEOUT);
				#endif
				if(ret!=1)
					{
						if(ret==E_TIMEDOUT)
							{
								#ifndef USE_POOL
									continue;
								#else
									pMixPacket->channel=DUMMY_CHANNEL;
									pMixPacket->flags=CHANNEL_DUMMY;
									getRandom(pMixPacket->data,DATA_SIZE);
									pPool->pool(pPoolEntry);
									if(pMix->m_pMuxIn->send(pMixPacket)==SOCKET_ERROR)
										pMix->m_bRun=false;
								#endif
							}
						else
							{
								CAMsg::printMsg(LOG_CRIT,"loopReadFromMixAfter -- Fehler bei select() -- goto ERR!\n");
								pMix->m_bRun=false;
								MONITORING_FIRE_NET_EVENT(ev_net_nextConnectionClosed);
							}
					}
				else
					{

						ret=pMix->m_pMuxOut->receive(pMixPacket);
						if ((ret!=SOCKET_ERROR)&&(pMixPacket->flags & ~CHANNEL_ALLOWED_FLAGS))
							{
								CAMsg::printMsg(LOG_INFO,"loopReadFromMixAfter -- received a packet with invalid flags: %0X .  Removing them.\n",(pMixPacket->flags & ~CHANNEL_ALLOWED_FLAGS));
								pMixPacket->flags&=CHANNEL_ALLOWED_FLAGS;
							}
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"loopReadFromMixAfter -- Fehler bei receive() -- goto ERR!\n");
								pMix->m_bRun=false;
								MONITORING_FIRE_NET_EVENT(ev_net_nextConnectionClosed);
							}
						#ifdef USE_POOL
						else if(pMixPacket->channel==DUMMY_CHANNEL)
							{
								pMixPacket->flags=CHANNEL_DUMMY;
								getRandom(pMixPacket->data,DATA_SIZE);
								pPool->pool(pPoolEntry);
								if(pMix->m_pMuxIn->send(pMixPacket)==SOCKET_ERROR)
									pMix->m_bRun=false;
							}
						#endif
						#ifdef REPLAY_DETECTION
						else if(pMixPacket->channel==REPLAY_CONTROL_CHANNEL_ID)
							{
								pQueue->add(pPoolEntry,sizeof(tPoolEntry));
							}
						#endif
						else if(pMix->m_pMiddleMixChannelList->getOutToIn(&channelIn,pMixPacket->channel,&pCipher)==E_SUCCESS)
							{//connection found
								HCHANNEL channelOut = pMixPacket->channel;
								pMixPacket->channel=channelIn;
								#ifdef LOG_CRIME
								if((pMixPacket->flags&CHANNEL_SIG_CRIME)==CHANNEL_SIG_CRIME)
								{
									getRandom(pMixPacket->data,DATA_SIZE);
									//Log in and out channel number, to allow
									CAMsg::printMsg(LOG_CRIT,"Detecting crime activity - previous mix channel: %u, "
											"next mix channel: %u\n", channelIn, channelOut);
								}
								else
								#endif
								pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
								pCipher->unlock();
								#ifdef USE_POOL
									pPool->pool(pPoolEntry);
								#endif
								if((pMixPacket->flags&CHANNEL_CLOSE)!=0)
									{//Channel close received -->remove channel form channellist
										pMix->m_pMiddleMixChannelList->remove(channelIn);
									}
								pQueue->add(pPoolEntry,sizeof(tPoolEntry));
							}
					}
			}

		CAMsg::printMsg(LOG_CRIT,"loopReadFromMixAfter -- Exiting clean ups...\n");
		pMix->m_bRun=false;
		UINT8 b[sizeof(tQueueEntry)+1];
		/* write bytes to the send queues to accelerate loop()-joins for the send threads*/
		if(pMix->m_pQueueSendToMixBefore!=NULL)
		{
			pMix->m_pQueueSendToMixBefore->add(b,sizeof(tQueueEntry)+1);
		}
		if(pMix->m_pQueueSendToMixAfter!=NULL)
		{
			pMix->m_pQueueSendToMixAfter->add(b,sizeof(tQueueEntry)+1);
		}
		delete pPoolEntry;
		pPoolEntry = NULL;
		#ifdef USE_POOL
			delete pPool;
			pPool = NULL;
		#endif
		CAMsg::printMsg(LOG_CRIT,"loopReadFromMixAfter -- Now Exiting!\n");
		THREAD_RETURN_SUCCESS;
	}

SINT32 CAMiddleMix::connectToNextMix(CASocketAddr* a_pAddrNext)
{
#define RETRIES 100
#define RETRYTIME 30
		CAMsg::printMsg(LOG_INFO,"Init: Try to connect to next Mix...\n");
		UINT32 i = 0;
		SINT32 err = E_UNKNOWN;
		for(i=0; i < RETRIES; i++)
		{
#ifdef DYNAMIC_MIX
			/** If someone reconfigured the mix, we need to break the loop as ne next mix might have changed */
			if(m_bBreakNeeded != m_bReconfigured)
			{
				CAMsg::printMsg(LOG_DEBUG, "CAMiddleMix::connectToNextMix - Broken the connect loop!\n");
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
				sSleep(RETRYTIME);
			}
			else
			{
				break;
			}
		}
		return err;
}

/** Processes Upstream-MixPackets.*/
SINT32 CAMiddleMix::loop()
	{

		CASingleSocketGroup oSocketGroup(false);
		oSocketGroup.add(*m_pMuxIn);
		m_pMuxIn->setCrypt(true);
		m_pMuxOut->setCrypt(true);

		m_bRun=true;

		CAThread oThread;
		oThread.setMainLoop(mm_loopReadFromMixAfter);
		oThread.start(this);

		CAThread bThread;
		bThread.setMainLoop(mm_loopSendToMixBefore);
		bThread.start(this);

		CAThread cThread;
		cThread.setMainLoop(mm_loopReadFromMixBefore);
		cThread.start(this);

		CAThread dThread;
		dThread.setMainLoop(mm_loopSendToMixAfter);
		dThread.start(this);

		cThread.join();

		CAMsg::printMsg(LOG_CRIT,"loop(): Preparing for restart...\n");
		m_bRun=false;

		oThread.join();
		bThread.join();
		dThread.join();

		m_pMuxIn->close();
		m_pMuxOut->close();

		CAMsg::printMsg(LOG_CRIT,"loop(): Seems that we are restarting now!!\n");
		return E_UNKNOWN;
	}
SINT32 CAMiddleMix::clean()
{
		delete m_pQueueSendToMixBefore;
		m_pQueueSendToMixBefore=NULL;

		delete m_pQueueSendToMixAfter;
		m_pQueueSendToMixAfter=NULL;

#ifdef REPLAY_DETECTION
		delete m_pReplayMsgProc;
		m_pReplayMsgProc=NULL;
#endif
		if(m_pMuxIn!=NULL)
		{
			m_pMuxIn->close();
			delete m_pMuxIn;
			m_pMuxIn=NULL;
		}

		if(m_pMuxOut!=NULL)
		{
			m_pMuxOut->close();
			delete m_pMuxOut;
			m_pMuxOut=NULL;
		}

		delete m_pRSA;
		m_pRSA=NULL;

		delete m_pMiddleMixChannelList;
		m_pMiddleMixChannelList=NULL;
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY

#ifdef LOG_CRIME
	static void crimeSurveillanceUpstream(MIXPACKET *pMixPacket, HCHANNEL prevMixChannel)
	{
		if(pMixPacket->flags & CHANNEL_SIG_CRIME)
		{
			CAMsg::printMsg(LOG_CRIT,"Crime detection: User surveillance, previous mix channel: %u, "
					"next mix channel %u\n", prevMixChannel, pMixPacket->channel);
		}
	}
#endif
