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
#include "CALastMix.hpp"
#include "CALastMixChannelList.hpp"
#include "CASocketGroup.hpp"
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CASocketAddrUnix.hpp"
#include "CASocketAddrINet.hpp"
#include "CAUtil.hpp"
#include "CAInfoService.hpp"
#include "CACertStore.hpp"
#include "CABase64.hpp"
#include "CAPool.hpp"
#include "xml/DOM_Output.hpp"

extern CACmdLnOptions options;

#ifdef LOG_CHANNEL
	#define MACRO_DO_LOG_CHANNEL\
		getcurrentTimeMillis(current_millis);\
		diff_time=diff64(current_millis,pChannelListEntry->timeCreated);\
		CAMsg::printMsg(LOG_DEBUG,"Channel %u closed - Start %Lu - End %Lu - Time [ms] - %u, Upload - %u, Download - %u, DataPacketsFromUser %u, DataPacketsToUser %u\n",\
			pChannelListEntry->channelIn,pChannelListEntry->timeCreated,\
			current_millis,diff_time,pChannelListEntry->trafficInFromUser,pChannelListEntry->trafficOutToUser,\
			pChannelListEntry->packetsDataInFromUser,pChannelListEntry->packetsDataOutToUser); 
#endif

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
		
		UINT8 strTarget[255];
		options.getSOCKSHost(strTarget,255);
		maddrSocks.setAddr(strTarget,options.getSOCKSPort());

		m_pSignature=options.getSignKey();
		if(m_pSignature==NULL)
			return E_UNKNOWN;
		if(options.getListenerInterfaceCount()<1)
			{
				CAMsg::printMsg(LOG_CRIT,"No ListenerInterfaces specified!\n");
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

SINT32 CALastMix::init()
	{
		if(mRSA.generateKeyPair(1024)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not generate a valid key pair\n");
				return E_UNKNOWN;
			}

		CAMsg::printMsg(LOG_INFO,"Waiting for Connection from previous Mix...\n");
/*		CASocketAddr* pAddrListen;
		UINT8 path[255];
		if(options.getServerHost(path,255)==E_SUCCESS&&path[0]=='/') //unix domain
			{
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
				pAddrListen=new CASocketAddrUnix();
				((CASocketAddrUnix*)pAddrListen)->setPath((char*)path);
#else
				CAMsg::printMsg(LOG_CRIT,"I do not understand the Unix Domain Protocol!\n");
				return E_UNKNOWN;
#endif
			}
		else
			{
				pAddrListen=new CASocketAddrINet();
				((CASocketAddrINet*)pAddrListen)->setAddr(path,options.getServerPort());
			}
			*/
		ListenerInterface oListener;
		options.getListenerInterface(oListener,1);
		m_pMuxIn=new CAMuxSocket();
		if(m_pMuxIn->accept(*oListener.addr)!=E_SUCCESS)
		    {
					delete oListener.addr;
					CAMsg::printMsg(LOG_CRIT," failed!\n");
					return E_UNKNOWN;
		    }
		delete oListener.addr;
		((CASocket*)*m_pMuxIn)->setRecvBuff(500*MIXPACKET_SIZE);
		((CASocket*)*m_pMuxIn)->setSendBuff(500*MIXPACKET_SIZE);
		if(((CASocket*)*m_pMuxIn)->setSendLowWat(MIXPACKET_SIZE)!=E_SUCCESS)
			CAMsg::printMsg(LOG_INFO,"SOCKET Option SENDLOWWAT not set!\n");
		if(((CASocket*)*m_pMuxIn)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)*m_pMuxIn)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}
		
		CAMsg::printMsg(LOG_INFO,"connected!\n");
#ifdef LOG_CRIME
		m_nCrimeRegExp=0;
		m_pCrimeRegExps=options.getCrimeRegExps(&m_nCrimeRegExp);
