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
#include "CAMiddleMix.hpp"
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocketAddrUnix.hpp"
#include "CAThread.hpp"
#include "CAInfoService.hpp"
#include "CAUtil.hpp"
#include "CABase64.hpp"
#include "xml/DOM_Output.hpp"

extern CACmdLnOptions options;

SINT32 CAMiddleMix::initOnce()
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting MiddleMix InitOnce\n");
		m_pSignature=options.getSignKey();
		if(m_pSignature==NULL)
			{
				return E_UNKNOWN;
			}
		if(options.getListenerInterfaceCount()<1)
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
SINT32 CAMiddleMix::proccessKeyExchange()
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
		MemBufInputSource oInput(recvBuff,len,"tmpID");
		DOMParser oParser;
		oParser.parse(oInput);		
		DOM_Document doc=oParser.getDocument();
		delete []recvBuff;
		if(doc.isNull())
			{
				CAMsg::printMsg(LOG_INFO,"Error parsing Key Info from Mix n+1!\n");
				return E_UNKNOWN;
			}

		DOM_Element root=doc.getDocumentElement();
		
		//Finding first <Mix> entry and sending symetric key...
		bool bFoundNextMix=false;
		DOM_Node child=root.getFirstChild();
		while(child!=NULL)
			{
				if(child.getNodeName().equals("Mix"))
					{
						//check Signature....
						CASignature oSig;
						CACertificate* nextCert=options.getNextMixTestCertificate();
						oSig.setVerifyKey(nextCert);
						ret=oSig.verifyXML(child,NULL);
						delete nextCert;
						if(ret!=E_SUCCESS)
							{
								CAMsg::printMsg(LOG_INFO,"Could not verify Key Info from Mix n+1!\n");
								return E_UNKNOWN;
							}
						//extracting Nonce and computing Hash of them
						DOM_Element elemNonce;
						getDOMChildByName(child,(UINT8*)"Nonce",elemNonce,false);
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
						
						//Extracting PubKey of Mix n+1, generating SymKey for lin encryption
						//with Mix n+1, encrypt and send them
						DOM_Node rsaKey=child.getFirstChild();
						CAASymCipher oRSA;
						oRSA.setPublicKeyAsDOMNode(rsaKey);
						UINT8 key[64];
						getRandom(key,64);
						DOM_DocumentFragment docfragSymKey;
						encodeXMLEncryptedKey(key,64,docfragSymKey,&oRSA);
						DOM_Document docSymKey=DOM_Document::createDocument();
						docSymKey.appendChild(docSymKey.importNode(docfragSymKey,true));
						DOM_Element elemRoot=docSymKey.getDocumentElement();
						DOM_Element elemNonceHash=docSymKey.createElement("Nonce");
						setDOMElementValue(elemNonceHash,arNonce);						
						elemRoot.appendChild(elemNonceHash);
						m_pSignature->signXML(elemRoot);
						m_pMuxOut->setSendKey(key,32);
						m_pMuxOut->setReceiveKey(key+32,32);
						UINT32 outlen=0;
						UINT8* out=DOM_Output::dumpToMem(docSymKey,&outlen);
						UINT16 size=htons(outlen);
						((CASocket*)m_pMuxOut)->send((UINT8*)&size,2);
						((CASocket*)m_pMuxOut)->send(out,outlen);
						delete[] out;
						break;
					}
				child=child.getNextSibling();
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
		DOMString tmpDOMStr=root.getAttribute("count");
		char* tmpStr=tmpDOMStr.transcode();
		if(tmpStr==NULL)
			{
				return E_UNKNOWN;
			}
		UINT32 count=atol(tmpStr);
		delete tmpStr;
		count++;
		setDOMElementAttribute(root,"count",count);
		
		DOM_Element mixNode=doc.createElement(DOMString("Mix"));
		UINT8 tmpBuff[50];
		options.getMixId(tmpBuff,50); //the mix id...
		mixNode.setAttribute("id",DOMString((char*)tmpBuff));
		//Supported Mix Protocol -->currently "0.3"
		DOM_Element elemMixProtocolVersion=doc.createElement("MixProtocolVersion");
		mixNode.appendChild(elemMixProtocolVersion);
		setDOMElementValue(elemMixProtocolVersion,(UINT8*)"0.3");

		DOM_DocumentFragment pDocFragment;
		m_pRSA->getPublicKeyAsDocumentFragment(pDocFragment); //the key
		mixNode.appendChild(doc.importNode(pDocFragment,true));
		pDocFragment=0;
		//inserting Nonce
		DOM_Element elemNonce=doc.createElement("Nonce");
		UINT8 arNonce[16];
		getRandom(arNonce,16);
		UINT32 tmpLen=50;
		CABase64::encode(arNonce,16,tmpBuff,&tmpLen);
		tmpBuff[tmpLen]=0;
		setDOMElementValue(elemNonce,tmpBuff);
		mixNode.appendChild(elemNonce);
		
		m_pSignature->signXML(mixNode);
		
		root.insertBefore(mixNode,root.getFirstChild());

		UINT8* out=new UINT8[0xFFFF];
		UINT32 outlen=0xFFFD;
		DOM_Output::dumpToMem(doc,out+2,&outlen);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n",outlen);
		#endif
		len=htons(outlen);
		memcpy(out,&len,2);
		ret=((CASocket*)*m_pMuxIn)->send(out,outlen+2);
		delete[] out;
		if(ret!=E_SUCCESS)
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
		MemBufInputSource oInput1(recvBuff,len,"tmp");
		oParser.parse(oInput1);
		doc=oParser.getDocument();
		delete[] recvBuff;
		if(doc.isNull())
			{
				CAMsg::printMsg(LOG_INFO,"Error parsing Key Info from Mix n-1!\n");
				return E_UNKNOWN;
			}
		DOM_Element elemRoot=doc.getDocumentElement();
		//verify signature
		CASignature oSig;
		CACertificate* pCert=options.getPrevMixTestCertificate();
		oSig.setVerifyKey(pCert);
		delete pCert;
		if(oSig.verifyXML(elemRoot)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Couldt not verify the symetric key form Mix n-1!\n");		
				return E_UNKNOWN;
			}
		//Verifying nonce
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
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_INFO,"Verified the symetric key!\n");		

		UINT8 key[150];
		UINT32 keySize=150;
		decodeXMLEncryptedKey(key,&keySize,recvBuff,len,m_pRSA);
		if(keySize!=64)
			{
				CAMsg::printMsg(LOG_CRIT,"Couldt not set the symetric key to be used by the MuxSocket!\n");		
				return E_UNKNOWN;
			}
		m_pMuxIn->setReceiveKey(key,32);
		m_pMuxIn->setSendKey(key+32,32);
		return E_SUCCESS;
	}

