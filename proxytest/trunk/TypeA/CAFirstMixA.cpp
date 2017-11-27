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
#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_FIRST_MIX
#include "CAFirstMixA.hpp"
#include "../CALibProxytest.hpp"
#include "../CAThread.hpp"
#include "../CASingleSocketGroup.hpp"
#include "../CAInfoService.hpp"
#include "../CAPool.hpp"
#include "../CACmdLnOptions.hpp"
#include "../CAAccountingInstance.hpp"
#include "../CAStatusManager.hpp"
#ifdef HAVE_EPOLL
	#include "../CASocketGroupEpoll.hpp"
#endif

void CAFirstMixA::shutDown()
{
	m_bIsShuttingDown = true;
	m_bLoop=false;
//#ifdef PAYMENT
	UINT32 connectionsClosed = 0;
	fmHashTableEntry* timeoutHashEntry;

#ifdef PAYMENT

	/* make sure no reconnect is possible when shutting down */
	if(m_pthreadAcceptUsers!=NULL)
	{
		CAMsg::printMsg(LOG_CRIT,"Wait for LoopAcceptUsers!\n");
		m_bRestart=true;
		m_pthreadAcceptUsers->join();
		delete m_pthreadAcceptUsers;
	}
	m_pthreadAcceptUsers=NULL;

	if(m_pInfoService != NULL)
	{
		CAMsg::printMsg(LOG_DEBUG,"Shutting down infoservice.\n");
		m_pInfoService->stop();
	}
	if(m_pChannelList!=NULL) // may happen if mixes did not yet connect to each other
	{
		while ((timeoutHashEntry = m_pChannelList->popTimeoutEntry(true)) != NULL)
		{
			CAMsg::printMsg(LOG_DEBUG,"Shutting down, closing client connection.\n");
			connectionsClosed++;
			closeConnection(timeoutHashEntry);
		}
		CAMsg::printMsg(LOG_DEBUG,"Closed %i client connections.\n", connectionsClosed);
	}
#endif
	m_bRestart = true;
	m_bIsShuttingDown = false;
}

#ifndef MULTI_THREADED_PACKET_PROCESSING

SINT32 CAFirstMixA::closeConnection(fmHashTableEntry* pHashEntry)
{
	if (pHashEntry == NULL)
	{
		return E_UNKNOWN;
	}

	INIT_STACK;
	BEGIN_STACK("CAFirstMixA::closeConnection");


	fmChannelListEntry* pEntry;
	tQueueEntry* pQueueEntry = new tQueueEntry;
	MIXPACKET* pMixPacket=&pQueueEntry->packet;

	#ifdef LOG_TRAFFIC_PER_USER
		UINT64 current_time;
		getcurrentTimeMillis(current_time);
		CAMsg::printMsg(LOG_DEBUG,"Removing Connection wiht ID: %Lu -- login time [ms] %Lu -- logout time [ms] %Lu -- Traffic was: IN: %u  --  OUT: %u\n",pHashEntry->id,pHashEntry->timeCreated,current_time,pHashEntry->trafficIn,pHashEntry->trafficOut);
	#endif
	m_pIPList->removeIP(pHashEntry->peerIP);

	m_psocketgroupUsersRead->remove(*(pHashEntry->pMuxSocket));
	m_psocketgroupUsersWrite->remove(*(pHashEntry->pMuxSocket));
	pEntry = m_pChannelList->getFirstChannelForSocket(pHashEntry->pMuxSocket);

	while(pEntry!=NULL)
	{
		getRandom(pMixPacket->data,DATA_SIZE);
		pMixPacket->flags=CHANNEL_CLOSE;
		pMixPacket->channel=pEntry->channelOut;
		#ifdef LOG_PACKET_TIMES
			setZero64(pQueueEntry->timestamp_proccessing_start);
		#endif
		m_pQueueSendToMix->add(pQueueEntry, sizeof(tQueueEntry));
		delete pEntry->pCipher;
		pEntry->pCipher = NULL;
		pEntry=m_pChannelList->getNextChannel(pEntry);
#ifdef CH_LOG_STUDY
		nrOfChOpMutex->lock();
		currentOpenedChannels--;
		nrOfChOpMutex->unlock();
#endif //CH_LOG_STUDY
	}
	ASSERT(pHashEntry->pQueueSend!=NULL,"Send queue is NULL");
	delete pHashEntry->pQueueSend;
	pHashEntry->pQueueSend = NULL;
	delete pHashEntry->pSymCipher;
	pHashEntry->pSymCipher = NULL;

	#ifdef COUNTRY_STATS
		decUsers(pHashEntry);
	#else
		decUsers();
	#endif

	CAMuxSocket* pMuxSocket = pHashEntry->pMuxSocket;
	// Save the socket - its pointer will be deleted in this method!!! Crazy mad programming...
	m_pChannelList->remove(pHashEntry->pMuxSocket);
	delete pMuxSocket;
	pMuxSocket = NULL;
	//pHashEntry->pMuxSocket = NULL; // not needed now, but maybe in the future...

	delete pQueueEntry;
	pQueueEntry = NULL;

	FINISH_STACK("CAFirstMixA::closeConnection");

	return E_SUCCESS;
}

#define FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS 2*KEY_SIZE