#endif	
		return processKeyExchange();
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
		doc.appendChild(elemMixes);
		DOM_Element elemMix=doc.createElement("Mix");
		UINT8 idBuff[50];
		options.getMixId(idBuff,50);
		elemMix.setAttribute("id",(char*)idBuff);
		elemMixes.appendChild(elemMix);

		//Inserting MixProtocol Version (0.3)
		DOM_Element elemMixProtocolVersion=doc.createElement("MixProtocolVersion");
		elemMix.appendChild(elemMixProtocolVersion);
		setDOMElementValue(elemMixProtocolVersion,(UINT8*)"0.3");

		//Inserting RSA-Key
		DOM_DocumentFragment tmpDocFrag;
		mRSA.getPublicKeyAsDocumentFragment(tmpDocFrag);
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
		
		CACertificate* ownCert=options.getOwnCertificate();
		if(ownCert==NULL)
			{
				CAMsg::printMsg(LOG_DEBUG,"Own Test Cert is NULL -- so it could not be inserted into signed KeyInfo send to users...\n");
			}	
		CACertStore* tmpCertStore=new CACertStore();
		tmpCertStore->add(ownCert);
		if(m_pSignature->signXML(elemMix,tmpCertStore)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo send to users...\n");
			}
		delete ownCert;
		delete tmpCertStore;
		
		UINT32 len=0;
		UINT8* messageBuff=DOM_Output::dumpToMem(doc,&len);
		UINT16 tmp=htons(len);
		CAMsg::printMsg(LOG_INFO,"Sending Infos (chain length and RSA-Key, Message-Size %u)\n",len);
		
		if(	((CASocket*)*m_pMuxIn)->send((UINT8*)&tmp,2)!=2 ||
				((CASocket*)*m_pMuxIn)->send(messageBuff,len)!=(SINT32)len)
			{
				CAMsg::printMsg(LOG_ERR,"Error sending Key-Info!\n");
				delete []messageBuff;
				return E_UNKNOWN;
			}
		delete messageBuff;
		
		//Now receiving the symmetric key
		((CASocket*)*m_pMuxIn)->receive((UINT8*)&tmp,2);
		len=ntohs(tmp);
		messageBuff=new UINT8[len+1]; //+1 for the closing Zero
		if(((CASocket*)*m_pMuxIn)->receive(messageBuff,len)!=(SINT32)len)
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
		CACertificate* pCert=options.getPrevMixTestCertificate();
		oSig.setVerifyKey(pCert);
		delete pCert;
		if(oSig.verifyXML(messageBuff,len)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Couldt not verify the symetric key!\n");		
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
		SINT32 ret=decodeXMLEncryptedKey(key,&keySize,messageBuff,len,&mRSA);
		delete []messageBuff;
		if(ret!=E_SUCCESS||keySize!=64)
			{
				CAMsg::printMsg(LOG_CRIT,"Couldt not decrypt the symetric key!\n");		
				return E_UNKNOWN;
			}
		if(m_pMuxIn->setReceiveKey(key,32)!=E_SUCCESS||m_pMuxIn->setSendKey(key+32,32))
			{
				CAMsg::printMsg(LOG_CRIT,"Couldt not set the symetric key to be used by the MuxSocket!\n");		
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

SINT32 CALastMix::reconfigure()
	{
		CAMsg::printMsg(LOG_DEBUG,"Reconfiguring Last Mix\n");
		CAMsg::printMsg(LOG_DEBUG,"Re-read cache proxies\n");
		if(setTargets()!=E_SUCCESS)
			CAMsg::printMsg(LOG_DEBUG,"Could not set new cache proxies\n");
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

#define _SEND_TIMEOUT (UINT32)5000 //5 Seconds...

/**How to end this thread:
1. Close connection to next mix
2. put a byte in the Mix-Output-Queue
*/
THREAD_RETURN lm_loopSendToMix(void* param)
	{
		CAQueue* pQueue=((CALastMix*)param)->m_pQueueSendToMix;
		CAMuxSocket* pMuxSocket=((CALastMix*)param)->m_pMuxIn;
		
		UINT32 len;
#ifndef USE_POOL
		UINT8* buff=new UINT8[0xFFFF];
		for(;;)
			{
				len=MIXPACKET_SIZE;
				SINT32 ret=pQueue->getOrWait(buff,&len);
				if(ret!=E_SUCCESS||len!=MIXPACKET_SIZE)
					break;
				if(pMuxSocket->send((MIXPACKET*)buff)!=MIXPACKET_SIZE)
					break;
			}
		delete []buff;
#else
		CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		MIXPACKET* pMixPacket=new MIXPACKET;
		for(;;)
			{
				len=MIXPACKET_SIZE;
				SINT32 ret=pQueue->getOrWait((UINT8*)pMixPacket,&len,MIX_POOL_TIMEOUT);
				if(ret==E_TIMEDOUT)
					{
						pMixPacket->flags=0;
						pMixPacket->channel=DUMMY_CHANNEL;
						getRandom(pMixPacket->data,DATA_SIZE);
					}
				else if(ret!=E_SUCCESS||len!=MIXPACKET_SIZE)
					break;
				pPool->pool(pMixPacket);
				if(pMuxSocket->send(pMixPacket)!=MIXPACKET_SIZE)
					break;
			}
		delete pMixPacket;
		delete pPool;
#endif
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread SendToMix\n");
		THREAD_RETURN_SUCCESS;
	}


SINT32 CALastMix::loop()
	{
		//CASocketList  oSocketList;
		CALastMixChannelList* pChannelList=new CALastMixChannelList;
		CASocketGroup osocketgroupCacheRead;
		CASocketGroup osocketgroupCacheWrite;
		CASingleSocketGroup osocketgroupMixIn;
		MIXPACKET* pMixPacket=new MIXPACKET;
		SINT32 ret;
		SINT32 countRead;
		lmChannelListEntry* pChannelListEntry;
		UINT8 rsaBuff[RSA_SIZE];
		//CONNECTION* tmpCon;
//		HCHANNEL tmpID;
		m_pQueueSendToMix=new CAQueue();
		UINT8* tmpBuff=new UINT8[MIXPACKET_SIZE];
		osocketgroupMixIn.add(*m_pMuxIn);
		((CASocket*)*m_pMuxIn)->setNonBlocking(true);
		m_pMuxIn->setCrypt(true);
		bool bAktiv;
		UINT32 countCacheAddresses=m_oCacheLB.getElementCount();
		CAInfoService* pInfoService=NULL;
		if(m_pSignature!=NULL&&options.isInfoServiceEnabled())
			{
				pInfoService=new CAInfoService();
				pInfoService->setSignature(m_pSignature);
				pInfoService->sendHelo();
				pInfoService->start();
			}
		m_logUploadedPackets=m_logDownloadedPackets=0;
		set64((UINT64&)m_logUploadedBytes,(UINT32)0);
		set64((UINT64&)m_logDownloadedBytes,(UINT32)0);
		CAThread oLogThread;
		oLogThread.setMainLoop(lm_loopLog);
		oLogThread.start(this);

		#ifdef LOG_CHANNEL
			UINT64 current_millis;
			UINT32 diff_time; 
		#endif
		#ifdef USE_POOL
			CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		#endif
		CAThread threadSendToMix;
		threadSendToMix.setMainLoop(lm_loopSendToMix);
		threadSendToMix.start(this);

		for(;;)
			{
				bAktiv=false;
//Step one.. reading from previous Mix
// reading maximal number of current channels packets
				countRead=osocketgroupMixIn.select(false,0);
				if(countRead==1)
					{
						bAktiv=true;
						UINT32 channels=pChannelList->getSize()+1;
						for(UINT32 k=0;k<channels;k++)
							{
								ret=m_pMuxIn->receive(pMixPacket,0);
								if(ret==SOCKET_ERROR)
									{
										CAMsg::printMsg(LOG_CRIT,"Channel to previous mix closed -- Restarting!\n");
										goto ERR;
									}
								if(ret!=MIXPACKET_SIZE)
									break;
								//else one packet received
								m_logUploadedPackets++;
								#ifdef USE_POOL
									pPool->pool(pMixPacket);
								#endif
								pChannelListEntry=pChannelList->get(pMixPacket->channel);
								if(pChannelListEntry==NULL)
									{
										if(pMixPacket->flags==CHANNEL_OPEN)
											{
												#if defined(_DEBUG1) 
														CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
												#endif
				
												CASymCipher* newCipher=new CASymCipher();
												mRSA.decrypt(pMixPacket->data,rsaBuff);
												newCipher->setKeyAES(rsaBuff);
												newCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																							pMixPacket->data+RSA_SIZE-KEY_SIZE,
																							DATA_SIZE-RSA_SIZE);
												memcpy(pMixPacket->data,rsaBuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
												CASocket* tmpSocket=new CASocket;										
												int ret;
												if(pMixPacket->payload.type==MIX_PAYLOAD_SOCKS)
													ret=tmpSocket->connect(maddrSocks,LAST_MIX_TO_PROXY_CONNECT_TIMEOUT); 
												else
													{
														UINT32 count=0;
														do
															{
																tmpSocket->close();
																tmpSocket->create();
																tmpSocket->setRecvBuff(50000);
																tmpSocket->setSendBuff(5000);
																ret=tmpSocket->connect(*m_oCacheLB.get(),LAST_MIX_TO_PROXY_CONNECT_TIMEOUT);
																count++;
															}
														while(ret!=E_SUCCESS&&count<countCacheAddresses);
													}	
												if(ret!=E_SUCCESS)
														{
	    												#ifdef _DEBUG
																CAMsg::printMsg(LOG_DEBUG,"Cannot connect to Squid!\n");
															#endif
															delete tmpSocket;
															delete newCipher;
															getRandom(pMixPacket->data,DATA_SIZE);
															pMixPacket->flags=CHANNEL_CLOSE;
															m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);			
															m_logDownloadedPackets++;	
													}
												else
														{    
															UINT16 payLen=ntohs(pMixPacket->payload.len);
															#ifdef _DEBUG1
																UINT8 c=pMixPacket->payload.data[30];
																pMixPacket->payload.data[30]=0;
																CAMsg::printMsg(LOG_DEBUG,"Try sending data to Squid: %s\n",pMixPacket->payload.data);
																pMixPacket->payload.data[30]=c;
															#endif
															#ifdef LOG_CRIME
																if(payLen<=PAYLOAD_SIZE&&checkCrime(pMixPacket->payload.data,payLen))
																	{
																		UINT8 crimeBuff[PAYLOAD_SIZE+1];
																		memset(crimeBuff,0,PAYLOAD_SIZE+1);
																		memcpy(crimeBuff,pMixPacket->payload.data,payLen);
																		UINT32 id=m_pMuxIn->sigCrime(pMixPacket->channel,tmpBuff);
																		oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);
																		CAMsg::printMsg(LOG_SPECIAL,"Crime detected -- ID: %u -- Content: \n%s\n",id,crimeBuff,payLen);
																	}
															#endif
															if(payLen>PAYLOAD_SIZE||tmpSocket->sendTimeOut(pMixPacket->payload.data,payLen,_SEND_TIMEOUT)==SOCKET_ERROR)
																{
																	#ifdef _DEBUG
																		CAMsg::printMsg(LOG_DEBUG,"Error sending Data to Squid!\n");
																	#endif
																	tmpSocket->close();
																	delete tmpSocket;
																	delete newCipher;
																	getRandom(pMixPacket->data,DATA_SIZE);
																	pMixPacket->flags=CHANNEL_CLOSE;
																	m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);			
																	m_logDownloadedPackets++;	
																}
															else
																{
																	tmpSocket->setNonBlocking(true);
																	#ifdef LOG_CHANNEL
																		getcurrentTimeMillis(current_millis);
																		pChannelList->add(pMixPacket->channel,tmpSocket,newCipher,new CAQueue(),current_millis,payLen);
																	#else
																		pChannelList->add(pMixPacket->channel,tmpSocket,newCipher,new CAQueue());
																	#endif
																	osocketgroupCacheRead.add(*tmpSocket);
																}
														}
											}
									}
								else
									{
										if(pMixPacket->flags==CHANNEL_CLOSE)
											{
												osocketgroupCacheRead.remove(*(pChannelListEntry->pSocket));
												osocketgroupCacheWrite.remove(*(pChannelListEntry->pSocket));
												pChannelListEntry->pSocket->close();
												delete pChannelListEntry->pSocket;
												delete pChannelListEntry->pCipher;
												delete pChannelListEntry->pQueueSend;										
												#ifdef LOG_CHANNEL
													MACRO_DO_LOG_CHANNEL
												#endif
												pChannelList->removeChannel(pMixPacket->channel);
											}
										else if(pMixPacket->flags==CHANNEL_SUSPEND)
											{
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_DEBUG,"Suspending channel %u Socket: %u\n",pMixPacket->channel,(SOCKET)(*pChannelListEntry->pSocket));
												#endif
												osocketgroupCacheRead.remove(*(pChannelListEntry->pSocket));
											}
										else if(pMixPacket->flags==CHANNEL_RESUME)
											{
												osocketgroupCacheRead.add(*(pChannelListEntry->pSocket));
											}
										else if(pMixPacket->flags==CHANNEL_DATA)
											{
												#ifdef LOG_CHANNEL
													pChannelListEntry->packetsDataInFromUser++;
												#endif
												pChannelListEntry->pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
												ret=ntohs(pMixPacket->payload.len);
												if(ret>=0&&ret<=PAYLOAD_SIZE)
													{
														#ifdef LOG_CHANNEL
															pChannelListEntry->trafficInFromUser+=ret;
														#endif
														ret=pChannelListEntry->pQueueSend->add(pMixPacket->payload.data,ret);
													}
												else
													ret=SOCKET_ERROR;
												if(ret==SOCKET_ERROR)
													{
														osocketgroupCacheRead.remove(*(pChannelListEntry->pSocket));
														osocketgroupCacheWrite.remove(*(pChannelListEntry->pSocket));
														pChannelListEntry->pSocket->close();
														#ifdef LOG_CHANNEL
															MACRO_DO_LOG_CHANNEL
														#endif
														delete pChannelListEntry->pSocket;
														delete pChannelListEntry->pCipher;
														delete pChannelListEntry->pQueueSend;
														pChannelList->removeChannel(pMixPacket->channel);
														getRandom(pMixPacket->data,DATA_SIZE);
														pMixPacket->flags=CHANNEL_CLOSE;
														m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);
														m_logDownloadedPackets++;	
													}
												else
													{
														osocketgroupCacheWrite.add(*(pChannelListEntry->pSocket));
													}
											}
									}
							}
					}
