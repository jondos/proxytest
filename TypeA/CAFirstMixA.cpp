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
#ifndef ONLY_LOCAL_PROXY
#include "CAFirstMixA.hpp"
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
extern CACmdLnOptions* pglobalOptions;
/* Cleanup order: 
		 * 1. stop threads.
		 * 2. close connections
		 * 3. close sockets
		 */
SINT32 CAFirstMixA::clean()
	{
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMixA::clean() start\n");
		#endif
		m_bRunLog=false;
		m_bRestart=true;
		MONITORING_FIRE_NET_EVENT(ev_net_nextConnectionClosed);
		
		/*1. stop threads */
		if(m_pthreadAcceptUsers!=NULL)
		{
			CAMsg::printMsg(LOG_CRIT,"Wait for LoopAcceptUsers!\n");
			m_pthreadAcceptUsers->join();
			delete m_pthreadAcceptUsers;
		}
		m_pthreadAcceptUsers=NULL;
		
		CAMsg::printMsg(LOG_DEBUG,"Cleaning up login threads\n");
		if(m_pthreadsLogin!=NULL)
		{
			delete m_pthreadsLogin;
			m_pthreadsLogin=NULL;
		}
		
		//writing some bytes to the queue...
		if(m_pQueueSendToMix!=NULL)
		{
			UINT8 b[sizeof(tQueueEntry)+1];
			m_pQueueSendToMix->add(b,sizeof(tQueueEntry)+1);
		}
		
		if(m_pthreadSendToMix!=NULL)
		{
			CAMsg::printMsg(LOG_CRIT,"Wait for LoopSendToMix!\n");
			m_pthreadSendToMix->join();
			delete m_pthreadSendToMix;
		}
		m_pthreadSendToMix=NULL;
		
		if(m_pthreadReadFromMix!=NULL)
		{
			CAMsg::printMsg(LOG_CRIT,"Wait for LoopReadFromMix!\n");
			m_pthreadReadFromMix->join();
			delete m_pthreadReadFromMix;
		}
		m_pthreadReadFromMix=NULL;
		
		/* 2. close connections*/
#ifdef PAYMENT
			UINT32 connectionsClosed = 0;
			fmHashTableEntry* timeoutHashEntry;
			
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
		/* 3. close sockets*/
		if(m_pMuxOut!=NULL)
			{
				m_pMuxOut->close();
			}
		if(m_arrSocketsIn!=NULL)
			{
				for(UINT32 i=0;i<m_nSocketsIn;i++)
					m_arrSocketsIn[i].close();
			}
		
		/**/

		
		
    //     if(m_pInfoService!=NULL)
    //     {
    //         CAMsg::printMsg(LOG_CRIT,"Stopping InfoService....\n");
    //         CAMsg::printMsg	(LOG_CRIT,"Memory usage before: %u\n",getMemoryUsage());
    //         m_pInfoService->stop();
    //         CAMsg::printMsg	(LOG_CRIT,"Memory usage after: %u\n",getMemoryUsage());
    //         CAMsg::printMsg(LOG_CRIT,"Stopped InfoService!\n");
    //         delete m_pInfoService;
    //     }
    //     m_pInfoService=NULL;

#ifdef LOG_PACKET_TIMES
		if(m_pLogPacketStats!=NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"Wait for LoopLogPacketStats to terminate!\n");
				m_pLogPacketStats->stop();
				delete m_pLogPacketStats;
			}
		m_pLogPacketStats=NULL;
#endif
		if(m_arrSocketsIn!=NULL)
			delete[] m_arrSocketsIn;
		m_arrSocketsIn=NULL;
#ifdef REPLAY_DETECTION
		if(m_pReplayMsgProc!=NULL)
			{
				delete m_pReplayMsgProc;
			}
		m_pReplayMsgProc=NULL;
#endif

		if(m_pMuxOutControlChannelDispatcher!=NULL)
		{
			delete m_pMuxOutControlChannelDispatcher;
		}
		m_pMuxOutControlChannelDispatcher=NULL;

		if(m_pMuxOut!=NULL)
			{
				m_pMuxOut->close();
				delete m_pMuxOut;
			}
		m_pMuxOut=NULL;
#ifdef COUNTRY_STATS
		deleteCountryStats();