SINT32 CAFirstMixA::loop()
	{
#ifndef NEW_MIX_TYPE
#ifdef DELAY_USERS
		m_pChannelList->setDelayParameters(	CALibProxytest::getOptions()->getDelayChannelUnlimitTraffic(),
																			CALibProxytest::getOptions()->getDelayChannelBucketGrow(),
																			CALibProxytest::getOptions()->getDelayChannelBucketGrowIntervall());
#endif

	//	CASingleSocketGroup osocketgroupMixOut;
		SINT32 countRead=0;
		//#ifdef LOG_PACKET_TIMES
		//	tPoolEntry* pPoolEntry=new tPoolEntry;
		//	MIXPACKET* pMixPacket=&pPoolEntry->mixpacket;
		//#else
		tQueueEntry* pQueueEntry = new tQueueEntry;
		MIXPACKET* pMixPacket=&pQueueEntry->packet;
		//#endif
		m_nUser=0;
		SINT32 ret;
		//osocketgroupMixOut.add(*m_pMuxOut);

		UINT8* tmpBuff=new UINT8[sizeof(tQueueEntry)];
		CAMsg::printMsg(LOG_DEBUG,"Starting Message Loop... \n");
		bool bAktiv;
		UINT8 rsaBuff[RSA_SIZE];

#ifdef LOG_TRAFFIC_PER_USER
		UINT64 current_time;
		UINT32 diff_time;
		CAMsg::printMsg(LOG_DEBUG,"Channel log formats:\n");
		CAMsg::printMsg(LOG_DEBUG,"1. Close received from user (times in micros) - 1:Channel-ID,Connection-ID,Channel open timestamp (microseconds),PacketsIn (only data and open),PacketsOut (only data),ChannelDuration (open packet received --> close packet put into send queue to next mix)\n");
		CAMsg::printMsg(LOG_DEBUG,"2. Channel close from Mix(times in micros)- 2.:Channel-ID,Connection-ID,Channel open timestamp (microseconds),PacketsIn (only data and open), PacketsOut (only data),ChannelDuration (open packet received)--> close packet put into send queue to next user\n");
#endif
/** @todo check if thread is closed */
#ifdef _DEBUG
		CAThread* pLogThread=new CAThread((UINT8*)"CAFirstMixA - LogLoop");
		pLogThread->setMainLoop(fm_loopLog);
		pLogThread->start(this);
#endif

#ifdef LOG_CRIME
		CAIPAddrWithNetmask* surveillanceIPs = CALibProxytest::getOptions()->getCrimeSurveillanceIPs();
		UINT32 nrOfSurveillanceIPs = CALibProxytest::getOptions()->getNrOfCrimeSurveillanceIPs();
#endif
//		CAThread threadReadFromUsers;
//		threadReadFromUsers.setMainLoop(loopReadFromUsers);
//		threadReadFromUsers.start(this);

		while(!m_bRestart) /* the main mix loop as long as there are things that are not handled by threads. */
			{

				bAktiv=false;

//LOOP_START:
#ifdef PAYMENT
				// while checking if there are connections to close: synch with login threads
				m_pmutexLogin->lock();
				checkUserConnections();
				m_pmutexLogin->unlock();
#endif
//First Step
//Checking for new connections
// Now in a separate Thread....

// Second Step
// Checking for data from users
// Now in a separate Thread (see loopReadFromUsers())
//Only proccess user data, if queue to next mix is not to long!!

				if(m_pQueueSendToMix->getSize()<MAX_NEXT_MIX_QUEUE_SIZE)
					{
						countRead=m_psocketgroupUsersRead->select(/*false,*/0);				// how many JAP<->mix connections have received data from their coresponding JAP
						if(countRead>0)
							bAktiv=true;
#ifdef HAVE_EPOLL
						//if we have epoll we do not need to search the whole list
						//of connected JAPs to find the ones who have sent data
						//as epoll will return ONLY these connections.
						fmHashTableEntry* pHashEntry=(fmHashTableEntry*)m_psocketgroupUsersRead->getFirstSignaledSocketData();
						while(pHashEntry!=NULL)
							{
								CAMuxSocket* pMuxSocket=pHashEntry->pMuxSocket;
#else
						//if we do not have epoll we have to go to the whole
						//list of open connections to find the ones which
						//actually have sent some data
						fmHashTableEntry* pHashEntry=m_pChannelList->getFirst();
						while(pHashEntry!=NULL&&countRead>0)											// iterate through all connections as long as there is at least one active left
							{
								CAMuxSocket* pMuxSocket=pHashEntry->pMuxSocket;
								if(m_psocketgroupUsersRead->isSignaled(*pMuxSocket))	// if this one seems to have data
									{
#endif
/*#ifdef DELAY_USERS
 * Don't delay upstream
								if( m_pChannelList->hasDelayBuckets(pHashEntry->delayBucketID) )
								{
#endif*/
										countRead--;
										ret=pMuxSocket->receive(pMixPacket,0);

										#if defined LOG_PACKET_TIMES||defined(LOG_CHANNEL)
											getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start);
											set64(pQueueEntry->timestamp_proccessing_start_OP,pQueueEntry->timestamp_proccessing_start);
										#endif
										#ifdef DATA_RETENTION_LOG
											pQueueEntry->dataRetentionLogEntry.t_in=htonl(time(NULL));
										#endif
										if(ret<0&&ret!=E_AGAIN/*||pHashEntry->accessUntil<time()*/)
										{
											// remove dead connections
											closeConnection(pHashEntry);
										}
										else if(ret==MIXPACKET_SIZE) 											// we've read enough data for a whole mix packet. nice!
											{
#ifdef PAYMENT
												if (pHashEntry->bRecoverTimeout)
												{
													// renew the timeout only if recovery is allowed
													m_pChannelList->pushTimeoutEntry(pHashEntry);
												}
#endif
												#ifdef LOG_TRAFFIC_PER_USER
													pHashEntry->trafficIn++;
												#endif
												#ifdef COUNTRY_STATS
													m_PacketsPerCountryIN[pHashEntry->countryID].inc();
												#endif
												//New control channel code...!
												if(pMixPacket->channel>0&&pMixPacket->channel<256)
												{
													if (pHashEntry->pControlChannelDispatcher->proccessMixPacket(pMixPacket))
													{
														goto NEXT_USER;
													}
													else
													{
														CAMsg::printMsg(LOG_DEBUG, "Control channel packet is invalid and could not be processed!\n");
														closeConnection(pHashEntry);
														goto NEXT_USER;
													}
												}
#ifdef ANON_DEBUG_MODE
												bool bIsDebugPacket=false;
												if (pMixPacket->flags&CHANNEL_DEBUG)
													{
													bIsDebugPacket=true;
													UINT8 base64Payload[DATA_SIZE << 1];
													EVP_EncodeBlock(base64Payload, pMixPacket->data, DATA_SIZE);//base64 encoding (without newline!)
													pMixPacket->flags &= ~CHANNEL_DEBUG;
													CAMsg::printMsg(LOG_DEBUG, "AN.ON packet debug: %s\n", base64Payload);
													}

#endif
#ifdef PAYMENT
												if(accountTrafficUpstream(pHashEntry) != E_SUCCESS) goto NEXT_USER;
#endif
												if(pMixPacket->flags==CHANNEL_DUMMY) // just a dummy to keep the connection alife in e.g. NAT gateways
												{
													CAMsg::printMsg(LOG_DEBUG,"received dummy traffic\n");
													getRandom(pMixPacket->data,DATA_SIZE);
													#ifdef LOG_PACKET_TIMES
														setZero64(pQueueEntry->timestamp_proccessing_start);
													#endif
													#ifdef LOG_TRAFFIC_PER_USER
														pHashEntry->trafficOut++;
													#endif
													#ifdef COUNTRY_STATS
														m_PacketsPerCountryOUT[pHashEntry->countryID].inc();
													#endif
													pHashEntry->pQueueSend->add(pQueueEntry,sizeof(tQueueEntry));
													#ifdef HAVE_EPOLL
														//m_psocketgroupUsersWrite->add(*pMuxSocket,pHashEntry);
													#else
														m_psocketgroupUsersWrite->add(*pMuxSocket);
													#endif
												}
												else if(pMixPacket->flags==CHANNEL_CLOSE)			// closing one mix-channel (not the JAP<->mix connection!)
												{
													fmChannelListEntry* pEntry;
													pEntry=m_pChannelList->get(pMuxSocket,pMixPacket->channel);
													if(pEntry!=NULL)
													{
														pMixPacket->channel=pEntry->channelOut;
														getRandom(pMixPacket->data,DATA_SIZE);
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
														#endif
														m_pQueueSendToMix->add(pQueueEntry, sizeof(tQueueEntry));
														/* Don't delay upstream
														#ifdef DELAY_USERS
														m_pChannelList->decDelayBuckets(pHashEntry->delayBucketID);
														#endif*/
														#ifdef LOG_CHANNEL
															//pEntry->packetsInFromUser++;
															getcurrentTimeMicros(current_time);
															diff_time=diff64(current_time,pEntry->timeCreated);
															CAMsg::printMsg(LOG_DEBUG,"1:%u,%Lu,%Lu,%u,%u,%u\n",
																												pEntry->channelIn,pEntry->pHead->id,pEntry->timeCreated,pEntry->packetsInFromUser,pEntry->packetsOutToUser,
																												diff_time);
														#endif
														delete pEntry->pCipher;              // forget the symetric key of this connection
														pEntry->pCipher = NULL;
														m_pChannelList->removeChannel(pMuxSocket,pEntry->channelIn);

														#ifdef CH_LOG_STUDY
														nrOfChOpMutex->lock();
														currentOpenedChannels--;
														nrOfChOpMutex->unlock();
														#endif //CH_LOG_STUDY
													}
													#ifdef _DEBUG
													else
													{
//														CAMsg::printMsg(LOG_DEBUG,"Invalid ID to close from Browser!\n");
													}
													#endif
												}
												else		                                     // finally! a normal mix packet
												{
													CASymCipher* pCipher=NULL;
													fmChannelListEntry* pEntry;
													pEntry=m_pChannelList->get(pMuxSocket,pMixPacket->channel);
													if (pEntry != NULL&&pMixPacket->flags == CHANNEL_DATA)
														{
														pMixPacket->channel = pEntry->channelOut;
														pCipher = pEntry->pCipher;
														pCipher->crypt1(pMixPacket->data, pMixPacket->data, DATA_SIZE);
														// queue the packet for sending to the next mix.
#ifdef LOG_PACKET_TIMES
														getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
#endif

														//check if this IP must be logged due to crime detection
#ifdef LOG_CRIME
														crimeSurveillance(surveillanceIPs, nrOfSurveillanceIPs, pEntry->pHead->peerIP,pEntry->pHead->peerPort, pMixPacket);
#endif
#ifdef ANON_DEBUG_MODE
														if(bIsDebugPacket)
															{
															pMixPacket->flags|=CHANNEL_DEBUG;
															}
														#endif
														m_pQueueSendToMix->add(pQueueEntry, sizeof(tQueueEntry));
														/* Don't delay upstream
														#ifdef DELAY_USERS
														m_pChannelList->decDelayBuckets(pHashEntry->delayBucketID);
														#endif*/
														incMixedPackets();
														#ifdef LOG_CHANNEL
															pEntry->packetsInFromUser++;
														#endif
													}
													else if(pEntry==NULL&&pMixPacket->flags==CHANNEL_OPEN)  // open a new mix channel
													{ // stefan: muesste das nicht vor die behandlung von CHANNEL_DATA? oder gilt OPEN => !DATA ?
														//es gilt: open -> data
														pHashEntry->pSymCipher->crypt1(pMixPacket->data,rsaBuff,FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS);
														#ifdef REPLAY_DETECTION
														// replace time(NULL) with the real timestamp ()
														// packet-timestamp*REPLAY_BASE + m_u64ReferenceTime
															if(m_pReplayDB->insert(rsaBuff,time(NULL))!=E_SUCCESS)
															{
																CAMsg::printMsg(LOG_INFO,"Replay: Duplicate packet ignored.\n");
																continue;
															}
														#endif
														pCipher= new CASymCipher();
														pCipher->setKeys(rsaBuff,FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS);
														for(int i=0;i<16;i++)
															rsaBuff[i]=0xFF;
														pCipher->setIV2(rsaBuff);
														pCipher->crypt1(pMixPacket->data+FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS,pMixPacket->data,DATA_SIZE-FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS);
														getRandom(pMixPacket->data+DATA_SIZE-FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS,FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS);
														#if defined (LOG_CHANNEL) ||defined(DATA_RETENTION_LOG)
															HCHANNEL tmpC=pMixPacket->channel;
														#endif

														HCHANNEL inChannel = pMixPacket->channel;
														if(m_pChannelList->addChannel(pMuxSocket,pMixPacket->channel,pCipher,&pMixPacket->channel)!=E_SUCCESS)
														{ //todo move up ?
															delete pCipher;
															pCipher = NULL;
														}
														else
														{
															#ifdef CH_LOG_STUDY
															nrOfChOpMutex->lock();
															if(pHashEntry->channelOpenedLastIntervalTS !=
																lastLogTime)
															{
																pHashEntry->channelOpenedLastIntervalTS =
																	lastLogTime;
																nrOfOpenedChannels++;
															}
															currentOpenedChannels++;
															nrOfChOpMutex->unlock();
															#endif //CH_LOG_STUDY

															#ifdef LOG_PACKET_TIMES
																getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
															#endif
															#ifdef LOG_CHANNEL
																fmChannelListEntry* pTmpEntry=m_pChannelList->get(pMuxSocket,tmpC);
																pTmpEntry->packetsInFromUser++;
																set64(pTmpEntry->timeCreated,pQueueEntry->timestamp_proccessing_start);
															#endif
															#ifdef DATA_RETENTION_LOG
																pQueueEntry->dataRetentionLogEntry.entity.first.channelid=htonl(pMixPacket->channel);
																fmChannelListEntry* pTmpEntry1=m_pChannelList->get(pMuxSocket,tmpC);
																memcpy(pQueueEntry->dataRetentionLogEntry.entity.first.ip_in,pTmpEntry1->pHead->peerIP,4);
																pQueueEntry->dataRetentionLogEntry.entity.first.port_in=(UINT16)pTmpEntry1->pHead->peerPort;
																pQueueEntry->dataRetentionLogEntry.entity.first.port_in=htons(pQueueEntry->dataRetentionLogEntry.entity.first.port_in);
															#endif

															//check if this IP must be logged due to crime detection
															#ifdef LOG_CRIME

																pEntry=m_pChannelList->get(pMuxSocket, inChannel);
																if(pEntry != NULL)
																{
																	crimeSurveillance(surveillanceIPs, nrOfSurveillanceIPs, pEntry->pHead->peerIP,pEntry->pHead->peerPort, pMixPacket);
																}
															#endif
#ifdef ANON_DEBUG_MODE
																if (bIsDebugPacket)
																	{
																		pMixPacket->flags |= CHANNEL_DEBUG;
																		pEntry = m_pChannelList->get(pMuxSocket, inChannel);
																		pEntry->bDebug = true;
																	}
#endif
															m_pQueueSendToMix->add(pQueueEntry, sizeof(tQueueEntry));
															/* Don't delay upstream
															#ifdef DELAY_USERS
															m_pChannelList->decDelayBuckets(pHashEntry->delayBucketID);
															#endif*/
															incMixedPackets();
															#ifdef _DEBUG
//																			CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",pMixPacket->channel);
															#endif
														}
													}
												}
											}
/*#ifdef DELAY_USERS
									}
#endif*/
								#ifdef HAVE_EPOLL
NEXT_USER:
									pHashEntry=(fmHashTableEntry*)m_psocketgroupUsersRead->getNextSignaledSocketData();
								#else
									}//if is signaled
NEXT_USER:
									pHashEntry=m_pChannelList->getNext();
								#endif
							}
							if(countRead>0)
							{
								CAMsg::printMsg(LOG_DEBUG,"CAFirstMixA::loop() - read from user --> countRead >0 after processing all connections!\n");
							}
					}
//Third step
//Sending to next mix

// Now in a separate Thread (see loopSendToMix())

//Step 4
//Step 4a Receiving from mix to queue now in a separate thread
//Step 4b Processing MixPackets received from Mix
//todo check for error!!!
				countRead=m_nUser+1;
				while(countRead>0&&m_pQueueReadFromMix->getSize()>=sizeof(tQueueEntry))
					{
						bAktiv=true;
						countRead--;
						ret=sizeof(tQueueEntry);
						m_pQueueReadFromMix->get((UINT8*)pQueueEntry,(UINT32*)&ret);

						#ifdef LOG_PACKET_TIMES
							getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start_OP);
						#endif
#ifdef ANON_DEBUG_MODE
						if (pMixPacket->flags&CHANNEL_DEBUG)
							{
								UINT8 base64Payload[DATA_SIZE << 1];
								EVP_EncodeBlock(base64Payload, pMixPacket->data, DATA_SIZE);//base64 encoding (without newline!)
								CAMsg::printMsg(LOG_DEBUG, "Dequeued Downstream AN.ON packet from previous Mix debug: %s\n", base64Payload);
								pMixPacket->flags &= ~CHANNEL_DEBUG;
							}

#endif

						if(pMixPacket->flags==CHANNEL_CLOSE) //close event
							{
								#if defined(_DEBUG) && !defined(__MIX_TEST)
//									CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ...\n",pMixPacket->channel);
								#endif
								fmChannelList* pEntry=m_pChannelList->get(pMixPacket->channel);
								if(pEntry!=NULL)
									{
										/* a hack to solve the SSL problem:
										 * set channel of downstream packet to in channel after they are dequeued
										 * from pEntry->pQueueSend so we can retrieve the channel entry to decrement
										 * the per channel count of enqueued downstream bytes.
										 */
										#ifndef SSL_HACK
										pMixPacket->channel=pEntry->channelIn;
										#endif

										pEntry->pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										//getRandom(pMixPacket->data,DATA_SIZE);
										#ifdef LOG_PACKET_TIMES
											getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
										#endif
										pEntry->pHead->pQueueSend->add(pQueueEntry, sizeof(tQueueEntry));
										#ifdef LOG_TRAFFIC_PER_USER
											pEntry->pHead->trafficOut++;
										#endif
										#ifdef COUNTRY_STATS
											m_PacketsPerCountryOUT[pEntry->pHead->countryID].inc();
										#endif
										#ifdef SSL_HACK
											/* a hack to solve the SSL problem:
											 * per channel count of enqueued downstream bytes
											 */
											pEntry->downStreamBytes += sizeof(tQueueEntry);
										#endif
										#ifdef LOG_CHANNEL
											pEntry->packetsOutToUser++;
											getcurrentTimeMicros(current_time);
											diff_time=diff64(current_time,pEntry->timeCreated);
											CAMsg::printMsg(LOG_DEBUG,"2:%u,%Lu,%Lu,%u,%u,%u\n",
																								pEntry->channelIn,pEntry->pHead->id,pEntry->timeCreated,pEntry->packetsInFromUser,pEntry->packetsOutToUser,
																								diff_time);
										#endif

										#ifdef HAVE_EPOLL
											//m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket,pEntry->pHead);
										#else
											m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket);
										#endif


										#ifndef SSL_HACK
											delete pEntry->pCipher;              // forget the symetric key of this connection
											pEntry->pCipher = NULL;
											m_pChannelList->removeChannel(pEntry->pHead->pMuxSocket, pEntry->channelIn);
										#ifdef CH_LOG_STUDY
											nrOfChOpMutex->lock();
											currentOpenedChannels--;
											nrOfChOpMutex->unlock();
										#endif
										/* a hack to solve the SSL problem:
										 * remove channel after the close packet is enqueued
										 * from pEntry->pQueueSend
										 */
										#endif
									}
									else
									{
										#ifdef DEBUG
											CAMsg::printMsg(LOG_DEBUG, "CAFirstMixA: close channel -> client but channel does not exist.\n");
										#endif
									}

							}
						else
							{//flag !=close
								#if defined(_DEBUG) && !defined(__MIX_TEST)
//									CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!\n");
								#endif
								fmChannelList* pEntry=m_pChannelList->get(pMixPacket->channel);

								if(pEntry!=NULL)
									{
										#ifdef LOG_CRIME
											if((pMixPacket->flags&CHANNEL_SIG_CRIME)==CHANNEL_SIG_CRIME)
												{
													//UINT32 id=(pMixPacket->flags>>8)&0x000000FF;
													int log=LOG_ENCRYPTED;
													if(!CALibProxytest::getOptions()->isEncryptedLogEnabled())
														log=LOG_CRIT;
													CAMsg::printMsg(log,"Detecting crime activity - next mix channel: %u -- "
															"Incoming (User) IP:Port is: %u.%u.%u.%u:%u \n", pMixPacket->channel,
															pEntry->pHead->peerIP[0],
															pEntry->pHead->peerIP[1],
															pEntry->pHead->peerIP[2],
															pEntry->pHead->peerIP[3],
															pEntry->pHead->peerPort);
													continue;
												}
										#endif

										/* a hack to solve the SSL problem:
										 * same as CHANNEL_CLOSE packets
										 */
										#ifndef SSL_HACK
										pMixPacket->channel=pEntry->channelIn;
										#endif

										pEntry->pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);

										#ifdef LOG_PACKET_TIMES
											getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
										#endif
#ifdef ANON_DEBUG_MODE
											if (pEntry->bDebug)
												{	
												pMixPacket->flags |= CHANNEL_DEBUG;
												UINT8 base64Payload[DATA_SIZE << 1];
												EVP_EncodeBlock(base64Payload, pMixPacket->data, DATA_SIZE);//base64 encoding (without newline!)
												CAMsg::printMsg(LOG_DEBUG, "Send Dowwstream AN.ON packet debug: %s\n", base64Payload);
												}

#endif
											pEntry->pHead->pQueueSend->add(pQueueEntry, sizeof(tQueueEntry));
										/*CAMsg::printMsg(
												LOG_INFO,"adding data packet to queue: %x, queue size: %u bytes\n",
												pEntry->pHead->pQueueSend, pEntry->pHead->pQueueSend->getSize());*/
										#ifdef LOG_TRAFFIC_PER_USER
											pEntry->pHead->trafficOut++;
										#endif
										#ifdef COUNTRY_STATS
											m_PacketsPerCountryOUT[pEntry->pHead->countryID].inc();
										#endif
										#ifdef LOG_CHANNEL
											pEntry->packetsOutToUser++;
										#endif
										#ifdef SSL_HACK
											/* a hack to solve the SSL problem:
											 * per channel count of downstream packets in bytes
											 */
											pEntry->downStreamBytes += sizeof(tQueueEntry);
										#endif

										#ifdef HAVE_EPOLL
											/*int epret = m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket,pEntry->pHead);
											if(epret == E_UNKNOWN)
											{
												epret=errno;
												CAMsg::printMsg(LOG_INFO,"epoll_add returns: %s (return value: %d) \n", strerror(epret), epret);
											}*/
											#else
											m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket);
										#endif

										incMixedPackets();
									}
								else
									{
										#ifdef _DEBUG
											if(pMixPacket->flags!=CHANNEL_DUMMY)
												{
/*													CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- "
															"Channel-Id %u not valid!\n",pMixPacket->channel
														);*/
													#ifdef LOG_CHANNEL
														CAMsg::printMsg(LOG_INFO,"Packet late arrive for channel: %u\n",pMixPacket->channel);
													#endif
												}
										#endif
									}
							}
					}

