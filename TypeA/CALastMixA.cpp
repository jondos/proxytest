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
#ifdef HAVE_EPOLL
#include "../CASocketGroupEpoll.hpp"
#endif

extern CACmdLnOptions options;
#ifdef LOG_CHANNEL
		//CAMsg::printMsg(LOG_DEBUG,"Channel time log format is as follows: Channel-ID,Channel duration [micros], Upload (bytes), Download (bytes), DataAndOpenPacketsFromUser, DataPacketsToUser\n"); 
	#define MACRO_DO_LOG_CHANNEL\
		diff_time=diff64(pQueueEntry->timestamp_proccessing_end,pChannelListEntry->timeCreated);\
		CAMsg::printMsg(LOG_DEBUG,"%u,%u,%u,%u,%u,%u\n",\
			pChannelListEntry->channelIn,\
			diff_time,pChannelListEntry->trafficInFromUser,pChannelListEntry->trafficOutToUser,\
			pChannelListEntry->packetsDataInFromUser,pChannelListEntry->packetsDataOutToUser); 
#endif

SINT32 CALastMixA::loop()
	{
#ifndef NEW_MIX_TYPE
		//CASocketList  oSocketList;
#ifdef DELAY_CHANNELS
		m_pChannelList->setDelayParameters(	options.getDelayChannelUnlimitTraffic(),
																			options.getDelayChannelBucketGrow(),
																			options.getDelayChannelBucketGrowIntervall());	
#endif		
#ifdef HAVE_EPOLL	
		CASocketGroupEpoll osocketgroupCacheRead(false);
		CASocketGroupEpoll osocketgroupCacheWrite(true);
#else
		CASocketGroup osocketgroupCacheRead(false);
		CASocketGroup osocketgroupCacheWrite(true);
#endif
		tQueueEntry* pQueueEntry=new tQueueEntry;
		MIXPACKET* pMixPacket=&pQueueEntry->packet;
		SINT32 ret;
		SINT32 countRead;
		lmChannelListEntry* pChannelListEntry;
		UINT8 rsaBuff[RSA_SIZE];
		UINT8* tmpBuff=new UINT8[MIXPACKET_SIZE];
		bool bAktiv;
		m_logUploadedPackets=m_logDownloadedPackets=0;
		set64((UINT64&)m_logUploadedBytes,(UINT32)0);
		set64((UINT64&)m_logDownloadedBytes,(UINT32)0);
		CAThread oLogThread((UINT8*)"CALastMixA - LogLoop");
		oLogThread.setMainLoop(lm_loopLog);
		oLogThread.start(this);

		#ifdef LOG_CHANNEL
			UINT32 diff_time; 
			CAMsg::printMsg(LOG_DEBUG,"Channel time log format is as follows: Channel-ID,Channel duration [micros], Upload (bytes), Download (bytes), DataAndOpenPacketsFromUser, DataPacketsToUser\n"); 
		#endif

		while(!m_bRestart)
			{
				bAktiv=false;
//Step 1a reading from previous Mix --> now in separate thread
//Step 1b processing MixPackets from previous mix
// processing maximal number of current channels packets
				if(m_pQueueReadFromMix->getSize()>=sizeof(tQueueEntry))
					{
						bAktiv=true;
						UINT32 channels=m_pChannelList->getSize()+1;
						for(UINT32 k=0;k<channels&&m_pQueueReadFromMix->getSize()>=sizeof(tQueueEntry);k++)
							{
								ret=sizeof(tQueueEntry);
								m_pQueueReadFromMix->get((UINT8*)pQueueEntry,(UINT32*)&ret);
								#ifdef LOG_PACKET_TIMES
									getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start_OP);
								#endif
							// one packet received
								m_logUploadedPackets++;
								pChannelListEntry=m_pChannelList->get(pMixPacket->channel);
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
												#ifdef LOG_PACKET_TIMES
													getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
												#endif
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
															#ifdef LOG_PACKET_TIMES
																setZero64(pQueueEntry->timestamp_proccessing_start);
															#endif
															m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));			
															m_logDownloadedPackets++;	
													}
												else
														{ //connection to proxy successfull
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
																		tQueueEntry oSigCrimeQueueEntry;
																		memset(&oSigCrimeQueueEntry,0,sizeof(tQueueEntry));
																		memset(crimeBuff,0,PAYLOAD_SIZE+1);
																		memcpy(crimeBuff,pMixPacket->payload.data,payLen);
																		UINT32 id=m_pMuxIn->sigCrime(pMixPacket->channel,&oSigCrimeQueueEntry.packet);
																		m_pQueueSendToMix->add(&oSigCrimeQueueEntry,sizeof(tQueueEntry));
																		int log=LOG_ENCRYPTED;
																		if(!options.isEncryptedLogEnabled())
																			log=LOG_CRIT;
																		CAMsg::printMsg(log,"Crime detected -- ID: %u -- Content: \n%s\n",id,crimeBuff);
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
																	#ifdef LOG_PACKET_TIMES
																		setZero64(pQueueEntry->timestamp_proccessing_start);
																	#endif
																	m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));			
																	m_logDownloadedPackets++;	
																}
															else
																{
																	tmpSocket->setNonBlocking(true);
																	#ifdef LOG_CHANNEL
																		m_pChannelList->add(pMixPacket->channel,tmpSocket,newCipher,new CAQueue(),pQueueEntry->timestamp_proccessing_start,payLen);
																	#else
																		m_pChannelList->add(pMixPacket->channel,tmpSocket,newCipher,new CAQueue(PAYLOAD_SIZE));
																	#endif
#ifdef HAVE_EPOLL
																	osocketgroupCacheRead.add(*tmpSocket,m_pChannelList->get(pMixPacket->channel));
#else
																	osocketgroupCacheRead.add(*tmpSocket);
#endif
																	#ifdef LOG_PACKET_TIMES
																		getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
																		m_pLogPacketStats->addToTimeingStats(*pQueueEntry,CHANNEL_OPEN,true);
																	#endif
																}
														}
											}
									}
								else
									{//channellsit entry !=NULL
										if(pMixPacket->flags==CHANNEL_CLOSE)
											{
												osocketgroupCacheRead.remove(*(pChannelListEntry->pSocket));
												osocketgroupCacheWrite.remove(*(pChannelListEntry->pSocket));
												pChannelListEntry->pSocket->close();
												delete pChannelListEntry->pSocket;
												delete pChannelListEntry->pCipher;
												delete pChannelListEntry->pQueueSend;										
												m_pChannelList->removeChannel(pMixPacket->channel);
												#ifdef LOG_PACKET_TIMES
													getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
													set64(pQueueEntry->timestamp_proccessing_end_OP,pQueueEntry->timestamp_proccessing_end);
													m_pLogPacketStats->addToTimeingStats(*pQueueEntry,CHANNEL_CLOSE,true);
													#ifdef LOG_CHANNEL
														MACRO_DO_LOG_CHANNEL
													#endif
												#endif
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
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_DEBUG,"Resumeing channel %u Socket: %u\n",pMixPacket->channel,(SOCKET)(*pChannelListEntry->pSocket));
												#endif
	
#ifdef HAVE_EPOLL
												osocketgroupCacheRead.add(*(pChannelListEntry->pSocket),pChannelListEntry);
#else
												osocketgroupCacheRead.add(*(pChannelListEntry->pSocket));
#endif
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
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
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
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
															MACRO_DO_LOG_CHANNEL
														#endif
														delete pChannelListEntry->pSocket;
														delete pChannelListEntry->pCipher;
														delete pChannelListEntry->pQueueSend;
														m_pChannelList->removeChannel(pMixPacket->channel);
														getRandom(pMixPacket->data,DATA_SIZE);
														pMixPacket->flags=CHANNEL_CLOSE;
														#ifdef LOG_PACKET_TIMES
															setZero64(pQueueEntry->timestamp_proccessing_start);
														#endif
														m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));			
														m_logDownloadedPackets++;	
													}
												else
													{
#ifdef HAVE_EPOLL
														osocketgroupCacheWrite.add(*(pChannelListEntry->pSocket),pChannelListEntry);
#else
														osocketgroupCacheWrite.add(*(pChannelListEntry->pSocket));
#endif
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
															m_pLogPacketStats->addToTimeingStats(*pQueueEntry,CHANNEL_DATA,true);
														#endif
													}
											}
									}
							}
					}