#endif		
		if(m_pIPList!=NULL)
			delete m_pIPList;
		m_pIPList=NULL;
		if(m_pQueueSendToMix!=NULL)
			delete m_pQueueSendToMix;
		m_pQueueSendToMix=NULL;
		if(m_pQueueReadFromMix!=NULL)
			delete m_pQueueReadFromMix;
		m_pQueueReadFromMix=NULL;

		if(m_pChannelList!=NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"Before deleting CAFirstMixChannelList()!\n");
				CAMsg::printMsg	(LOG_CRIT,"Memory usage before: %u\n",getMemoryUsage());	
				fmHashTableEntry* pHashEntry=m_pChannelList->getFirst();
				while(pHashEntry!=NULL)
					{
						CAMuxSocket * pMuxSocket=pHashEntry->pMuxSocket;
						delete pHashEntry->pQueueSend;
						delete pHashEntry->pSymCipher; 

						fmChannelListEntry* pEntry=m_pChannelList->getFirstChannelForSocket(pHashEntry->pMuxSocket);
						while(pEntry!=NULL)
							{
								delete pEntry->pCipher;
								pEntry=m_pChannelList->getNextChannel(pEntry);
							}
						m_pChannelList->remove(pHashEntry->pMuxSocket);
						//CAMsg::printMsg	(LOG_CRIT,"pMuxSocket ref %0x%x\n", (UINT32) pMuxSocket);	
						pMuxSocket->close();
						delete pMuxSocket;
						pHashEntry=m_pChannelList->getNext();
					}
			}
		if(m_pChannelList!=NULL)
			delete m_pChannelList;
		m_pChannelList=NULL;
		CAMsg::printMsg	(LOG_CRIT,"Memory usage after: %u\n",getMemoryUsage());	

#ifdef PAYMENT
	CAAccountingInstance::clean();
	CAAccountingDBInterface::cleanup();
#endif
		if(m_psocketgroupUsersRead!=NULL)
			delete m_psocketgroupUsersRead;
		m_psocketgroupUsersRead=NULL;
		if(m_psocketgroupUsersWrite!=NULL)
			delete m_psocketgroupUsersWrite;
		m_psocketgroupUsersWrite=NULL;
		if(m_pRSA!=NULL)
			delete m_pRSA;
		m_pRSA=NULL;
		if(m_xmlKeyInfoBuff!=NULL)
			delete[] m_xmlKeyInfoBuff;
		m_xmlKeyInfoBuff=NULL;
		m_docMixCascadeInfo=NULL;
		if(m_arMixParameters!=NULL)
			{
				for(UINT32 i=0;i<m_u32MixCount-1;i++)
					{
						delete[] m_arMixParameters[i].m_strMixID;
					}
				delete[] m_arMixParameters;
			}
		m_arMixParameters=NULL;
		m_u32MixCount=0;
		m_nMixedPackets=0; //reset to zero after each restart (at the moment neccessary for infoservice)
		m_nUser=0;

#ifdef _DEBUG
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMixA::clean() finished\n");
#endif
		return E_SUCCESS;
	}

void CAFirstMixA::shutDown()
{
	m_bIsShuttingDown = true;
	clean();
	m_bIsShuttingDown = false;
	//m_bRestart = true;
}
/*#ifdef PAYMENT
	UINT32 connectionsClosed = 0;
	fmHashTableEntry* timeoutHashEntry;
	
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
}*/


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
	
	m_psocketgroupUsersRead->remove(*(CASocket*)pHashEntry->pMuxSocket);
	m_psocketgroupUsersWrite->remove(*(CASocket*)pHashEntry->pMuxSocket);
	pEntry = m_pChannelList->getFirstChannelForSocket(pHashEntry->pMuxSocket);
	
	while(pEntry!=NULL)
	{
		getRandom(pMixPacket->data,DATA_SIZE);
		pMixPacket->flags=CHANNEL_CLOSE;
		pMixPacket->channel=pEntry->channelOut;
		#ifdef LOG_PACKET_TIMES
			setZero64(pQueueEntry->timestamp_proccessing_start);
		#endif
		m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));
		delete pEntry->pCipher;
		pEntry=m_pChannelList->getNextChannel(pEntry);
	}
	ASSERT(pHashEntry->pQueueSend!=NULL,"Send queue is NULL");
	delete pHashEntry->pQueueSend;
	delete pHashEntry->pSymCipher;
	
	#ifdef COUNTRY_STATS
		decUsers(pHashEntry);
	#else
		decUsers();
	#endif	
	
	CAMuxSocket* pMuxSocket = pHashEntry->pMuxSocket;
	// Save the socket - its pointer will be deleted in this method!!! Crazy mad programming...
	m_pChannelList->remove(pHashEntry->pMuxSocket);
	delete pMuxSocket;	
	
	delete pQueueEntry;
	
	FINISH_STACK("CAFirstMixA::closeConnection");
	
	return E_SUCCESS;
}



