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
#include "CAFirstMix.hpp"
#include "CASocketGroup.hpp"
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CAFirstMixChannelList.hpp"
#include "CAASymCipher.hpp"
#include "CAInfoService.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocketAddrUnix.hpp"
#include "CAThread.hpp"
#include "CAUtil.hpp"
#include "CASignature.hpp"
#include "xml/DOM_Output.hpp"

extern CACmdLnOptions options;


SINT32 CAFirstMix::initOnce()
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting FirstMix InitOnce\n");
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

SINT32 CAFirstMix::init()
	{
		m_nMixedPackets=0; //reset to zero after each restart (at the moment neccessary for infoservice)
		//Establishing all Listeners
		m_nSocketsIn=options.getListenerInterfaceCount();
		m_arrSocketsIn=new CASocket[m_nSocketsIn];
		UINT32 i;
		for(i=1;i<=m_nSocketsIn;i++)
			{
				ListenerInterface oListener;
				if(options.getListenerInterface(oListener,i)!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
						return E_UNKNOWN;
					}
				m_arrSocketsIn[i-1].create();
				m_arrSocketsIn[i-1].setReuseAddr(true);
#ifndef _WIN32
        //we have to be a temporaly superuser if port <1024...
				int old_uid=geteuid();
				if(oListener.addr->getType()==AF_INET&&((CASocketAddrINet*)oListener.addr)->getPort()<1024)
					{
						if(seteuid(0)==-1) //changing to root
							CAMsg::printMsg(LOG_CRIT,"Setuid failed!\n");
					}
#endif				
				SINT32 ret=m_arrSocketsIn[i-1].listen(*oListener.addr);
				delete oListener.addr;
#ifndef _WIN32
				seteuid(old_uid);
#endif
				if(ret!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
						return E_UNKNOWN;
					}
			}

		
		CASocketAddr* pAddrNext=NULL;
		for(i=0;i<options.getTargetInterfaceCount();i++)
			{
				TargetInterface oNextMix;
				options.getTargetInterface(oNextMix,i+1);
				if(oNextMix.target_type==TARGET_MIX)
					{
						pAddrNext=oNextMix.addr;
						break;
					}
				delete oNextMix.addr;
			}
		if(pAddrNext==NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"No next Mix specified!\n");
				return E_UNKNOWN;
			}
		m_pMuxOut=new CAMuxSocket();
		if(((CASocket*)(*m_pMuxOut))->create(pAddrNext->getType())!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot create SOCKET for connection to next Mix!\n");
				return E_UNKNOWN;
			}
		((CASocket*)(*m_pMuxOut))->setSendBuff(500*MIXPACKET_SIZE);
		((CASocket*)(*m_pMuxOut))->setRecvBuff(500*MIXPACKET_SIZE);
		if(((CASocket*)(*m_pMuxOut))->setSendLowWat(MIXPACKET_SIZE)!=E_SUCCESS)
			CAMsg::printMsg(LOG_INFO,"SOCKET Option SENDLOWWAT not set!\n");
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET RecvBuffSize: %i\n",((CASocket*)(*m_pMuxOut))->getRecvBuff());
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET SendBuffSize: %i\n",((CASocket*)(*m_pMuxOut))->getSendBuff());
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET SendLowWatSize: %i\n",((CASocket*)(*m_pMuxOut))->getSendLowWat());


		if(m_pMuxOut->connect(*pAddrNext,10,10)!=E_SUCCESS)
			{
				delete pAddrNext;
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix!\n");
				return E_UNKNOWN;
			}
		delete pAddrNext;

		CAMsg::printMsg(LOG_INFO," connected!\n");
		if(((CASocket*)(*m_pMuxOut))->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)(*m_pMuxOut))->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}

		UINT16 len;
		if(((CASocket*)(*m_pMuxOut))->receiveFully((UINT8*)&len,2)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info lenght!\n");
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_CRIT,"Received Key Info lenght %u\n",ntohs(len));
		m_pRSA=new CAASymCipher;
		m_pRSA->generateKeyPair(1024);
		len=ntohs(len);
		UINT8* recvBuff=new UINT8[len+1];

		if(((CASocket*)(*m_pMuxOut))->receiveFully(recvBuff,len)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info!\n");
				delete []recvBuff;
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_CRIT,"Received Key Info...\n");
		recvBuff[len]=0; //get the Key's from the other mixes (and the Mix-Id's...!)
		if(initMixCascadeInfo(recvBuff,len+1)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error in establishing secure communication with next Mix!\n");
				delete []recvBuff;
				return E_UNKNOWN;
			}			
		
    m_pIPList=new CAIPList();
		m_pQueueSendToMix=new CAQueue();
		m_pChannelList=new CAFirstMixChannelList();
		m_psocketgroupUsersRead=new CASocketGroup;
		m_psocketgroupUsersWrite=new CASocketGroup;
		m_pInfoService=new CAInfoService(this);
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix init() succeded\n");
		return E_SUCCESS;
	}

/**How to end this thread:
1. Close connection to next mix
2. put a byte in the Mix-Output-Queue
*/
THREAD_RETURN loopSendToMix(void* param)
	{
		CAQueue* pQueue=((CAFirstMix*)param)->m_pQueueSendToMix;
		CASocket* pSocket=(CASocket *)(*((CAFirstMix*)param)->m_pMuxOut);
		UINT8* buff=new UINT8[0xFFFF];
		UINT32 len;
		for(;;)
			{
				len=0xFFFF;
				pQueue->getOrWait(buff,&len);
				if(pSocket->sendFully(buff,len)!=E_SUCCESS)
					break;
			}
		delete []buff;
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread SendToMix\n");
		THREAD_RETURN_SUCCESS;
	}