SINT32 CAMiddleMix::init()
	{		
		CAMsg::printMsg(LOG_INFO,"Creating Key...\n");
		m_pRSA=new CAASymCipher();
		if(m_pRSA->generateKeyPair(1024)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Init: Error generating Key-Pair...\n");
				return E_UNKNOWN;		
			}
		
		CASocketAddr* pAddrNext=NULL;
		for(UINT32 i=0;i<options.getTargetInterfaceCount();i++)
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

		if(((CASocket*)*m_pMuxOut)->create(pAddrNext->getType())!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Init: Cannot create SOCKET for outgoing conncetion...\n");
				return E_UNKNOWN;
			}
		((CASocket*)*m_pMuxOut)->setRecvBuff(50*MIXPACKET_SIZE);
		((CASocket*)*m_pMuxOut)->setSendBuff(50*MIXPACKET_SIZE);
#define RETRIES 100
#define RETRYTIME 30
		CAMsg::printMsg(LOG_INFO,"Init: Try to connect to next Mix...\n");
		if(m_pMuxOut->connect(*pAddrNext,RETRIES,RETRYTIME)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix -- Exiting!\n");
				return E_UNKNOWN;
			}
//		mSocketGroup.add(muxOut);
		CAMsg::printMsg(LOG_INFO," connected!\n");
		if(((CASocket*)*m_pMuxOut)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)*m_pMuxOut)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}
		
		ListenerInterface oListener;
		options.getListenerInterface(oListener,1);
		m_pMuxIn=new CAMuxSocket();
		if(m_pMuxIn->accept(*oListener.addr)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error waiting for previous Mix... -- Exiting!\n");				
				return E_UNKNOWN;
			}
		((CASocket*)*m_pMuxIn)->setRecvBuff(50*MIXPACKET_SIZE);
		((CASocket*)*m_pMuxIn)->setSendBuff(50*MIXPACKET_SIZE);
		if(((CASocket*)*m_pMuxIn)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)*m_pMuxIn)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}
		

		proccessKeyExchange();

		m_pMiddleMixChannelList=new CAMiddleMixChannelList();
		
		return E_SUCCESS;
	}

	