////Step 5
////Writing to users...
				bAktiv = sendToUsers();
#ifndef FAST_PROCESSING
				if(!bAktiv)
				  msSleep(1);
#endif
			}
//ERR:
		CAMsg::printMsg(LOG_CRIT,"Seems that we are restarting now!!\n");
		m_bRunLog=false;
		clean();
		delete pQueueEntry;
		pQueueEntry = NULL;
		delete []tmpBuff;
		tmpBuff = NULL;
#ifdef _DEBUG
		pLogThread->join();
		delete pLogThread;
		pLogThread = NULL;
#endif
		CAMsg::printMsg(LOG_CRIT,"Main Loop exited!!\n");
#endif//NEW_MIX_TYPE
		return E_UNKNOWN;
	}

/* last part of the main loop:
 * return true if the loop when at least one packet was sent
 * or false otherwise.
 */
bool CAFirstMixA::sendToUsers()
{
	SINT32 countRead = m_psocketgroupUsersWrite->select(/*true,*/0);
	tQueueEntry *packetToSend = NULL;
	SINT32 packetSize = sizeof(tQueueEntry);
	CAQueue *controlMessageUserQueue = NULL;
	CAQueue *dataMessageUserQueue = NULL;

	CAQueue *processedQueue = NULL; /* one the above queues that should be used for processing*/
	UINT32 extractSize = 0;
	bool bAktiv = false;
	UINT32 iSocketErrors = 0;

/* Cyclic polling: gets all open sockets that will not block when invoking send()
 * but will only send at most one packet. After that control is returned to loop()
 */
#ifdef HAVE_EPOLL
	fmHashTableEntry* pfmHashEntry=
		(fmHashTableEntry*) m_psocketgroupUsersWrite->getFirstSignaledSocketData();

	while(pfmHashEntry != NULL)
	{

#else
	fmHashTableEntry* pfmHashEntry=m_pChannelList->getFirst();
	while( (countRead > 0) && (pfmHashEntry != NULL) )
	{
		if(m_psocketgroupUsersWrite->isSignaled(*pfmHashEntry->pMuxSocket))
		{
			countRead--;
#endif
			/* loop turn init */
			extractSize = 0;
			processedQueue = NULL;
			packetToSend = &(pfmHashEntry->oQueueEntry);
			controlMessageUserQueue = pfmHashEntry->pControlMessageQueue;
			dataMessageUserQueue = pfmHashEntry->pQueueSend;

			//Control messages have a higher priority.
			if(controlMessageUserQueue->getSize() > 0)
			{
				processedQueue = controlMessageUserQueue;
				pfmHashEntry->bCountPacket = false;
			}
			else if( (dataMessageUserQueue->getSize() > 0)
#ifdef DELAY_USERS
					&& m_pChannelList->hasDelayBuckets(pfmHashEntry->delayBucketID)
#endif
			)
			{
				processedQueue = dataMessageUserQueue;
				pfmHashEntry->bCountPacket = true;
			}

			if(processedQueue != NULL)
			{
				extractSize = packetSize;
				bAktiv=true;

				if(pfmHashEntry->uAlreadySendPacketSize == -1)
				{
					processedQueue->get((UINT8*) packetToSend, &extractSize);

					/* Hack for SSL BUG */
#ifdef SSL_HACK
					finishPacket(pfmHashEntry);
#endif //SSL_HACK
#ifdef ANON_DEBUG_MODE
					if (packetToSend->packet.flags&CHANNEL_DEBUG)
						{
						UINT8 base64Payload[DATA_SIZE << 1];
						EVP_EncodeBlock(base64Payload, packetToSend->packet.data, DATA_SIZE);//base64 encoding (without newline!)
						CAMsg::printMsg(LOG_DEBUG, "Send Downstream AN.ON packet to user debug: %s\n", base64Payload);
						packetToSend->packet.flags &= ~CHANNEL_DEBUG;
						}

#endif
					pfmHashEntry->pMuxSocket->prepareForSend(&(packetToSend->packet));
					pfmHashEntry->uAlreadySendPacketSize = 0;
				}
			}

			if( (extractSize > 0) || (pfmHashEntry->uAlreadySendPacketSize > 0) )
			{
				SINT32 len =  MIXPACKET_SIZE - pfmHashEntry->uAlreadySendPacketSize;
				UINT8* packetToSendOffset = ((UINT8*)&(packetToSend->packet)) + pfmHashEntry->uAlreadySendPacketSize;
				CASocket* clientSocket = pfmHashEntry->pMuxSocket->getCASocket();

				SINT32 ret = clientSocket->send(packetToSendOffset, len);

				if(ret > 0)
				{
#ifdef PAYMENT
					SINT32 accounting = E_SUCCESS;
#endif
					pfmHashEntry->uAlreadySendPacketSize += ret;

					if(pfmHashEntry->uAlreadySendPacketSize == MIXPACKET_SIZE)
					{
						#ifdef DELAY_USERS
						if(processedQueue != controlMessageUserQueue)
						{
							m_pChannelList->decDelayBuckets(pfmHashEntry->delayBucketID);
						}
						#endif

						#ifdef LOG_PACKET_TIMES
							if(!isZero64(pfmHashEntry->oQueueEntry.timestamp_proccessing_start))
								{
									getcurrentTimeMicros(pfmHashEntry->oQueueEntry.timestamp_proccessing_end);
									m_pLogPacketStats->addToTimeingStats(pfmHashEntry->oQueueEntry,CHANNEL_DATA,false);
								}
						#endif
						pfmHashEntry->uAlreadySendPacketSize=-1;
#ifdef PAYMENT
						/* count this packet for accounting */
						accounting = accountTrafficDownstream(pfmHashEntry);
#endif
					}

				}
				else if(ret<0&&ret!=E_AGAIN)
				{
					iSocketErrors++;
					// if (iSocketErrors == 1) // show debug message only at the first error; otherwise, the log may get huge
					{
						SOCKET sock=clientSocket->getSocket();
						CAMsg::printMsg(LOG_DEBUG,"CAFirstMixA::sendtoUser() - send error %d on socket %d. Reason: %s (%i)\n", ret, sock, GET_NET_ERROR_STR(GET_NET_ERROR), GET_NET_ERROR);
					}
					// kick the user out - these only happens in extreme situations...
					closeConnection(pfmHashEntry);
				}
				//TODO error handling
			}

#ifdef HAVE_EPOLL
		pfmHashEntry=(fmHashTableEntry*)m_psocketgroupUsersWrite->getNextSignaledSocketData();
	}
#else
		}//if is socket signaled
		pfmHashEntry=m_pChannelList->getNext();
	}
#endif
	if (iSocketErrors > 1)
	{
		CAMsg::printMsg(LOG_ERR, "CAFirstMixA::sendtoUser() - %d send errors on a socket occured!\n", iSocketErrors);
	}
	
	return bAktiv;
}