/*How to end this thread
1. Set m_bRestart in firstMix to true
2. close all accept sockets
*/
THREAD_RETURN loopAcceptUsers(void* param)
	{
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		CASocket* socketsIn=pFirstMix->m_arrSocketsIn;
		CAIPList* pIPList=pFirstMix->m_pIPList;
//		CAInfoService* pInfoService=pFirstMix->m_pInfoService;
		CAFirstMixChannelList* pChannelList=pFirstMix->m_pChannelList;
		CASocketGroup* psocketgroupUsersRead=pFirstMix->m_psocketgroupUsersRead;
		UINT32 nSocketsIn=pFirstMix->m_nSocketsIn;
		UINT8* pKeyInfoBuff=pFirstMix->m_KeyInfoBuff;
		UINT32 nKeyInfoSize=pFirstMix->m_KeyInfoSize;

		UINT8* pxmlKeyInfoBuff=pFirstMix->m_xmlKeyInfoBuff;
		UINT32 nxmlKeyInfoSize=pFirstMix->m_xmlKeyInfoSize;
		CASocketGroup osocketgroupAccept;
		CAMuxSocket* pNewMuxSocket;
		UINT8* peerIP=new UINT8[4];
		UINT32 i=0;
//		UINT32& nUser=pFirstMix->m_nUser;
		SINT32 countRead;
		SINT32 ret;
		for(i=0;i<nSocketsIn;i++)
			osocketgroupAccept.add(socketsIn[i]);
		while(!pFirstMix->getRestart())
			{
				countRead=osocketgroupAccept.select(false,10000);
				if(countRead<0)
					{ //check for Error - are we restarting ?
						if(pFirstMix->getRestart()||countRead!=E_TIMEDOUT)
							goto END_THREAD;
					}
				i=0;
#ifdef _DEBUG
				CAMsg::printMsg(LOG_DEBUG,"UserAcceptLoop: countRead=%i\n",countRead);
#endif
				while(countRead>0&&i<nSocketsIn)
					{						
						if(osocketgroupAccept.isSignaled(socketsIn[i]))
							{
								countRead--;
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"New direct Connection from Client!\n");
								#endif
								pNewMuxSocket=new CAMuxSocket;
								ret=socketsIn[i].accept(*(CASocket*)pNewMuxSocket);
								if(ret!=E_SUCCESS)
									{
										CAMsg::printMsg(LOG_ERR,"Accept Error %u - direct Connection from Client!\n",GET_NET_ERROR);
										delete pNewMuxSocket;
										if(ret==E_SOCKETCLOSED&&pFirstMix->getRestart()) //Hm, should we restart ??
											goto END_THREAD;
									}
								else
									{
		//Prüfen ob schon vorhanden..	
										ret=((CASocket*)pNewMuxSocket)->getPeerIP(peerIP);
										if(ret!=E_SUCCESS||pIPList->insertIP(peerIP)<0)
											{
												pNewMuxSocket->close();
												delete pNewMuxSocket;
											}
										else
											{
		//Weiter wie bisher...								
												#ifdef _DEBUG
													int ret=((CASocket*)pNewMuxSocket)->setKeepAlive(true);
													if(ret!=E_SUCCESS)
														CAMsg::printMsg(LOG_DEBUG,"Fehler bei KeepAlive!");
												#else
													((CASocket*)pNewMuxSocket)->setKeepAlive(true);
												#endif
#ifdef NEW_KEY2USER_PROTOCOL
												((CASocket*)pNewMuxSocket)->send(pxmlKeyInfoBuff,nxmlKeyInfoSize);
#else
												((CASocket*)pNewMuxSocket)->send(pKeyInfoBuff,nKeyInfoSize);
#endif
												((CASocket*)pNewMuxSocket)->setNonBlocking(true);
												pChannelList->add(pNewMuxSocket,peerIP,new CAQueue);
												pFirstMix->incUsers();
												psocketgroupUsersRead->add(*pNewMuxSocket);
											}
									}
							}
						i++;
					}
			}
END_THREAD:
		delete []peerIP;
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread AcceptUser\n");
		THREAD_RETURN_SUCCESS;
	}

