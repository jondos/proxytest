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
#include "xml/DOM_Output.hpp"

extern CACmdLnOptions options;

/*******************************************************************************/
// ----------START NEW VERSION -----------------------
//---------------------------------------------------------
/********************************************************************************/

SINT32 CALastMix::initOnce()
	{
		UINT32 cntTargets=options.getTargetInterfaceCount();
		if(cntTargets==0)
			{
				CAMsg::printMsg(LOG_CRIT,"No Targets (proxies) specified!\n");
				return E_UNKNOWN;
			}
//		CASocketAddrINet oAddr;
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
	
		return processKeyExchange();
	}

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

		DOM_DocumentFragment tmpDocFrag;
		mRSA.getPublicKeyAsDocumentFragment(tmpDocFrag);
		DOM_Node nodeRsaKey=doc.importNode(tmpDocFrag,true);
		elemMix.appendChild(nodeRsaKey);
		tmpDocFrag=0;

		m_pSignature->signXML(elemMix);
		
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
		CAMsg::printMsg(LOG_INFO,"Verified the symetric key!\n");		
		
		UINT8 key[50];
		UINT32 keySize=50;
		SINT32 ret=decodeXMLEncryptedKey(key,&keySize,messageBuff,len,&mRSA);
		delete []messageBuff;
		if(ret!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Couldt not decrypt the symetric key!\n");		
				return E_UNKNOWN;
			}
		if(m_pMuxIn->setKey(key,keySize)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Couldt not set the symetric key to be used by the MuxSocket!\n");		
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

THREAD_RETURN loopLog(void* param)
	{
		CALastMix* pLastMix=(CALastMix*)param;
		pLastMix->m_bRunLog=true;
		UINT32 countLog=0;
		UINT8 buff[256];
		while(pLastMix->m_bRunLog)
			{
				if(countLog==0)
					{
						CAMsg::printMsg(LOG_DEBUG,"Uploadaed  Packets: %u\n",pLastMix->m_logUploadedPackets);
						CAMsg::printMsg(LOG_DEBUG,"Downloaded Packets: %u\n",pLastMix->m_logDownloadedPackets);
						print64(buff,(UINT64&)pLastMix->m_logUploadedBytes);
						CAMsg::printMsg(LOG_DEBUG,"Uploadaed  Bytes  : %s\n",buff);
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
		CAQueue oqueueMixIn;
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
		oLogThread.setMainLoop(loopLog);
		oLogThread.start(this);

		#ifdef LOG_CHANNEL
			UINT64 current_millis;
			UINT32 diff_time; 
		#endif

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
								pChannelListEntry=pChannelList->get(pMixPacket->channel);
								if(pChannelListEntry==NULL)
									{
										if(pMixPacket->flags==CHANNEL_OPEN_OLD||pMixPacket->flags==CHANNEL_OPEN_NEW)
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
															m_pMuxIn->close(pMixPacket->channel,tmpBuff);
															oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
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
															if(payLen>PAYLOAD_SIZE||tmpSocket->sendTimeOut(pMixPacket->payload.data,payLen,_SEND_TIMEOUT)==SOCKET_ERROR)
																{
																	#ifdef _DEBUG
																		CAMsg::printMsg(LOG_DEBUG,"Error sending Data to Squid!\n");
																	#endif
																	tmpSocket->close();
																	delete tmpSocket;
																	delete newCipher;
																	m_pMuxIn->close(pMixPacket->channel,tmpBuff);
																	oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
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
													getcurrentTimeMillis(current_millis);
													diff_time=diff64(current_millis,pChannelListEntry->timeCreated);
													CAMsg::printMsg(LOG_DEBUG,"Channel %u closed: Time [ms] - %u, Upload - %u, Download - %u\n",pChannelListEntry->channelIn,diff_time,pChannelListEntry->trafficIn,pChannelListEntry->trafficOut); 
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
												pChannelListEntry->pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
												ret=ntohs(pMixPacket->payload.len);
												if(ret>=0&&ret<=PAYLOAD_SIZE)
													{
														#ifdef LOG_CHANNEL
															pChannelListEntry->trafficIn+=ret;
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
															getcurrentTimeMillis(current_millis);
															diff_time=diff64(current_millis,pChannelListEntry->timeCreated);
															CAMsg::printMsg(LOG_DEBUG,"Channel %u closed: Time [ms] - %u, Upload - %u, Download - %u\n",pChannelListEntry->channelIn,diff_time,pChannelListEntry->trafficIn,pChannelListEntry->trafficOut); 
														#endif
														delete pChannelListEntry->pSocket;
														delete pChannelListEntry->pCipher;
														delete pChannelListEntry->pQueueSend;
														pChannelList->removeChannel(pMixPacket->channel);
														m_pMuxIn->close(pMixPacket->channel,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);
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
															getcurrentTimeMillis(current_millis);
															diff_time=diff64(current_millis,pChannelListEntry->timeCreated);
															CAMsg::printMsg(LOG_DEBUG,"Channel %u closed: Time [ms] - %u, Upload - %u, Download - %u\n",pChannelListEntry->channelIn,diff_time,pChannelListEntry->trafficIn,pChannelListEntry->trafficOut); 
														#endif
														delete pChannelListEntry->pSocket;
														delete pChannelListEntry->pCipher;
														delete pChannelListEntry->pQueueSend;
														m_pMuxIn->close(pChannelListEntry->channelIn,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
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
										if(oqueueMixIn.getSize()<MAX_MIXIN_SEND_QUEUE_SIZE
												#ifdef DELAY_CHANNELS
													&&(pChannelListEntry->trafficOut<DELAY_CHANNEL_TRAFFIC||isGreater64(aktTime,pChannelListEntry->timeNextSend))
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
															getcurrentTimeMillis(current_millis);
															diff_time=diff64(current_millis,pChannelListEntry->timeCreated);
															CAMsg::printMsg(LOG_DEBUG,"Channel %u closed: Time [ms] - %u, Upload - %u, Download - %u\n",pChannelListEntry->channelIn,diff_time,pChannelListEntry->trafficIn,pChannelListEntry->trafficOut); 
														#endif
														delete pChannelListEntry->pSocket;
														delete pChannelListEntry->pCipher;
														delete pChannelListEntry->pQueueSend;
														m_pMuxIn->close(pChannelListEntry->channelIn,tmpBuff);
														pChannelList->removeChannel(pChannelListEntry->channelIn);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
														m_logDownloadedPackets++;	
													}
												else 
													{
														add64((UINT64&)m_logDownloadedBytes,ret);
														#if defined(LOG_CHANNEL)||defined(DELAY_CHANNELS)
															pChannelListEntry->trafficOut+=ret;
														#endif
														pMixPacket->channel=pChannelListEntry->channelIn;
														pMixPacket->flags=CHANNEL_DATA;
														pMixPacket->payload.len=htons((UINT16)ret);
														pMixPacket->payload.type=0;
														pChannelListEntry->pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														m_pMuxIn->send(pMixPacket,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);
														m_logDownloadedPackets++;
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
				if(!oqueueMixIn.isEmpty()&&osocketgroupMixIn.select(true,0)==1)
					{
						countRead=pChannelList->getSize()+1;
						while(countRead>0&&!oqueueMixIn.isEmpty())
							{
								bAktiv=true;
								countRead--;
								UINT32 len=MIXPACKET_SIZE;
								oqueueMixIn.peek(tmpBuff,&len);
								ret=((CASocket*)*m_pMuxIn)->send(tmpBuff,len);
								if(ret>0)
									{
										oqueueMixIn.remove((UINT32*)&ret);
									}
								else if(ret==E_AGAIN)
									break;
								else
									goto ERR;
							}
					}
//end step 4
				if(!bAktiv)
					msSleep(100);
			}




ERR:
		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
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
		oLogThread.join();
		return E_UNKNOWN;
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

