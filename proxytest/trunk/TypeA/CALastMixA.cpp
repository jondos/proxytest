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
#include "CALastMixA.hpp"
#include "../CALibProxytest.hpp"
#include "../CALastMixChannelList.hpp"
#include "../CASingleSocketGroup.hpp"
#include "../CAPool.hpp"
#include "../CACmdLnOptions.hpp"
#ifdef HAVE_EPOLL
#include "../CASocketGroupEpoll.hpp"
#endif

#ifdef LOG_CHANNEL
//CAMsg::printMsg(LOG_DEBUG,"Channel time log format is as follows: Channel-ID,Channel Start [micros], Channel End [micros], Upload (bytes), Download (bytes), DataAndOpenAndClosePacketsFromUser, DataAndClosePacketsToUser\n");
#define MACRO_DO_LOG_CHANNEL(a)\
	CAMsg::printMsg(LOG_DEBUG,#a ":%u,%Lu,%Lu,%u,%u,%u,%u\n",\
			pChannelListEntry->channelIn,pChannelListEntry->timeCreated,pQueueEntry->timestamp_proccessing_end,\
			pChannelListEntry->trafficInFromUser,pChannelListEntry->trafficOutToUser,\
			pChannelListEntry->packetsDataInFromUser,pChannelListEntry->packetsDataOutToUser);
#define MACRO_DO_LOG_CHANNEL_CLOSE_FROM_USER MACRO_DO_LOG_CHANNEL(1)
#define MACRO_DO_LOG_CHANNEL_CLOSE_FROM_MIX MACRO_DO_LOG_CHANNEL(2)
#endif

#define LAST_MIX_SIZE_OF_SYMMETRIC_KEYS 2*KEY_SIZE


SINT32 CALastMixA::loop()
	{
#ifndef NEW_MIX_TYPE
		//CASocketList  oSocketList;
#ifdef DELAY_CHANNELS
		m_pChannelList->setDelayParameters(	CALibProxytest::getOptions()->getDelayChannelUnlimitTraffic(),
																			CALibProxytest::getOptions()->getDelayChannelBucketGrow(),
																			CALibProxytest::getOptions()->getDelayChannelBucketGrowIntervall());
#endif
#ifdef DELAY_CHANNELS_LATENCY
		m_pChannelList->setDelayLatencyParameters(	CALibProxytest::getOptions()->getDelayChannelLatency());
#endif
#ifdef HAVE_EPOLL
		CASocketGroupEpoll* psocketgroupCacheRead=new CASocketGroupEpoll(false);
		CASocketGroupEpoll* psocketgroupCacheWrite=new CASocketGroupEpoll(true);
#else
		CASocketGroup* psocketgroupCacheRead=new CASocketGroup(false);
		CASocketGroup* psocketgroupCacheWrite=new CASocketGroup(true);
#endif
		tQueueEntry* pQueueEntry=new tQueueEntry;
		MIXPACKET* pMixPacket=&pQueueEntry->packet;
		SINT32 ret;
		SINT32 countRead;
		lmChannelListEntry* pChannelListEntry;
		UINT8* rsaBuff=new UINT8[RSA_SIZE];
		UINT32 rsaOutLen=RSA_SIZE;
		UINT8* tmpBuff=new UINT8[MIXPACKET_SIZE];
		UINT8* ciphertextBuff=new UINT8[DATA_SIZE];
		UINT8* plaintextBuff=new UINT8[DATA_SIZE - GCM_MAC_SIZE];
#ifdef WITH_INTEGRITY_CHECK
		UINT16 payloadLen;
		SINT32 retval;
#endif
		bool bAktiv;
		m_logUploadedPackets=m_logDownloadedPackets=0;
		set64((UINT64&)m_logUploadedBytes,(UINT32)0);
		set64((UINT64&)m_logDownloadedBytes,(UINT32)0);
		CAThread* pLogThread=new CAThread((UINT8*)"CALastMixA - LogLoop");
		pLogThread->setMainLoop(lm_loopLog);
		pLogThread->start(this);

		#ifdef LOG_CRIME
			bool bUserSurveillance = false;
		#endif
#ifdef ANON_DEBUG_MODE
			bool bIsDebugPacket = false;
#endif

		#ifdef LOG_CHANNEL
			CAMsg::printMsg(LOG_DEBUG,"Channel time log format is as follows: Channel-ID,Channel Start [micros], Channel End [micros], Upload (bytes), Download (bytes), DataAndOpenPacketsFromUser, DataPacketsToUser\n");
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
								#if defined(LOG_PACKET_TIMES) ||defined(LOG_CHANNEL)
									getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start);
								#endif
								#ifdef LOG_PACKET_TIMES
									set64(pQueueEntry->timestamp_proccessing_start,pQueueEntry->timestamp_proccessing_start_OP);
								#endif
								if(pMixPacket->channel>0&&pMixPacket->channel<256)
									{
										m_pMuxInControlChannelDispatcher->proccessMixPacket(pMixPacket);
										continue;
									}
								// one packet received
								m_logUploadedPackets++;
#ifdef ANON_DEBUG_MODE
								if (pMixPacket->flags&CHANNEL_DEBUG)
									{
										UINT8 base64Payload[DATA_SIZE << 1];
										EVP_EncodeBlock(base64Payload, pMixPacket->data,DATA_SIZE);//base64 encoding (without newline!)
										pMixPacket->flags &= ~CHANNEL_DEBUG;
										CAMsg::printMsg(LOG_DEBUG, "AN.ON packet debug: %s\n",base64Payload);
										bIsDebugPacket = true;
									}
								else
									{
										bIsDebugPacket = false;
									}
