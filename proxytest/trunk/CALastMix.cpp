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
#include "CASocketList.hpp"
#include "CASocketGroup.hpp"
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CASocketAddrUnix.hpp"
#include "CASocketAddrINet.hpp"
#include "CAUtil.hpp"
#include "CAInfoService.hpp"

extern CACmdLnOptions options;

/*******************************************************************************/
// ----------START NEW VERSION -----------------------
//---------------------------------------------------------
/********************************************************************************/

SINT32 CALastMix::initOnce()
	{
		UINT32 cntTargets=options.getTargetCount();
		if(cntTargets==0)
			return E_UNKNOWN;
		CASocketAddrINet oAddr;
		UINT32 i;
		for(i=1;i<=cntTargets;i++)
			{
				options.getTargetAddr(oAddr,i);
				m_oCacheLB.add(oAddr);
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

		int handle;
		SINT32 len;
		UINT8* fileBuff=new UINT8[2048];
		if(fileBuff==NULL||options.getKeyFileName(fileBuff,2048)!=E_SUCCESS)
			goto END;
		handle=open((char*)fileBuff,O_BINARY|O_RDONLY);
		if(handle==-1)
			goto END;
		len=read(handle,fileBuff,2048);
		close(handle);
		if(len<1)
			goto END;
		m_pSignature=new CASignature();
		if(m_pSignature->setSignKey(fileBuff,len,SIGKEY_XML)!=E_SUCCESS)
			{
				delete m_pSignature;
				m_pSignature=NULL;
			}
END:		
		delete []fileBuff;
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
		CASocketAddr* pAddrListen;
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
		m_pMuxIn=new CAMuxSocket();
		if(m_pMuxIn->accept(*pAddrListen)!=E_SUCCESS)
		    {
					delete pAddrListen;
					CAMsg::printMsg(LOG_CRIT," failed!\n");
					return E_UNKNOWN;
		    }
		delete pAddrListen;
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
		
		UINT16 messageSize=0;
		UINT32 keySize=0;
		UINT8* buff=NULL;
		//Constructing the XML Key Info in buff
		buff=new UINT8[1024];
		keySize=1024;
		UINT32 aktIndex=2; //First two bytes for len reserved
		//Beginn with: <?XML version="1.0"?><Mixes count="1"><Mix id="  
		memcpy(buff+aktIndex,"<?xml version=\"1.0\"?><Mixes count=\"1\"><Mix id=\"",47);
		aktIndex+=47;
		//than insert the Mix id
		options.getMixId(buff+aktIndex,50);
		aktIndex=strlen((char*)buff+2)+2;
		//the closing chars for the <Mix> tag
		memcpy(buff+aktIndex,"\">",2);
		aktIndex+=2;
		//the Key Value
		mRSA.getPublicKeyAsXML(buff+aktIndex,&keySize);
		aktIndex+=keySize;
		//the closing Mix and Mixes tags
		memcpy(buff+aktIndex,"</Mix></Mixes>",14);
		aktIndex+=14;
		messageSize=aktIndex;		
		UINT16 tmp=htons(messageSize-2);
		memcpy(buff,&tmp,2);
		buff[aktIndex]=0;
		CAMsg::printMsg(LOG_INFO,"Key Info is:\n");
		CAMsg::printMsg(LOG_INFO,"%s\n",(char*)buff+2);		
		CAMsg::printMsg(LOG_INFO,"Sending Infos (chain length and RSA-Key, Message-Size %u)\n",messageSize);
		if(((CASocket*)*m_pMuxIn)->send(buff,messageSize)!=messageSize)
			{
				CAMsg::printMsg(LOG_ERR,"Error sending Key-Info!\n");
				delete []buff;
				return E_UNKNOWN;
			}
		delete []buff;
		return E_SUCCESS;
	}

THREAD_RETURN loopLog(void* param)
	{
		CALastMix* pLastMix=(CALastMix*)param;
		pLastMix->m_bRunLog=true;
		UINT32 countLog=0;
		while(pLastMix->m_bRunLog)
			{
				if(countLog==0)
					{
						CAMsg::printMsg(LOG_DEBUG,"Uploadaed  Packets: %u\n",pLastMix->m_logUploadedPackets);
						CAMsg::printMsg(LOG_DEBUG,"Downloaded Packets: %u\n",pLastMix->m_logDownloadedPackets);
						CAMsg::printMsg(LOG_DEBUG,"Uploadaed  Bytes  : %u\n",pLastMix->m_logUploadedBytes);
						CAMsg::printMsg(LOG_DEBUG,"Downloaded Bytes  : %u\n",pLastMix->m_logDownloadedBytes);
						countLog=30;
					}
				sSleep(30);
				countLog--;
			}
		THREAD_RETURN_SUCCESS;
	}

#define _CONNECT_TIMEOUT 5000 //5 Seconds...
#define _SEND_TIMEOUT (UINT32)5000 //5 Seconds...

SINT32 CALastMix::loop()
	{
		CASocketList  oSocketList;
		CASocketGroup osocketgroupCacheRead;
		CASocketGroup osocketgroupCacheWrite;
		CASingleSocketGroup osocketgroupMixIn;
		MIXPACKET* pMixPacket=new MIXPACKET;
		SINT32 ret;
		SINT32 countRead;
		CONNECTION oConnection;
		UINT8 rsaBuff[RSA_SIZE];
		CONNECTION* tmpCon;
		HCHANNEL tmpID;
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
		m_logUploadedPackets=m_logDownloadedPackets=m_logUploadedBytes=m_logDownloadedBytes=0;
		CAThread oLogThread;
		oLogThread.setMainLoop(loopLog);
		oLogThread.start(this);
		
		for(;;)
			{
				bAktiv=false;
//Step one.. reading from previous Mix
// reading maximal number of current channels packets
				countRead=osocketgroupMixIn.select(false,0);
				if(countRead==1)
					{
						bAktiv=true;
						UINT32 channels=oSocketList.getSize()+1;
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
								if(!oSocketList.get(pMixPacket->channel,&oConnection))
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
													ret=tmpSocket->connect(maddrSocks,_CONNECT_TIMEOUT); 
												else
													{
														UINT32 count=0;
														do
															{
																tmpSocket->close();
																tmpSocket->create();
																tmpSocket->setRecvBuff(50000);
																tmpSocket->setSendBuff(5000);
																ret=tmpSocket->connect(*m_oCacheLB.get(),_CONNECT_TIMEOUT);
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
																	oSocketList.add(pMixPacket->channel,tmpSocket,newCipher,new CAQueue());
																	osocketgroupCacheRead.add(*tmpSocket);
																}
														}
											}
									}
								else
									{
										if(pMixPacket->flags==CHANNEL_CLOSE)
											{
												osocketgroupCacheRead.remove(*(oConnection.pSocket));
												osocketgroupCacheWrite.remove(*(oConnection.pSocket));
												oConnection.pSocket->close();
												oSocketList.remove(pMixPacket->channel);
												delete oConnection.pSocket;
												delete oConnection.pCipher;
												delete oConnection.pSendQueue;										
											}
										else if(pMixPacket->flags==CHANNEL_SUSPEND)
											{
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_DEBUG,"Suspending channel %u Socket: %u\n",pMixPacket->channel,(SOCKET)(*oConnection.pSocket));
												#endif
												osocketgroupCacheRead.remove(*(oConnection.pSocket));
											}
										else if(pMixPacket->flags==CHANNEL_RESUME)
											{
												osocketgroupCacheRead.add(*(oConnection.pSocket));
											}
										else if(pMixPacket->flags==CHANNEL_DATA)
											{
												oConnection.pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
												ret=ntohs(pMixPacket->payload.len);
												if(ret>=0&&ret<=PAYLOAD_SIZE)
													ret=oConnection.pSendQueue->add(pMixPacket->payload.data,ret);
												else
													ret=SOCKET_ERROR;
												if(ret==SOCKET_ERROR)
													{
														osocketgroupCacheRead.remove(*(oConnection.pSocket));
														osocketgroupCacheWrite.remove(*(oConnection.pSocket));
														oConnection.pSocket->close();
														delete oConnection.pSocket;
														delete oConnection.pCipher;
														delete oConnection.pSendQueue;
														oSocketList.remove(pMixPacket->channel);
														m_pMuxIn->close(pMixPacket->channel,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);
														m_logDownloadedPackets++;	
													}
												else
													{
														osocketgroupCacheWrite.add(*(oConnection.pSocket));
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
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL&&countRead>0)
							{
								if(osocketgroupCacheWrite.isSignaled(*(tmpCon->pSocket)))
									{
										countRead--;
										SINT32 len=MIXPACKET_SIZE;
										tmpCon->pSendQueue->peek(tmpBuff,(UINT32*)&len);
										len=tmpCon->pSocket->send(tmpBuff,len);
										if(len>0)
											{
												m_logUploadedBytes+=len;
												tmpCon->pSendQueue->remove((UINT32*)&len);
												if(tmpCon->pSendQueue->isEmpty())
													{
														osocketgroupCacheWrite.remove(*(tmpCon->pSocket));
													}
											}
										else
											{
												if(len==SOCKET_ERROR)
													{ //do something if send error
														osocketgroupCacheRead.remove(*(tmpCon->pSocket));
														osocketgroupCacheWrite.remove(*(tmpCon->pSocket));
														tmpCon->pSocket->close();
														delete tmpCon->pSocket;
														delete tmpCon->pCipher;
														delete tmpCon->pSendQueue;
														m_pMuxIn->close(tmpCon->id,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
														m_logDownloadedPackets++;	
														oSocketList.remove(tmpCon->id);											 
													}
											}
									}
								tmpCon=oSocketList.getNext();
							}
					}
//End Step 2

//Step 3 Reading from Cache....
#define MAX_MIXIN_SEND_QUEUE_SIZE 1000000
				countRead=osocketgroupCacheRead.select(false,0);
				if(countRead>0)
					{
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL&&countRead>0)
							{
								if(osocketgroupCacheRead.isSignaled(*(tmpCon->pSocket)))
									{
										countRead--;
										if(oqueueMixIn.getSize()<MAX_MIXIN_SEND_QUEUE_SIZE)
											{
												bAktiv=true;
												ret=tmpCon->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE);
												if(ret==SOCKET_ERROR||ret==0)
													{
														osocketgroupCacheRead.remove(*(tmpCon->pSocket));
														osocketgroupCacheWrite.remove(*(tmpCon->pSocket));
														tmpCon->pSocket->close();
														delete tmpCon->pSocket;
														delete tmpCon->pCipher;
														delete tmpCon->pSendQueue;
														tmpID=tmpCon->id;
														oSocketList.remove(tmpID);
														m_pMuxIn->close(tmpID,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
														m_logDownloadedPackets++;	
													}
												else 
													{
														m_logDownloadedBytes+=ret;
														pMixPacket->channel=tmpCon->id;
														pMixPacket->flags=CHANNEL_DATA;
														pMixPacket->payload.len=htons((UINT16)ret);
														pMixPacket->payload.type=0;
														tmpCon->pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														m_pMuxIn->send(pMixPacket,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);
														m_logDownloadedPackets++;
													}
											}
										else
											break;
									}
								tmpCon=oSocketList.getNext();
							}
					}
//end Step 3

//Step 4 Writing to previous Mix
				if(!oqueueMixIn.isEmpty()&&osocketgroupMixIn.select(true,0)==1)
					{
						countRead=oSocketList.getSize()+1;
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
		tmpCon=oSocketList.getFirst();
		while(tmpCon!=NULL)
			{
				delete tmpCon->pCipher;
				delete tmpCon->pSendQueue;
				tmpCon->pSocket->close();
				delete tmpCon->pSocket;
				tmpCon=tmpCon->next;
			}
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
		oSuspendList.clear();
		return E_SUCCESS;
	}