SINT32 CAFirstMixA::loop()
	{
#ifndef NEW_MIX_TYPE
#ifdef DELAY_USERS
		m_pChannelList->setDelayParameters(	pglobalOptions->getDelayChannelUnlimitTraffic(),
																			pglobalOptions->getDelayChannelBucketGrow(),
																			pglobalOptions->getDelayChannelBucketGrowIntervall());	
#endif		

	//	CASingleSocketGroup osocketgroupMixOut;
		SINT32 countRead;
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
//		CAThread threadReadFromUsers;
//		threadReadFromUsers.setMainLoop(loopReadFromUsers);
//		threadReadFromUsers.start(this);
		while(!m_bRestart) /* the main mix loop as long as there are things that are not handled by threads. */
			{
				bAktiv=false;
#ifdef PAYMENT
				// check the timeout for all connections
				fmHashTableEntry* timeoutHashEntry;
				while ((timeoutHashEntry = m_pChannelList->popTimeoutEntry()) != NULL)
				{			
					if (timeoutHashEntry->bRecoverTimeout)
					{
						CAMsg::printMsg(LOG_DEBUG,"Client connection closed due to timeout.\n");
					}
					else
					{
						// This should not happen if all client connections are closed as defined in the protocols.
//#ifdef PAYMENT
						UINT32 authFlags = CAAccountingInstance::getAuthFlags(timeoutHashEntry);
						if (authFlags > 0)
						{
							CAMsg::printMsg(LOG_ERR,"Client connection closed due to forced timeout! Payment auth flags: %u\n", authFlags);
						}
						else
//#endif						
						{
							CAMsg::printMsg(LOG_ERR,"Client connection closed due to forced timeout!\n");
						}
					}					
					
					closeConnection(timeoutHashEntry);
				}

#endif				
//LOOP_START:

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
						//if we have epool we do not need to search the whole list
						//of connected JAPs to find the ones who have sent data
						//as epool will return ONLY these connections.
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
										countRead--;
#endif																				
										ret=pMuxSocket->receive(pMixPacket,0);
										#if defined LOG_PACKET_TIMES||defined(LOG_CHANNEL)
											getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start);
											set64(pQueueEntry->timestamp_proccessing_start_OP,pQueueEntry->timestamp_proccessing_start);
										#endif	
										if(ret==SOCKET_ERROR/*||pHashEntry->accessUntil<time()*/) 
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
#ifdef PAYMENT
												SINT32 ret = CAAccountingInstance::handleJapPacket(pHashEntry, false, false);  
												if (CAAccountingInstance::HANDLE_PACKET_CONNECTION_OK == ret)
												{
													// renew the timeout
													pHashEntry->bRecoverTimeout = true;														
													m_pChannelList->pushTimeoutEntry(pHashEntry);													
												}		
												else if (CAAccountingInstance::HANDLE_PACKET_HOLD_CONNECTION == ret)
												{
													// @todo this packet should be queued for later processing
													pHashEntry->bRecoverTimeout = false;	
												}						
												else if (CAAccountingInstance::HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION == ret)
												{
													// do not forward this packet
													pHashEntry->bRecoverTimeout = false;	
													goto NEXT_USER;
												}
												else if (CAAccountingInstance::HANDLE_PACKET_CLOSE_CONNECTION == ret)
												{
													// kickout this user - he deserves it...
													closeConnection(pHashEntry);
													goto NEXT_USER;
												}

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
													pHashEntry->pQueueSend->add(pMixPacket,sizeof(tQueueEntry));
													#ifdef HAVE_EPOLL
														m_psocketgroupUsersWrite->add(*pMuxSocket,pHashEntry); 
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
														m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));
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
													if(pEntry!=NULL&&pMixPacket->flags==CHANNEL_DATA)
													{
														pMixPacket->channel=pEntry->channelOut;
														pCipher=pEntry->pCipher;
														pCipher->crypt1(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														                     // queue the packet for sending to the next mix.
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
														#endif
							
														m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));
														incMixedPackets();
														#ifdef LOG_CHANNEL
															pEntry->packetsInFromUser++;
														#endif
													}
													else if(pEntry==NULL&&pMixPacket->flags==CHANNEL_OPEN)  // open a new mix channel
													{ // stefan: muesste das nicht vor die behandlung von CHANNEL_DATA? oder gilt OPEN => !DATA ? 
														//es gilt: open -> data
														pHashEntry->pSymCipher->crypt1(pMixPacket->data,rsaBuff,KEY_SIZE);
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
														pCipher->setKey(rsaBuff);
														for(int i=0;i<16;i++)
															rsaBuff[i]=0xFF;
														pCipher->setIV2(rsaBuff);
														pCipher->crypt1(pMixPacket->data+KEY_SIZE,pMixPacket->data,DATA_SIZE-KEY_SIZE);
														getRandom(pMixPacket->data+DATA_SIZE-KEY_SIZE,KEY_SIZE);
														#ifdef LOG_CHANNEL
															HCHANNEL tmpC=pMixPacket->channel;
														#endif
														if(m_pChannelList->addChannel(pMuxSocket,pMixPacket->channel,pCipher,&pMixPacket->channel)!=E_SUCCESS)
														{ //todo move up ?
															delete pCipher;
														}
														else
														{
															#ifdef LOG_PACKET_TIMES
																getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
															#endif
															#ifdef LOG_CHANNEL
																fmChannelListEntry* pTmpEntry=m_pChannelList->get(pMuxSocket,tmpC);
																pTmpEntry->packetsInFromUser++;
																set64(pTmpEntry->timeCreated,pQueueEntry->timestamp_proccessing_start);
															#endif
															m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));
															incMixedPackets();
															#ifdef _DEBUG