#endif
								pChannelListEntry=m_pChannelList->get(pMixPacket->channel);

								//check if this packet was marked by the previous mixes for user surveillance
								#ifdef LOG_CRIME
								bUserSurveillance = ((pMixPacket->flags & CHANNEL_SIG_CRIME) != 0);
								pMixPacket->flags &= ~CHANNEL_SIG_CRIME;
								#endif

								if(pChannelListEntry==NULL)
									{
										if(pMixPacket->flags==CHANNEL_OPEN)
										{
											#if defined(ANON_DEBUG_MODE)
												CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
												//keep a copy of whole packet and output it, if something with integrity check went wrong...
												UINT8 tmpPacketData[DATA_SIZE];
												memcpy(tmpPacketData,pMixPacket->data,DATA_SIZE);
											#endif

											
											SINT32 retAsymDecryption=m_pRSA->decryptOAEP(pMixPacket->data,rsaBuff,&rsaOutLen);
											#ifdef _DEBUG
												if(retAsymDecryption==E_UNKNOWN)
													{
														CAMsg::printMsg(LOG_DEBUG,"Error in channel open asym decryption - channel!\n");
													}
											#endif
											#ifdef REPLAY_DETECTION
												// replace time(NULL) with the real timestamp ()
												// packet-timestamp + m_u64ReferenceTime
												UINT32 stamp=((UINT32)(rsaBuff[13]<<16)+(UINT32)(rsaBuff[14]<<8)+(UINT32)(rsaBuff[15]))*REPLAY_BASE;
												if(m_pReplayDB->insert(rsaBuff,stamp+m_u64ReferenceTime)!=E_SUCCESS)
//													if(m_pReplayDB->insert(rsaBuff,time(NULL))!=E_SUCCESS)
													{
														CAMsg::printMsg(LOG_INFO,"Replay: Duplicate packet ignored.\n");
														continue;
													}
											#endif
											CASymCipher* newCipher = new CASymCipher();
											#ifdef WITH_INTEGRITY_CHECK
												newCipher->setGCMKeys(rsaBuff, rsaBuff + KEY_SIZE);

												//Decrypt only the first two bytes to get the payload length
												UINT16 lengthAndFlagsField=0;
												newCipher->decryptMessage(rsaBuff + LAST_MIX_SIZE_OF_SYMMETRIC_KEYS, 2,(UINT8*) &lengthAndFlagsField, false);
												payloadLen = ntohs(lengthAndFlagsField);
												payloadLen &= PAYLOAD_LEN_MASK;
												if (payloadLen > (PAYLOAD_SIZE-LAST_MIX_SIZE_OF_SYMMETRIC_KEYS-RSA_SIZE-GCM_MAC_SIZE - PAYLOAD_HEADER_SIZE+rsaOutLen))
													retval=E_UNKNOWN;
												else
													{
														//prepend the asym decrypted sym encrypted part of the Mix packet to the sym only encrypted part of the mix packet
														memcpy(pMixPacket->data+RSA_SIZE-rsaOutLen+LAST_MIX_SIZE_OF_SYMMETRIC_KEYS,rsaBuff + LAST_MIX_SIZE_OF_SYMMETRIC_KEYS,rsaOutLen-LAST_MIX_SIZE_OF_SYMMETRIC_KEYS);
														//now decrpyt the whole sym encrypted part
														retval = newCipher->decryptMessage(pMixPacket->data +RSA_SIZE-rsaOutLen+LAST_MIX_SIZE_OF_SYMMETRIC_KEYS,  payloadLen+ GCM_MAC_SIZE + PAYLOAD_HEADER_SIZE , pMixPacket->data, true);
													}
											#else
												newCipher->setKeys(rsaBuff,LAST_MIX_SIZE_OF_SYMMETRIC_KEYS);
												newCipher->crypt1(
														pMixPacket->data+RSA_SIZE,
														pMixPacket->data+rsaOutLen-LAST_MIX_SIZE_OF_SYMMETRIC_KEYS,
														DATA_SIZE-RSA_SIZE);
												memcpy(	pMixPacket->data,rsaBuff+LAST_MIX_SIZE_OF_SYMMETRIC_KEYS,
															rsaOutLen-LAST_MIX_SIZE_OF_SYMMETRIC_KEYS);
											#endif

											#ifdef LOG_PACKET_TIMES
												getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
											#endif
											#ifdef WITH_INTEGRITY_CHECK
												if (retval != E_SUCCESS)
													{
													/* invalid MAC -> send channel close packet with integrity error flag */
													getRandom(pMixPacket->data, DATA_SIZE);
													pMixPacket->flags = CHANNEL_CLOSE;
													pMixPacket->payload.len = htons(INTEGRITY_ERROR_FLAG);
													pMixPacket->payload.type = 0;
													newCipher->encryptMessage(pMixPacket->data, 3, ciphertextBuff);
													memcpy(pMixPacket->data, ciphertextBuff, 3 + GCM_MAC_SIZE);
													delete newCipher;
													newCipher = NULL;
													#ifdef LOG_PACKET_TIMES
														setZero64(pQueueEntry->timestamp_proccessing_start);
													#endif
													m_pQueueSendToMix->add(pQueueEntry,sizeof(tQueueEntry));
													m_logDownloadedPackets++;
													#if defined(ANON_DEBUG_MODE)
														UINT8 tmpPacketBase64[DATA_SIZE<<1];
														EVP_EncodeBlock(tmpPacketBase64,tmpPacketData,DATA_SIZE);
														CAMsg::printMsg(LOG_ERR, "Integrity check failed in channel-open packet: %s\n",tmpPacketBase64);
													#else
														CAMsg::printMsg(LOG_ERR, "Integrity check failed in channel-open packet!\n");
													#endif

												} else {
													#if defined(ANON_DEBUG_MODE)
														UINT8 tmpPacketBase64[DATA_SIZE<<1];
														EVP_EncodeBlock(tmpPacketBase64,tmpPacketData,DATA_SIZE);
														CAMsg::printMsg(LOG_ERR, "Integrity check ok in channel-open packet: %s\n",tmpPacketBase64);
													#endif
											#endif
													
											CASocket* tmpSocket=new CASocket;
											CACacheLoadBalancing* ptmpLB=m_pCacheLB;
											ret=E_UNKNOWN;
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
													#if defined (_DEBUG) || defined (DELAY_CHANNELS_LATENCY)
														CAMsg::printMsg(LOG_DEBUG,"Cannot connect to Squid!\n");
													#endif
													delete tmpSocket;
													tmpSocket = NULL;
													/* send a close packet signaling the connect error */
													getRandom(pMixPacket->data, DATA_SIZE);
													pMixPacket->flags = CHANNEL_CLOSE;
													pMixPacket->payload.len = 0;
													pMixPacket->payload.type = CONNECTION_ERROR_FLAG;
													#ifdef WITH_INTEGRITY_CHECK
														newCipher->encryptMessage(pMixPacket->data, 3, ciphertextBuff);
														memcpy(pMixPacket->data, ciphertextBuff, 3 + GCM_MAC_SIZE);
													#else
														newCipher->crypt2(pMixPacket->data, pMixPacket->data, DATA_SIZE);
													#endif
													#ifdef LOG_PACKET_TIMES
														setZero64(pQueueEntry->timestamp_proccessing_start);
													#endif
													m_pQueueSendToMix->add(pQueueEntry,sizeof(tQueueEntry));
													m_logDownloadedPackets++;
													delete newCipher;
													newCipher = NULL;
												}
												else
													{ //connection to proxy successful
														UINT16 payLen=ntohs(pMixPacket->payload.len);

														//output payload if packet is marked for user surveillance
														#ifdef LOG_CRIME
														UINT32 timeChannelOpened=0;
														if(bUserSurveillance)
														{
															if(CALibProxytest::getOptions()->isPayloadLogged())
																{
																	UINT8 base64Payload[PAYLOAD_SIZE<<1];
																	EVP_EncodeBlock(base64Payload,pMixPacket->payload.data,payLen);//base64 encoding (without newline!)
																	timeChannelOpened=time(NULL);
																	CAMsg::printMsg(LOG_CRIT,"Crime detection: User surveillance, previous mix channel (opened at: %u): %u - Upstream Payload (Base64 encoded): %s\n", timeChannelOpened,pMixPacket->channel,base64Payload);
																}
															/*UINT8 *domain = parseDomainFromPayload(pMixPacket->payload.data, payLen);

															if(domain != NULL || (CALibProxytest::getOptions()->isPayloadLogged()) )
															{
																CAMsg::printMsg(LOG_CRIT,"Crime detection: User surveillance, previous mix channel: %u\n", pMixPacket->channel);
																if(domain != NULL)
																{
																	CAMsg::printMsg(LOG_CRIT, "Domain: %s\n", domain);
																	delete [] domain;
																}
																if(CALibProxytest::getOptions()->isPayloadLogged())
																{
																	///todo: We need some printyRawBytes here -->otherwise we will only print the part until the first '0' character...
																	UINT8 tempPayload[PAYLOAD_SIZE+1];
																	memcpy(tempPayload, pMixPacket->payload.data,payLen);
																	tempPayload[payLen]=0;
																	CAMsg::printMsg(LOG_CRIT, "Payload: %s\n",tempPayload);
																}
															}*/
														}
														#endif

														#ifdef _DEBUG
															UINT8 c=pMixPacket->payload.data[30];
															pMixPacket->payload.data[30]=0;
															CAMsg::printMsg(LOG_DEBUG,"Try sending data to Squid: %s\n",pMixPacket->payload.data);
															pMixPacket->payload.data[30]=c;
														#endif
														#ifdef LOG_CRIME
															if(payLen<=PAYLOAD_SIZE&&checkCrime(pMixPacket->payload.data,payLen,true))
																{
																	UINT8 crimeBuff[PAYLOAD_SIZE+1];
																	tQueueEntry oSigCrimeQueueEntry;
																	memset(&oSigCrimeQueueEntry,0,sizeof(tQueueEntry));
																	memset(crimeBuff,0,PAYLOAD_SIZE+1);
																	memcpy(crimeBuff,pMixPacket->payload.data,payLen);
																	m_pMuxIn->sigCrime(pMixPacket->channel,&oSigCrimeQueueEntry.packet);
																	m_pQueueSendToMix->add(&oSigCrimeQueueEntry,sizeof(tQueueEntry));
																	int log=LOG_ENCRYPTED;
																	if(!CALibProxytest::getOptions()->isEncryptedLogEnabled())
																		log=LOG_CRIT;
																	CAMsg::printMsg(log,"Crime detected -- previous mix channel: "
																			"%u -- Content: \n%s\n", pMixPacket->channel,
																			(CALibProxytest::getOptions()->isPayloadLogged() ? crimeBuff : (UINT8 *)"<not logged>"));
																}
														#endif
														if(payLen>PAYLOAD_SIZE||tmpSocket->sendTimeOut(pMixPacket->payload.data,payLen,LAST_MIX_TO_PROXY_SEND_TIMEOUT)==SOCKET_ERROR)
														{
															#ifdef _DEBUG
																CAMsg::printMsg(LOG_DEBUG,"Error sending Data to Squid!\n");
															#endif
															tmpSocket->close();
															delete tmpSocket;
															tmpSocket = NULL;
															/* send a close packet signaling the connect error */
															getRandom(pMixPacket->data, DATA_SIZE);
															pMixPacket->flags = CHANNEL_CLOSE;
															pMixPacket->payload.len = 0;
															pMixPacket->payload.type = CONNECTION_ERROR_FLAG;
															#ifdef WITH_INTEGRITY_CHECK
																newCipher->encryptMessage(pMixPacket->data, 3, ciphertextBuff);
																memcpy(pMixPacket->data, ciphertextBuff, 3 + GCM_MAC_SIZE);
															#else
																newCipher->crypt2(pMixPacket->data, pMixPacket->data, DATA_SIZE);
															#endif
															#ifdef LOG_PACKET_TIMES
																setZero64(pQueueEntry->timestamp_proccessing_start);
															#endif
															m_pQueueSendToMix->add(pQueueEntry,sizeof(tQueueEntry));
															m_logDownloadedPackets++;
																delete newCipher;
																newCipher = NULL;
														}
														else
														{
															tmpSocket->setNonBlocking(true);
															#if defined (DELAY_CHANNELS_LATENCY)
																UINT64 u64temp;
																getcurrentTimeMillis(u64temp);
															#endif
															CAQueue* pQueue=new CAQueue(PAYLOAD_SIZE);
															#ifdef LASTMIX_CHECK_MEMORY
																pQueue->logIfSizeGreaterThen(100000);
															#endif
															m_pChannelList->add(pMixPacket->channel,tmpSocket,newCipher,pQueue
															#if defined (LOG_CHANNEL)
																									,pQueueEntry->timestamp_proccessing_start,payLen
															#endif
															#if defined (DELAY_CHANNELS_LATENCY)
																									,u64temp
															#endif
															#ifdef LOG_CRIME
																									,(bUserSurveillance&&CALibProxytest::getOptions()->isPayloadLogged()),timeChannelOpened
															#endif
	#ifdef ANON_DEBUG_MODE
,bIsDebugPacket
#endif
																										);
#ifdef HAVE_EPOLL
															psocketgroupCacheRead->add(*tmpSocket,m_pChannelList->get(pMixPacket->channel));
#else
															psocketgroupCacheRead->add(*tmpSocket);
#endif
															#ifdef LOG_PACKET_TIMES
																getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
																m_pLogPacketStats->addToTimeingStats(*pQueueEntry,CHANNEL_OPEN,true);
															#endif
															#ifdef DATA_RETENTION_LOG
																pQueueEntry->dataRetentionLogEntry.t_out=htonl(time(NULL));
																pQueueEntry->dataRetentionLogEntry.entity.last.channelid=htonl(pMixPacket->channel);
																pQueueEntry->dataRetentionLogEntry.entity.last.port_out=tmpSocket->getLocalPort();
																pQueueEntry->dataRetentionLogEntry.entity.last.port_out=htons(pQueueEntry->dataRetentionLogEntry.entity.last.port_out);
																tmpSocket->getLocalIP(pQueueEntry->dataRetentionLogEntry.entity.last.ip_out);
																m_pDataRetentionLog->log(&pQueueEntry->dataRetentionLogEntry);
															#endif

														}
													}
#ifdef WITH_INTEGRITY_CHECK
											}
#endif
										}
									}
								else
									{//channellist entry !=NULL
										if(pMixPacket->flags==CHANNEL_CLOSE)
											{
												/** Do not realy close the connection - just inform the Queue that it is closed,
												so that the remaining data will be sent to the server*/
												/*
												psocketgroupCacheRead->remove(*(pChannelListEntry->pSocket));
												psocketgroupCacheWrite->remove(*(pChannelListEntry->pSocket));
												pChannelListEntry->pSocket->close();
												delete pChannelListEntry->pSocket;
												pChannelListEntry->pSocket = NULL;
												delete pChannelListEntry->pCipher;
												pChannelListEntry->pCipher = NULL;
												delete pChannelListEntry->pQueueSend;
												pChannelListEntry->pQueueSend = NULL;
												*/
												pChannelListEntry->pQueueSend->close();
#ifdef HAVE_EPOLL
												psocketgroupCacheWrite->add(*(pChannelListEntry->pSocket),pChannelListEntry);
#else
												psocketgroupCacheWrite->add(*(pChannelListEntry->pSocket));
#endif
												#if defined (LOG_PACKET_TIMES) ||defined (LOG_CHANNEL)
													getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
												#endif
												#if defined (LOG_PACKET_TIMES)
													set64(pQueueEntry->timestamp_proccessing_end_OP,pQueueEntry->timestamp_proccessing_end);
													m_pLogPacketStats->addToTimeingStats(*pQueueEntry,CHANNEL_CLOSE,true);
												#endif
												#ifdef LOG_CHANNEL
													pChannelListEntry->packetsDataInFromUser++;
													MACRO_DO_LOG_CHANNEL_CLOSE_FROM_USER
												#endif
												//m_pChannelList->removeChannel(pMixPacket->channel);
											}
										else if(pMixPacket->flags==CHANNEL_DATA)
											{
#if defined(ANON_DEBUG_MODE)
											//keep a copy of whole packet and output it, if something with integrity check went wrong...
											UINT8 tmpPacket[DATA_SIZE];
											memcpy(tmpPacket, pMixPacket->data, DATA_SIZE);
#endif
												#ifdef LOG_CHANNEL
													pChannelListEntry->packetsDataInFromUser++;
												#endif
												#ifdef WITH_INTEGRITY_CHECK
													/* decrypt only the first 2 bytes to get the payload length */
													UINT16 lengthAndFlagsField=0;
													pChannelListEntry->pCipher->decryptMessage(pMixPacket->data, 2,(UINT8*) &lengthAndFlagsField, false);
													payloadLen = ntohs(lengthAndFlagsField);
													payloadLen &= PAYLOAD_LEN_MASK;
													if (payloadLen > PAYLOAD_SIZE)
														retval=E_UNKNOWN;
													else
														{
															retval = pChannelListEntry->pCipher->decryptMessage(pMixPacket->data, payloadLen + 3 + GCM_MAC_SIZE, plaintextBuff, true);
														}
													if (retval != E_SUCCESS) {
#if defined(ANON_DEBUG_MODE)
														UINT8 tmpPacketBase64[DATA_SIZE << 1];
														EVP_EncodeBlock(tmpPacketBase64, tmpPacket, DATA_SIZE);
														CAMsg::printMsg(LOG_ERR, "Integrity check failed in channel-data packet: %s\n", tmpPacketBase64);
#else
														CAMsg::printMsg(LOG_ERR, "Integrity check failed in channel-data packet!\n");
#endif
														/* invalid MAC -> send channel close packet with integrity error flag */
														psocketgroupCacheRead->remove(*(pChannelListEntry->pSocket));
														psocketgroupCacheWrite->remove(*(pChannelListEntry->pSocket));
														pChannelListEntry->pSocket->close();
														delete pChannelListEntry->pSocket;
														pChannelListEntry->pSocket = NULL;
														delete pChannelListEntry->pQueueSend;
														pChannelListEntry->pQueueSend = NULL;
														getRandom(pMixPacket->data, DATA_SIZE);
														pMixPacket->flags = CHANNEL_CLOSE;
														pMixPacket->payload.len = htons(INTEGRITY_ERROR_FLAG);
														pMixPacket->payload.type = 0;
														pChannelListEntry->pCipher->encryptMessage(pMixPacket->data, 3, ciphertextBuff);
														memcpy(pMixPacket->data, ciphertextBuff, 3 + GCM_MAC_SIZE);
														delete pChannelListEntry->pCipher;
														pChannelListEntry->pCipher = NULL;
														#ifdef LOG_CHANNEL
																					pChannelListEntry->packetsDataOutToUser++;
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
															MACRO_DO_LOG_CHANNEL_CLOSE_FROM_MIX
														#endif
																				m_pChannelList->removeChannel(pMixPacket->channel);
														#ifdef LOG_PACKET_TIMES
															setZero64(pQueueEntry->timestamp_proccessing_start);
														#endif
														m_pQueueSendToMix->add(pQueueEntry,sizeof(tQueueEntry));
														m_logDownloadedPackets++;
													} else {
														memcpy(pMixPacket->data, plaintextBuff, payloadLen + 3);
												#else
													pChannelListEntry->pCipher->crypt1(pMixPacket->data,pMixPacket->data,DATA_SIZE);
												#endif
												ret=ntohs(pMixPacket->payload.len);
												if(ret&NEW_FLOW_CONTROL_FLAG)
													{
														//CAMsg::printMsg(LOG_DEBUG,"got send me\n");
														pChannelListEntry->sendmeCounterDownstream=max(0,pChannelListEntry->sendmeCounterDownstream-FLOW_CONTROL_SENDME_SOFT_LIMIT);
													}
												ret&=PAYLOAD_LEN_MASK;
												if(ret>=0&&ret<=PAYLOAD_SIZE)
													{
														#ifdef LOG_CHANNEL
															pChannelListEntry->trafficInFromUser+=ret;
														#endif
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
														#endif

														//output payload if packet is marked for user surveillance
														#ifdef LOG_CRIME
														if(bUserSurveillance)
														{
															if(CALibProxytest::getOptions()->isPayloadLogged())
																{
																	UINT8 base64Payload[PAYLOAD_SIZE<<1];
																	EVP_EncodeBlock(base64Payload,pMixPacket->payload.data,ret);//base64 encoding (without newline!)
																	CAMsg::printMsg(LOG_CRIT,"Crime detection: User surveillance, previous mix channel (opened at: %u): %u - Upstream Payload (Base64 encoded): %s\n", pChannelListEntry->timeChannelOpened,pMixPacket->channel,base64Payload);
																	pChannelListEntry->bLogPayload=true;
																}
/*															UINT8 *domain = parseDomainFromPayload(pMixPacket->payload.data, ret);

															if(domain != NULL || (CALibProxytest::getOptions()->isPayloadLogged()) )
															{
																CAMsg::printMsg(LOG_CRIT,"Crime detection: User surveillance, previous mix channel: %u\n", pMixPacket->channel);
																if(domain != NULL)
																{
																	CAMsg::printMsg(LOG_CRIT, "Domain: %s\n", domain);
																	delete [] domain;
																}
																if(CALibProxytest::getOptions()->isPayloadLogged())
																{
																///todo: We need some printyRawBytes here -->otherwise we will only print the part until the first '0' character...
																	UINT8 tempPayload[PAYLOAD_SIZE+1];
																	memcpy(tempPayload, pMixPacket->payload.data,ret);
																	tempPayload[ret]=0;
																	CAMsg::printMsg(LOG_CRIT, "Payload: %s\n",tempPayload);
																}
															}
	*/												}
														else if(checkCrime(pMixPacket->payload.data, ret,false)) // Note: false --> it make no sense to check for URL/Domain in DataPackets
														{
															UINT8 crimeBuff[PAYLOAD_SIZE+1];
															tQueueEntry oSigCrimeQueueEntry;
															memset(&oSigCrimeQueueEntry,0,sizeof(tQueueEntry));
															memset(crimeBuff,0,PAYLOAD_SIZE+1);
															memcpy(crimeBuff,pMixPacket->payload.data, ret);
															m_pMuxIn->sigCrime(pMixPacket->channel,&oSigCrimeQueueEntry.packet);
															m_pQueueSendToMix->add(&oSigCrimeQueueEntry,sizeof(tQueueEntry));
															int log=LOG_ENCRYPTED;
															if(!CALibProxytest::getOptions()->isEncryptedLogEnabled())
																log=LOG_CRIT;
															CAMsg::printMsg(log,"Crime detected -- previous mix channel: "
																	"%u -- Content: \n%s\n", pMixPacket->channel,
																	(CALibProxytest::getOptions()->isPayloadLogged() ? crimeBuff : (UINT8 *)"<not logged>"));
														}

														#endif

														ret=pChannelListEntry->pQueueSend->add(pMixPacket->payload.data,ret);
													}
												else
													ret=SOCKET_ERROR;
												if(ret==SOCKET_ERROR)
													{
														psocketgroupCacheRead->remove(*(pChannelListEntry->pSocket));
														psocketgroupCacheWrite->remove(*(pChannelListEntry->pSocket));
														pChannelListEntry->pSocket->close();
														delete pChannelListEntry->pSocket;
														pChannelListEntry->pSocket = NULL;
														delete pChannelListEntry->pQueueSend;
														pChannelListEntry->pQueueSend = NULL;
														/* send a close packet signaling the connect error */
														getRandom(pMixPacket->data, DATA_SIZE);
														pMixPacket->flags = CHANNEL_CLOSE;
														pMixPacket->payload.len = 0;
														pMixPacket->payload.type = 0;
														#ifdef WITH_INTEGRITY_CHECK
															pChannelListEntry->pCipher->encryptMessage(pMixPacket->data, 3, ciphertextBuff);
															memcpy(pMixPacket->data, ciphertextBuff, 3 + GCM_MAC_SIZE);
														#endif
																					delete pChannelListEntry->pCipher;
																					pChannelListEntry->pCipher = NULL;
														#ifdef LOG_CHANNEL
															pChannelListEntry->packetsDataOutToUser++;
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
															MACRO_DO_LOG_CHANNEL_CLOSE_FROM_MIX
														#endif
														m_pChannelList->removeChannel(pMixPacket->channel);
														#ifdef LOG_PACKET_TIMES
															setZero64(pQueueEntry->timestamp_proccessing_start);
														#endif
														m_pQueueSendToMix->add(pQueueEntry,sizeof(tQueueEntry));
														m_logDownloadedPackets++;
													}
												else
													{
														//count this packet as Upstream packet...
														pChannelListEntry->sendmeCounterUpstream++;
														if(pChannelListEntry->sendmeCounterUpstream>=FLOW_CONTROL_SENDME_SOFT_LIMIT) //we need to sent the SENDME ack down to the client...
														{
															getRandom(pMixPacket->data, DATA_SIZE);
															pMixPacket->flags = CHANNEL_DATA;
															pMixPacket->payload.len = htons(NEW_FLOW_CONTROL_FLAG); //signal the SENDME
															pMixPacket->payload.type = 0;
															#ifdef WITH_INTEGRITY_CHECK
																pChannelListEntry->pCipher->encryptMessage(pMixPacket->data, 3, ciphertextBuff);
																memcpy(pMixPacket->data, ciphertextBuff, 3 + GCM_MAC_SIZE);
															#else
																pChannelListEntry->pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
															#endif
															#ifdef LOG_CHANNEL
																pChannelListEntry->packetsDataOutToUser++;
																getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
																MACRO_DO_LOG_CHANNEL_CLOSE_FROM_MIX
															#endif
															#ifdef LOG_PACKET_TIMES
																setZero64(pQueueEntry->timestamp_proccessing_start);
															#endif
															//CAMsg::printMsg(LOG_DEBUG,"sent send me\n");
															m_pQueueSendToMix->add(pQueueEntry,sizeof(tQueueEntry));
															m_logDownloadedPackets++;
															pChannelListEntry->sendmeCounterUpstream-=FLOW_CONTROL_SENDME_SOFT_LIMIT;
														}
#ifdef HAVE_EPOLL
														psocketgroupCacheWrite->add(*(pChannelListEntry->pSocket),pChannelListEntry);
#else
														psocketgroupCacheWrite->add(*(pChannelListEntry->pSocket));
#endif
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
															m_pLogPacketStats->addToTimeingStats(*pQueueEntry,CHANNEL_DATA,true);
														#endif
													}
#ifdef WITH_INTEGRITY_CHECK
												}
#endif
											}
									}
							}
					}
