#include "../StdAfx.h"
#include "CAFirstMixB.hpp"
#include "../CASingleSocketGroup.hpp"
#include "../CAInfoService.hpp"
#include "../CAPool.hpp"
#ifndef NO_LOOPACCEPTUSER
	#define NO_LOOPACCEPTUSER
#endif
#ifdef NEW_MIX_TYPE
SINT32 CAFirstMixB::loop()
	{
		CASingleSocketGroup osocketgroupMixOut;
		SINT32 countRead;
		MIXPACKET* pMixPacket=new MIXPACKET;
		m_nUser=0;
		m_bRestart=false;
		SINT32 ret;
		osocketgroupMixOut.add(*m_pMuxOut);
		m_pMuxOut->setCrypt(true);

		m_pInfoService->setSignature(m_pSignature,NULL);
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix InfoService - Signature set\n");
		m_pInfoService->start();
		CAMsg::printMsg(LOG_DEBUG,"InfoService Loop started\n");

		UINT8* tmpBuff=new UINT8[MIXPACKET_SIZE];
		CAMsg::printMsg(LOG_DEBUG,"Starting Message Loop... \n");
		bool bAktiv;
		//Starting thread for Step 1
		UINT32 i;
#if !defined(_DEBUG) && !defined(NO_LOOPACCEPTUSER)
		CAThread threadAcceptUsers;
		threadAcceptUsers.setMainLoop(loopAcceptUsers);
		threadAcceptUsers.start(this);
#else
		CASocketGroup osocketgroupAccept;
		for(i=0;i<m_nSocketsIn;i++)
			osocketgroupAccept.add(m_arrSocketsIn[i]);

#endif
		//Starting thread for Step 2
		UINT8 peerIP[4];
//		UINT8 rsaBuff[RSA_SIZE];
#ifdef LOG_CHANNEL
		UINT64 current_time;
		UINT32 diff_time;
#endif
//		CAThread threadReadFromUsers;
//		threadReadFromUsers.setMainLoop(loopReadFromUsers);
//		threadReadFromUsers.start(this);

		#ifdef USE_POOL
			CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		#endif

		//Starting thread for Step 4
		CAThread threadSendToMix;
		threadSendToMix.setMainLoop(fm_loopSendToMix);
		threadSendToMix.start(this);

		while(!m_bRestart)	                                                          // the main mix loop as long as there are things that are not handled by threads.
			{
				bAktiv=false;
//LOOP_START:

//First Step
//Checking for new connections
// Now in a separat Thread.... (if NOT _DEBUG defined!)

#if defined(_DEBUG) || defined(NO_LOOPACCEPTUSER)

				countRead=osocketgroupAccept.select(false,0);	                // how many new JAP<->mix connections are there
				i=0;
				if(countRead>0)
					bAktiv=true;
				while(countRead>0&&i<m_nSocketsIn)														// iterate through all those sockets as long as there is a new one left.
					{
						if(osocketgroupAccept.isSignaled(m_arrSocketsIn[i]))			// if new client connection
							{
								countRead--;
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"New direct Connection from Client!\n");
								#endif
								CAMuxSocket* pnewMuxSocket=new CAMuxSocket;						// create a socket object for this JAP<->mix connection
								if(m_arrSocketsIn[i].accept(*(CASocket*)pnewMuxSocket)!=E_SUCCESS)  // establish the connection and IF that fails
									{
										CAMsg::printMsg(LOG_ERR,"Accept Error %u - direct Connection from Client!\n",GET_NET_ERROR);
										delete pnewMuxSocket;
									}
								else																									// connection established
									{
																																			// check for simple DoS attack.
											ret=((CASocket*)pnewMuxSocket)->getPeerIP(peerIP);
											if(ret!=E_SUCCESS||m_pIPList->insertIP(peerIP)<0) // IF we can't read the peer's IP or he already has too many connections
												{
													pnewMuxSocket->close();
													delete pnewMuxSocket;
												}
											else
												{
																																			// no DoS attack. business as usual.
													#ifdef _DEBUG
														int ret=((CASocket*)pnewMuxSocket)->setKeepAlive(true);
														if(ret!=E_SUCCESS)
															CAMsg::printMsg(LOG_DEBUG,"Error setting KeepAlive!");
													#else
														((CASocket*)pnewMuxSocket)->setKeepAlive(true);
													#endif
													//
													//	ADDITIONAL PREREQUISITE:
													//	The timestamps in the messages require the user to sync his time
													//	with the time of the cascade. Hence, the current time needs to be
													//	added to the data that is sent to the user below.
													//	For the mixes that form the cascade, the synchronization can be
													//	left to an external protocol such as NTP. Unfortunately, this is
													//	not enforceable for all users.
													//
													((CASocket*)pnewMuxSocket)->send(m_xmlKeyInfoBuff,m_xmlKeyInfoSize);  // send the mix-keys to JAP
													((CASocket*)pnewMuxSocket)->setNonBlocking(true);	                    // stefan: sendet das send in der letzten zeile doch noch nicht? wenn doch, kann dann ein JAP nicht durch verweigern der annahme hier den mix blockieren? vermutlich nciht, aber andersherum faend ich das einleuchtender.
													// es kann nicht blockieren unter der Annahme das der TCP-Sendbuffer > m_xmlKeyInfoSize ist....
													m_pChannelList->add(pnewMuxSocket,peerIP,new CAQueue); // adding user connection to mix->JAP channel list (stefan: sollte das nicht connection list sein? --> es handelt sich um eine Datenstruktu fuer Connections/Channels ).
													incUsers();																	// increment the user counter by one
													m_psocketgroupUsersRead->add(*pnewMuxSocket); // add user socket to the established ones that we read data from.
												}
									}
							}
						i++;
					}
#endif

// Second Step
// Checking for data from users
// Now in a separate Thread (see loopReadFromUsers())
//Only proccess user data, if queue to next mix is not to long!!
/*
#define MAX_NEXT_MIX_QUEUE_SIZE 10000000 //10 MByte
				if(m_pQueueSendToMix->getSize()<MAX_NEXT_MIX_QUEUE_SIZE)
					{
						fmHashTableEntry* pHashEntry=m_pChannelList->getFirst();
						countRead=m_psocketgroupUsersRead->select(false,0);				// how many JAP<->mix connections have received data from their coresponding JAP
						if(countRead>0)
							bAktiv=true;
						while(pHashEntry!=NULL&&countRead>0)											// iterate through all connections as long as there is at least one active left
							{
								CAMuxSocket* pMuxSocket=pHashEntry->pMuxSocket;
								if(m_psocketgroupUsersRead->isSignaled(*pMuxSocket))	// if this one seems to have data
									{
										countRead--;
										ret=pMuxSocket->receive(pMixPacket,0);
										if(ret==SOCKET_ERROR)
											{																								// remove dead connections
												#ifndef LOG_CHANNEL
													m_pIPList->removeIP(pHashEntry->peerIP);
												#else
													getcurrentTimeMillis(current_time);
													diff_time=diff64(current_time,pHashEntry->timeCreated);
													CAMsg::printMsg(LOG_DEBUG,"Connection closed - Connection: %Lu, PacketsIn %u, Packets Out %u - Connection start %Lu, Connection end %Lu, Connection duration %u\n",
																						pHashEntry->id,pHashEntry->trafficIn,pHashEntry->trafficOut,pHashEntry->timeCreated,current_time,diff_time);
													m_pIPList->removeIP(pHashEntry->peerIP,diff_time,pHashEntry->trafficIn,pHashEntry->trafficOut);
												#endif
												m_psocketgroupUsersRead->remove(*(CASocket*)pMuxSocket);
												m_psocketgroupUsersWrite->remove(*(CASocket*)pMuxSocket);
												fmChannelListEntry* pEntry;
												pEntry=m_pChannelList->getFirstChannelForSocket(pMuxSocket);
												while(pEntry!=NULL)
													{
														getRandom(pMixPacket->data,DATA_SIZE);
														pMixPacket->flags=CHANNEL_CLOSE;
														pMixPacket->channel=pEntry->channelOut;
														//m_pMuxOut->send(pMixPacket,tmpBuff);
														m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);
														delete pEntry->pCipher;
														pEntry=m_pChannelList->getNextChannel(pEntry);
													}
												ASSERT(pHashEntry->pQueueSend!=NULL,"Send queue is NULL");
												delete pHashEntry->pQueueSend;
												m_pChannelList->remove(pMuxSocket);
												pMuxSocket->close();
												delete pMuxSocket;
												decUsers();
											}
										else if(ret==MIXPACKET_SIZE) 											// we've read enough data for a whole mix packet. nice!
											{
												if(!pMuxSocket->getIsEncrypted())	            //Encryption is not set yet ->
																																			//so we assume that this is
																																			//the first packet of a connection
																																			//which contains the key
														// stefan: ist das eine der protokoll-leichen die bald rausfliegen? --> ja hoffentlich...
													{
														m_pRSA->decrypt(pMixPacket->data,rsaBuff);
														if(memcmp("KEYPACKET",rsaBuff,9)!=0)
															{
																m_pIPList->removeIP(pHashEntry->peerIP);
																m_psocketgroupUsersRead->remove(*(CASocket*)pMuxSocket);
																m_psocketgroupUsersWrite->remove(*(CASocket*)pMuxSocket);
																delete pHashEntry->pQueueSend;
																m_pChannelList->remove(pMuxSocket);
																pMuxSocket->close();
																delete pMuxSocket;
																decUsers();
															}
														else
															{
																pMuxSocket->setKey(rsaBuff+9,32);
																pMuxSocket->setCrypt(true);
															}
														goto NEXT_USER_CONNECTION;
													}
												#ifdef LOG_CHANNEL
													pHashEntry->trafficIn++;
												#endif
												if(pMixPacket->flags==CHANNEL_DUMMY)					// just a dummy to keep the connection alife in e.g. NAT gateways
													{
														getRandom(pMixPacket->data,DATA_SIZE);
														pMuxSocket->send(pMixPacket,tmpBuff);
														pHashEntry->pQueueSend->add(tmpBuff,MIXPACKET_SIZE);
														m_psocketgroupUsersWrite->add(*pMuxSocket);
													}
												else if(pMixPacket->flags==CHANNEL_CLOSE)			// closing one mix-channel (not the JAP<->mix connection!)
													{
														fmChannelListEntry* pEntry;
														pEntry=m_pChannelList->get(pMuxSocket,pMixPacket->channel);
														if(pEntry!=NULL)
															{
																pMixPacket->channel=pEntry->channelOut;
																getRandom(pMixPacket->data,DATA_SIZE);
																//m_pMuxOut->send(pMixPacket,tmpBuff);
																m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);
																#ifdef LOG_CHANNEL
																	//pEntry->packetsInFromUser++;
																	getcurrentTimeMillis(current_time);
																	diff_time=diff64(current_time,pEntry->timeCreated);
																	CAMsg::printMsg(LOG_DEBUG,"Channel close - Channel: %u, Connection: %Lu - PacketsIn (only data): %u, PacketsOut (only data): %u - ChannelStart: %Lu, ChannelEnd: %Lu, ChannelDuration: %u\n",
																														pEntry->channelIn,pEntry->pHead->id,pEntry->packetsInFromUser,pEntry->packetsOutToUser,pEntry->timeCreated,current_time,diff_time);
																#endif
																delete pEntry->pCipher;              // forget the symetric key of this connection
																m_pChannelList->removeChannel(pMuxSocket,pEntry->channelIn);
															}
														#ifdef _DEBUG
														else
															{
																	CAMsg::printMsg(LOG_DEBUG,"Invalid ID to close from Browser!\n");
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
																pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
																//m_pMuxOut->send(pMixPacket,tmpBuff);            // prepare packet for sending (apply symmetric cypher to first block)
																m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE); // queue the packet for sending to the next mix.
																incMixedPackets();
																#ifdef LOG_CHANNEL
																	pEntry->packetsInFromUser++;
																#endif
															}
														else if(pEntry==NULL&&pMixPacket->flags==CHANNEL_OPEN)  // open a new mix channel
														{ // stefan: muesste das nicht vor die behandlung von CHANNEL_DATA? oder gilt OPEN => !DATA ?
														   //es gilt: open -> data
																m_pRSA->decrypt(pMixPacket->data,rsaBuff); // stefan: das hier ist doch eine ziemlich kostspielige operation. sollte das pruefen auf Max_Number_Of_Channels nicht vorher passieren? --> ok sollte aufs TODO ...
																#ifdef REPLAY_DETECTION
																	if(!validTimestampAndFingerprint(rsaBuff, KEY_SIZE, (rsaBuff+KEY_SIZE)))
																		{
																			CAMsg::printMsg(LOG_INFO,"Duplicate packet ignored.\n");
																			goto NEXT_USER_CONNECTION;
																		}
																#endif
																pCipher= new CASymCipher();
																pCipher->setKeyAES(rsaBuff);
																pCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																								 pMixPacket->data+RSA_SIZE-(KEY_SIZE+TIMESTAMP_SIZE),
																								 DATA_SIZE-RSA_SIZE);
																memcpy(pMixPacket->data,rsaBuff+KEY_SIZE+TIMESTAMP_SIZE,RSA_SIZE-(KEY_SIZE+TIMESTAMP_SIZE));
																#ifdef LOG_CHANNEL
																	HCHANNEL tmpC=pMixPacket->channel;
																#endif
																if(m_pChannelList->addChannel(pMuxSocket,pMixPacket->channel,pCipher,&pMixPacket->channel)!=E_SUCCESS)
																	{ //todo move up ?
																		delete pCipher;
																	}
																else
																	{
																		#ifdef LOG_CHANNEL
																			m_pChannelList->get(pMuxSocket,tmpC)->packetsInFromUser++;
																		#endif
																		//m_pMuxOut->send(pMixPacket,tmpBuff);
																		m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);
																		incMixedPackets();
																		#ifdef _DEBUG
																			CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",pMixPacket->channel);
																		#endif
																	}
															}
													}
											}
									}
	NEXT_USER_CONNECTION:
									pHashEntry=m_pChannelList->getNext();
							}
					}
//Third step
//Sending to next mix

// Now in a separate Thread (see loopSendToMix())

//Step 4
//Reading from Mix
				countRead=m_nUser+1;
				while(countRead>0&&osocketgroupMixOut.select(false,0)==1)
					{
						bAktiv=true;
						countRead--;
						ret=m_pMuxOut->receive(pMixPacket,0);
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Mux-Out-Channel Receiving Data Error - Exiting!\n");
								goto ERR;
							}
						if(ret!=MIXPACKET_SIZE)
							break;
						#ifdef USE_POOL
							pPool->pool(pMixPacket);
						#endif
						if(pMixPacket->flags==CHANNEL_CLOSE) //close event
							{
								#if defined(_DEBUG) && !defined(__MIX_TEST)
									CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ...\n",pMixPacket->channel);
								#endif
								fmChannelList* pEntry=m_pChannelList->get(pMixPacket->channel);
								if(pEntry!=NULL)
									{
										pMixPacket->channel=pEntry->channelIn;
										getRandom(pMixPacket->data,DATA_SIZE);
										pEntry->pHead->pMuxSocket->send(pMixPacket,tmpBuff);
										pEntry->pHead->pQueueSend->add(tmpBuff,MIXPACKET_SIZE);
										#ifdef LOG_CHANNEL
											pEntry->pHead->trafficOut++;
											//pEntry->packetsOutToUser++;
											getcurrentTimeMillis(current_time);
											diff_time=diff64(current_time,pEntry->timeCreated);
											CAMsg::printMsg(LOG_DEBUG,"Channel close - Channel: %u, Connection: %Lu - PacketsIn (only data): %u, PacketsOut (only data): %u - ChannelStart: %Lu, ChannelEnd: %Lu, ChannelDuration: %u\n",
																								pEntry->channelIn,pEntry->pHead->id,pEntry->packetsInFromUser,pEntry->packetsOutToUser,pEntry->timeCreated,current_time,diff_time);
										#endif

										m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket);
										delete pEntry->pCipher;

										m_pChannelList->removeChannel(pEntry->pHead->pMuxSocket,pEntry->channelIn);
									}
							}
						else
							{
								#if defined(_DEBUG) && !defined(__MIX_TEST)
									CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!");
								#endif
								fmChannelList* pEntry=m_pChannelList->get(pMixPacket->channel);
								if(pEntry!=NULL)
									{
										#ifdef LOG_CRIME
											if((pMixPacket->flags&CHANNEL_SIG_CRIME)==CHANNEL_SIG_CRIME)
												{
													UINT32 id=(pMixPacket->flags>>8)&0x000000FF;
													CAMsg::printMsg(LOG_SPECIAL,"Detecting crime activity - ID: %u -- In-IP is: %u.%u.%u.%u \n",id,pEntry->pHead->peerIP[0],pEntry->pHead->peerIP[1],pEntry->pHead->peerIP[2],pEntry->pHead->peerIP[3]);
													continue;
												}
										#endif
										pMixPacket->channel=pEntry->channelIn;
										pEntry->pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);

										pEntry->pHead->pMuxSocket->send(pMixPacket,tmpBuff);
										pEntry->pHead->pQueueSend->add(tmpBuff,MIXPACKET_SIZE);
										#ifdef LOG_CHANNEL
											pEntry->pHead->trafficOut++;
											pEntry->packetsOutToUser++;
										#endif
										m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket);
#ifndef NO_PARKING
#define MAX_USER_SEND_QUEUE 100000
										if(pEntry->pHead->pQueueSend->getSize()>MAX_USER_SEND_QUEUE&&
												!pEntry->bIsSuspended)
											{
												pMixPacket->channel=pEntry->channelOut;
												pMixPacket->flags=CHANNEL_SUSPEND;
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_INFO,"Sending suspend for channel: %u\n",pMixPacket->channel);
												#endif
												//m_pMuxOut->send(pMixPacket,tmpBuff);
												m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);

												pEntry->bIsSuspended=true;
												pEntry->pHead->cSuspend++;
											}
#endif
										incMixedPackets();
									}
								else
									{
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- Channel-Id %u no valid!\n",pMixPacket->channel);
										#endif
										#ifdef LOG_CHANNEL
											CAMsg::printMsg(LOG_INFO,"Packet late arrive for channel: %u\n",pMixPacket->channel);
										#endif
									}
							}
					}

//Step 5
//Writing to users...
				fmHashTableEntry* pfmHashEntry=m_pChannelList->getFirst();
				countRead=m_psocketgroupUsersWrite->select(true,0);
				if(countRead>0)
					bAktiv=true;
				while(countRead>0&&pfmHashEntry!=NULL)
					{
						if(m_psocketgroupUsersWrite->isSignaled(*pfmHashEntry->pMuxSocket))
							{
								countRead--;
								UINT32 len=MIXPACKET_SIZE;
								pfmHashEntry->pQueueSend->peek(tmpBuff,&len); //We only make a peek() here because we do not know if sending will succeed or not (because send() is non blocking there!)
								ret=((CASocket*)pfmHashEntry->pMuxSocket)->send(tmpBuff,len);
								if(ret>0)
									{
										pfmHashEntry->pQueueSend->remove((UINT32*)&ret);
#define USER_SEND_BUFFER_RESUME 10000
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
																m_pQueueSendToMix->add(pMixPacket,MIXPACKET_SIZE);
																pEntry->bIsSuspended=false;
															}

														pEntry=m_pChannelList->getNextChannel(pEntry);
													}
												pfmHashEntry->cSuspend=0;
											}
										if(pfmHashEntry->pQueueSend->isEmpty())
											{
												m_psocketgroupUsersWrite->remove(*pfmHashEntry->pMuxSocket);
											}
									}
								//todo error handling

							}
						pfmHashEntry=m_pChannelList->getNext();
					}
*/				if(!bAktiv)
				  msSleep(100);
			}