//end Step 1

//Step 2 Sending to Cache...
				countRead=osocketgroupCacheWrite.select(0);
				if(countRead>0)
					{
						bAktiv=true;
#ifdef HAVE_EPOLL
						pChannelListEntry=(lmChannelListEntry*)osocketgroupCacheWrite.getFirstSignaledSocketData();
						while(pChannelListEntry!=NULL)
							{
#else
						pChannelListEntry=m_pChannelList->getFirstSocket();
						while(pChannelListEntry!=NULL&&countRead>0)
							{
								if(osocketgroupCacheWrite.isSignaled(*(pChannelListEntry->pSocket)))
									{
										countRead--;
#endif
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
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
															MACRO_DO_LOG_CHANNEL
														#endif
														delete pChannelListEntry->pSocket;
														delete pChannelListEntry->pCipher;
														delete pChannelListEntry->pQueueSend;
														pMixPacket->flags=CHANNEL_CLOSE;
														getRandom(pMixPacket->data,DATA_SIZE);
														pMixPacket->channel=pChannelListEntry->channelIn;
														#ifdef LOG_PACKET_TIMES
															setZero64(pQueueEntry->timestamp_proccessing_start);
														#endif
														m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));			
														m_logDownloadedPackets++;	
														m_pChannelList->removeChannel(pChannelListEntry->channelIn);											 
													}
											}
#ifdef HAVE_EPOLL
								pChannelListEntry=(lmChannelListEntry*)osocketgroupCacheWrite.getNextSignaledSocketData();