//end Step 1

//Step 2 Sending to Cache...
				countRead=osocketgroupCacheWrite.select(true,0 );
				if(countRead>0)
					{
						bAktiv=true;
						pChannelListEntry=pChannelList->getFirstSocket();
						while(pChannelListEntry!=NULL&&countRead>0)
							{
								if(osocketgroupCacheWrite.isSignaled(*(pChannelListEntry->pSocket)))
									{
										countRead--;
										SINT32 len=MIXPACKET_SIZE;
										pChannelListEntry->pQueueSend->peek(tmpBuff,(UINT32*)&len);
										len=pChannelListEntry->pSocket->send(tmpBuff,len);
										if(len>0)
											{
												add64((UINT64&)m_logUploadedBytes,len);
												pChannelListEntry->pQueueSend->remove((UINT32*)&len);
												if(pChannelListEntry->pQueueSend->isEmpty())
													{
														osocketgroupCacheWrite.remove(*(pChannelListEntry->pSocket));
													}
											}
										else
											{
												if(len==SOCKET_ERROR)
													{ //do something if send error
														osocketgroupCacheRead.remove(*(pChannelListEntry->pSocket));
														osocketgroupCacheWrite.remove(*(pChannelListEntry->pSocket));
														pChannelListEntry->pSocket->close();
														#ifdef LOG_CHANNEL
															MACRO_DO_LOG_CHANNEL
														#endif
														delete pChannelListEntry->pSocket;
														delete pChannelListEntry->pCipher;
														delete pChannelListEntry->pQueueSend;
														pMixPacket->flags=CHANNEL_CLOSE;
														getRandom(pMixPacket->data,DATA_SIZE);
														pMixPacket->channel=pChannelListEntry->channelIn;
														m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);			
														m_logDownloadedPackets++;	
														pChannelList->removeChannel(pChannelListEntry->channelIn);											 
													}
											}
									}
								pChannelListEntry=pChannelList->getNextSocket();
							}
					}