#ifdef PAYMENT
SINT32 CAFirstMixA::accountTrafficUpstream(fmHashTableEntry* pHashEntry)
{
	SINT32 ret = E_SUCCESS;

	SINT32 handleResult = CAAccountingInstance::handleJapPacket(pHashEntry, false, false);

	if (CAAccountingInstance::HANDLE_PACKET_CONNECTION_OK == handleResult)
	{
		// renew the timeout
		//pHashEntry->bRecoverTimeout = true;
		m_pChannelList->pushTimeoutEntry(pHashEntry);
	}
	else if (CAAccountingInstance::HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION == handleResult)
	{
		// do not forward this packet
		pHashEntry->bRecoverTimeout = false;
		m_pChannelList->setKickoutForced(pHashEntry, KICKOUT_FORCED);
		CAMsg::printMsg(LOG_DEBUG, "CAFirstMixA: 1. setting bRecover timout to false for entry %x!\n", pHashEntry);
		//m_pChannelList->pushTimeoutEntry(pHashEntry);
		//don't let any upstream data messages pass for this user.
		ret = E_UNKNOWN;
	}
	else if (CAAccountingInstance::HANDLE_PACKET_CLOSE_CONNECTION == handleResult)
	{
		// kickout this user - he deserves it...
		CAMsg::printMsg(LOG_DEBUG, "CAFirstMixA: kickout upstream!\n");
		closeConnection(pHashEntry);
		ret = E_UNKNOWN;
	}
	//please remember that these values also may be returned even though they do not require
	//any further processing
	/*else if ( (ret == CAAccountingInstance::HANDLE_PACKET_CONNECTION_UNCHECKED) &&
				(ret == CAAccountingInstance::HANDLE_PACKET_HOLD_CONNECTION) )
	{

	}*/
	return ret;
}
#endif

#ifdef PAYMENT
SINT32 CAFirstMixA::accountTrafficDownstream(fmHashTableEntry* pfmHashEntry)
{
	// count packet for payment
	SINT32 ret = CAAccountingInstance::handleJapPacket(pfmHashEntry, !(pfmHashEntry->bCountPacket), true);
	if (ret == CAAccountingInstance::HANDLE_PACKET_CONNECTION_OK )
	{
		// renew the timeout
		//pfmHashEntry->bRecoverTimeout = true;
		m_pChannelList->pushTimeoutEntry(pfmHashEntry);
	}
	else if (ret == CAAccountingInstance::HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION )
	{
		// when all control messages are sent the users connection will be closed
		//pfmHashEntry->bRecoverTimeout = false;
		m_pChannelList->setKickoutForced(pfmHashEntry, KICKOUT_FORCED);
		//m_pChannelList->pushTimeoutEntry(pfmHashEntry);
	}
	else if (ret == CAAccountingInstance::HANDLE_PACKET_CLOSE_CONNECTION )
	{
		// close users connection immediately
		CAMsg::printMsg(LOG_DEBUG, "CAFirstMixA: Closing JAP connection due to illegal payment status!\n", ret);
		closeConnection(pfmHashEntry);
		return ERR_INTERN_SOCKET_CLOSED;
	}
	//please remember that these values also may be returned even though they do not require
	//any further processing
	/*else if ( (ret == CAAccountingInstance::HANDLE_PACKET_CONNECTION_UNCHECKED) &&
				(ret == CAAccountingInstance::HANDLE_PACKET_HOLD_CONNECTION) )
	{

	}*/
	return E_SUCCESS;
}
#endif

void CAFirstMixA::notifyAllUserChannels(fmHashTableEntry *pfmHashEntry, UINT16 flags)
{
	if(pfmHashEntry == NULL) 
		return;
	fmChannelListEntry* pEntry = m_pChannelList->getFirstChannelForSocket(pfmHashEntry->pMuxSocket);
	tQueueEntry* pQueueEntry=new tQueueEntry;
	MIXPACKET *notifyPacket = &(pQueueEntry->packet);
	memset(notifyPacket, 0, MIXPACKET_SIZE);

	notifyPacket->flags = flags;
	while(pEntry != NULL)
	{
		if(pEntry->bIsSuspended)
		{
			notifyPacket->channel = pEntry->channelOut;
			getRandom(notifyPacket->data,DATA_SIZE);
#ifdef _DEBUG
			CAMsg::printMsg(LOG_INFO,"Sent flags %u for channel: %u\n", flags, notifyPacket->channel);
#endif
			m_pQueueSendToMix->add(pQueueEntry, sizeof(tQueueEntry));
			pEntry->bIsSuspended = false;
		}
		pEntry=m_pChannelList->getNextChannel(pEntry);
	}
	pfmHashEntry->cSuspend=0;
	delete pQueueEntry;
}

//@todo: not a reliable solution. Still have to find the bug that causes SSL connections to be resetted
//while large downloads are performed by the same user (only occurs in cascades with more than two mixes)
#ifdef SSL_HACK

void CAFirstMixA::finishPacket(fmHashTableEntry *pfmHashEntry)
{
	tQueueEntry *packetToSend = &(pfmHashEntry->oQueueEntry);
	fmChannelList* cListEntry=m_pChannelList->get(packetToSend->packet.channel);
	if(cListEntry != NULL)
	{
		packetToSend->packet.channel = cListEntry->channelIn;
		cListEntry->downStreamBytes -= sizeof(tQueueEntry);
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "CAFirstMixA: channels of current packet, in: %u, out: %u, count: %u, flags: 0x%x\n",
				cListEntry->channelIn, cListEntry->channelOut, cListEntry->downStreamBytes,
				packetToSend->packet.flags);
#endif
		if(packetToSend->packet.flags == CHANNEL_CLOSE)
		{
			delete cListEntry->pCipher;
			cListEntry->pCipher = NULL;
			m_pChannelList->removeChannel(pfmHashEntry->pMuxSocket, cListEntry->channelIn);
#ifdef CH_LOG_STUDY
			nrOfChOpMutex->lock();
			currentOpenedChannels--;
			nrOfChOpMutex->unlock();
#endif //CH_LOG_STUDY
		}
	}
}

#endif //SSL_HACK

#ifdef PAYMENT
void CAFirstMixA::checkUserConnections()
{
	// check the timeout for all connections
	fmHashTableEntry* timeoutHashEntry;
	fmHashTableEntry* firstIteratorEntry = NULL;
	bool currentEntryKickoutForced = false;
	/* this check also includes forced kickouts which have not bRecoverTimeout set. */
	while ( (timeoutHashEntry = m_pChannelList->popTimeoutEntry(true)) != NULL )
	{
		currentEntryKickoutForced = m_pChannelList->isKickoutForced(timeoutHashEntry);
		if(firstIteratorEntry == timeoutHashEntry)
		{
			m_pChannelList->pushTimeoutEntry(timeoutHashEntry, currentEntryKickoutForced);
			break;
		}

		if (!currentEntryKickoutForced)
		{
			//CAMsg::printMsg(LOG_ERR, "%p\n, ", timeoutHashEntry);
			if(m_pChannelList->isTimedOut(timeoutHashEntry) )
			{
				CAMsg::printMsg(LOG_DEBUG,"Client connection closed due to timeout.\n");
				closeConnection(timeoutHashEntry);
				continue;
			}
		}
		else
		{
			//A user to be kicked out: empty his downstream data queue.
			timeoutHashEntry->pQueueSend->clean();

			if( (timeoutHashEntry->pControlMessageQueue->getSize() == 0) ||
				(timeoutHashEntry->kickoutSendRetries <= 0) )
			{
				CAMsg::printMsg(LOG_WARNING, "Kickout immediately owner %x!\n", timeoutHashEntry);
				UINT32 authFlags = CAAccountingInstance::getAuthFlags(timeoutHashEntry);
				if (authFlags > 0)
				{
					CAMsg::printMsg(LOG_WARNING,"Client connection closed due to forced timeout! Payment auth flags: %u\n", authFlags);
				}
				else
				{
					CAMsg::printMsg(LOG_WARNING,"Client connection closed due to forced timeout!\n");
				}
				//CAAccountingInstance::setPrepaidBytesToZero(timeoutHashEntry->pAccountingInfo);
				closeConnection(timeoutHashEntry);
				continue;
			}
			else
			{
				//Note this counter initialized by calling CAFirstMixChannelList::add
				//and accessed by this thread, both do never run concurrently.
				//So we can avoid locking.
				timeoutHashEntry->kickoutSendRetries--;
				CAMsg::printMsg(LOG_INFO, "Size of control message queue for user to be kicked out: %u bytes, retries %d.\n",
						timeoutHashEntry->pControlMessageQueue->getSize(), timeoutHashEntry->kickoutSendRetries);
			}
			// Let the client obtain all his remaining control message packets
			//(which in most cases contain the error message with the kickout reason.
			CAMsg::printMsg(LOG_WARNING,"A kickout is supposed to happen. Let the user get his %u control message bytes before...\n",
					timeoutHashEntry->pControlMessageQueue->getSize());
		}
		if(firstIteratorEntry == NULL)
		{
			firstIteratorEntry = timeoutHashEntry;
		}
		m_pChannelList->pushTimeoutEntry(timeoutHashEntry, currentEntryKickoutForced);
	}
}
#endif