#else
									}
								pChannelListEntry=m_pChannelList->getNextSocket();
#endif
							}
					}
//End Step 2

//Step 3 Reading from Cache....
#define MAX_MIXIN_SEND_QUEUE_SIZE 1000000
				countRead=osocketgroupCacheRead.select(0);
				if(countRead>0)
					{
#ifdef HAVE_EPOLL
						pChannelListEntry=(lmChannelListEntry*)osocketgroupCacheRead.getFirstSignaledSocketData();
						while(pChannelListEntry!=NULL)
							{
#else
						pChannelListEntry=m_pChannelList->getFirstSocket();
						while(pChannelListEntry!=NULL&&countRead>0)
							{
								if(osocketgroupCacheRead.isSignaled(*(pChannelListEntry->pSocket)))
									{
										countRead--;
#endif
										if(m_pQueueSendToMix->getSize()<MAX_MIXIN_SEND_QUEUE_SIZE
												#ifdef DELAY_CHANNELS
													&&(pChannelListEntry->delayBucket>0)
												#endif
											)
											{
												bAktiv=true;
												#ifndef DELAY_CHANNELS
													ret=pChannelListEntry->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE);
												#else
													UINT32 readLen=min(pChannelListEntry->delayBucket,PAYLOAD_SIZE);
													ret=pChannelListEntry->pSocket->receive(pMixPacket->payload.data,readLen);
												#endif
												#ifdef LOG_PACKET_TIMES
													getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start);
													set64(pQueueEntry->timestamp_proccessing_start_OP,pQueueEntry->timestamp_proccessing_start);
												#endif
												if(ret==SOCKET_ERROR||ret==0)
													{
														osocketgroupCacheRead.remove(*(pChannelListEntry->pSocket));
														osocketgroupCacheWrite.remove(*(pChannelListEntry->pSocket));
														pChannelListEntry->pSocket->close();
														#ifdef LOG_CHANNEL
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
															MACRO_DO_LOG_CHANNEL
														#endif
														delete pChannelListEntry->pSocket;
														delete pChannelListEntry->pCipher;
														delete pChannelListEntry->pQueueSend;
														pMixPacket->flags=CHANNEL_CLOSE;
														pMixPacket->channel=pChannelListEntry->channelIn;
														getRandom(pMixPacket->data,DATA_SIZE);
														m_pChannelList->removeChannel(pChannelListEntry->channelIn);
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
														#endif
														m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));			
														m_logDownloadedPackets++;	
													}
												else 
													{
														add64((UINT64&)m_logDownloadedBytes,ret);
														#if defined(LOG_CHANNEL)
															pChannelListEntry->trafficOutToUser+=ret;
														#endif
														#ifdef DELAY_CHANNELS
															pChannelListEntry->delayBucket-=ret;
														#endif
														pMixPacket->channel=pChannelListEntry->channelIn;
														pMixPacket->flags=CHANNEL_DATA;
														pMixPacket->payload.len=htons((UINT16)ret);
														pMixPacket->payload.type=0;
														pChannelListEntry->pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
														#endif
														m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));			
														m_logDownloadedPackets++;
														#if defined(LOG_CHANNEL)
															pChannelListEntry->packetsDataOutToUser++;
														#endif													
													}
											}
										else
											break;
#ifdef HAVE_EPOLL
								pChannelListEntry=(lmChannelListEntry*)osocketgroupCacheRead.getNextSignaledSocketData();
#else
									}
								pChannelListEntry=m_pChannelList->getNextSocket();
#endif
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
		//writng some bytes to the queue...
		UINT8 b[sizeof(tQueueEntry)+1];
		m_pQueueSendToMix->add(b,sizeof(tQueueEntry)+1);
		CAMsg::printMsg(LOG_CRIT,"Wait for LoopSendToMix!\n");
		m_pthreadSendToMix->join(); //will not join if queue is empty (and so wating)!!!
		m_bRunLog=false;
		CAMsg::printMsg(LOG_CRIT,"Wait for LoopReadFromMix!\n");
		m_pthreadReadFromMix->join();
		CAMsg::printMsg(LOG_CRIT,"done.\n");
		#ifdef LOG_PACKET_TIMES
			CAMsg::printMsg(LOG_CRIT,"Wait for LoopLogPacketStats to terminate!\n");
			m_pLogPacketStats->stop();
		#endif	
		pChannelListEntry=m_pChannelList->getFirstSocket();
		while(pChannelListEntry!=NULL)
			{
				delete pChannelListEntry->pCipher;
				delete pChannelListEntry->pQueueSend;
				pChannelListEntry->pSocket->close();
				delete pChannelListEntry->pSocket;
				pChannelListEntry=m_pChannelList->getNextSocket();
			}
		delete []tmpBuff;
		delete pQueueEntry;
		oLogThread.join();
#endif //! NEW_MIX_TYPE
		return E_UNKNOWN;
	}

