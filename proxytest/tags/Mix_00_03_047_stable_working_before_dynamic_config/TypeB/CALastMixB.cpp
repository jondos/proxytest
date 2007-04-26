#include "../StdAfx.h"
#include "CALastMixB.hpp"
#include "../CALastMixChannelList.hpp"
#include "../CASingleSocketGroup.hpp"

#define _SEND_TIMEOUT 2000
SINT32 CALastMixB::loop()
	{
/*
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
												newCipher->setKeyAES(rsaBuff);
												newCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																							pMixPacket->data+RSA_SIZE-(KEY_SIZE+TIMESTAMP_SIZE),
																							DATA_SIZE-RSA_SIZE);
												memcpy(	pMixPacket->data,rsaBuff+KEY_SIZE+TIMESTAMP_SIZE,
																RSA_SIZE-(KEY_SIZE+TIMESTAMP_SIZE));
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
															add64(pChannelListEntry->timeNextSend,delayTime);
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
		oLogThread.join();*/
		return E_UNKNOWN;
	}