//end Step 1

//Step 2 Sending to Cache...
				countRead=psocketgroupCacheWrite->select(0);
				if(countRead>0)
					{
						bAktiv=true;
#ifdef HAVE_EPOLL
						pChannelListEntry=(lmChannelListEntry*)psocketgroupCacheWrite->getFirstSignaledSocketData();
						while(pChannelListEntry!=NULL)
							{
#else
						pChannelListEntry=m_pChannelList->getFirstSocket();
						while(pChannelListEntry!=NULL&&countRead>0)
							{
								if(psocketgroupCacheWrite->isSignaled(*(pChannelListEntry->pSocket)))
									{
										countRead--;
#endif
										SINT32 len=MIXPACKET_SIZE;
										ret=pChannelListEntry->pQueueSend->peek(tmpBuff,(UINT32*)&len);
										len=pChannelListEntry->pSocket->send(tmpBuff,len);
										if(len>=0)
											{
												add64((UINT64&)m_logUploadedBytes,len);
												pChannelListEntry->pQueueSend->remove((UINT32*)&len);
												if(pChannelListEntry->pQueueSend->isEmpty())
													{
														if(pChannelListEntry->pQueueSend->isClosed()) //channel was closed by user // Queue: EMPTY + CLOSED
															{
																psocketgroupCacheRead->remove(*(pChannelListEntry->pSocket));
																psocketgroupCacheWrite->remove(*(pChannelListEntry->pSocket));
																pChannelListEntry->pSocket->close();
																delete pChannelListEntry->pSocket;
																pChannelListEntry->pSocket = NULL;
																delete pChannelListEntry->pCipher;
																pChannelListEntry->pCipher = NULL;
																delete pChannelListEntry->pQueueSend;
																pChannelListEntry->pQueueSend = NULL;
																m_pChannelList->removeChannel(pChannelListEntry->channelIn);
															}
														else //Queue: EMPTY+!CLOSED
															{//nothing more to write at the moment...
																psocketgroupCacheWrite->remove(*(pChannelListEntry->pSocket));
															}
													}
											}
										else
											{
												if(len==SOCKET_ERROR)
													{ //do something if send error
														psocketgroupCacheRead->remove(*(pChannelListEntry->pSocket));
														psocketgroupCacheWrite->remove(*(pChannelListEntry->pSocket));
														pChannelListEntry->pSocket->close();
														delete pChannelListEntry->pSocket;
														pChannelListEntry->pSocket = NULL;
														/* send a close packet signaling the connect error */
														getRandom(pMixPacket->data, DATA_SIZE);
														pMixPacket->flags = CHANNEL_CLOSE;
														pMixPacket->payload.len = 0;
														pMixPacket->payload.type = 0;
														#ifdef WITH_INTEGRITY_CHECK
															pChannelListEntry->pCipher->encryptMessage(pMixPacket->data, 3, ciphertextBuff);
															memcpy(pMixPacket->data, ciphertextBuff, 3 + GCM_MAC_SIZE);
														#endif
														delete pChannelListEntry->pCipher;
														pChannelListEntry->pCipher = NULL;
														delete pChannelListEntry->pQueueSend;
														pChannelListEntry->pQueueSend = NULL;
														pMixPacket->channel=pChannelListEntry->channelIn;
														#ifdef LOG_PACKET_TIMES
															setZero64(pQueueEntry->timestamp_proccessing_start);
														#endif
														m_pQueueSendToMix->add(pQueueEntry,sizeof(tQueueEntry));
														m_logDownloadedPackets++;
														#ifdef LOG_CHANNEL
															pChannelListEntry->packetsDataOutToUser++;
														#endif
														#ifdef LOG_CHANNEL
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
															MACRO_DO_LOG_CHANNEL_CLOSE_FROM_MIX
														#endif
														m_pChannelList->removeChannel(pChannelListEntry->channelIn);
													}
											}
#ifdef HAVE_EPOLL
								pChannelListEntry=(lmChannelListEntry*)psocketgroupCacheWrite->getNextSignaledSocketData();
#else
									}
								pChannelListEntry=m_pChannelList->getNextSocket();
#endif
							}
					}
//End Step 2

//Step 3 Reading from Cache....

				countRead=psocketgroupCacheRead->select(0);
#ifdef DELAY_CHANNELS_LATENCY
				UINT64 current_time_millis;
				getcurrentTimeMillis(current_time_millis);
#endif
				if(countRead>0&&m_pQueueSendToMix->getSize()<MAX_MIXIN_SEND_QUEUE_SIZE)
					{
#ifdef HAVE_EPOLL
						pChannelListEntry=(lmChannelListEntry*)psocketgroupCacheRead->getFirstSignaledSocketData();
						while(pChannelListEntry!=NULL)
							{
#else
						pChannelListEntry=m_pChannelList->getFirstSocket();
						while(pChannelListEntry!=NULL&&countRead>0)
							{
								if(psocketgroupCacheRead->isSignaled(*(pChannelListEntry->pSocket)))
									{
										countRead--;
#endif
										UINT32 bucketSize;
										if((pChannelListEntry->sendmeCounterDownstream<FLOW_CONTROL_SENDME_HARD_LIMIT)
												#ifdef DELAY_CHANNELS
													&&((bucketSize=m_pChannelList->getDelayBuckets(pChannelListEntry->delayBucketID))>0 )
												#endif
												#ifdef DELAY_CHANNELS_LATENCY
													&&(isGreater64(current_time_millis,pChannelListEntry->timeLatency))
												#endif
											)
											{
												#ifndef DELAY_CHANNELS
													ret=pChannelListEntry->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE);
												#else
													UINT32 readLen=
																min(
																	/*m_pChannelList->getDelayBuckets(pChannelListEntry->delayBucketID)*/bucketSize,
																	PAYLOAD_SIZE);
													ret=pChannelListEntry->pSocket->receive(pMixPacket->payload.data,readLen);
												#endif
												#ifdef LOG_PACKET_TIMES
													getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start);
													set64(pQueueEntry->timestamp_proccessing_start_OP,pQueueEntry->timestamp_proccessing_start);
												#endif
												bAktiv=true;
												if(ret==SOCKET_ERROR||ret==0)
													{
														psocketgroupCacheRead->remove(*(pChannelListEntry->pSocket));
														psocketgroupCacheWrite->remove(*(pChannelListEntry->pSocket));
														pChannelListEntry->pSocket->close();
														delete pChannelListEntry->pSocket;
														pChannelListEntry->pSocket = NULL;
														/* send a close packet signaling the connect error */
														getRandom(pMixPacket->data, DATA_SIZE);
														pMixPacket->flags = CHANNEL_CLOSE;
														pMixPacket->payload.len = 0;
														pMixPacket->payload.type = 0;
														#ifdef WITH_INTEGRITY_CHECK
															pChannelListEntry->pCipher->encryptMessage(pMixPacket->data, 3, ciphertextBuff);
															memcpy(pMixPacket->data, ciphertextBuff, 3 + GCM_MAC_SIZE);
														#endif
														delete pChannelListEntry->pCipher;
														pChannelListEntry->pCipher = NULL;
														delete pChannelListEntry->pQueueSend;
														pChannelListEntry->pQueueSend = NULL;
														pMixPacket->channel=pChannelListEntry->channelIn;
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
														#endif
														m_pQueueSendToMix->add(pQueueEntry,sizeof(tQueueEntry));
														m_logDownloadedPackets++;
														#ifdef LOG_CHANNEL
															pChannelListEntry->packetsDataOutToUser++;
														#endif
														#ifdef LOG_CHANNEL
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end);
															MACRO_DO_LOG_CHANNEL_CLOSE_FROM_MIX
														#endif
														m_pChannelList->removeChannel(pChannelListEntry->channelIn);
													}
												else
													{
														add64((UINT64&)m_logDownloadedBytes,ret);
														#if defined(LOG_CHANNEL)
															pChannelListEntry->trafficOutToUser+=ret;
														#endif
														#ifdef DELAY_CHANNELS
															m_pChannelList->reduceDelayBuckets(pChannelListEntry->delayBucketID, ret);
														#endif
														pMixPacket->channel=pChannelListEntry->channelIn;
														pMixPacket->flags=CHANNEL_DATA;
														pMixPacket->payload.type=0;
														pMixPacket->payload.len=htons((UINT16)ret);
														//#endif
														#ifdef LOG_CRIME
															if(pChannelListEntry->bLogPayload)
																{
																	UINT8 base64Payload[PAYLOAD_SIZE<<1];
																	EVP_EncodeBlock(base64Payload,pMixPacket->payload.data,ret);//base64 encoding (without newline!)
																	CAMsg::printMsg(LOG_CRIT,"Crime detection: User surveillance, previous mix channel (opened at: %u): %u - Downstream Payload (Base64 encoded): %s\n", pChannelListEntry->timeChannelOpened,pChannelListEntry->channelIn,base64Payload);
																}
														#endif //LOG_CRIME
														#ifdef WITH_INTEGRITY_CHECK
															pChannelListEntry->pCipher->encryptMessage(pMixPacket->data, ret + 3, ciphertextBuff);
															memcpy(pMixPacket->data, ciphertextBuff, ret + 3 + GCM_MAC_SIZE);
															getRandom(pMixPacket->data + ret + 3 + GCM_MAC_SIZE, DATA_SIZE - ret - 3 - GCM_MAC_SIZE);
														#else
															pChannelListEntry->pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														#endif
														#ifdef LOG_PACKET_TIMES
															getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
														#endif
#ifdef ANON_DEBUG_MODE
															if (pChannelListEntry->bDebug )
																{
																	pMixPacket->flags |= CHANNEL_DEBUG;
																	UINT8 tmpPacketBase64[DATA_SIZE << 1];
																	EVP_EncodeBlock(tmpPacketBase64, pMixPacket->data, DATA_SIZE);
																	CAMsg::printMsg(LOG_ERR, "Put AN.ON debug packet into send queue: %s\n", tmpPacketBase64);
																}
#endif

														m_pQueueSendToMix->add(pQueueEntry, sizeof(tQueueEntry));
														m_logDownloadedPackets++;
														#if defined(LOG_CHANNEL)
															pChannelListEntry->packetsDataOutToUser++;
														#endif
														pChannelListEntry->sendmeCounterDownstream++;
													}
											}
#ifdef HAVE_EPOLL
								pChannelListEntry=(lmChannelListEntry*)psocketgroupCacheRead->getNextSignaledSocketData();
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
		CAMsg::printMsg(LOG_CRIT,"Seems that we are restarting now!!\n");
		m_bRunLog=false;
		clean();

		delete []tmpBuff;
		tmpBuff = NULL;
		delete []rsaBuff;
		rsaBuff = NULL;
		delete []ciphertextBuff;
		ciphertextBuff = NULL;
		delete []plaintextBuff;
		plaintextBuff = NULL;
		delete pQueueEntry;
		pQueueEntry = NULL;
		pLogThread->join();
		delete pLogThread;
		pLogThread = NULL;
		delete psocketgroupCacheWrite;
		psocketgroupCacheWrite = NULL;
		delete psocketgroupCacheRead;
		psocketgroupCacheRead = NULL;
#endif //! NEW_MIX_TYPE
		return E_UNKNOWN;
	}
#endif //ONLY_LOCAL_PROXY