THREAD_RETURN loopDownStream(void *p)
	{
		CAMiddleMix* pMix=(CAMiddleMix*)p;
		HCHANNEL channelIn;
		CASymCipher* pCipher;
		MIXPACKET* pMixPacket=new MIXPACKET;
		SINT32 ret;
		CASingleSocketGroup oSocketGroup;
		oSocketGroup.add(*(pMix->m_pMuxOut));
		for(;;)
			{
				ret=oSocketGroup.select(false,1000);
				if(ret!=1)
					{
						if(ret==E_TIMEDOUT)
							continue;
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
						
						if(pMix->m_pMiddleMixChannelList->getOutToIn(&channelIn,pMixPacket->channel,&pCipher)==E_SUCCESS)
							{//connection found
								if(pMixPacket->flags!=CHANNEL_CLOSE)
									{
										pMixPacket->channel=channelIn;
										pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										pCipher->unlock();
										if(pMix->m_pMuxIn->send(pMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
								else
									{//connection should be closed
										pCipher->unlock();
										if(pMix->m_pMuxIn->close(channelIn)==SOCKET_ERROR)
											goto ERR;
										pMix->m_pMiddleMixChannelList->remove(channelIn);
									}
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Exiting!\n");
		delete pMixPacket;
		pMix->m_pMuxIn->close();
		pMix->m_pMuxOut->close();
		THREAD_RETURN_SUCCESS;		
	}


SINT32 CAMiddleMix::loop()
	{
		CAInfoService* pInfoService=NULL;
		if(m_pSignature!=NULL&&options.isInfoServiceEnabled())
			{
				pInfoService=new CAInfoService();
				pInfoService->setSignature(m_pSignature);
				pInfoService->sendHelo();
				pInfoService->start();
			}
		MIXPACKET* pMixPacket=new MIXPACKET;
		HCHANNEL channelOut;
		CASymCipher* pCipher;
		SINT32 ret;
		UINT8* tmpRSABuff=new UINT8[RSA_SIZE];
		CASingleSocketGroup oSocketGroup;
		oSocketGroup.add(*m_pMuxIn);
		m_pMuxIn->setCrypt(true);
		m_pMuxOut->setCrypt(true);
		CAThread oThread;
		oThread.setMainLoop(loopDownStream);
		oThread.start(this);
		for(;;)
			{
				ret=oSocketGroup.select(false,1000);
				if(ret!=1)
					{
						if(ret==E_TIMEDOUT)
							continue;
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
						if(m_pMiddleMixChannelList->getInToOut(pMixPacket->channel,&channelOut,&pCipher)!=E_SUCCESS)
							{//new connection
								if(pMixPacket->flags==CHANNEL_OPEN)
									{
										#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										pCipher=new CASymCipher();
										m_pRSA->decrypt(pMixPacket->data,tmpRSABuff);
										pCipher->setKeyAES(tmpRSABuff);
										pCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																				pMixPacket->data+RSA_SIZE-KEY_SIZE,
																				DATA_SIZE-RSA_SIZE);
										memcpy(pMixPacket->data,tmpRSABuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
										m_pMiddleMixChannelList->add(pMixPacket->channel,pCipher,&channelOut);
										pMixPacket->channel=channelOut;
										if(m_pMuxOut->send(pMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
							}
						else
							{//established connection
								if(pMixPacket->flags==CHANNEL_CLOSE)
									{
										pCipher->unlock();
										if(m_pMuxOut->close(channelOut)==SOCKET_ERROR)
											goto ERR;
										m_pMiddleMixChannelList->remove(pMixPacket->channel);
									}
								else
									{
										pMixPacket->channel=channelOut;
										pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										pCipher->unlock();
										if(m_pMuxOut->send(pMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"Preparing for restart...\n");
		m_pMuxIn->close();
		m_pMuxOut->close();
		oThread.join();		

		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		delete tmpRSABuff;
		delete pMixPacket;
		if(pInfoService!=NULL)
			delete pInfoService;
		return E_UNKNOWN;
	}
SINT32 CAMiddleMix::clean()
	{
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