//End Step 2

//Step 3 Reading from Cache....
#define MAX_MIXIN_SEND_QUEUE_SIZE 1000000
				countRead=osocketgroupCacheRead.select(false,0);
				if(countRead>0)
					{
						#ifdef DELAY_CHANNELS
							UINT64 aktTime;
							getcurrentTimeMillis(aktTime);
						#endif
						pChannelListEntry=pChannelList->getFirstSocket();
						while(pChannelListEntry!=NULL&&countRead>0)
							{
								if(osocketgroupCacheRead.isSignaled(*(pChannelListEntry->pSocket)))
									{
										countRead--;
										if(m_pQueueSendToMix->getSize()<MAX_MIXIN_SEND_QUEUE_SIZE
												#ifdef DELAY_CHANNELS
													&&(pChannelListEntry->trafficOutToUser<DELAY_CHANNEL_TRAFFIC||isGreater64(aktTime,pChannelListEntry->timeNextSend))
												#endif
											)
											{
												bAktiv=true;
												ret=pChannelListEntry->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE);
												if(ret==SOCKET_ERROR||ret==0)
													{
														osocketgroupCacheRead.remove(*(pChannelListEntry->pSocket));
														osocketgroupCacheWrite.remove(*(pChannelListEntry->pSocket));
														pChannelListEntry->pSocket->close();
														#ifdef LOG_CHANNEL
															MACRO_DO_LOG_CHANNEL
														#endif
														delete pChannelListEntry->pSocket;
														delete pChannelListEntry->pCipher;
														delete pChannelListEntry->pQueueSend;
														pMixPacket->flags=CHANNEL_CLOSE;
														pMixPacket->channel=pChannelListEntry->channelIn;
														getRandom(pMixPacket->data,DATA_SIZE);
														pChannelList->removeChannel(pChannelListEntry->channelIn);
														m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);			
														m_logDownloadedPackets++;	
													}
												else 
													{
														add64((UINT64&)m_logDownloadedBytes,ret);
														#if defined(LOG_CHANNEL)||defined(DELAY_CHANNELS)
															pChannelListEntry->trafficOutToUser+=ret;
														#endif
														pMixPacket->channel=pChannelListEntry->channelIn;
														pMixPacket->flags=CHANNEL_DATA;
														pMixPacket->payload.len=htons((UINT16)ret);
														pMixPacket->payload.type=0;
														pChannelListEntry->pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);
														m_logDownloadedPackets++;
														#if defined(LOG_CHANNEL)
															pChannelListEntry->packetsDataOutToUser++;
														#endif													
														#ifdef DELAY_CHANNELS
															set64(pChannelListEntry->timeNextSend,aktTime);
															UINT32 delayTime=(ret>>5);
															delayTime+=(ret>>4);
															add64(pChannelListEntry->timeNextSend,delayTime/*DELAY_CHANNEL_SEND_INTERVALL*/);
														#endif
													}
											}
										else
											break;
									}
								pChannelListEntry=pChannelList->getNextSocket();
							}
					}
