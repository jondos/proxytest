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

extern CACmdLnOptions* pglobalOptions;

SINT32 CAMiddleMix::initOnce()
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting MiddleMix InitOnce\n");
		m_pSignature=pglobalOptions->getSignKey();
		if(m_pSignature==NULL)
			{
				return E_UNKNOWN;
			}
		if(pglobalOptions->getListenerInterfaceCount()<1)
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
		UINT16 len;
		SINT32 ret;

		if(((CASocket*)*m_pMuxOut)->receiveFully((UINT8*)&len,2)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Error receiving Key Info lenght from Mix n+1!\n");
				return E_UNKNOWN;
			}
		len=ntohs(len);
		CAMsg::printMsg(LOG_INFO,"Received Key Info lenght %u\n",len);
		recvBuff=new UINT8[len+1]; //for the \0 at the end
		if(recvBuff==NULL)
			return E_UNKNOWN;
		if(((CASocket*)*m_pMuxOut)->receiveFully(recvBuff,len)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Error receiving Key Info from Mix n+1!\n");
				delete []recvBuff;
				return E_UNKNOWN;
			}
		recvBuff[len]=0; //make a string
		CAMsg::printMsg(LOG_INFO,"Received Key Info...\n");
		CAMsg::printMsg(LOG_INFO,"%s\n",recvBuff);
		
		//Parsing KeyInfo received from Mix n+1
		XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(recvBuff,len);
		delete []recvBuff;
		if(doc==NULL)
			{
				CAMsg::printMsg(LOG_INFO,"Error parsing Key Info from Mix n+1!\n");
				return E_UNKNOWN;
			}

		DOMElement* root=doc->getDocumentElement();
		
		//Finding first <Mix> entry and sending symetric key...
		bool bFoundNextMix=false;
		DOMNode* child=root->getFirstChild();
		while(child!=NULL)
			{
				if(equals(child->getNodeName(),"Mix"))
					{
						//check Signature....
						CASignature oSig;
						CACertificate* nextCert=pglobalOptions->getNextMixTestCertificate();
						oSig.setVerifyKey(nextCert);
						ret=oSig.verifyXML(child,NULL);
						delete nextCert;
						if(ret!=E_SUCCESS)
							{
								CAMsg::printMsg(LOG_INFO,"Could not verify Key Info from Mix n+1!\n");
								return E_UNKNOWN;
							}
						//extracting Nonce and computing Hash of them
						DOMElement* elemNonce;
						getDOMChildByName(child,"Nonce",elemNonce,false);
						UINT32 lenNonce=1024;
						UINT8 arNonce[1024];
						UINT32 tmpLen=1024;
						if(elemNonce==NULL)
							{
								CAMsg::printMsg(LOG_INFO,"No nonce found in Key Info from Mix n+1!\n");
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
						DOMElement* elemNonceHash=createDOMElement(docSymKey,"Nonce");
						setDOMElementValue(elemNonceHash,arNonce);						
						elemRoot->appendChild(elemNonceHash);
						m_pSignature->signXML(elemRoot);
						m_pMuxOut->setSendKey(key,32);
						m_pMuxOut->setReceiveKey(key+32,32);
						UINT32 outlen=0;
						UINT8* out=DOM_Output::dumpToMem(docSymKey,&outlen);
						UINT16 size=htons((UINT16)outlen);
						((CASocket*)m_pMuxOut)->send((UINT8*)&size,2);
						((CASocket*)m_pMuxOut)->send(out,outlen);
						docSymKey->release();
						delete[] out;
						bFoundNextMix=true;
						break;
					}
				child=child->getNextSibling();
			}
		if(!bFoundNextMix)
			{
				CAMsg::printMsg(LOG_INFO,"Error -- no Key Info from Mix n+1 found!\n");
				return E_UNKNOWN;
			}
		// -----------------------------------------
		// ---- Start exchange with Mix n-1 --------
		// -----------------------------------------		
		//Inserting own (key) info
		UINT32 count=0;
		if(getDOMElementAttribute(root,"count",count)!=E_SUCCESS)
			{
				return E_UNKNOWN;
			}
		count++;
		setDOMElementAttribute(root,"count",count);
		
		addMixInfo(root, true);
		DOMElement* mixNode;
		getDOMChildByName(root, "Mix", mixNode, false);
		
		
		UINT8 tmpBuff[50];
		pglobalOptions->getMixId(tmpBuff,50); //the mix id...
		setDOMElementAttribute(mixNode,"id",tmpBuff);
		//Supported Mix Protocol -->currently "0.3"
		DOMElement* elemMixProtocolVersion=createDOMElement(doc,"MixProtocolVersion");
		mixNode->appendChild(elemMixProtocolVersion);
		setDOMElementValue(elemMixProtocolVersion,(UINT8*)"0.3");

		DOMElement* elemKey=NULL;
		m_pRSA->getPublicKeyAsDOMElement(elemKey,doc); //the key
		mixNode->appendChild(elemKey);
		//inserting Nonce
		DOMElement* elemNonce=createDOMElement(doc,"Nonce");
		UINT8 arNonce[16];
		getRandom(arNonce,16);
		UINT32 tmpLen=50;
		CABase64::encode(arNonce,16,tmpBuff,&tmpLen);
		tmpBuff[tmpLen]=0;
		setDOMElementValue(elemNonce,tmpBuff);
		mixNode->appendChild(elemNonce);
		
		// create signature
		if(signXML(mixNode)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo send to users...\n");
			}
		
		

		UINT8* out=new UINT8[0xFFFF];
		UINT32 outlen=0xFFFD;
		DOM_Output::dumpToMem(doc,out+2,&outlen);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n",outlen);
		#endif
		len=htons((UINT16)outlen);
		memcpy(out,&len,2);
		ret=((CASocket*)*m_pMuxIn)->send(out,outlen+2);
		delete[] out;
		if(ret<0||(UINT32)ret!=outlen+2)
			{
				CAMsg::printMsg(LOG_DEBUG,"Error sending new New Key Info\n");
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_DEBUG,"Sending new New Key Info succeded\n");
		
			//Now receiving the symmetric key form Mix n-1
		((CASocket*)*m_pMuxIn)->receive((UINT8*)&len,2);
		len=ntohs(len);
		recvBuff=new UINT8[len+1]; //for \0 at the end
		if(((CASocket*)*m_pMuxIn)->receive(recvBuff,len)!=len)
			{
				CAMsg::printMsg(LOG_ERR,"Error receiving symetric key from Mix n-1!\n");
				delete []recvBuff;
				return E_UNKNOWN;
			}
		recvBuff[len]=0;
		CAMsg::printMsg(LOG_INFO,"Symmetric Key Info received is:\n");
		CAMsg::printMsg(LOG_INFO,"%s\n",(char*)recvBuff);		
		//Parsing doc received
		doc=parseDOMDocument(recvBuff,len);
		delete[] recvBuff;
		if(doc==NULL)
			{
				CAMsg::printMsg(LOG_INFO,"Error parsing Key Info from Mix n-1!\n");
				return E_UNKNOWN;
			}
		DOMElement* elemRoot=doc->getDocumentElement();
		//verify signature
		CASignature oSig;
		CACertificate* pCert=pglobalOptions->getPrevMixTestCertificate();
		oSig.setVerifyKey(pCert);
		delete pCert;
		if(oSig.verifyXML(elemRoot)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not verify the symetric key form Mix n-1!\n");		
				return E_UNKNOWN;
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
				CAMsg::printMsg(LOG_CRIT,"Could not verify the Nonce!\n");		
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_INFO,"Verified the symmetric key!\n");		

		UINT8 key[150];
		UINT32 keySize=150;
	
		ret=decodeXMLEncryptedKey(key,&keySize,elemRoot,m_pRSA);
		if(ret!=E_SUCCESS||keySize!=64)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not set the symmetric key to be used by the MuxSocket!\n");		
				return E_UNKNOWN;
			}
		m_pMuxIn->setReceiveKey(key,32);
		m_pMuxIn->setSendKey(key+32,32);
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
		for(UINT32 i=0;i<pglobalOptions->getTargetInterfaceCount();i++)
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

		if(((CASocket*)*m_pMuxOut)->create(pAddrNext->getType())!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Init: Cannot create SOCKET for outgoing conncetion...\n");
				return E_UNKNOWN;
			}
		((CASocket*)*m_pMuxOut)->setRecvBuff(50*MIXPACKET_SIZE);
		((CASocket*)*m_pMuxOut)->setSendBuff(50*MIXPACKET_SIZE);

		
    CAMsg::printMsg(LOG_INFO,"Waiting for Connection from previous Mix...\n");    
		CAListenerInterface* pListener=NULL;
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
#ifdef DYNAMIC_MIX
		// LERNGRUPPE Do not block if we are currently reconfiguring
		if(m_bBreakNeeded != m_bReconfigured)
		{
			return E_UNKNOWN;
		}