#ifdef LOG_CRIME
void CAFirstMixA::crimeSurveillance(CAIPAddrWithNetmask* surveillanceIPs, UINT32 nrOfSurveillanceIPs,UINT8 *peerIP,SINT32 peerPort, MIXPACKET *pMixPacket)
{
	if( (nrOfSurveillanceIPs > 0) && (surveillanceIPs != NULL) )
	{
		for(UINT32 i = 0; i < nrOfSurveillanceIPs; i++)
		{
			if(surveillanceIPs[i].equals(peerIP))
			{
				CAMsg::printMsg(LOG_CRIT,"Crime detection: User surveillance, IP %u.%u.%u.%u Port %i with next mix channel %u\n",peerIP[0], peerIP[1], peerIP[2], peerIP[3],peerPort,pMixPacket->channel);
				pMixPacket->flags |= CHANNEL_SIG_CRIME;
				return;
			}
		}
	}
}
#endif







#else //MULTIPLE THREADS





#ifdef HAVE_EPOLL
SINT32 CAFirstMixA::closeConnection(fmHashTableEntry* pHashEntry, CASocketGroupEpoll* psocketgroupUsersRead,CASocketGroupEpoll* psocketgroupUsersWrite, CAFirstMixChannelList* pChannelList)
#else
SINT32 CAFirstMixA::closeConnection(fmHashTableEntry* pHashEntry, CASocketGroup* psocketgroupUsersRead,CASocketGroup* psocketgroupUsersWrite, CAFirstMixChannelList* pChannelList)
#endif
{
	if (pHashEntry == NULL)
	{
		return E_UNKNOWN;
	}

	INIT_STACK;
	BEGIN_STACK("CAFirstMixA::closeConnection");


	fmChannelListEntry* pEntry;
	tQueueEntry* pQueueEntry = new tQueueEntry;
	MIXPACKET* pMixPacket=&pQueueEntry->packet;

	#ifdef LOG_TRAFFIC_PER_USER
		UINT64 current_time;
		getcurrentTimeMillis(current_time);
		CAMsg::printMsg(LOG_DEBUG,"Removing Connection wiht ID: %Lu -- login time [ms] %Lu -- logout time [ms] %Lu -- Traffic was: IN: %u  --  OUT: %u\n",pHashEntry->id,pHashEntry->timeCreated,current_time,pHashEntry->trafficIn,pHashEntry->trafficOut);
	#endif
	m_pIPList->removeIP(pHashEntry->peerIP);

	psocketgroupUsersRead->remove(*(pHashEntry->pMuxSocket));
	psocketgroupUsersWrite->remove(*(pHashEntry->pMuxSocket));
	pEntry = pChannelList->getFirstChannelForSocket(pHashEntry->pMuxSocket);

	while(pEntry!=NULL)
	{
		getRandom(pMixPacket->data,DATA_SIZE);
		pMixPacket->flags=CHANNEL_CLOSE;
		m_pChannelToQueueList->removeChannel(pEntry->channelOut);
		pMixPacket->channel=pEntry->channelOut;
		#ifdef LOG_PACKET_TIMES
			setZero64(pQueueEntry->timestamp_proccessing_start);
		#endif
		m_pQueueSendToMix->add(pQueueEntry, sizeof(tQueueEntry));
		delete pEntry->pCipher;
		pEntry->pCipher = NULL;
		pEntry=pChannelList->getNextChannel(pEntry);
#ifdef CH_LOG_STUDY
		nrOfChOpMutex->lock();
		currentOpenedChannels--;
		nrOfChOpMutex->unlock();
#endif //CH_LOG_STUDY
	}
	ASSERT(pHashEntry->pQueueSend!=NULL,"Send queue is NULL");
	delete pHashEntry->pQueueSend;
	pHashEntry->pQueueSend = NULL;
	delete pHashEntry->pSymCipher;
	pHashEntry->pSymCipher = NULL;

	#ifdef COUNTRY_STATS
		decUsers(pHashEntry);
	#else
		decUsers();
	#endif

	CAMuxSocket* pMuxSocket = pHashEntry->pMuxSocket;
	// Save the socket - its pointer will be deleted in this method!!! Crazy mad programming...
	pChannelList->remove(pHashEntry->pMuxSocket);
	delete pMuxSocket;
	pMuxSocket = NULL;
	//pHashEntry->pMuxSocket = NULL; // not needed now, but maybe in the future...

	delete pQueueEntry;
	pQueueEntry = NULL;

	FINISH_STACK("CAFirstMixA::closeConnection");

	return E_SUCCESS;
}

#define FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS 2*KEY_SIZE


struct fm_packet_proccessing_loop_args_t
	{
		CAFirstMixA* pFirstMix;
		CAQueue* pIncomingPacketQueue;
		CAFirstMixChannelList* pChannelList;
#ifdef HAVE_EPOLL
		CASocketGroupEpoll* psocketgroupUsersRead;
		CASocketGroupEpoll* socketgroupUsersWrite;
#else
		CASocketGroup* psocketgroupUsersRead;
		CASocketGroup* psocketgroupUsersWrite;
#endif
	};
typedef struct fm_packet_proccessing_loop_args_t tPacketProcessingLoopArgs;


SINT32 CAFirstMixA::loop()
	{
#ifndef NEW_MIX_TYPE
	m_pthreadsPacketProccessingLoop = new CAThreadPool(m_numThreads, m_numThreads, false);

	for (UINT32 i = 0; i < m_numThreads; i++)
		{
#ifdef DELAY_USERS
		m_arpChannelList[i]->setDelayParameters(CALibProxytest::getOptions()->getDelayChannelUnlimitTraffic(),
			CALibProxytest::getOptions()->getDelayChannelBucketGrow(),
			CALibProxytest::getOptions()->getDelayChannelBucketGrowIntervall());
#endif
		tPacketProcessingLoopArgs* params = new tPacketProcessingLoopArgs;
		params->pFirstMix = this;
		params->pChannelList = m_arpChannelList[i];
		params->psocketgroupUsersRead = m_arpsocketgroupUsersRead[i];
		params->psocketgroupUsersWrite = m_arpsocketgroupUsersWrite[i];
		params->pIncomingPacketQueue = new CAQueue();
		m_pthreadsPacketProccessingLoop->addRequest(fm_loopPacketProcessing, params);
		}
	//	CASingleSocketGroup osocketgroupMixOut;
		SINT32 countRead=0;
		//#ifdef LOG_PACKET_TIMES
		//	tPoolEntry* pPoolEntry=new tPoolEntry;
		//	MIXPACKET* pMixPacket=&pPoolEntry->mixpacket;
		//#else
		tQueueEntry* pQueueEntry = new tQueueEntry;
		MIXPACKET* pMixPacket=&pQueueEntry->packet;
		//#endif
		m_nUser=0;
		SINT32 ret;
		//osocketgroupMixOut.add(*m_pMuxOut);

		UINT8* tmpBuff=new UINT8[sizeof(tQueueEntry)];
		CAMsg::printMsg(LOG_DEBUG,"Starting Message Loop... \n");
		bool bAktiv;
		UINT8 rsaBuff[RSA_SIZE];

#ifdef LOG_TRAFFIC_PER_USER
		UINT64 current_time;
		UINT32 diff_time;
		CAMsg::printMsg(LOG_DEBUG,"Channel log formats:\n");
		CAMsg::printMsg(LOG_DEBUG,"1. Close received from user (times in micros) - 1:Channel-ID,Connection-ID,Channel open timestamp (microseconds),PacketsIn (only data and open),PacketsOut (only data),ChannelDuration (open packet received --> close packet put into send queue to next mix)\n");
		CAMsg::printMsg(LOG_DEBUG,"2. Channel close from Mix(times in micros)- 2.:Channel-ID,Connection-ID,Channel open timestamp (microseconds),PacketsIn (only data and open), PacketsOut (only data),ChannelDuration (open packet received)--> close packet put into send queue to next user\n");
#endif
/** @todo check if thread is closed */
#ifdef _DEBUG
//		CAThread* pLogThread=new CAThread((UINT8*)"CAFirstMixA - LogLoop");
//		pLogThread->setMainLoop(fm_loopLog);
//		pLogThread->start(this);
#endif

#ifdef LOG_CRIME
		CAIPAddrWithNetmask* surveillanceIPs = CALibProxytest::getOptions()->getCrimeSurveillanceIPs();
		UINT32 nrOfSurveillanceIPs = CALibProxytest::getOptions()->getNrOfCrimeSurveillanceIPs();
#endif
//		CAThread threadReadFromUsers;
//		threadReadFromUsers.setMainLoop(loopReadFromUsers);
//		threadReadFromUsers.start(this);

		while(!m_bRestart) /* the main mix loop as long as there are things that are not handled by threads. */
			{
//Step 4
//Step 4a Receiving from mix to queue now in a separate thread
//Step 4b Processing MixPackets received from Mix
//todo check for error!!!
				while(m_pQueueReadFromMix->getSize()>=sizeof(tQueueEntry))
					{
						bAktiv=true;
						ret=sizeof(tQueueEntry);
						m_pQueueReadFromMix->get((UINT8*)pQueueEntry,(UINT32*)&ret);
						CAQueue* pQueue = m_pChannelToQueueList->get(pMixPacket->channel);
						if(pQueue!=NULL)
							pQueue->add(pQueueEntry, ret);
						#ifdef LOG_PACKET_TIMES
							getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start_OP);
						#endif
#ifdef ANON_DEBUG_MODE
						if (pMixPacket->flags&CHANNEL_DEBUG)
							{
								UINT8 base64Payload[DATA_SIZE << 1];
								EVP_EncodeBlock(base64Payload, pMixPacket->data, DATA_SIZE);//base64 encoding (without newline!)
								CAMsg::printMsg(LOG_DEBUG, "Dequeued Downstream AN.ON packet from previous Mix debug: %s\n", base64Payload);
								pMixPacket->flags &= ~CHANNEL_DEBUG;
							}
#endif
					}

#ifndef FAST_PROCESSING
				if(!bAktiv)
				  msSleep(1);
#endif
			}