/*
THREAD_RETURN loopReadFromUsers(void* param)
	{
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		CAFirstMixChannelList* pChannelList=pFirstMix->m_pChannelList;
		CASocketGroup* psocketgroupUsersRead=pFirstMix->m_psocketgroupUsersRead;
		CASocketGroup* psocketgroupUsersWrite=pFirstMix->m_psocketgroupUsersWrite;
		CAQueue* pQueueSendToMix=pFirstMix->m_pQueueSendToMix;
//		CAInfoService* pInfoService=pFirstMix->m_pInfoService;
		CAASymCipher* pRSA=pFirstMix->m_pRSA;
		CAIPList* pIPList=pFirstMix->m_pIPList;
		CAMuxSocket* pNextMix=pFirstMix->m_pMuxOut;

		CAMuxSocket* pMuxSocket;
		
		SINT32 countRead;
		SINT32 ret;
		UINT8* ip=new UINT8[4];
		UINT8* tmpBuff=new UINT8[MIXPACKET_SIZE];
		MIXPACKET* pMixPacket=new MIXPACKET;
		UINT8* rsaBuff=new UINT8[RSA_SIZE];

		fmHashTableEntry* pHashEntry;
		fmChannelListEntry* pEntry;
		CASymCipher* pCipher=NULL;
	
		for(;;)
			{
				countRead=psocketgroupUsersRead->select(false,1000); //if we sleep her forever, we will not notice new sockets...
				if(countRead<0)
					{ //check for error
						if(pFirstMix->getRestart()||countRead!=E_TIMEDOUT)
							goto END_THREAD;
					}
				pHashEntry=pChannelList->getFirst();
				while(pHashEntry!=NULL&&countRead>0)
					{
						pMuxSocket=pHashEntry->pMuxSocket;
						if(psocketgroupUsersRead->isSignaled(*pMuxSocket))
							{
								countRead--;
								ret=pMuxSocket->receive(pMixPacket,0);
								if(ret==SOCKET_ERROR)
									{
										((CASocket*)pMuxSocket)->getPeerIP(ip);
										pIPList->removeIP(ip);
										psocketgroupUsersRead->remove(*(CASocket*)pMuxSocket);
										psocketgroupUsersWrite->remove(*(CASocket*)pMuxSocket);
										pEntry=pChannelList->getFirstChannelForSocket(pMuxSocket);
										while(pEntry!=NULL)
											{
												pNextMix->close(pEntry->channelOut,tmpBuff);
												pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
												delete pEntry->pCipher;
												pEntry=pChannelList->getNextChannel(pEntry);
											}
										ASSERT(pHashEntry->pQueueSend!=NULL,"Send queue is NULL");
										delete pHashEntry->pQueueSend;
										pChannelList->remove(pMuxSocket);
										pMuxSocket->close();
										delete pMuxSocket;
										pFirstMix->decUsers();
									}
								else if(ret==MIXPACKET_SIZE)
									{
										if(pMixPacket->flags==CHANNEL_CLOSE)
											{
												pEntry=pChannelList->get(pMuxSocket,pMixPacket->channel);
												if(pEntry!=NULL)
													{
														pNextMix->close(pEntry->channelOut,tmpBuff);
														pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
														delete pEntry->pCipher;
														pChannelList->removeChannel(pMuxSocket,pMixPacket->channel);
													}
												else
													{
														#if defined(_DEBUG) && ! defined(__MIX_TEST)
															CAMsg::printMsg(LOG_DEBUG,"Invalid ID to close from Browser!\n");
														#endif
													}
											}
										else
											{
												pEntry=pChannelList->get(pMuxSocket,pMixPacket->channel);
												if(pEntry!=NULL&&pMixPacket->flags==CHANNEL_DATA)
													{
														pMixPacket->channel=pEntry->channelOut;
														pCipher=pEntry->pCipher;
														pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														pNextMix->send(pMixPacket,tmpBuff);
														pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
														pFirstMix->incMixedPackets();
													}
												else if(pEntry==NULL&&(pMixPacket->flags==CHANNEL_OPEN_OLD||pMixPacket->flags==CHANNEL_OPEN_NEW))
													{
														pCipher= new CASymCipher();
														pRSA->decrypt(pMixPacket->data,rsaBuff);
														pCipher->setKeyAES(rsaBuff);
														pCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																						 pMixPacket->data+RSA_SIZE-KEY_SIZE,
																						 DATA_SIZE-RSA_SIZE);
														memcpy(pMixPacket->data,rsaBuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);

														if(pChannelList->addChannel(pMuxSocket,pMixPacket->channel,pCipher,&pMixPacket->channel)!=E_SUCCESS)
															{//todo --> maybe move up to not make decryption!!
																delete pCipher;
															}
														else
															{
																#if defined(_DEBUG) && !defined(__MIX_TEST)
																	CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",pMixPacket->channel);
																#endif
																pNextMix->send(pMixPacket,tmpBuff);
																pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
																pFirstMix->incMixedPackets();
															}
													}
											}
									}
							}
						pHashEntry=pChannelList->getNext();
					}
			}
END_THREAD:
		delete ip;
		delete tmpBuff;
		delete pMixPacket;
		delete rsaBuff;
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread ReadFromUser\n");
		THREAD_RETURN_SUCCESS;
	}
*/