#endif
		SINT32 ret=m_pMuxIn->accept(*pAddr);
		delete pAddr;
		if(ret!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error waiting for previous Mix... -- Exiting!\n");				
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_INFO," connected!\n");
		((CASocket*)*m_pMuxIn)->setRecvBuff(50*MIXPACKET_SIZE);
		((CASocket*)*m_pMuxIn)->setSendBuff(50*MIXPACKET_SIZE);
		if(((CASocket*)*m_pMuxIn)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)*m_pMuxIn)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}
		
		/** Connect to next mix */
		if(connectToNextMix(pAddrNext) != E_SUCCESS)
		{
			delete pAddrNext;
			CAMsg::printMsg(LOG_DEBUG, "CAMiddleMix::init - Unable to connect to next mix\n");
			return E_UNKNOWN;
		}
		delete pAddrNext;

//		mSocketGroup.add(muxOut);
		CAMsg::printMsg(LOG_INFO," connected!\n");
		if(((CASocket*)*m_pMuxOut)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)*m_pMuxOut)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}

    if((ret = processKeyExchange())!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error in proccessKeyExchange()!\n");
        return ret;
			}
	

		m_pMiddleMixChannelList=new CAMiddleMixChannelList();
		
		return E_SUCCESS;
	}

/** Processes Downstream-MixPackets.*/	
THREAD_RETURN mm_loopDownStream(void *p)
	{
		CAMiddleMix* pMix=(CAMiddleMix*)p;
		HCHANNEL channelIn;
		CASymCipher* pCipher;
		tPoolEntry* pPoolEntry=new tPoolEntry;
		MIXPACKET* pMixPacket=&pPoolEntry->packet;
		SINT32 ret;
		CASingleSocketGroup oSocketGroup(false);
		oSocketGroup.add(*(pMix->m_pMuxOut));
#ifdef USE_POOL		
		CAPool* pPool=new CAPool(MIX_POOL_SIZE);
#endif
		while(pMix->m_bRun)
			{
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
										goto ERR;								
								#endif
							}
						else
							{
								CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Fehler bei select() -- goto ERR!\n");
								goto ERR;
							}
					}
				else
					{
						ret=pMix->m_pMuxOut->receive(pMixPacket);
						if(ret==SOCKET_ERROR)
								{
									CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Fehler bei receive() -- goto ERR!\n");
									goto ERR;
								}
						if (pMixPacket->flags & ~CHANNEL_ALLOWED_FLAGS)
							{
								CAMsg::printMsg(LOG_INFO,"loopDownStream received a packet with invalid flags: %0X .  Removing them.\n",(pMixPacket->flags & ~CHANNEL_ALLOWED_FLAGS));
								pMixPacket->flags&=CHANNEL_ALLOWED_FLAGS;
							}
						#ifdef USE_POOL	
							if(pMixPacket->channel==DUMMY_CHANNEL)
								{
									pMixPacket->flags=CHANNEL_DUMMY;
									getRandom(pMixPacket->data,DATA_SIZE);
									pPool->pool(pPoolEntry);
									if(pMix->m_pMuxIn->send(pMixPacket)==SOCKET_ERROR)
										goto ERR;								
								}
							else
						#endif
						if(pMix->m_pMiddleMixChannelList->getOutToIn(&channelIn,pMixPacket->channel,&pCipher)==E_SUCCESS)
							{//connection found
								pMixPacket->channel=channelIn;
								#ifdef LOG_CRIME
								if((pMixPacket->flags&CHANNEL_SIG_CRIME)==CHANNEL_SIG_CRIME)
									getRandom(pMixPacket->data,DATA_SIZE);
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
								if(pMix->m_pMuxIn->send(pMixPacket)==SOCKET_ERROR)
									goto ERR;
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Exiting clean ups...\n");
		pMix->m_bRun=false;
		delete pPoolEntry;
		#ifdef USE_POOL
			delete pPool;
		#endif
		pMix->m_pMuxIn->close();
		pMix->m_pMuxOut->close();
		CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Now Exiting!\n");
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
		tPoolEntry* pPoolEntry=new tPoolEntry;
		MIXPACKET* pMixPacket=&pPoolEntry->packet;
		HCHANNEL channelOut;
		CASymCipher* pCipher;
		SINT32 ret;
		UINT8* tmpRSABuff=new UINT8[RSA_SIZE];
		CASingleSocketGroup oSocketGroup(false);
		oSocketGroup.add(*m_pMuxIn);
		m_pMuxIn->setCrypt(true);
		m_pMuxOut->setCrypt(true);
		#ifdef USE_POOL		
			CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		#endif
		m_bRun=true;
		CAThread oThread;
		oThread.setMainLoop(mm_loopDownStream);
		oThread.start(this);
		while(m_bRun)
			{
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
										goto ERR;								
								#endif
							}
						else
							{
								CAMsg::printMsg(LOG_CRIT,"loopUpStream -- Fehler bei select() -- goto ERR!\n");
								goto ERR;
							}
					}
				else
					{
						ret=m_pMuxIn->receive(pMixPacket);
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								goto ERR;
							}
						if (pMixPacket->flags & ~CHANNEL_ALLOWED_FLAGS)
							{
								CAMsg::printMsg(LOG_INFO,"loopUpStream received a packet with invalid flags: %0X .  Removing them.\n",(pMixPacket->flags & ~CHANNEL_ALLOWED_FLAGS));
								pMixPacket->flags&=CHANNEL_ALLOWED_FLAGS;
							}
						#ifdef USE_POOL	
							if(pMixPacket->channel==DUMMY_CHANNEL)
								{
									pMixPacket->flags=CHANNEL_DUMMY;
									getRandom(pMixPacket->data,DATA_SIZE);
									pPool->pool(pPoolEntry);
									if(m_pMuxOut->send(pMixPacket)==SOCKET_ERROR)
										goto ERR;								
								}
							else
						#endif
						if(m_pMiddleMixChannelList->getInToOut(pMixPacket->channel,&channelOut,&pCipher)!=E_SUCCESS)
							{//new connection ?
								if(pMixPacket->flags==CHANNEL_OPEN) //if not channel-open flag set -->drop this packet
									{
										#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										m_pRSA->decrypt(pMixPacket->data,tmpRSABuff);
										#ifdef REPLAY_DETECTION
											if(m_pReplayDB->insert(tmpRSABuff)!=E_SUCCESS)
												{
													CAMsg::printMsg(LOG_INFO,"Replay: Duplicate packet ignored.\n");
													continue;
												}
										#endif
										pCipher=new CASymCipher();
										pCipher->setKey(tmpRSABuff);
										pCipher->crypt1(pMixPacket->data+RSA_SIZE,
																				pMixPacket->data+RSA_SIZE-KEY_SIZE,
																				DATA_SIZE-RSA_SIZE);
										memcpy(pMixPacket->data,tmpRSABuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
										getRandom(pMixPacket->data+DATA_SIZE-KEY_SIZE,KEY_SIZE);
										m_pMiddleMixChannelList->add(pMixPacket->channel,pCipher,&channelOut);
										pMixPacket->channel=channelOut;
										#ifdef USE_POOL
											pPool->pool(pPoolEntry);
										#endif
										if(m_pMuxOut->send(pMixPacket)==SOCKET_ERROR)
											goto ERR;
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
											m_pMiddleMixChannelList->remove(pMixPacket->channel);
										}
									pMixPacket->channel=channelOut;
									if(m_pMuxOut->send(pMixPacket)==SOCKET_ERROR)
										goto ERR;
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"loop(): Preparing for restart...\n");
		m_bRun=false;
		m_pMuxIn->close();
		m_pMuxOut->close();
		oThread.join();		

		delete tmpRSABuff;
		delete pPoolEntry;
		#ifdef USE_POOL
			delete pPool;
		#endif
		CAMsg::printMsg(LOG_CRIT,"loop(): Seams that we are restarting now!!\n");
		return E_UNKNOWN;
	}
SINT32 CAMiddleMix::clean()
{
    /*
		if(m_pInfoService!=NULL)
    {
        m_pInfoService->stop();
			delete m_pInfoService;
		m_pInfoService=NULL;
    }
    */
		if(m_pMuxIn!=NULL)
			{
				m_pMuxIn->close();
				delete m_pMuxIn;
			}
		m_pMuxIn=NULL;
		if(m_pMuxOut!=NULL)
			{
				m_pMuxOut->close();
				delete m_pMuxOut;
			}
		m_pMuxOut=NULL;
		if(m_pRSA!=NULL)
			delete m_pRSA;
		m_pRSA=NULL;
		if(m_pMiddleMixChannelList!=NULL)
			delete m_pMiddleMixChannelList;
		m_pMiddleMixChannelList=NULL;
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