//ERR:
		CAMsg::printMsg(LOG_CRIT,"Seems that we are restarting now!!\n");
		m_bRunLog=false;
		clean();
		delete pQueueEntry;
		pQueueEntry = NULL;
		delete []tmpBuff;
		tmpBuff = NULL;
#ifdef _DEBUG
//		pLogThread->join();
//		delete pLogThread;
//		pLogThread = NULL;
#endif
		CAMsg::printMsg(LOG_CRIT,"Main Loop exited!!\n");
#endif//NEW_MIX_TYPE
		return E_UNKNOWN;
	}


THREAD_RETURN fm_loopPacketProcessing(void *params)
	{
#ifndef NEW_MIX_TYPE
		CAFirstMixA* pMix = static_cast<tPacketProcessingLoopArgs*>(params)->pFirstMix;
#ifdef HAVE_EPOLL
		CASocketGroupEpoll* psocketgroupUsersRead = static_cast<tPacketProcessingLoopArgs*>(params)->psocketgroupUsersRead;
		CASocketGroupEpoll* psocketgroupUsersWrite= static_cast<tPacketProcessingLoopArgs*>(params)->psocketgroupUsersWrite;
#else
		CASocketGroup* psocketgroupUsersRead = static_cast<tPacketProcessingLoopArgs*>(params)->psocketgroupUsersRead;
		CASocketGroup* psocketgroupUsersWrite= static_cast<tPacketProcessingLoopArgs*>(params)->psocketgroupUsersWrite;
#endif
		CAQueue* pQueueReadFromMix = static_cast<tPacketProcessingLoopArgs*>(params)->pIncomingPacketQueue;
		CAFirstMixChannelList* pChannelList = static_cast<tPacketProcessingLoopArgs*>(params)->pChannelList;




#ifdef DELAY_USERS
		pChannelList->setDelayParameters(	CALibProxytest::getOptions()->getDelayChannelUnlimitTraffic(),
																			CALibProxytest::getOptions()->getDelayChannelBucketGrow(),
																			CALibProxytest::getOptions()->getDelayChannelBucketGrowIntervall());
#endif

	//	CASingleSocketGroup osocketgroupMixOut;
		SINT32 countRead=0;
		//#ifdef LOG_PACKET_TIMES
		//	tPoolEntry* pPoolEntry=new tPoolEntry;
		//	MIXPACKET* pMixPacket=&pPoolEntry->mixpacket;
		//#else
		tQueueEntry* pQueueEntry = new tQueueEntry;
		MIXPACKET* pMixPacket=&pQueueEntry->packet;
		//#endif
		SINT32 ret;
		//osocketgroupMixOut.add(*m_pMuxOut);

		UINT8* tmpBuff=new UINT8[sizeof(tQueueEntry)];
		CAMsg::printMsg(LOG_DEBUG,"Starting Message Loop... \n");
		bool bAktiv;
		UINT8 rsaBuff[RSA_SIZE];

#ifdef LOG_TRAFFIC_PER_USER
		UINT64 current_time;
		UINT32 diff_time;
		CAMsg::printMsg(LOG_DEBUG,"Channel log formats:\n");
		CAMsg::printMsg(LOG_DEBUG,"1. Close received from user (times in micros) - 1:Channel-ID,Connection-ID,Channel open timestamp (microseconds),PacketsIn (only data and open),PacketsOut (only data),ChannelDuration (open packet received --> close packet put into send queue to next mix)\n");
		CAMsg::printMsg(LOG_DEBUG,"2. Channel close from Mix(times in micros)- 2.:Channel-ID,Connection-ID,Channel open timestamp (microseconds),PacketsIn (only data and open), PacketsOut (only data),ChannelDuration (open packet received)--> close packet put into send queue to next user\n");
#endif

#ifdef LOG_CRIME
		CAIPAddrWithNetmask* surveillanceIPs = CALibProxytest::getOptions()->getCrimeSurveillanceIPs();
		UINT32 nrOfSurveillanceIPs = CALibProxytest::getOptions()->getNrOfCrimeSurveillanceIPs();
#endif
//		CAThread threadReadFromUsers;
//		threadReadFromUsers.setMainLoop(loopReadFromUsers);
//		threadReadFromUsers.start(this);

		while(!pMix->m_bRestart) /* the main mix loop as long as there are things that are not handled by threads. */
			{

				bAktiv=false;

//LOOP_START:
#ifdef PAYMENT
				// while checking if there are connections to close: synch with login threads
				m_pmutexLogin->lock();
				checkUserConnections();
				m_pmutexLogin->unlock();
#endif
//First Step
//Checking for new connections
// Now in a separate Thread....

// Second Step
// Checking for data from users
// Now in a separate Thread (see loopReadFromUsers())
//Only proccess user data, if queue to next mix is not to long!!

				if(pMix->m_pQueueSendToMix->getSize()<MAX_NEXT_MIX_QUEUE_SIZE)
					{
						countRead=psocketgroupUsersRead->select(/*false,*/0);				// how many JAP<->mix connections have received data from their coresponding JAP
						if(countRead>0)
							bAktiv=true;
#ifdef HAVE_EPOLL
						//if we have epoll we do not need to search the whole list
						//of connected JAPs to find the ones who have sent data
						//as epoll will return ONLY these connections.
						fmHashTableEntry* pHashEntry=(fmHashTableEntry*)m_psocketgroupUsersRead->getFirstSignaledSocketData();
						while(pHashEntry!=NULL)
							{
								CAMuxSocket* pMuxSocket=pHashEntry->pMuxSocket;
#else
						//if we do not have epoll we have to go to the whole
						//list of open connections to find the ones which
						//actually have sent some data
						fmHashTableEntry* pHashEntry=pChannelList->getFirst();
						while(pHashEntry!=NULL&&countRead>0)											// iterate through all connections as long as there is at least one active left
							{
								CAMuxSocket* pMuxSocket=pHashEntry->pMuxSocket;
								if(psocketgroupUsersRead->isSignaled(*pMuxSocket))	// if this one seems to have data
									{
#endif
/*#ifdef DELAY_USERS
 * Don't delay upstream
								if( m_pChannelList->hasDelayBuckets(pHashEntry->delayBucketID) )
								{
#endif*/
										countRead--;
										ret=pMuxSocket->receive(pMixPacket,0);

										#if defined LOG_PACKET_TIMES||defined(LOG_CHANNEL)
											getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start);
											set64(pQueueEntry->timestamp_proccessing_start_OP,pQueueEntry->timestamp_proccessing_start);
										#endif
										#ifdef DATA_RETENTION_LOG
											pQueueEntry->dataRetentionLogEntry.t_in=htonl(time(NULL));
										#endif
										if(ret<0&&ret!=E_AGAIN/*||pHashEntry->accessUntil<time()*/)
										{
											// remove dead connections
											pMix->closeConnection(pHashEntry,psocketgroupUsersRead,psocketgroupUsersRead,pChannelList);
										}
										else if(ret==MIXPACKET_SIZE) 											// we've read enough data for a whole mix packet. nice!
											{
#ifdef PAYMENT
												if (pHashEntry->bRecoverTimeout)
												{
													// renew the timeout only if recovery is allowed
													m_pChannelList->pushTimeoutEntry(pHashEntry);
												}
#endif
												#ifdef LOG_TRAFFIC_PER_USER
													pHashEntry->trafficIn++;
												#endif
												#ifdef COUNTRY_STATS
													m_PacketsPerCountryIN[pHashEntry->countryID].inc();
												#endif
												//New control channel code...!
												if(pMixPacket->channel>0&&pMixPacket->channel<256)
												{
													if (pHashEntry->pControlChannelDispatcher->proccessMixPacket(pMixPacket))
													{
														goto NEXT_USER;
													}
													else
													{
														CAMsg::printMsg(LOG_DEBUG, "Control channel packet is invalid and could not be processed!\n");
														pMix->closeConnection(pHashEntry,psocketgroupUsersRead,psocketgroupUsersRead,pChannelList);
														goto NEXT_USER;
													}
												}
#ifdef ANON_DEBUG_MODE
												bool bIsDebugPacket=false;
												if (pMixPacket->flags&CHANNEL_DEBUG)
													{
													bIsDebugPacket=true;
													UINT8 base64Payload[DATA_SIZE << 1];
													EVP_EncodeBlock(base64Payload, pMixPacket->data, DATA_SIZE);//base64 encoding (without newline!)
													pMixPacket->flags &= ~CHANNEL_DEBUG;
													CAMsg::printMsg(LOG_DEBUG, "AN.ON packet debug: %s\n", base64Payload);
													}

#endif
#ifdef PAYMENT
												if(accountTrafficUpstream(pHashEntry) != E_SUCCESS) goto NEXT_USER;
#endif
												if(pMixPacket->flags==CHANNEL_DUMMY) // just a dummy to keep the connection alife in e.g. NAT gateways
												{
													CAMsg::printMsg(LOG_DEBUG,"received dummy traffic\n");
													getRandom(pMixPacket->data,DATA_SIZE);
													#ifdef LOG_PACKET_TIMES
														setZero64(pQueueEntry->timestamp_proccessing_start);
													#endif
													#ifdef LOG_TRAFFIC_PER_USER
														pHashEntry->trafficOut++;
													#endif
													#ifdef COUNTRY_STATS
														m_PacketsPerCountryOUT[pHashEntry->countryID].inc();
													#endif
													pHashEntry->pQueueSend->add(pQueueEntry,sizeof(tQueueEntry));
													#ifdef HAVE_EPOLL
														//m_psocketgroupUsersWrite->add(*pMuxSocket,pHashEntry);
													#else
														psocketgroupUsersWrite->add(*pMuxSocket);
													#endif
												}
												else if(pMixPacket->flags==CHANNEL_CLOSE)			// closing one mix-channel (not the JAP<->mix connection!)
												{
													
													fmChannelListEntry* pEntry;
													pEntry=pChannelList->get(pMuxSocket,pMixPacket->channel);
													if(pEntry!=NULL)
													{
														pMix->m_pChannelToQueueList->removeChannel(pEntry->channelOut);
														pMixPacket->channel=pEntry->channelOut;
														getRandom(pMixPacket->data,DATA_SIZE);
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
														#endif
														pMix->m_pQueueSendToMix->add(pQueueEntry, sizeof(tQueueEntry));
														/* Don't delay upstream
														#ifdef DELAY_USERS
														m_pChannelList->decDelayBuckets(pHashEntry->delayBucketID);
														#endif*/
														#ifdef LOG_CHANNEL
															//pEntry->packetsInFromUser++;
															getcurrentTimeMicros(current_time);
															diff_time=diff64(current_time,pEntry->timeCreated);
															CAMsg::printMsg(LOG_DEBUG,"1:%u,%Lu,%Lu,%u,%u,%u\n",
																												pEntry->channelIn,pEntry->pHead->id,pEntry->timeCreated,pEntry->packetsInFromUser,pEntry->packetsOutToUser,
																												diff_time);
														#endif
														delete pEntry->pCipher;              // forget the symetric key of this connection
														pEntry->pCipher = NULL;
														pChannelList->removeChannel(pMuxSocket,pEntry->channelIn);

														#ifdef CH_LOG_STUDY
														nrOfChOpMutex->lock();
														currentOpenedChannels--;
														nrOfChOpMutex->unlock();
														#endif //CH_LOG_STUDY
													}
													#ifdef _DEBUG
													else
													{
//														CAMsg::printMsg(LOG_DEBUG,"Invalid ID to close from Browser!\n");
													}
													#endif
												}
												else		                                     // finally! a normal mix packet
												{
													CASymCipher* pCipher=NULL;
													fmChannelListEntry* pEntry;
													pEntry=pChannelList->get(pMuxSocket,pMixPacket->channel);
													if (pEntry != NULL&&pMixPacket->flags == CHANNEL_DATA)
														{
														pMixPacket->channel = pEntry->channelOut;
														pCipher = pEntry->pCipher;
														pCipher->crypt1(pMixPacket->data, pMixPacket->data, DATA_SIZE);
														// queue the packet for sending to the next mix.
#ifdef LOG_PACKET_TIMES
														getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
#endif

														//check if this IP must be logged due to crime detection
#ifdef LOG_CRIME
														crimeSurveillance(surveillanceIPs, nrOfSurveillanceIPs, pEntry->pHead->peerIP,pEntry->pHead->peerPort, pMixPacket);
#endif
#ifdef ANON_DEBUG_MODE
														if(bIsDebugPacket)
															{
															pMixPacket->flags|=CHANNEL_DEBUG;
															}
														#endif
														pMix->m_pQueueSendToMix->add(pQueueEntry, sizeof(tQueueEntry));
														/* Don't delay upstream
														#ifdef DELAY_USERS
														m_pChannelList->decDelayBuckets(pHashEntry->delayBucketID);
														#endif*/
														pMix->incMixedPackets();
														#ifdef LOG_CHANNEL
															pEntry->packetsInFromUser++;
														#endif
													}
													else if(pEntry==NULL&&pMixPacket->flags==CHANNEL_OPEN)  // open a new mix channel
													{ // stefan: muesste das nicht vor die behandlung von CHANNEL_DATA? oder gilt OPEN => !DATA ?
														//es gilt: open -> data
														pHashEntry->pSymCipher->crypt1(pMixPacket->data,rsaBuff,FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS);
														#ifdef REPLAY_DETECTION
														// replace time(NULL) with the real timestamp ()
														// packet-timestamp*REPLAY_BASE + m_u64ReferenceTime
															if(m_pReplayDB->insert(rsaBuff,time(NULL))!=E_SUCCESS)
															{
																CAMsg::printMsg(LOG_INFO,"Replay: Duplicate packet ignored.\n");
																continue;
															}
														#endif
														pCipher= new CASymCipher();
														pCipher->setKeys(rsaBuff,FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS);
														for(int i=0;i<16;i++)
															rsaBuff[i]=0xFF;
														pCipher->setIV2(rsaBuff);
														pCipher->crypt1(pMixPacket->data+FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS,pMixPacket->data,DATA_SIZE-FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS);
														getRandom(pMixPacket->data+DATA_SIZE-FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS,FIRST_MIX_SIZE_OF_SYMMETRIC_KEYS);
														#if defined (LOG_CHANNEL) ||defined(DATA_RETENTION_LOG)
															HCHANNEL tmpC=pMixPacket->channel;
														#endif

														HCHANNEL inChannel = pMixPacket->channel;
														if(pChannelList->addChannel(pMuxSocket,pMixPacket->channel,pCipher,&pMixPacket->channel)!=E_SUCCESS)
														{ //todo move up ?
															delete pCipher;
															pCipher = NULL;
														}
														else
														{
															pMix->m_pChannelToQueueList->add(pMixPacket->channel, pQueueReadFromMix);
															#ifdef CH_LOG_STUDY
															nrOfChOpMutex->lock();
															if(pHashEntry->channelOpenedLastIntervalTS !=
																lastLogTime)
															{
																pHashEntry->channelOpenedLastIntervalTS =
																	lastLogTime;
																nrOfOpenedChannels++;
															}
															currentOpenedChannels++;
															nrOfChOpMutex->unlock();
															#endif //CH_LOG_STUDY

															#ifdef LOG_PACKET_TIMES
																getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
															#endif
															#ifdef LOG_CHANNEL
																fmChannelListEntry* pTmpEntry=m_pChannelList->get(pMuxSocket,tmpC);
																pTmpEntry->packetsInFromUser++;
																set64(pTmpEntry->timeCreated,pQueueEntry->timestamp_proccessing_start);
															#endif
															#ifdef DATA_RETENTION_LOG
																pQueueEntry->dataRetentionLogEntry.entity.first.channelid=htonl(pMixPacket->channel);
																fmChannelListEntry* pTmpEntry1=m_pChannelList->get(pMuxSocket,tmpC);
																memcpy(pQueueEntry->dataRetentionLogEntry.entity.first.ip_in,pTmpEntry1->pHead->peerIP,4);
																pQueueEntry->dataRetentionLogEntry.entity.first.port_in=(UINT16)pTmpEntry1->pHead->peerPort;
																pQueueEntry->dataRetentionLogEntry.entity.first.port_in=htons(pQueueEntry->dataRetentionLogEntry.entity.first.port_in);
															#endif

															//check if this IP must be logged due to crime detection
															#ifdef LOG_CRIME

																pEntry=m_pChannelList->get(pMuxSocket, inChannel);
																if(pEntry != NULL)
																{
																	crimeSurveillance(surveillanceIPs, nrOfSurveillanceIPs, pEntry->pHead->peerIP,pEntry->pHead->peerPort, pMixPacket);
																}
															#endif
#ifdef ANON_DEBUG_MODE
																if (bIsDebugPacket)
																	{
																		pMixPacket->flags |= CHANNEL_DEBUG;
																		pEntry = m_pChannelList->get(pMuxSocket, inChannel);
																		pEntry->bDebug = true;
																	}
#endif
															pMix->m_pQueueSendToMix->add(pQueueEntry, sizeof(tQueueEntry));
															/* Don't delay upstream
															#ifdef DELAY_USERS
															m_pChannelList->decDelayBuckets(pHashEntry->delayBucketID);
															#endif*/
															pMix->incMixedPackets();
															#ifdef _DEBUG
//																			CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",pMixPacket->channel);
															#endif
														}
													}
												}
											}
/*#ifdef DELAY_USERS
									}
#endif*/
								#ifdef HAVE_EPOLL
NEXT_USER:
									pHashEntry=(fmHashTableEntry*)m_psocketgroupUsersRead->getNextSignaledSocketData();
								#else
									}//if is signaled
NEXT_USER:
									pHashEntry=pChannelList->getNext();
								#endif
							}
							if(countRead>0)
							{
								CAMsg::printMsg(LOG_DEBUG,"CAFirstMixA::loop() - read from user --> countRead >0 after processing all connections!\n");
							}
					}
//Third step
//Sending to next mix

// Now in a separate Thread (see loopSendToMix())

//Step 4
//Step 4a Receiving from mix to queue now in a separate thread
//Step 4b Processing MixPackets received from Mix
//todo check for error!!!
				countRead=pMix->m_nUser+1;
				while(countRead>0&&pQueueReadFromMix->getSize()>=sizeof(tQueueEntry))
					{
						bAktiv=true;
						countRead--;
						ret=sizeof(tQueueEntry);
						pQueueReadFromMix->get((UINT8*)pQueueEntry,(UINT32*)&ret);

						#ifdef LOG_PACKET_TIMES
							getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start_OP);
						#endif
#ifdef ANON_DEBUG_MODE
						if (pMixPacket->flags&CHANNEL_DEBUG)
							{
								UINT8 base64Payload[DATA_SIZE << 1];
								EVP_EncodeBlock(base64Payload, pMixPacket->data, DATA_SIZE);//base64 encoding (without newline!)
								CAMsg::printMsg(LOG_DEBUG, "Dequeued Downstream AN.ON packet from previous Mix debug: %s\n", base64Payload);
								pMixPacket->flags &= ~CHANNEL_DEBUG;
							}

#endif

						if(pMixPacket->flags==CHANNEL_CLOSE) //close event
							{
								#if defined(_DEBUG) && !defined(__MIX_TEST)
//									CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ...\n",pMixPacket->channel);
								#endif
								pMix->m_pChannelToQueueList->removeChannel(pMixPacket->channel);
								fmChannelList* pEntry=pChannelList->get(pMixPacket->channel);
								if(pEntry!=NULL)
									{
										/* a hack to solve the SSL problem:
										 * set channel of downstream packet to in channel after they are dequeued
										 * from pEntry->pQueueSend so we can retrieve the channel entry to decrement
										 * the per channel count of enqueued downstream bytes.
										 */
										#ifndef SSL_HACK
										pMixPacket->channel=pEntry->channelIn;
										#endif

										pEntry->pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										//getRandom(pMixPacket->data,DATA_SIZE);
										#ifdef LOG_PACKET_TIMES
											getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
										#endif
										pEntry->pHead->pQueueSend->add(pQueueEntry, sizeof(tQueueEntry));
										#ifdef LOG_TRAFFIC_PER_USER
											pEntry->pHead->trafficOut++;
										#endif
										#ifdef COUNTRY_STATS
											m_PacketsPerCountryOUT[pEntry->pHead->countryID].inc();
										#endif
										#ifdef SSL_HACK
											/* a hack to solve the SSL problem:
											 * per channel count of enqueued downstream bytes
											 */
											pEntry->downStreamBytes += sizeof(tQueueEntry);
										#endif
										#ifdef LOG_CHANNEL
											pEntry->packetsOutToUser++;
											getcurrentTimeMicros(current_time);
											diff_time=diff64(current_time,pEntry->timeCreated);
											CAMsg::printMsg(LOG_DEBUG,"2:%u,%Lu,%Lu,%u,%u,%u\n",
																								pEntry->channelIn,pEntry->pHead->id,pEntry->timeCreated,pEntry->packetsInFromUser,pEntry->packetsOutToUser,
																								diff_time);
										#endif

										#ifdef HAVE_EPOLL
											//m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket,pEntry->pHead);
										#else
											psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket);
										#endif


										#ifndef SSL_HACK
											delete pEntry->pCipher;              // forget the symetric key of this connection
											pEntry->pCipher = NULL;
											pChannelList->removeChannel(pEntry->pHead->pMuxSocket, pEntry->channelIn);
										#ifdef CH_LOG_STUDY
											nrOfChOpMutex->lock();
											currentOpenedChannels--;
											nrOfChOpMutex->unlock();
										#endif
										/* a hack to solve the SSL problem:
										 * remove channel after the close packet is enqueued
										 * from pEntry->pQueueSend
										 */
										#endif
									}
									else
									{
										#ifdef DEBUG
											CAMsg::printMsg(LOG_DEBUG, "CAFirstMixA: close channel -> client but channel does not exist.\n");
										#endif
									}

							}
						else
							{//flag !=close
								#if defined(_DEBUG) && !defined(__MIX_TEST)
//									CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!\n");
								#endif
								fmChannelList* pEntry=pChannelList->get(pMixPacket->channel);

								if(pEntry!=NULL)
									{
										#ifdef LOG_CRIME
											if((pMixPacket->flags&CHANNEL_SIG_CRIME)==CHANNEL_SIG_CRIME)
												{
													//UINT32 id=(pMixPacket->flags>>8)&0x000000FF;
													int log=LOG_ENCRYPTED;
													if(!CALibProxytest::getOptions()->isEncryptedLogEnabled())
														log=LOG_CRIT;
													CAMsg::printMsg(log,"Detecting crime activity - next mix channel: %u -- "
															"Incoming (User) IP:Port is: %u.%u.%u.%u:%u \n", pMixPacket->channel,
															pEntry->pHead->peerIP[0],
															pEntry->pHead->peerIP[1],
															pEntry->pHead->peerIP[2],
															pEntry->pHead->peerIP[3],
															pEntry->pHead->peerPort);
													continue;
												}
										#endif

										/* a hack to solve the SSL problem:
										 * same as CHANNEL_CLOSE packets
										 */
										#ifndef SSL_HACK
										pMixPacket->channel=pEntry->channelIn;
										#endif

										pEntry->pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);

										#ifdef LOG_PACKET_TIMES
											getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
										#endif
#ifdef ANON_DEBUG_MODE
											if (pEntry->bDebug)
												{	
												pMixPacket->flags |= CHANNEL_DEBUG;
												UINT8 base64Payload[DATA_SIZE << 1];
												EVP_EncodeBlock(base64Payload, pMixPacket->data, DATA_SIZE);//base64 encoding (without newline!)
												CAMsg::printMsg(LOG_DEBUG, "Send Dowwstream AN.ON packet debug: %s\n", base64Payload);
												}

#endif
											pEntry->pHead->pQueueSend->add(pQueueEntry, sizeof(tQueueEntry));
										/*CAMsg::printMsg(
												LOG_INFO,"adding data packet to queue: %x, queue size: %u bytes\n",
												pEntry->pHead->pQueueSend, pEntry->pHead->pQueueSend->getSize());*/
										#ifdef LOG_TRAFFIC_PER_USER
											pEntry->pHead->trafficOut++;
										#endif
										#ifdef COUNTRY_STATS
											m_PacketsPerCountryOUT[pEntry->pHead->countryID].inc();
										#endif
										#ifdef LOG_CHANNEL
											pEntry->packetsOutToUser++;
										#endif
										#ifdef SSL_HACK
											/* a hack to solve the SSL problem:
											 * per channel count of downstream packets in bytes
											 */
											pEntry->downStreamBytes += sizeof(tQueueEntry);
										#endif

										#ifdef HAVE_EPOLL
											/*int epret = m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket,pEntry->pHead);
											if(epret == E_UNKNOWN)
											{
												epret=errno;
												CAMsg::printMsg(LOG_INFO,"epoll_add returns: %s (return value: %d) \n", strerror(epret), epret);
											}*/
											#else
											psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket);
										#endif

										pMix->incMixedPackets();
									}
								else
									{
										#ifdef _DEBUG
											if(pMixPacket->flags!=CHANNEL_DUMMY)
												{
/*													CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- "
															"Channel-Id %u not valid!\n",pMixPacket->channel
														);*/
													#ifdef LOG_CHANNEL
														CAMsg::printMsg(LOG_INFO,"Packet late arrive for channel: %u\n",pMixPacket->channel);
													#endif
												}
										#endif
									}
							}
					}