//																			CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",pMixPacket->channel);
															#endif
														}
													}
												}
											}
								#ifdef HAVE_EPOLL
NEXT_USER:
									pHashEntry=(fmHashTableEntry*)m_psocketgroupUsersRead->getNextSignaledSocketData();
								#else
									}//if is signaled
NEXT_USER:
									pHashEntry=m_pChannelList->getNext();
								#endif
							}
					}
//Third step
//Sending to next mix

// Now in a separate Thread (see loopSendToMix())

//Step 4
//Stepa 4a Receiving form Mix to Queue now in separat Thread
//Step 4b Proccesing MixPackets received from Mix
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
										pEntry->pHead->pQueueSend->add(pMixPacket,sizeof(tQueueEntry));
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
											m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket,pEntry->pHead); 
										#else
											m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket); 
										#endif
										
	
										#ifndef SSL_HACK	
											delete pEntry->pCipher;              // forget the symetric key of this connection
											m_pChannelList->removeChannel(pEntry->pHead->pMuxSocket, pEntry->channelIn);
										/* a hack to solve the SSL problem: 
										 * remove channel after the close packet is enqueued
										 * from pEntry->pQueueSend
										 */
										#endif
									}
									else
									{
										CAMsg::printMsg(LOG_DEBUG, "CAFirstMixA: close channel -> client but channel does not exist.\n");
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
													UINT32 id=(pMixPacket->flags>>8)&0x000000FF;
													int log=LOG_ENCRYPTED;
													if(!pglobalOptions->isEncryptedLogEnabled())
														log=LOG_CRIT;
													CAMsg::printMsg(log,"Detecting crime activity - ID: %u -- In-IP is: %u.%u.%u.%u \n",id,pEntry->pHead->peerIP[0],pEntry->pHead->peerIP[1],pEntry->pHead->peerIP[2],pEntry->pHead->peerIP[3]);
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
										pEntry->pHead->pQueueSend->add(pMixPacket,sizeof(tQueueEntry));
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
											m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket,pEntry->pHead); 
										#else
											m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket); 
										#endif
		
