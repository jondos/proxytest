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
#include "../StdAfx.h"
#include "CALastMixA.hpp"
#include "../CALastMixChannelList.hpp"
#include "../CASingleSocketGroup.hpp"
#include "../CAPool.hpp"
#include "../CACmdLnOptions.hpp"

extern CACmdLnOptions options;

SINT32 CALastMixA::loop()
	{
#ifndef NEW_MIX_TYPE
		//CASocketList  oSocketList;
		CALastMixChannelList* pChannelList=new CALastMixChannelList;
		CASocketGroup osocketgroupCacheRead;
		CASocketGroup osocketgroupCacheWrite;
		//CASingleSocketGroup osocketgroupMixIn;
		#ifdef LOG_PACKET_TIMES
			tPoolEntry* pPoolEntry=new tPoolEntry;
			MIXPACKET* pMixPacket=&pPoolEntry->mixpacket;
		#else
			MIXPACKET* pMixPacket=new MIXPACKET;
		#endif	
		SINT32 ret;
		SINT32 countRead;
		lmChannelListEntry* pChannelListEntry;
		UINT8 rsaBuff[RSA_SIZE];
		//CONNECTION* tmpCon;
//		HCHANNEL tmpID;
		UINT8* tmpBuff=new UINT8[MIXPACKET_SIZE];
		//osocketgroupMixIn.add(*m_pMuxIn);
		//((CASocket*)*m_pMuxIn)->setNonBlocking(true);
		bool bAktiv;
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

		while(!m_bRestart)
			{
				bAktiv=false;
//Step 1a reading from previous Mix --> now in separate thread
//Step 1b processing MixPackets from previous mix
// processing maximal number of current channels packets
				if(m_pQueueReadFromMix->getSize()>=MIXPACKET_SIZE)
					{
						bAktiv=true;
						UINT32 channels=pChannelList->getSize()+1;
						for(UINT32 k=0;k<channels&&m_pQueueReadFromMix->getSize()>=MIXPACKET_SIZE;k++)
							{
								ret=MIXPACKET_SIZE;
								ret=m_pQueueReadFromMix->get((UINT8*)pMixPacket,(UINT32*)&ret);
								// one packet received
								m_logUploadedPackets++;
								pChannelListEntry=pChannelList->get(pMixPacket->channel);
								if(pChannelListEntry==NULL)
									{
										if(pMixPacket->flags==CHANNEL_OPEN)
											{
												#if defined(_DEBUG1) 
														CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
												#endif
												
												m_pRSA->decrypt(pMixPacket->data,rsaBuff);
												#ifdef WITH_TIMESTAMP
													//CAMsg::printMsg(LOG_DEBUG,"Timestamp is: %X %X\n", *(rsaBuff+KEY_SIZE), *(rsaBuff+KEY_SIZE+1));
												#endif
												#ifdef REPLAY_DETECTION
													if(!validTimestampAndFingerprint(rsaBuff, KEY_SIZE, (rsaBuff+KEY_SIZE)))
														{
															CAMsg::printMsg(LOG_INFO,"Duplicate packet ignored.\n");
															continue;
														}
												#endif
												CASymCipher* newCipher=new CASymCipher();
												newCipher->setKey(rsaBuff);
												newCipher->crypt1(pMixPacket->data+RSA_SIZE,
																							pMixPacket->data+RSA_SIZE-(KEY_SIZE+TIMESTAMP_SIZE),
																							DATA_SIZE-RSA_SIZE);
												memcpy(	pMixPacket->data,rsaBuff+KEY_SIZE+TIMESTAMP_SIZE,
																RSA_SIZE-(KEY_SIZE+TIMESTAMP_SIZE));
												CASocket* tmpSocket=new CASocket;
												CACacheLoadBalancing* ptmpLB=m_pCacheLB;
												int ret=E_UNKNOWN;
												if(pMixPacket->payload.type==MIX_PAYLOAD_SOCKS)
													ptmpLB=m_pSocksLB;
												for(UINT32 count=0;count<ptmpLB->getElementCount();count++)
													{
														tmpSocket->create();
														tmpSocket->setRecvBuff(50000);
														tmpSocket->setSendBuff(5000);
														ret=tmpSocket->connect(*ptmpLB->get(),LAST_MIX_TO_PROXY_CONNECT_TIMEOUT);
														if(ret==E_SUCCESS)
															break;
														tmpSocket->close();
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
																		MIXPACKET sigPacket;
																		memset(crimeBuff,0,PAYLOAD_SIZE+1);
																		memcpy(crimeBuff,pMixPacket->payload.data,payLen);
																		UINT32 id=m_pMuxIn->sigCrime(pMixPacket->channel,&sigPacket);
																		m_pQueueSendToMix->add(&sigPacket,MIXPACKET_SIZE);
																		int log=LOG_ENCRYPTED;
																		if(!options.isEncryptedLogEnabled())
																			log=LOG_CRIT;
																		CAMsg::printMsg(log,"Crime detected -- ID: %u -- Content: \n%s\n",id,crimeBuff,payLen);
																	}
															#endif
															if(payLen>PAYLOAD_SIZE||tmpSocket->sendTimeOut(pMixPacket->payload.data,payLen,LAST_MIX_TO_PROXY_SEND_TIMEOUT)==SOCKET_ERROR)
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
																		pChannelList->add(pMixPacket->channel,tmpSocket,newCipher,new CAQueue(PAYLOAD_SIZE));
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
												pChannelListEntry->pCipher->crypt1(pMixPacket->data,pMixPacket->data,DATA_SIZE);
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
														pChannelListEntry->pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
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




//ERR:
		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		m_bRestart=true;
		m_pMuxIn->close();
		UINT8 b;
		m_pQueueSendToMix->add(&b,1);
		CAMsg::printMsg(LOG_CRIT,"Wait for LoopSendToMix!\n");
		m_pthreadSendToMix->join(); //will not join if queue is empty (and so wating)!!!
		m_bRunLog=false;
		CAMsg::printMsg(LOG_CRIT,"Wait for LoopReadFromMix!\n");
		m_pthreadReadFromMix->join();
		CAMsg::printMsg(LOG_CRIT,"done.\n");
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
#endif //! NEW_MIX_TYPE
		return E_UNKNOWN;
	}