////Step 5
////Writing to users...
				bAktiv =pMix->sendToUsers(psocketgroupUsersWrite,psocketgroupUsersRead,pChannelList);
#ifndef FAST_PROCESSING
				if(!bAktiv)
				  msSleep(1);
#endif
			}
//ERR:
		CAMsg::printMsg(LOG_CRIT,"Seems that we are restarting now!!\n");
		delete pQueueEntry;
		pQueueEntry = NULL;
		delete []tmpBuff;
		tmpBuff = NULL;
		CAMsg::printMsg(LOG_CRIT,"Main Loop exited!!\n");
#endif//NEW_MIX_TYPE
		THREAD_RETURN_SUCCESS;
	}
	
/* last part of the main loop:
 * return true if the loop when at least one packet was sent
 * or false otherwise.
 */
#ifdef HAVE_EPOLL
bool CAFirstMixA::sendToUsers(CASocketGroupEpoll* psocketgroupUsersWrite,CASocketGroupEpoll* psocketgroupUsersRead,CAFirstMixChannelList* pChannelList)
#else
bool CAFirstMixA::sendToUsers(CASocketGroup* psocketgroupUsersWrite,CASocketGroup* psocketgroupUsersRead,CAFirstMixChannelList* pChannelList)
#endif
{
	SINT32 countRead = psocketgroupUsersWrite->select(/*true,*/0);
	tQueueEntry *packetToSend = NULL;
	SINT32 packetSize = sizeof(tQueueEntry);
	CAQueue *controlMessageUserQueue = NULL;
	CAQueue *dataMessageUserQueue = NULL;

	CAQueue *processedQueue = NULL; /* one the above queues that should be used for processing*/
	UINT32 extractSize = 0;
	bool bAktiv = false;
	UINT32 iSocketErrors = 0;

/* Cyclic polling: gets all open sockets that will not block when invoking send()
 * but will only send at most one packet. After that control is returned to loop()
 */
#ifdef HAVE_EPOLL
	fmHashTableEntry* pfmHashEntry=
		(fmHashTableEntry*) m_psocketgroupUsersWrite->getFirstSignaledSocketData();

	while(pfmHashEntry != NULL)
	{

#else
	fmHashTableEntry* pfmHashEntry=pChannelList->getFirst();
	while( (countRead > 0) && (pfmHashEntry != NULL) )
	{
		if(psocketgroupUsersWrite->isSignaled(*pfmHashEntry->pMuxSocket))
		{
			countRead--;
#endif
			/* loop turn init */
			extractSize = 0;
			processedQueue = NULL;
			packetToSend = &(pfmHashEntry->oQueueEntry);
			controlMessageUserQueue = pfmHashEntry->pControlMessageQueue;
			dataMessageUserQueue = pfmHashEntry->pQueueSend;

			//Control messages have a higher priority.
			if(controlMessageUserQueue->getSize() > 0)
			{
				processedQueue = controlMessageUserQueue;
				pfmHashEntry->bCountPacket = false;
			}
			else if( (dataMessageUserQueue->getSize() > 0)
#ifdef DELAY_USERS
					&& pChannelList->hasDelayBuckets(pfmHashEntry->delayBucketID)
#endif
			)
			{
				processedQueue = dataMessageUserQueue;
				pfmHashEntry->bCountPacket = true;
			}

			if(processedQueue != NULL)
			{
				extractSize = packetSize;
				bAktiv=true;

				if(pfmHashEntry->uAlreadySendPacketSize == -1)
				{
					processedQueue->get((UINT8*) packetToSend, &extractSize);

					/* Hack for SSL BUG */
#ifdef SSL_HACK
					finishPacket(pfmHashEntry);
#endif //SSL_HACK
#ifdef ANON_DEBUG_MODE
					if (packetToSend->packet.flags&CHANNEL_DEBUG)
						{
						UINT8 base64Payload[DATA_SIZE << 1];
						EVP_EncodeBlock(base64Payload, packetToSend->packet.data, DATA_SIZE);//base64 encoding (without newline!)
						CAMsg::printMsg(LOG_DEBUG, "Send Downstream AN.ON packet to user debug: %s\n", base64Payload);
						packetToSend->packet.flags &= ~CHANNEL_DEBUG;
						}

#endif
					pfmHashEntry->pMuxSocket->prepareForSend(&(packetToSend->packet));
					pfmHashEntry->uAlreadySendPacketSize = 0;
				}
			}

			if( (extractSize > 0) || (pfmHashEntry->uAlreadySendPacketSize > 0) )
			{
				SINT32 len =  MIXPACKET_SIZE - pfmHashEntry->uAlreadySendPacketSize;
				UINT8* packetToSendOffset = ((UINT8*)&(packetToSend->packet)) + pfmHashEntry->uAlreadySendPacketSize;
				CASocket* clientSocket = pfmHashEntry->pMuxSocket->getCASocket();

				SINT32 ret = clientSocket->send(packetToSendOffset, len);

				if(ret > 0)
				{
#ifdef PAYMENT
					SINT32 accounting = E_SUCCESS;
#endif
					pfmHashEntry->uAlreadySendPacketSize += ret;

					if(pfmHashEntry->uAlreadySendPacketSize == MIXPACKET_SIZE)
					{
						#ifdef DELAY_USERS
						if(processedQueue != controlMessageUserQueue)
						{
							pChannelList->decDelayBuckets(pfmHashEntry->delayBucketID);
						}
						#endif

						#ifdef LOG_PACKET_TIMES
							if(!isZero64(pfmHashEntry->oQueueEntry.timestamp_proccessing_start))
								{
									getcurrentTimeMicros(pfmHashEntry->oQueueEntry.timestamp_proccessing_end);
									m_pLogPacketStats->addToTimeingStats(pfmHashEntry->oQueueEntry,CHANNEL_DATA,false);
								}
						#endif
						pfmHashEntry->uAlreadySendPacketSize=-1;
#ifdef PAYMENT
						/* count this packet for accounting */
						accounting = accountTrafficDownstream(pfmHashEntry);
#endif
					}

				}
				else if(ret<0&&ret!=E_AGAIN)
				{
					iSocketErrors++;
					// if (iSocketErrors == 1) // show debug message only at the first error; otherwise, the log may get huge
					{
						SOCKET sock=clientSocket->getSocket();
						CAMsg::printMsg(LOG_DEBUG,"CAFirstMixA::sendtoUser() - send error %d on socket %d. Reason: %s (%i)\n", ret, sock, GET_NET_ERROR_STR(GET_NET_ERROR), GET_NET_ERROR);
					}
					// kick the user out - these only happens in extreme situations...
					closeConnection(pfmHashEntry,psocketgroupUsersRead,psocketgroupUsersWrite,pChannelList);
				}
				//TODO error handling
			}

#ifdef HAVE_EPOLL
		pfmHashEntry=(fmHashTableEntry*)m_psocketgroupUsersWrite->getNextSignaledSocketData();
	}
#else
		}//if is socket signaled
		pfmHashEntry=pChannelList->getNext();
	}
#endif
	if (iSocketErrors > 1)
	{
		CAMsg::printMsg(LOG_ERR, "CAFirstMixA::sendtoUser() - %d send errors on a socket occured!\n", iSocketErrors);
	}
	
	return bAktiv;
}
#endif //MULTIPLE THREADS


#endif //ONLY_LOCAL_PROXY