#ifndef NO_PARKING
//										UINT32 uQueueSize=pEntry->pHead->pQueueSend->getSize();
//										if(uQueueSize>200000)
//											CAMsg::printMsg(LOG_INFO,"User Send Queue size is now %u\n",uQueueSize);
										if(	
											#ifndef SSL_HACK
											pEntry->pHead->pQueueSend->getSize() > MAX_USER_SEND_QUEUE 
											#else
											/* a hack to solve the SSL problem:
											 * only suspend channels with > MAX_DATA_PER_CHANNEL bytes
											 */
											pEntry->downStreamBytes > MAX_DATA_PER_CHANNEL 
											#endif 
											&& !pEntry->bIsSuspended)
												
											{
												pMixPacket->channel=pEntry->channelOut;
												pMixPacket->flags=CHANNEL_SUSPEND;
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_INFO,"Sending suspend for channel: %u\n",pMixPacket->channel);
												#endif												
												#ifdef LOG_PACKET_TIMES
													setZero64(pQueueEntry->timestamp_proccessing_start);
												#endif
												m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));
												pEntry->bIsSuspended=true;
												pEntry->pHead->cSuspend++;
											}
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

//Step 5 
//Writing to users...
				countRead=m_psocketgroupUsersWrite->select(/*true,*/0);