#define NO_LOOPACCEPTUSER
//#define NO_LOOP_SEND_TO_MIX
SINT32 CAFirstMix::loop()
	{
		CASingleSocketGroup osocketgroupMixOut;
		SINT32 countRead;
		MIXPACKET* pMixPacket=new MIXPACKET;
		m_nUser=0;
		m_bRestart=false;
		SINT32 ret;
		osocketgroupMixOut.add(*m_pMuxOut);
		m_pMuxOut->setCrypt(true);
		
		m_pInfoService->setSignature(m_pSignature);
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix InfoService - Signature set\n");
		m_pInfoService->sendHelo();
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix Helo sended\n");
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
		UINT8 rsaBuff[RSA_SIZE];
#ifdef LOG_CHANNEL
		UINT64 current_time;
		UINT32 diff_time;
#endif
//		CAThread threadReadFromUsers;
//		threadReadFromUsers.setMainLoop(loopReadFromUsers);
//		threadReadFromUsers.start(this);

		//Starting thread for Step 4
#if !defined(_DEBUG)&&!defined(NO_LOOP_SEND_TO_MIX)
		CAThread threadSendToMix;
		threadSendToMix.setMainLoop(loopSendToMix);
		threadSendToMix.start(this);
#else
		UINT8* sendbuff=new UINT8[0xFFFF];
		((CASocket*)m_pMuxOut)->setNonBlocking(true);
#endif
		for(;;)
			{
				bAktiv=false;
//LOOP_START:

//First Step
//Checking for new connections		
// Now in a separat Thread.... (if NOT _DEBUG defined!)

#if defined(_DEBUG) || defined(NO_LOOPACCEPTUSER)				
				
				countRead=osocketgroupAccept.select(false,0);
				i=0;
				if(countRead>0)
					bAktiv=true;
				while(countRead>0&&i<m_nSocketsIn)
					{						
						if(osocketgroupAccept.isSignaled(m_arrSocketsIn[i]))
							{
								countRead--;
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"New direct Connection from Client!\n");
								#endif
								CAMuxSocket* pnewMuxSocket=new CAMuxSocket;
								if(m_arrSocketsIn[i].accept(*(CASocket*)pnewMuxSocket)!=E_SUCCESS)
									{
										CAMsg::printMsg(LOG_ERR,"Accept Error %u - direct Connection from Client!\n",GET_NET_ERROR);
										delete pnewMuxSocket;
									}
								else
									{
		//Prüfen ob schon vorhanden..	
											ret=((CASocket*)pnewMuxSocket)->getPeerIP(peerIP);
											if(ret!=E_SUCCESS||m_pIPList->insertIP(peerIP)<0)
												{
													pnewMuxSocket->close();
													delete pnewMuxSocket;
												}
											else
												{
		//Weiter wie bisher...								
													#ifdef _DEBUG
														int ret=((CASocket*)pnewMuxSocket)->setKeepAlive(true);
														if(ret!=E_SUCCESS)
															CAMsg::printMsg(LOG_DEBUG,"Fehler bei KeepAlive!");
													#else
														((CASocket*)pnewMuxSocket)->setKeepAlive(true);
													#endif
#ifdef NEW_KEY2USER_PROTOCOL
													((CASocket*)pnewMuxSocket)->send(m_xmlKeyInfoBuff,m_xmlKeyInfoSize);
#else
													((CASocket*)pnewMuxSocket)->send(m_KeyInfoBuff,m_KeyInfoSize);
#endif
													((CASocket*)pnewMuxSocket)->setNonBlocking(true);
													m_pChannelList->add(pnewMuxSocket,peerIP,new CAQueue);
													incUsers();
													m_psocketgroupUsersRead->add(*pnewMuxSocket);
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
#define MAX_NEXT_MIX_QUEUE_SIZE 10000000 //10 MByte
				if(m_pQueueSendToMix->getSize()<MAX_NEXT_MIX_QUEUE_SIZE)
					{
						fmHashTableEntry* pHashEntry=m_pChannelList->getFirst();
						countRead=m_psocketgroupUsersRead->select(false,0);
						if(countRead>0)
							bAktiv=true;
						while(pHashEntry!=NULL&&countRead>0)
							{
								CAMuxSocket* pMuxSocket=pHashEntry->pMuxSocket;
								if(m_psocketgroupUsersRead->isSignaled(*pMuxSocket))
									{
										countRead--;
										ret=pMuxSocket->receive(pMixPacket,0);
										if(ret==SOCKET_ERROR/*||pHashEntry->accessUntil<time()*/)
											{
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
														m_pMuxOut->close(pEntry->channelOut,tmpBuff);
														m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
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
										else if(ret==MIXPACKET_SIZE)
											{
												#ifdef NEW_KEY2USER_PROTOCOL
													if(!pMuxSocket->getIsEncrypted())//Encryption is not set yet -> 
																													 //so we assume that this is
																													//the first packet of a connection
																													//which contains the key
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
												#endif
												#ifdef LOG_CHANNEL
													pHashEntry->trafficIn++;
												#endif
												if(pMixPacket->flags==CHANNEL_DUMMY)
													{
														pMuxSocket->send(pMixPacket,tmpBuff);
														pHashEntry->pQueueSend->add(tmpBuff,MIXPACKET_SIZE);
														m_psocketgroupUsersWrite->add(*pMuxSocket);
													}
												else if(pMixPacket->flags==CHANNEL_CLOSE)
													{
														fmChannelListEntry* pEntry;
														pEntry=m_pChannelList->get(pMuxSocket,pMixPacket->channel);
														if(pEntry!=NULL)
															{
																m_pMuxOut->close(pEntry->channelOut,tmpBuff);
																m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
																#ifdef LOG_CHANNEL
																	pEntry->packetsInFromUser++;
																	getcurrentTimeMillis(current_time);
																	diff_time=diff64(current_time,pEntry->timeCreated);
																	CAMsg::printMsg(LOG_DEBUG,"Channel close - Channel: %u, Connection: %Lu - PacketsIn: %u, PacketsOut: %u - ChannelStart: %Lu, ChannelEnd: %Lu, ChannelDuration: %u\n",
																														pEntry->channelIn,pEntry->pHead->id,pEntry->packetsInFromUser,pEntry->packetsOutToUser,pEntry->timeCreated,current_time,diff_time);
																#endif
																delete pEntry->pCipher;
																m_pChannelList->removeChannel(pMuxSocket,pMixPacket->channel);
															}
														#ifdef _DEBUG
														else
															{
																	CAMsg::printMsg(LOG_DEBUG,"Invalid ID to close from Browser!\n");
															}
														#endif
													}
												else
													{
														CASymCipher* pCipher=NULL;
														fmChannelListEntry* pEntry;
														pEntry=m_pChannelList->get(pMuxSocket,pMixPacket->channel);
														if(pEntry!=NULL&&pMixPacket->flags==CHANNEL_DATA)
															{
																pMixPacket->channel=pEntry->channelOut;
																pCipher=pEntry->pCipher;
																pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
																m_pMuxOut->send(pMixPacket,tmpBuff);
																m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
																incMixedPackets();
																#ifdef LOG_CHANNEL
																	pEntry->packetsInFromUser++;
																#endif
															}
														else if(pEntry==NULL&&(pMixPacket->flags==CHANNEL_OPEN_OLD||pMixPacket->flags==CHANNEL_OPEN_NEW))
															{
																pCipher= new CASymCipher();
																m_pRSA->decrypt(pMixPacket->data,rsaBuff);
																pCipher->setKeyAES(rsaBuff);
																pCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																								 pMixPacket->data+RSA_SIZE-KEY_SIZE,
																								 DATA_SIZE-RSA_SIZE);
																memcpy(pMixPacket->data,rsaBuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
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
																		m_pMuxOut->send(pMixPacket,tmpBuff);
																		m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
																		incMixedPackets();
																		#ifdef _DEBUG
																			CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",pMixPacket->channel);
																		#endif
																	}
															}
													}
											}
									}
#ifdef NEW_KEY2USER_PROTOCOL
	NEXT_USER_CONNECTION:
#endif
									pHashEntry=m_pChannelList->getNext();
							}
					}
//Third step
//Sending to next mix

// Now in a separate Thread (see loopSendToMix())

// if not _DEBUG or NO_LOOP_SEND_TO_MIX defined
#if defined (_DEBUG) || defined(NO_LOOP_SEND_TO_MIX)
		SINT32 sendlen=0x0FFFF;
		if(osocketgroupMixOut.select(true,0)==1&&m_pQueueSendToMix->peek(sendbuff,(UINT32*)&sendlen)==E_SUCCESS)
			{
				bAktiv=true;
				sendlen=((CASocket*)m_pMuxOut)->send(sendbuff,sendlen);
				if(sendlen>0)
					m_pQueueSendToMix->remove((UINT32*)&sendlen);
			}
#endif
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
						if(pMixPacket->flags==CHANNEL_CLOSE) //close event
							{
								#if defined(_DEBUG) && !defined(__MIX_TEST)
									CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ... ",pMixPacket->channel);
								#endif
								fmChannelList* pEntry=m_pChannelList->get(pMixPacket->channel);
								if(pEntry!=NULL)
									{
										pEntry->pHead->pMuxSocket->close(pEntry->channelIn,tmpBuff);
										pEntry->pHead->pQueueSend->add(tmpBuff,MIXPACKET_SIZE);
										#ifdef LOG_CHANNEL
											pEntry->pHead->trafficOut++;
											pEntry->packetsOutToUser++;
											getcurrentTimeMillis(current_time);
											diff_time=diff64(current_time,pEntry->timeCreated);
											CAMsg::printMsg(LOG_DEBUG,"Channel close - Channel: %u, Connection: %Lu - PacketsIn: %u, PacketsOut: %u - ChannelStart: %Lu, ChannelEnd: %Lu, ChannelDuration: %u\n",
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
										pMixPacket->channel=pEntry->channelIn;
										pEntry->pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										
										pEntry->pHead->pMuxSocket->send(pMixPacket,tmpBuff);
										pEntry->pHead->pQueueSend->add(tmpBuff,MIXPACKET_SIZE);
										#ifdef LOG_CHANNEL
											pEntry->pHead->trafficOut++;
											pEntry->packetsOutToUser++;
										#endif
										m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket);
#define MAX_USER_SEND_QUEUE 100000
										if(pEntry->pHead->pQueueSend->getSize()>MAX_USER_SEND_QUEUE&&
												!pEntry->bIsSuspended)
											{
												pMixPacket->channel=pEntry->channelOut;
												pMixPacket->flags=CHANNEL_SUSPEND;
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_INFO,"Sending suspend for channel: %u\n",pMixPacket->channel);
												#endif												
												m_pMuxOut->send(pMixPacket,tmpBuff);
												m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
												
												pEntry->bIsSuspended=true;
												pEntry->pHead->cSuspend++;
											}
										incMixedPackets();
									}
								else
									{
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- Channel-Id %u no valid!\n",pMixPacket->channel);
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
								pfmHashEntry->pQueueSend->peek(tmpBuff,&len);
								ret=((CASocket*)pfmHashEntry->pMuxSocket)->send(tmpBuff,len);
								if(ret>0)
									{
										pfmHashEntry->pQueueSend->remove((UINT32*)&ret);
#define USER_SEND_BUFFER_RESUME 10000
										if(pfmHashEntry->cSuspend>0&&
												pfmHashEntry->pQueueSend->getSize()<USER_SEND_BUFFER_RESUME)
											{
												fmChannelListEntry* pEntry;
												pEntry=m_pChannelList->getFirstChannelForSocket(pfmHashEntry->pMuxSocket);
												while(pEntry!=NULL)
													{
														if(pEntry->bIsSuspended)
															{
																pMixPacket->flags=CHANNEL_RESUME;
																pMixPacket->channel=pEntry->channelOut;
																m_pMuxOut->send(pMixPacket,tmpBuff);
																m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
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
				if(!bAktiv)
				  msSleep(100);
			}
ERR:
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
#if !defined(_DEBUG) && !defined(NO_LOOP_SEND_TO_MIX)
		CAMsg::printMsg(LOG_CRIT,"Wait for LoopSendToMix!\n");
		threadSendToMix.join(); //will not join if queue is empty (and so wating)!!!
#else
		delete sendbuff;
#endif
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
		CAMsg::printMsg(LOG_CRIT,"Main Loop exited!!\n");
		return E_UNKNOWN;
	}






SINT32 CAFirstMix::clean()
	{
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::clean() start\n");
		#endif
		if(m_pInfoService!=NULL)
			delete m_pInfoService;
		m_pInfoService=NULL;
		if(m_arrSocketsIn!=NULL)
			delete[] m_arrSocketsIn;
		m_arrSocketsIn=NULL;
		if(m_pMuxOut!=NULL)
			{
				m_pMuxOut->close();
				delete m_pMuxOut;
			}
		m_pMuxOut=NULL;
		if(m_pIPList!=NULL)
			delete m_pIPList;
		m_pIPList=NULL;
		if(m_pQueueSendToMix!=NULL)
			delete m_pQueueSendToMix;
		m_pQueueSendToMix=NULL;
		if(m_pChannelList!=NULL)
			delete m_pChannelList;
		m_pChannelList=NULL;
		if(m_psocketgroupUsersRead!=NULL)
			delete m_psocketgroupUsersRead;
		m_psocketgroupUsersRead=NULL;
		if(m_psocketgroupUsersWrite!=NULL)
			delete m_psocketgroupUsersWrite;
		m_psocketgroupUsersWrite=NULL;
		if(m_pRSA!=NULL)
			delete m_pRSA;
		m_pRSA=NULL;
		if(m_KeyInfoBuff!=NULL)
			delete []m_KeyInfoBuff;
		m_KeyInfoBuff=NULL;
		if(m_xmlKeyInfoBuff!=NULL)
			delete []m_xmlKeyInfoBuff;
		m_xmlKeyInfoBuff=NULL;
		if(m_strXmlMixCascadeInfo!=NULL)
			delete []m_strXmlMixCascadeInfo;
		m_strXmlMixCascadeInfo=NULL;
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::clean() finished\n");
		#endif
		return E_SUCCESS;
	}
/*
 struct internal_mix_handler_data_struct
	{
		UINT32 mixcount;
		UINT32 aktMix;
		UINT8* cascadeInfo;
		BufferInputStream* input;
		CAASymCipher* m_arRSA; 
	};
	
typedef struct internal_mix_handler_data_struct INTERNAL_MIX_HANDLER_STRUCT;

static void sMixHandler(XMLElement &elem, void *userData)
	{
		INTERNAL_MIX_HANDLER_STRUCT* data=(INTERNAL_MIX_HANDLER_STRUCT*)userData;
		UINT32 aktPos=elem.GetInput().GetOffset();
		UINT32 size=data->input->getSize()-aktPos;
		data->m_arRSA[data->aktMix++].setPublicKeyAsXML(data->input->getBuffer()+aktPos,size);
		XMLAttribute attrs=elem.GetAttrList();
		while(strcmp(attrs.GetName(),"id")!=0)
			attrs=attrs.GetNext();
		strcat((char*)data->cascadeInfo,"<Mix id=\"");
		strcat((char*)data->cascadeInfo,attrs.GetValue());
		strcat((char*)data->cascadeInfo,"\"></Mix>");
	}

static void sMixesHandler(XMLElement &elem, void *userData)
	{	
		INTERNAL_MIX_HANDLER_STRUCT* data=(INTERNAL_MIX_HANDLER_STRUCT*)userData;
		// set up initial handler for RSA-Key
		XMLHandler handlers[] = 
			{
				XMLHandler("Mix",sMixHandler),
				XMLHandler::END
			};
		XMLAttribute attrs=elem.GetAttrList();
		while(strcmp(attrs.GetName(),"count")!=0)
			{
				attrs=attrs.GetNext();
			}
		data->mixcount=atol(attrs.GetValue());
		data->m_arRSA=new CAASymCipher[data->mixcount];
		data->aktMix=0;
		sprintf((char*)data->cascadeInfo,"<Mixes count=\"%u\">",data->mixcount+1);
		UINT32 tmpBuffLen=5000;
		UINT8 tmpBuff[5000];
		options.getMixXml(tmpBuff,&tmpBuffLen);
		tmpBuff[tmpBuffLen]=0; //make a string
		//options.getMixId(tmpBuff,50);
		//sprintf((char*)data->cascadeInfo+strlen((char*)data->cascadeInfo),"<Mix id=\"%s\"></Mix>",tmpBuff);
		char* startpos=strstr((char*)tmpBuff,"<Mix");
		if(startpos!=NULL)
			{
				strcpy((char*)data->cascadeInfo+strlen((char*)data->cascadeInfo),startpos);
			}
		else
			{
				options.getMixId(tmpBuff,50);
				sprintf((char*)data->cascadeInfo+strlen((char*)data->cascadeInfo),"<Mix id=\"%s\"></Mix>",tmpBuff);
			}
		elem.Process(handlers,userData);
		strcat((char*)data->cascadeInfo,"</Mixes>");
	}
*/
/** This will initialize the XML Cascade Info send to the InfoService and
  * the Key struct which is send to each user which connects
	* This function is called from init()
	*/
SINT32 CAFirstMix::initMixCascadeInfo(UINT8* recvBuff,UINT32 len)
	{ 
		//Parse the input, which is the XML send from the previos mix, containing keys and id's
		if(recvBuff==NULL||len==0)
			return E_UNKNOWN;

		CAMsg::printMsg(LOG_DEBUG,"Get KeyInfo (foolowing line)\n");
		CAMsg::printMsg(LOG_DEBUG,"%s\n",recvBuff);

		
		DOMParser oParser;
		MemBufInputSource oInput(recvBuff,len,"tmp");
		oParser.parse(oInput);
		DOM_Document doc=oParser.getDocument();
		DOM_Element elemMixes=doc.getDocumentElement();
		if(elemMixes==NULL)
			return E_UNKNOWN;
		int count=0;
		if(getDOMElementAttribute(elemMixes,"count",&count)!=E_SUCCESS)
			return E_UNKNOWN;
		
		DOM_Node child=elemMixes.getLastChild();
		
		m_KeyInfoBuff=new UINT8[(count+1)*256];
		m_KeyInfoSize=3;
		
		//tmp XML-Structure for constructing the XML which is send to each user
		DOM_Document docXmlKeyInfo=DOM_Document::createDocument();
		DOM_Element elemRootKey=docXmlKeyInfo.createElement("MixCascade");
		setDOMElementAttribute(elemRootKey,"version",(UINT8*)"0.1"); //set the Version of the XML to 0.1
		docXmlKeyInfo.appendChild(elemRootKey);
		DOM_Element elemMixProtocolVersion=docXmlKeyInfo.createElement("MixProtocolVersion");
		setDOMElementValue(elemMixProtocolVersion,(UINT8*)"0.2");
		elemRootKey.appendChild(elemMixProtocolVersion);
		DOM_Node elemMixesKey=docXmlKeyInfo.importNode(elemMixes,true);
		elemRootKey.appendChild(elemMixesKey);
		
		UINT32 tlen;
		while(child!=NULL)
			{
				if(child.getNodeName().equals("Mix"))
					{
						DOM_Node rsaKey=child.getFirstChild();
						CAASymCipher oRSA;
						oRSA.setPublicKeyAsDOMNode(rsaKey);
						tlen=256;
						oRSA.getPublicKey(m_KeyInfoBuff+m_KeyInfoSize,&tlen);
						m_KeyInfoSize+=tlen;
					}
				child=child.getPreviousSibling();
			}
		tlen=256;
		m_pRSA->getPublicKey(m_KeyInfoBuff+m_KeyInfoSize,&tlen);
		m_KeyInfoSize+=tlen;

		UINT16 tmp=htons(m_KeyInfoSize-2);
		memcpy(m_KeyInfoBuff,&tmp,2);
		m_KeyInfoBuff[2]=count+1;

		//Inserting own Key in XML-Key struct
		DOM_DocumentFragment docfragKey;
		m_pRSA->getPublicKeyAsDocumentFragment(docfragKey);
		DOM_Element elemOwnMix=docXmlKeyInfo.createElement("Mix");
		UINT8 buffId[255];
		options.getMixId(buffId,255);
		elemOwnMix.setAttribute("id",DOMString((char*)buffId));
		elemOwnMix.appendChild(docXmlKeyInfo.importNode(docfragKey,true));
		elemMixesKey.insertBefore(elemOwnMix,elemMixesKey.getFirstChild());
		setDOMElementAttribute((DOM_Element&)elemMixesKey,"count",count+1);
		tlen=0;
		UINT8* tmpB=DOM_Output::dumpToMem(docXmlKeyInfo,&tlen);
		m_xmlKeyInfoBuff=new UINT8[tlen+2];
		memcpy(m_xmlKeyInfoBuff+2,tmpB,tlen);
		UINT16 s=htons(tlen);
		memcpy(	m_xmlKeyInfoBuff,&s,2);
		m_xmlKeyInfoSize=tlen+2;
		delete []tmpB;

		//Sending symetric key...
		child=elemMixes.getFirstChild();
		while(child!=NULL)
			{
				if(child.getNodeName().equals("Mix"))
					{
						//check Signature....
						CAMsg::printMsg(LOG_DEBUG,"Try to verify next mix signature...");
						CASignature oSig;
						CACertificate* nextCert=options.getNextMixTestCertificate();
						oSig.setVerifyKey(nextCert);
						SINT32 ret=oSig.verifyXML(child,NULL);
						delete nextCert;
						if(ret!=E_SUCCESS)
							{
								CAMsg::printMsg(LOG_DEBUG,"failed!\n");
									return E_UNKNOWN;
							}
						CAMsg::printMsg(LOG_DEBUG,"success!\n");
						DOM_Node rsaKey=child.getFirstChild();
						CAASymCipher oRSA;
						oRSA.setPublicKeyAsDOMNode(rsaKey);
						UINT8 key[16];
						getRandom(key,16);
						UINT8 buff[400];
						UINT32 bufflen=400;
						encodeXMLEncryptedKey(key,16,buff,&bufflen,&oRSA);
						UINT32 outlen=bufflen+5000;
						UINT8* out=new UINT8[outlen];
						m_pSignature->signXML(buff,bufflen,out,&outlen);
						m_pMuxOut->setKey(key,16);
						UINT16 size=htons(outlen);
						((CASocket*)m_pMuxOut)->send((UINT8*)&size,2);
						((CASocket*)m_pMuxOut)->send(out,outlen);
						delete[] out;
						break;
					}
				child=child.getNextSibling();
			}
		
		UINT8* tmpBuff=new UINT8[2048];
		tlen=2048;
		options.getMixXml(tmpBuff,&tlen);
		MemBufInputSource oInput1(tmpBuff,tlen,"tmp1");
		oParser.parse(oInput1);
		DOM_Document docMix=oParser.getDocument();
		DOM_Node nodeMix=doc.importNode(docMix.getFirstChild(),true);
		elemMixes.insertBefore(nodeMix,elemMixes.getFirstChild());
		setDOMElementAttribute(elemMixes,"count",count+1);
		delete tmpBuff;

	//CascadeInfo		
		DOM_Document docCascade=DOM_Document::createDocument();
		DOM_Element elemRoot=docCascade.createElement("MixCascade");


		UINT8 id[50];
		ListenerInterface oListener;
//		CASocketAddrINet* pAddr;
		
		//if(options.getListenerInterface(oListener,1)!=E_SUCCESS||oListener.addr->getType()!=AF_INET)
		//	return E_UNKNOWN;
		//pAddr=(CASocketAddrINet*)oListener.addr;
		
		options.getMixId(id,50);
		elemRoot.setAttribute(DOMString("id"),DOMString((char*)id));
		
		UINT8 name[255];
		if(options.getCascadeName(name,255)!=E_SUCCESS)
			{
				return E_UNKNOWN;
			}
		docCascade.appendChild(elemRoot);
		DOM_Element elem=docCascade.createElement("Name");
		DOM_Text text=docCascade.createTextNode(DOMString((char*)name));
		elem.appendChild(text);
		elemRoot.appendChild(elem);
		
		UINT8 hostname[255];
		UINT8 ip[255];
	/*	if(oListener.hostname!=NULL)
			strcpy((char*)hostname,(char*)oListener.hostname);
		else if(pAddr->getIPAsStr(hostname,255)!=E_SUCCESS)
			return E_UNKNOWN;
		elem=docCascade.createElement("Host");
		text=docCascade.createTextNode(DOMString((char*)hostname));
		elem.appendChild(text);
		elemRoot.appendChild(elem);
		
		UINT8 ip[255];
		pAddr->getIPAsStr(ip,255);
		elem=docCascade.createElement("IP");
		text=docCascade.createTextNode(DOMString((char*)ip));
		elem.appendChild(text);
		elemRoot.appendChild(elem);
		
		DOM_Element e=docCascade.createElement("Port");
		elemRoot.appendChild(e);
		setDOMElementValue(e,pAddr->getPort());

		if(options.getListenerInterfaceCount()>1)
			{
				elem=docCascade.createElement(DOMString("ProxyPort"));
				setDOMElementValue(elem,443);
				elemRoot.appendChild(elem);
			}
 */
		elem=docCascade.createElement("Network");
		elemRoot.appendChild(elem);
		DOM_Element elemListenerInterfaces=docCascade.createElement("ListenerInterfaces");
		elem.appendChild(elemListenerInterfaces);
		
		for(UINT32 i=1;i<=options.getListenerInterfaceCount();i++)
			{
				options.getListenerInterface(oListener,i);
				if(oListener.type==RAW_TCP)
					{
						DOM_Element elemListenerInterface=docCascade.createElement("ListenerInterface");
						elemListenerInterfaces.appendChild(elemListenerInterface);
						elem=docCascade.createElement("Type");
						elemListenerInterface.appendChild(elem);
						setDOMElementValue(elem,(UINT8*)"RAW/TCP");
						elem=docCascade.createElement("Port");
						elemListenerInterface.appendChild(elem);
						setDOMElementValue(elem,((CASocketAddrINet*)oListener.addr)->getPort());
						if(oListener.hostname!=NULL)
							strcpy((char*)hostname,(char*)oListener.hostname);
						else 
							((CASocketAddrINet*)oListener.addr)->getIPAsStr(hostname,255);
						elem=docCascade.createElement("Host");
						elemListenerInterface.appendChild(elem);
						setDOMElementValue(elem,hostname);
						elem=docCascade.createElement("IP");
						elemListenerInterface.appendChild(elem);
						((CASocketAddrINet*)oListener.addr)->getIPAsStr(ip,255);
						setDOMElementValue(elem,ip);
					}
				delete oListener.addr;
			}
		
		elemRoot.appendChild(docCascade.importNode(elemMixes,true));

		
		tmpB=DOM_Output::dumpToMem(docCascade,&tlen);
		m_strXmlMixCascadeInfo=new UINT8[tlen+1];
		memcpy(m_strXmlMixCascadeInfo,tmpB,tlen);
		m_strXmlMixCascadeInfo[tlen]=0;
		delete[]tmpB;
		return E_SUCCESS;
	}

SINT32 CAFirstMix::getMixCascadeInfo(UINT8* buff,UINT32*len)
	{
		UINT32 xmlMixCascadeInfoLen=strlen((char*)m_strXmlMixCascadeInfo);
		if(buff==NULL||len==NULL||*len<xmlMixCascadeInfoLen)
			return E_UNKNOWN;
		
		memcpy(buff,m_strXmlMixCascadeInfo,xmlMixCascadeInfoLen);
		*len=xmlMixCascadeInfoLen;
		return E_SUCCESS;
	}