//end Step 3

//Step 4 Writing to previous Mix
// Now in a separate Thread!
//
//end step 4
				if(!bAktiv)
					msSleep(100);
			}




ERR:
		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		m_pMuxIn->close();
		UINT8 b;
		m_pQueueSendToMix->add(&b,1);
		CAMsg::printMsg(LOG_CRIT,"Wait for LoopSendToMix!\n");
		threadSendToMix.join(); //will not join if queue is empty (and so wating)!!!
		m_bRunLog=false;
		if(pInfoService!=NULL)
			{
				delete pInfoService;
			}
		pChannelListEntry=pChannelList->getFirstSocket();
		while(pChannelListEntry!=NULL)
			{
				delete pChannelListEntry->pCipher;
				delete pChannelListEntry->pQueueSend;
				pChannelListEntry->pSocket->close();
				delete pChannelListEntry->pSocket;
				pChannelListEntry=pChannelList->getNextSocket();
			}
		delete pChannelList;
		delete []tmpBuff;
		delete pMixPacket;
		delete m_pQueueSendToMix;
		m_pQueueSendToMix=NULL;
		#ifdef USE_POOL
			delete pPool;
		#endif
		oLogThread.join();
		return E_UNKNOWN;
	}

#ifdef LOG_CRIME
	bool CALastMix::checkCrime(UINT8* payLoad,UINT32 payLen)
		{ //Lots of TODO!!!!
			//DNS Lookup may block if Host does not exists!!!!!
			//so we use regexp....
			if(payLen<3)
				return false;
			UINT8* startOfUrl=(UINT8*)memchr(payLoad,32,payLen-1); //search for first space...
			if(startOfUrl==NULL)
				return false;
			startOfUrl++;
			UINT8* endOfUrl=(UINT8*)memchr(startOfUrl,32,payLen-(startOfUrl-payLoad)); //search for first space after start of URL 
			if(endOfUrl==NULL)
				return false;
			UINT32 strLen=endOfUrl-startOfUrl;
			for(UINT32 i=0;i<m_nCrimeRegExp;i++)
				{
					if(regnexec(&m_pCrimeRegExps[i],(char*)startOfUrl,strLen,0,NULL,0)==0)
						return true;
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
		UINT32 cntTargets=options.getTargetInterfaceCount();
		if(cntTargets==0)
			{
				CAMsg::printMsg(LOG_CRIT,"No Targets (proxies) specified!\n");
				return E_UNKNOWN;
			}
		m_oCacheLB.clean();
		UINT32 i;
		for(i=1;i<=cntTargets;i++)
			{
				TargetInterface oTargetInterface;
				options.getTargetInterface(oTargetInterface,i);
				if(oTargetInterface.target_type==TARGET_HTTP_PROXY)
					m_oCacheLB.add(oTargetInterface.addr);
				delete oTargetInterface.addr;
			}
		CAMsg::printMsg(LOG_DEBUG,"This mix will use the following proxies:\n");
		for(i=0;i<m_oCacheLB.getElementCount();i++)
			{
				CASocketAddrINet* pAddr=m_oCacheLB.get();
				UINT8 ip[4];
				pAddr->getIP(ip);
				UINT32 port=pAddr->getPort();
				CAMsg::printMsg(LOG_DEBUG,"%u. Proxy's Address: %u.%u.%u.%u:%u\n",i+1,ip[0],ip[1],ip[2],ip[3],port);
			}
		return E_SUCCESS;
	}			

SINT32 CALastMix::clean()
	{
		if(m_pMuxIn!=NULL)
			{
				m_pMuxIn->close();
				delete m_pMuxIn;
			}
		m_pMuxIn=NULL;
		mRSA.destroy();
//		oSuspendList.clear();
		return E_SUCCESS;
	}