#ifdef HAVE_EPOLL		
				fmHashTableEntry* pfmHashEntry=(fmHashTableEntry*)m_psocketgroupUsersWrite->getFirstSignaledSocketData();
				while(pfmHashEntry!=NULL)
					{
#else
				fmHashTableEntry* pfmHashEntry=m_pChannelList->getFirst();
				while(countRead>0&&pfmHashEntry!=NULL)
					{
						if(m_psocketgroupUsersWrite->isSignaled(*pfmHashEntry->pMuxSocket))
							{
								countRead--;
#endif
#ifdef DELAY_USERS
								if(pfmHashEntry->delayBucket>0)
								{
#endif
									
								if(pfmHashEntry->pQueueSend->getSize()>0)
								{
									//CAMsg::printMsg(LOG_CRIT,"turning!!\n");
									bAktiv=true;
									UINT32 len=sizeof(tQueueEntry);
									if(pfmHashEntry->uAlreadySendPacketSize==-1)
									{
										pfmHashEntry->pQueueSend->get((UINT8*)&pfmHashEntry->oQueueEntry,&len); 
										
										/* Hack for SSL BUG */
#ifdef SSL_HACK
										fmChannelList* cListEntry=m_pChannelList->get(pfmHashEntry->oQueueEntry.packet.channel);
										if(cListEntry != NULL)
										{
											pfmHashEntry->oQueueEntry.packet.channel = cListEntry->channelIn;
											cListEntry->downStreamBytes -= len;
#ifdef DEBUG
											CAMsg::printMsg(LOG_DEBUG, "CAFirstMixA: channels of current packet, in: %u, out: %u, count: %u, flags: 0x%x\n", 
													cListEntry->channelIn, cListEntry->channelOut, cListEntry->downStreamBytes,
													pfmHashEntry->oQueueEntry.packet.flags);
#endif
											if(pfmHashEntry->oQueueEntry.packet.flags == CHANNEL_CLOSE)
											{
												delete cListEntry->pCipher;
												m_pChannelList->removeChannel(pfmHashEntry->pMuxSocket, cListEntry->channelIn); 
											}
										}
#endif //SSL_HACK
										/* end hack */
										#ifdef PAYMENT
											//do not count control channel packets!
											if(pfmHashEntry->oQueueEntry.packet.channel>0&&pfmHashEntry->oQueueEntry.packet.channel<256)
												pfmHashEntry->bCountPacket=false;
											else
												pfmHashEntry->bCountPacket=true;
										#endif
										pfmHashEntry->pMuxSocket->prepareForSend(&(pfmHashEntry->oQueueEntry.packet));
										pfmHashEntry->uAlreadySendPacketSize=0;
									}
									
								len=MIXPACKET_SIZE-pfmHashEntry->uAlreadySendPacketSize;
								ret=((CASocket*)pfmHashEntry->pMuxSocket)->send(((UINT8*)&(pfmHashEntry->oQueueEntry))+pfmHashEntry->uAlreadySendPacketSize,len);
								if(ret>0)
									{
										pfmHashEntry->uAlreadySendPacketSize+=ret;
										if(pfmHashEntry->uAlreadySendPacketSize==MIXPACKET_SIZE)
											{
												#ifdef PAYMENT
												//if(pfmHashEntry->bCountPacket)
												{
													// count packet for payment
													SINT32 ret = CAAccountingInstance::handleJapPacket(pfmHashEntry, !(pfmHashEntry->bCountPacket), true);
													if (ret==CAAccountingInstance::HANDLE_PACKET_CONNECTION_OK)
													{
														// renew the timeout
														pfmHashEntry->bRecoverTimeout = true;														
														m_pChannelList->pushTimeoutEntry(pfmHashEntry);	
													}
													else if (ret==CAAccountingInstance::HANDLE_PACKET_HOLD_CONNECTION ||
															ret==CAAccountingInstance::HANDLE_PACKET_PREPARE_FOR_CLOSING_CONNECTION )
													{
														// the next timeout might be deadly for this connection...
														pfmHashEntry->bRecoverTimeout = false;	
													}
													else if (ret==CAAccountingInstance::HANDLE_PACKET_CLOSE_CONNECTION )
													{
														CAMsg::printMsg(LOG_DEBUG, "CAFirstMixA: Closing JAP connection due to illegal payment status!\n", ret);														
														closeConnection(pfmHashEntry);
goto NEXT_USER_WRITING;
													}
												}
												#endif
												#ifdef DELAY_USERS
													pfmHashEntry->delayBucket--;
												#endif
												pfmHashEntry->uAlreadySendPacketSize=-1;
												#ifdef LOG_PACKET_TIMES
													if(!isZero64(pfmHashEntry->oQueueEntry.timestamp_proccessing_start))
														{
															getcurrentTimeMicros(pfmHashEntry->oQueueEntry.timestamp_proccessing_end);
															m_pLogPacketStats->addToTimeingStats(pfmHashEntry->oQueueEntry,CHANNEL_DATA,false);
														}
												#endif
											}

										if( pfmHashEntry->cSuspend > 0 &&
												pfmHashEntry->pQueueSend->getSize() < USER_SEND_BUFFER_RESUME)
											{
												
												fmChannelListEntry* pEntry;
												pEntry=m_pChannelList->getFirstChannelForSocket(pfmHashEntry->pMuxSocket);
												while(pEntry!=NULL)
												{
													if(pEntry->bIsSuspended)
													{
														pMixPacket->flags=CHANNEL_RESUME;
														pMixPacket->channel=pEntry->channelOut;
														//m_pMuxOut->send(pMixPacket,tmpBuff);
														#ifdef LOG_PACKET_TIMES
															setZero64(pQueueEntry->timestamp_proccessing_start);
														#endif
														#ifdef _DEBUG
															CAMsg::printMsg(LOG_INFO,"Sending resume for channel: %u\n",pMixPacket->channel);
														#endif												
														m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));
														pEntry->bIsSuspended=false;	
													}
													
													pEntry=m_pChannelList->getNextChannel(pEntry);
												}
												pfmHashEntry->cSuspend=0;
											}
									}
								
								}
								
									
								
#ifdef DELAY_USERS
								}
#endif
									//todo error handling
#ifdef HAVE_EPOLL
NEXT_USER_WRITING:
						pfmHashEntry=(fmHashTableEntry*)m_psocketgroupUsersWrite->getNextSignaledSocketData();
						
#else
							}//if is socket signaled					
NEXT_USER_WRITING:							
						pfmHashEntry=m_pChannelList->getNext();
#endif
					}
				if(!bAktiv)
				  msSleep(100);
			}
//ERR:
		CAMsg::printMsg(LOG_CRIT,"Seems that we are restarting now!!\n");
		m_bRunLog=false;
		clean();
		delete pQueueEntry;
		delete []tmpBuff;
#ifdef _DEBUG
		pLogThread->join();
		delete pLogThread; 
#endif		
		CAMsg::printMsg(LOG_CRIT,"Main Loop exited!!\n");
#endif//NEW_MIX_TYPE
		return E_UNKNOWN;
	}
#endif //ONLY_LOCAL_PROXY
