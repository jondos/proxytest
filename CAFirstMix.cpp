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
#include "CABase64.hpp"
#include "xml/DOM_Output.hpp"
#include "CAPool.hpp"

extern CACmdLnOptions options;


SINT32 CAFirstMix::initOnce()
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting FirstMix InitOnce\n");
		m_pSignature=options.getSignKey();
		if(m_pSignature==NULL)
			return E_UNKNOWN;
		//Try to find out how many (real) ListenerInterfaces are specified
		UINT32 tmpSocketsIn=options.getListenerInterfaceCount();
		m_nSocketsIn=0;
		for(UINT32 i=1;i<=tmpSocketsIn;i++)
			{
				CAListenerInterface* pListener=NULL;
				pListener=options.getListenerInterface(i);
				if(pListener==NULL)
					continue;
				if(!pListener->isVirtual())
					m_nSocketsIn++;
				delete pListener;
			}
		if(m_nSocketsIn<1)
			{
				CAMsg::printMsg(LOG_CRIT,"No useable ListenerInterfaces specified (maybe wrong values or all are 'virtual'!\n");
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

SINT32 CAFirstMix::init()
	{
		m_nMixedPackets=0; //reset to zero after each restart (at the moment neccessary for infoservice)
		m_bRestart=false;
		//Establishing all Listeners
		m_arrSocketsIn=new CASocket[m_nSocketsIn];
		UINT32 i,aktSocket=0;
		for(i=1;i<=options.getListenerInterfaceCount();i++)
			{
				CAListenerInterface* pListener=NULL;
				pListener=options.getListenerInterface(i);
				if(pListener==NULL)
					{
						CAMsg::printMsg(LOG_CRIT,"Cannot listen (1)\n");
						return E_UNKNOWN;
					}
				if(pListener->isVirtual())
					{
						delete pListener;
						continue;
					}
				m_arrSocketsIn[aktSocket].create();
				m_arrSocketsIn[aktSocket].setReuseAddr(true);
				CASocketAddr* pAddr=pListener->getAddr();
				delete pListener;
#ifndef _WIN32
				//we have to be a temporaly superuser if port <1024...
				int old_uid=geteuid();
				if(pAddr->getType()==AF_INET&&((CASocketAddrINet*)pAddr)->getPort()<1024)
					{
						if(seteuid(0)==-1) //changing to root
							CAMsg::printMsg(LOG_CRIT,"Setuid failed!\n");
					}
#endif				
				SINT32 ret=m_arrSocketsIn[aktSocket].listen(*pAddr);
				delete pAddr;
#ifndef _WIN32
				seteuid(old_uid);
#endif
				if(ret!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"Cannot listen (2)\n");
						return E_UNKNOWN;
					}
				aktSocket++;
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
		
#ifdef PAYMENT
		m_pAccountingInstance = CAAccountingInstance::getInstance();
#endif

		m_pIPList=new CAIPList();
#ifdef LOG_PACKET_TIMES
		m_pQueueSendToMix=new CATimedQueue();
#else		
		m_pQueueSendToMix=new CAQueue(MIXPACKET_SIZE);
#endif		
		m_pQueueReadFromMix=new CAQueue(MIXPACKET_SIZE);
		m_pChannelList=new CAFirstMixChannelList();
		m_psocketgroupUsersRead=new CASocketGroup;
		m_psocketgroupUsersWrite=new CASocketGroup;
		m_pInfoService=new CAInfoService(this);

		m_pthreadsLogin=new CAThreadPool(NUM_LOGIN_WORKER_TRHEADS,MAX_LOGIN_QUEUE,false);

		//Starting thread for Step 1
		m_pthreadAcceptUsers=new CAThread();
		m_pthreadAcceptUsers->setMainLoop(fm_loopAcceptUsers);
		m_pthreadAcceptUsers->start(this);

		//Starting thread for Step 3
		m_pthreadSendToMix=new CAThread();
		m_pthreadSendToMix->setMainLoop(fm_loopSendToMix);
		m_pthreadSendToMix->start(this);

		//Startting thread for Step 4a
		m_pthreadReadFromMix=new CAThread();
		m_pthreadReadFromMix->setMainLoop(fm_loopReadFromMix);
		m_pthreadReadFromMix->start(this);
		
		//Starting InfoService
		CACertificate* tmp=options.getOwnCertificate();
		m_pInfoService->setSignature(m_pSignature,tmp);
		delete tmp;
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix InfoService - Signature set\n");
		m_pInfoService->start();
		CAMsg::printMsg(LOG_DEBUG,"InfoService Loop started\n");

		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix init() succeded\n");
		return E_SUCCESS;
	}

/**How to end this thread:
0. set bRestart=true;
1. Close connection to next mix
2. put a byte in the Mix-Output-Queue
*/
THREAD_RETURN fm_loopSendToMix(void* param)
	{
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
#ifdef LOG_PACKET_TIMES
		CATimedQueue* pQueue=pFirstMix->m_pQueueSendToMix;
#else	
		CAQueue* pQueue=((CAFirstMix*)param)->m_pQueueSendToMix;
#endif		
		CAMuxSocket* pMuxSocket=pFirstMix->m_pMuxOut;
		
		UINT32 len;
		SINT32 ret;
#ifdef LOG_PACKET_TIMES
		UINT64 timestamp;
		UINT64 tmpU64;
		UINT64 pool_timestamp;
#endif		
#ifndef USE_POOL
		UINT8* buff=new UINT8[0xFFFF];
		while(!pFirstMix->m_bRestart)
			{
				len=MIXPACKET_SIZE;
#ifdef LOG_PACKET_TIMES
				ret=pQueue->getOrWait(buff,&len,timestamp);
#else				
				ret=pQueue->getOrWait(buff,&len);
#endif				
				if(ret!=E_SUCCESS||len!=MIXPACKET_SIZE)
					break;
				if(pMuxSocket->send((MIXPACKET*)buff)!=MIXPACKET_SIZE)
					break;
#ifdef LOG_PACKET_TIMES
 					{
						if(!isZero64(timestamp))
							{
								getcurrentTimeMillis(tmpU64);
								CAMsg::printMsg(LOG_CRIT,"Upload Packet processing time (arrival <--> send): %u ms\n",diff64(tmpU64,timestamp));
							}	
					}
#endif					
			}
		delete []buff;
#else
		CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		tPoolEntry* pPoolEntry=new tPoolEntry;
		while(!pFirstMix->m_bRestart)
			{
				len=MIXPACKET_SIZE;
				#ifdef LOG_PACKET_TIMES
					ret=pQueue->getOrWait((UINT8*)&pPoolEntry->mixpacket,&len,pPoolEntry->overall_timestamp,MIX_POOL_TIMEOUT);
				#else
					ret=pQueue->getOrWait((UINT8*)&pPoolEntry->mixpacket,&len,MIX_POOL_TIMEOUT);
				#endif
				if(ret==E_TIMEDOUT)
					{
						pPoolEntry->mixpacket.flags=0;
						pPoolEntry->mixpacket.channel=DUMMY_CHANNEL;
						getRandom(pPoolEntry->mixpacket.data,DATA_SIZE);
						#ifdef LOG_PACKET_TIMES
							setZero64(pPoolEntry->overall_timestamp);
							setZero64(pPoolEntry->pool_timestamp);
						#endif	
					}
				else if(ret!=E_SUCCESS||len!=MIXPACKET_SIZE)
					break;
				#ifdef LOG_PACKET_TIMES
					if(ret==E_SUCCESS)
						{
							getcurrentTimeMillis(pPoolEntry->pool_timestamp);
						}
				#endif		
				pPool->pool(pPoolEntry);
				#ifdef LOG_PACKET_TIMES
					getcurrentTimeMillis(pool_timestamp);
				#endif
				if(pMuxSocket->send((MIXPACKET*)pPoolEntry)!=MIXPACKET_SIZE)
					break;
				#ifdef LOG_PACKET_TIMES
					if(!isZero64(pPoolEntry->overall_timestamp))
						{
							getcurrentTimeMillis(tmpU64);
							CAMsg::printMsg(LOG_CRIT,"Upload Packet processing time (arrival <--> send): %u ms [Pool Time: %u ms]\n",
																				diff64(tmpU64,pPoolEntry->overall_timestamp),
																				diff64(pool_timestamp,pPoolEntry->pool_timestamp));
						}	
				#endif	
			}
		delete pPoolEntry;
		delete pPool;
#endif
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread SendToMix\n");
		THREAD_RETURN_SUCCESS;
	}

/* How to end this thread:
	0. set bRestart=true
*/
#define MAX_READ_FROM_NEXT_MIX_QUEUE_SIZE 10000000 //How many bytes could be in the incoming queue ??
THREAD_RETURN fm_loopReadFromMix(void* pParam)
	{
		CAFirstMix* pFirstMix=(CAFirstMix*)pParam;
		CAMuxSocket* pMuxSocket=pFirstMix->m_pMuxOut;
		CAQueue* pQueue=pFirstMix->m_pQueueReadFromMix;
		MIXPACKET* pMixPacket=new MIXPACKET;
		CASingleSocketGroup* pSocketGroup=new CASingleSocketGroup();
		pSocketGroup->add(*pMuxSocket);
		#ifdef USE_POOL
			CAPool* pPool=new CAPool(MIX_POOL_SIZE);
		#endif
		
		while(!pFirstMix->m_bRestart)
			{
				if(pQueue->getSize()>MAX_READ_FROM_NEXT_MIX_QUEUE_SIZE)
					{
						msSleep(200);
						continue;
					}
				SINT32 ret=pSocketGroup->select(false,MIX_POOL_TIMEOUT);	
				if(ret==E_TIMEDOUT)
					{
						#ifdef USE_POOL
							pMixPacket->flags=0;
							pMixPacket->channel=DUMMY_CHANNEL;
							getRandom(pMixPacket->data,DATA_SIZE);
							ret=MIXPACKET_SIZE;
						#else
							continue;	
						#endif	
					}
				else if(ret>0)
					ret=pMuxSocket->receive(pMixPacket);
				if(ret!=MIXPACKET_SIZE)
					{
						pFirstMix->m_bRestart=true;
						break;
					}
				#ifdef USE_POOL
					pPool->pool((tPoolEntry*)pMixPacket);
				#endif		
				pQueue->add(pMixPacket,MIXPACKET_SIZE);	
			}
		delete pMixPacket;
		#ifdef USE_POOL
			delete pPool;
		#endif			
	}
	
struct T_UserLoginData
	{
		CAMuxSocket* pNewUser;
		CAFirstMix* pMix;
		UINT8 peerIP[4];
	};

typedef struct T_UserLoginData t_UserLoginData;

/*How to end this thread
1. Set m_bRestart in firstMix to true
2. close all accept sockets
*/
THREAD_RETURN fm_loopAcceptUsers(void* param)
	{
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		CASocket* socketsIn=pFirstMix->m_arrSocketsIn;
		CAIPList* pIPList=pFirstMix->m_pIPList;
		CAThreadPool* pthreadsLogin=pFirstMix->m_pthreadsLogin;
		UINT32 nSocketsIn=pFirstMix->m_nSocketsIn;
		CASocketGroup osocketgroupAccept;
		CAMuxSocket* pNewMuxSocket;
		UINT8* peerIP=new UINT8[4];
		UINT32 i=0;
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
										//Pr�fen ob schon vorhanden..	
										ret=((CASocket*)pNewMuxSocket)->getPeerIP(peerIP);
										#ifdef PAYMENT
											if(ret!=E_SUCCESS||pIPList->insertIP(peerIP)<0 ||
												pFirstMix->m_pAccountingInstance->isIPAddressBlocked(peerIP))
										#else
											if(ret!=E_SUCCESS||pIPList->insertIP(peerIP)<0)
										#endif
											{
												delete pNewMuxSocket;
											}
										else
											{
												t_UserLoginData* d=new t_UserLoginData;
												d->pNewUser=pNewMuxSocket;
												d->pMix=pFirstMix;
												memcpy(d->peerIP,peerIP,4);
												pthreadsLogin->addRequest(fm_loopDoUserLogin,d);
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

THREAD_RETURN fm_loopDoUserLogin(void* param)
	{
		t_UserLoginData* d=(t_UserLoginData*)param;
		d->pMix->doUserLogin(d->pNewUser,d->peerIP);
		delete d;
		THREAD_RETURN_SUCCESS;
	}
	
/** Sends and receives all data neccessary for a User to "login".
This is ending the public key of the Mixes and receiving the 
sym keys of JAP. This is done in a thread on a per user basis
@todo Cleanup of runing thread if mix restarts...
*/
SINT32 CAFirstMix::doUserLogin(CAMuxSocket* pNewUser,UINT8 peerIP[4])
	{
		#ifdef _DEBUG
			int ret=((CASocket*)pNewUser)->setKeepAlive(true);
			if(ret!=E_SUCCESS)
				CAMsg::printMsg(LOG_DEBUG,"Error setting KeepAlive!");
		#else
			((CASocket*)pNewUser)->setKeepAlive(true);
		#endif
		/*
			ADDITIONAL PREREQUISITE:
			The timestamps in the messages require the user to sync his time
			with the time of the cascade. Hence, the current time needs to be
			added to the data that is sent to the user below.
			For the mixes that form the cascade, the synchronization can be
			left to an external protocol such as NTP. Unfortunately, this is
			not enforceable for all users.
		*/
		((CASocket*)pNewUser)->send(m_xmlKeyInfoBuff,m_xmlKeyInfoSize);  // send the mix-keys to JAP
		((CASocket*)pNewUser)->setNonBlocking(true);	                    // stefan: sendet das send in der letzten zeile doch noch nicht? wenn doch, kann dann ein JAP nicht durch verweigern der annahme hier den mix blockieren? vermutlich nciht, aber andersherum faend ich das einleuchtender.
		// es kann nicht blockieren unter der Annahme das der TCP-Sendbuffer > m_xmlKeyInfoSize ist....
		//wait for keys from user
		MIXPACKET oMixPacket;
		if(pNewUser->receive(&oMixPacket,FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT)!=MIXPACKET_SIZE) //wait at most 10 second for user to send sym key
			{
				delete pNewUser;
				m_pIPList->removeIP(peerIP);
				return E_UNKNOWN;
			}
		m_pRSA->decrypt(oMixPacket.data,oMixPacket.data);
		if(memcmp("KEYPACKET",oMixPacket.data,9)!=0)
			{
				m_pIPList->removeIP(peerIP);
				delete pNewUser;
				return E_UNKNOWN;
			}
		pNewUser->setKey(oMixPacket.data+9,32);
		pNewUser->setCrypt(true);

#ifdef LOG_PACKET_TIMES
		CATimedQueue* tmpQueue=new CATimedQueue();
#else
		CAQueue* tmpQueue=new CAQueue(MIXPACKET_SIZE);
#endif
		if(m_pChannelList->add(pNewUser,peerIP,tmpQueue)!=E_SUCCESS)// adding user connection to mix->JAP channel list (stefan: sollte das nicht connection list sein? --> es handelt sich um eine Datenstruktu f�r Connections/Channels ).
			{	
				m_pIPList->removeIP(peerIP);
				delete tmpQueue;
				delete pNewUser;
				return E_UNKNOWN;
			}
#ifdef PAYMENT
		// set AI encryption keys
		fmHashTableEntry* pHashEntry=m_pChannelList->get(pNewUser);
		m_pAccountingInstance->setJapKeys(pHashEntry, oMixPacket.data+41, oMixPacket.data+57); 
#endif
#ifdef FIRST_MIX_SYMMETRIC
		fmHashTableEntry* pHashEntry=m_pChannelList->get(pNewUser);
		pHashEntry->pSymCipher=new CASymCipher();
		UINT8 buff[16];
		memset(buff,0,16);
		pHashEntry->pSymCipher->setKey(buff);
#endif
		incUsers();																	// increment the user counter by one
		m_psocketgroupUsersRead->add(*pNewUser); // add user socket to the established ones that we read data from.
		return E_SUCCESS;
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
				countRead=psocketgroupUsersRead->select(false,1000); //if we sleep here forever, we will not notice new sockets...
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

SINT32 CAFirstMix::clean()
	{
		if(m_pthreadsLogin!=NULL)
			delete m_pthreadsLogin;
		if(m_pInfoService!=NULL)
			{
				#ifdef _DEBUG
					CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::clean() start\n");
				#endif
				CAMsg::printMsg(LOG_CRIT,"Stopping InfoService....\n");
				CAMsg::printMsg	(LOG_CRIT,"Memory usage before: %u\n",getMemoryUsage());	
				m_pInfoService->stop();
				CAMsg::printMsg	(LOG_CRIT,"Memory usage after: %u\n",getMemoryUsage());	
				CAMsg::printMsg(LOG_CRIT,"Stopped InfoService!\n");
				delete m_pInfoService;
			}
		m_pInfoService=NULL;

		if(m_pthreadAcceptUsers!=NULL)
			delete m_pthreadAcceptUsers;
		m_pthreadAcceptUsers=NULL;
		if(m_pthreadSendToMix!=NULL)
			delete m_pthreadSendToMix;
		m_pthreadAcceptUsers=NULL;
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
		if(m_pQueueReadFromMix!=NULL)
			delete m_pQueueReadFromMix;
		m_pQueueReadFromMix=NULL;
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
		if(m_xmlKeyInfoBuff!=NULL)
			delete []m_xmlKeyInfoBuff;
		m_xmlKeyInfoBuff=NULL;
		m_docMixCascadeInfo=NULL;
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::clean() finished\n");
		#endif
		return E_SUCCESS;
	}

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
		
		//tmp XML-Structure for constructing the XML which is send to each user
		DOM_Document docXmlKeyInfo=DOM_Document::createDocument();
		DOM_Element elemRootKey=docXmlKeyInfo.createElement("MixCascade");
		setDOMElementAttribute(elemRootKey,"version",(UINT8*)"0.1"); //set the Version of the XML to 0.1
		docXmlKeyInfo.appendChild(elemRootKey);
		DOM_Element elemMixProtocolVersion=docXmlKeyInfo.createElement("MixProtocolVersion");
		setDOMElementValue(elemMixProtocolVersion,(UINT8*)MIX_CASCADE_PROTOCOL_VERSION);
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
					}
				child=child.getPreviousSibling();
			}
		tlen=256;
	
	
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
		CACertificate* ownCert=options.getOwnCertificate();
		if(ownCert==NULL)
			{
				CAMsg::printMsg(LOG_DEBUG,"Own Test Cert is NULL -- so it could not be inserted into signed KeyInfo send to users...\n");
			}	
		CACertStore* tmpCertStore=new CACertStore();
		tmpCertStore->add(ownCert);
		if(m_pSignature->signXML(elemRootKey,tmpCertStore)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo send to users...\n");
			}
		delete ownCert;
		delete tmpCertStore;
		
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
						CAMsg::printMsg(LOG_DEBUG,"Try to verify next mix signature...\n");
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
						DOM_Element elemNonce;
						getDOMChildByName(child,(UINT8*)"Nonce",elemNonce,false);
						UINT8 arNonce[1024];
						if(elemNonce!=NULL)
							{
								UINT32 lenNonce=1024;
								UINT32 tmpLen=1024;
								getDOMElementValue(elemNonce,arNonce,&lenNonce);
								CABase64::decode(arNonce,lenNonce,arNonce,&tmpLen);
								lenNonce=tmpLen;
								tmpLen=1024;
								CABase64::encode(SHA1(arNonce,lenNonce,NULL),SHA_DIGEST_LENGTH,
																	arNonce,&tmpLen);
								arNonce[tmpLen]=0;
							}
						UINT8 key[64];
						getRandom(key,64);
						//UINT8 buff[400];
						//UINT32 bufflen=400;
						DOM_DocumentFragment docfragSymKey;
						encodeXMLEncryptedKey(key,64,docfragSymKey,&oRSA);
						DOM_Document docSymKey=DOM_Document::createDocument();
						docSymKey.appendChild(docSymKey.importNode(docfragSymKey,true));
						DOM_Element elemRoot=docSymKey.getDocumentElement();
						if(elemNonce!=NULL)
							{
								DOM_Element elemNonceHash=docSymKey.createElement("Nonce");
								setDOMElementValue(elemNonceHash,arNonce);
								elemRoot.appendChild(elemNonceHash);
							}
						UINT32 outlen=5000;
						UINT8* out=new UINT8[outlen];

						m_pSignature->signXML(elemRoot);
						DOM_Output::dumpToMem(docSymKey,out,&outlen);
						m_pMuxOut->setSendKey(key,32);
						m_pMuxOut->setReceiveKey(key+32,32);
						UINT16 size=htons(outlen);
						((CASocket*)m_pMuxOut)->send((UINT8*)&size,2);
						((CASocket*)m_pMuxOut)->send(out,outlen);
						m_pMuxOut->setCrypt(true);
						delete[] out;
						break;
					}
				child=child.getNextSibling();
			}
		

	//CascadeInfo		
		m_docMixCascadeInfo=DOM_Document::createDocument();
		DOM_Element elemRoot=m_docMixCascadeInfo.createElement("MixCascade");


		UINT8 id[50];
		options.getMixId(id,50);
		elemRoot.setAttribute(DOMString("id"),DOMString((char*)id));
		
		UINT8 name[255];
		if(options.getCascadeName(name,255)!=E_SUCCESS)
			{
				return E_UNKNOWN;
			}
		m_docMixCascadeInfo.appendChild(elemRoot);
		DOM_Element elem=m_docMixCascadeInfo.createElement("Name");
		DOM_Text text=m_docMixCascadeInfo.createTextNode(DOMString((char*)name));
		elem.appendChild(text);
		elemRoot.appendChild(elem);
		
		elem=m_docMixCascadeInfo.createElement("Network");
		elemRoot.appendChild(elem);
		DOM_Element elemListenerInterfaces=m_docMixCascadeInfo.createElement("ListenerInterfaces");
		elem.appendChild(elemListenerInterfaces);
		
		for(UINT32 i=1;i<=options.getListenerInterfaceCount();i++)
			{
				CAListenerInterface* pListener=options.getListenerInterface(i);
				if(pListener->isHidden()){/*do nothing*/}
				else if(pListener->getType()==RAW_TCP)
					{
						DOM_DocumentFragment docFrag;
						pListener->toDOMFragment(docFrag,m_docMixCascadeInfo);
						elemListenerInterfaces.appendChild(docFrag);
					}
				delete pListener;
			}
		
		DOM_Element elemThisMix=m_docMixCascadeInfo.createElement("Mix");
		elemThisMix.setAttribute(DOMString("id"),DOMString((char*)id));
		DOM_Node elemMixesDocCascade=m_docMixCascadeInfo.createElement("Mixes");
		elemMixesDocCascade.appendChild(elemThisMix);
		count=1;
		elemRoot.appendChild(elemMixesDocCascade);
		
		DOM_Node node=elemMixes.getFirstChild();
		while(node!=NULL)
			{
				if(node.getNodeType()==DOM_Node::ELEMENT_NODE&&node.getNodeName().equals("Mix"))
					{
						elemMixesDocCascade.appendChild(m_docMixCascadeInfo.importNode(node,false));
						count++;
					}
				node=node.getNextSibling();
			}
		setDOMElementAttribute(elemMixesDocCascade,"count",count);
		return E_SUCCESS;
	}