//	ERR:
		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		m_bRestart=true;
		CAMsg::printMsg(LOG_CRIT,"Stopping InfoService....\n");
		CAMsg::printMsg	(LOG_CRIT,"Memory usage before: %u\n",getMemoryUsage());
		m_pInfoService->stop();
		CAMsg::printMsg	(LOG_CRIT,"Memory usage after: %u\n",getMemoryUsage());
		CAMsg::printMsg(LOG_CRIT,"Stopped InfoService!\n");
		m_pMuxOut->close();
		for(i=0;i<m_nSocketsIn;i++)
			m_arrSocketsIn[i].close();
		//writng one byte to the queue...
		UINT8 b;
		m_pQueueSendToMix->add(&b,1);
#if !defined(_DEBUG) && !defined(NO_LOOPACCEPTUSER)
		CAMsg::printMsg(LOG_CRIT,"Wait for LoopAcceptUsers!\n");
		threadAcceptUsers.join();
#endif
		CAMsg::printMsg(LOG_CRIT,"Wait for LoopSendToMix!\n");
		threadSendToMix.join(); //will not join if queue is empty (and so wating)!!!
		//		threadReadFromUsers.join();
		CAMsg::printMsg(LOG_CRIT,"Before deleting CAFirstMixChannelList()!\n");
		CAMsg::printMsg	(LOG_CRIT,"Memeory usage before: %u\n",getMemoryUsage());
		fmHashTableEntry* pHashEntry=m_pChannelList->getFirst();
		while(pHashEntry!=NULL)
			{
				CAMuxSocket * pMuxSocket=pHashEntry->pMuxSocket;
				delete pHashEntry->pQueueSend;

				fmChannelListEntry* pEntry=m_pChannelList->getFirstChannelForSocket(pHashEntry->pMuxSocket);
				while(pEntry!=NULL)
					{
						delete pEntry->pCipher;
						pEntry=m_pChannelList->getNextChannel(pEntry);
					}
				m_pChannelList->remove(pHashEntry->pMuxSocket);
				pMuxSocket->close();
				delete pMuxSocket;
				pHashEntry=m_pChannelList->getNext();
			}
		CAMsg::printMsg	(LOG_CRIT,"Memory usage after: %u\n",getMemoryUsage());
		delete pMixPacket;
		delete []tmpBuff;
		#ifdef USE_POOL
			delete pPool;
		#endif
		CAMsg::printMsg(LOG_CRIT,"Main Loop exited!!\n");
		return E_UNKNOWN;
	}
#endif
