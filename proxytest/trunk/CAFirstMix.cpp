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

extern CACmdLnOptions options;


SINT32 CAFirstMix::initOnce()
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting FirstMix InitOnce\n");
		SINT32 ret=E_UNKNOWN;
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
		if(mSignature.setSignKey(fileBuff,len,SIGKEY_XML)!=E_SUCCESS)
			goto END;
		ret=E_SUCCESS;
END:		
		delete fileBuff;
		return ret;

	}

SINT32 CAFirstMix::init()
	{
		CASocketAddr* pAddrNext=NULL;
		UINT8 strTarget[255];
		options.getTargetHost(strTarget,255);
		if(strTarget[0]=='/') //Unix-Domain
			{
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
				pAddrNext=new CASocketAddrUnix();
				((CASocketAddrUnix*)pAddrNext)->setPath((char*)strTarget);
				CAMsg::printMsg(LOG_INFO,"Try connecting to next Mix on Unix-Domain-Socket: %s\n",strTarget);
#else
				CAMsg::printMsg(LOG_CRIT,"I do not understand the Unix Domain Protocol!\n");
				return E_UNKNOWN;
#endif
			}
		else
			{
				pAddrNext=new CASocketAddrINet();
				((CASocketAddrINet*)pAddrNext)->setAddr(strTarget,options.getTargetPort());
				CAMsg::printMsg(LOG_INFO,"Try connecting to next Mix: %s:%u ...\n",strTarget,options.getTargetPort());
			}
		if(((CASocket*)muxOut)->create(pAddrNext->getType())!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot create SOCKET for connection to next Mix!\n");
				return E_UNKNOWN;
			}
		((CASocket*)muxOut)->setSendBuff(500*MIXPACKET_SIZE);
		((CASocket*)muxOut)->setRecvBuff(500*MIXPACKET_SIZE);
		if(((CASocket*)muxOut)->setSendLowWat(MIXPACKET_SIZE)!=E_SUCCESS)
			CAMsg::printMsg(LOG_INFO,"SOCKET Option SENDLOWWAT not set!\n");
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET RecvBuffSize: %i\n",((CASocket*)muxOut)->getRecvBuff());
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET SendBuffSize: %i\n",((CASocket*)muxOut)->getSendBuff());
		CAMsg::printMsg(LOG_INFO,"MUXOUT-SOCKET SendLowWatSize: %i\n",((CASocket*)muxOut)->getSendLowWat());


		if(muxOut.connect(*pAddrNext,10,10)!=E_SUCCESS)
			{
				delete pAddrNext;
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix!\n");
				return E_UNKNOWN;
			}
		delete pAddrNext;

		CAMsg::printMsg(LOG_INFO," connected!\n");
//		sleep(1);
		if(((CASocket*)muxOut)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)muxOut)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}

		UINT16 len;
		if(((CASocket*)muxOut)->receiveFully((UINT8*)&len,2)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info lenght!\n");
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_CRIT,"Received Key Info lenght %u\n",ntohs(len));
		UINT8* recvBuff=new unsigned char[ntohs(len)+2];
		memcpy(recvBuff,&len,2);

		if(((CASocket*)muxOut)->receiveFully(recvBuff+2,ntohs(len))!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error receiving Key Info!\n");
				delete recvBuff;
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_CRIT,"Received Key Info...\n");
		m_pRSA=new CAASymCipher;
		m_pRSA->generateKeyPair(1024);
		UINT32 keySize=m_pRSA->getPublicKeySize();
		m_KeyInfoSize=ntohs((*(UINT16*)recvBuff))+2;
		m_KeyInfoBuff=new UINT8[m_KeyInfoSize+keySize];
		memcpy(m_KeyInfoBuff,recvBuff,m_KeyInfoSize);
		delete recvBuff;
		m_KeyInfoBuff[2]++; //chainlen erhoehen
		m_pRSA->getPublicKey(m_KeyInfoBuff+m_KeyInfoSize,&keySize);
		m_KeyInfoSize+=keySize;
		(*(UINT16*)m_KeyInfoBuff)=htons(m_KeyInfoSize-2);

		
		CASocketAddrINet socketAddrIn;
		UINT8 serverHost[255];
		if(options.getServerHost(serverHost,255)!=E_SUCCESS)
			{
				socketAddrIn.setPort(options.getServerPort());
			}
		else
			{
				socketAddrIn.setAddr(serverHost,options.getServerPort());
			}
		m_nSocketsIn=1; //normal (and may be HTTPS)
		m_arrSocketsIn=new CASocket[2];
		
		m_arrSocketsIn[0].create();
		m_arrSocketsIn[0].setReuseAddr(true);
		if(m_arrSocketsIn[0].listen(socketAddrIn)==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
					return E_UNKNOWN;
		    }
    if(options.getProxySupport())
    	{
    		m_nSocketsIn=2;
				m_arrSocketsIn[1].create();
        m_arrSocketsIn[1].setReuseAddr(true);
				socketAddrIn.setPort(443);
#ifndef _WIN32
        //we have to be a temporaly superuser...
				int old_uid=geteuid();
				if(seteuid(0)==-1) //changing to root
					CAMsg::printMsg(LOG_CRIT,"Setuid failed!\n");
#endif				
				SINT32 ret=m_arrSocketsIn[1].listen(socketAddrIn);
#ifndef _WIN32
				seteuid(old_uid);
#endif
				if(ret==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen on HTTPS-Port\n");
					return E_UNKNOWN;
		    }
      }
    m_pIPList=new CAIPList();
		m_pQueueSendToMix=new CAQueue();
		m_pChannelList=new CAFirstMixChannelList();
		m_psocketgroupUsersRead=new CASocketGroup;
		m_pInfoService=new CAInfoService(this);
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix init() succeded\n");
		return E_SUCCESS;
	}


THREAD_RETURN loopSendToMix(void* param)
	{
		CAQueue* pQueue=((CAFirstMix*)param)->m_pQueueSendToMix;
		CASocket* pSocket=(CASocket *)((CAFirstMix*)param)->muxOut;
		UINT8* buff=new UINT8[0xFFFF];
		UINT32 len;
		for(;;)
			{
				len=0xFFFF;
				pQueue->getOrWait(buff,&len);
				if(pSocket->sendFully(buff,len)!=E_SUCCESS)
					break;
			}
		delete buff;
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread SendToMix\n");
		THREAD_RETURN_SUCCESS;
	}

THREAD_RETURN loopAcceptUsers(void* param)
	{
		CAFirstMix* pFirstMix=(CAFirstMix*)param;
		CASocket* socketsIn=pFirstMix->m_arrSocketsIn;
		CAIPList* pIPList=pFirstMix->m_pIPList;
		CAInfoService* pInfoService=pFirstMix->m_pInfoService;
		CAFirstMixChannelList* pChannelList=pFirstMix->m_pChannelList;
		CASocketGroup* psocketgroupUsersRead=pFirstMix->m_psocketgroupUsersRead;
		UINT32 nSocketsIn=pFirstMix->m_nSocketsIn;
		UINT8* pKeyInfoBuff=pFirstMix->m_KeyInfoBuff;
		UINT32 nKeyInfoSize=pFirstMix->m_KeyInfoSize;

		CASocketGroup osocketgroupAccept;
		CAMuxSocket* pNewMuxSocket;
		UINT8* ip=new UINT8[4];
		UINT32 i=0;
		UINT32& nUser=pFirstMix->m_nUser;
		SINT32 countRead;
		SINT32 ret;
		for(i=0;i<nSocketsIn;i++)
			osocketgroupAccept.add(socketsIn[i]);
		for(;;)
			{
				countRead=osocketgroupAccept.select(false,10000);
				if(countRead<0&&countRead!=E_TIMEDOUT)
					break;
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
									CAMsg::printMsg(LOG_DEBUG,"New direct Connection from Browser!\n");
								#endif
								pNewMuxSocket=new CAMuxSocket;
								if(socketsIn[i].accept(*(CASocket*)pNewMuxSocket)==SOCKET_ERROR)
									{
										CAMsg::printMsg(LOG_ERR,"Accept Error %u - direct Connection from Browser!\n",GET_NET_ERROR);
										delete pNewMuxSocket;
									}
								else
									{
		//Prüfen ob schon vorhanden..	
										ret=((CASocket*)pNewMuxSocket)->getPeerIP(ip);
										if(ret!=E_SUCCESS||pIPList->insertIP(ip)<0)
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
												((CASocket*)pNewMuxSocket)->send(pKeyInfoBuff,nKeyInfoSize);
												((CASocket*)pNewMuxSocket)->setNonBlocking(true);
												pChannelList->add(pNewMuxSocket,new CAQueue);
												nUser++;
												pInfoService->setLevel(nUser,-1,-1);
												psocketgroupUsersRead->add(*pNewMuxSocket);
											}
								}
						}
					i++;
				}
			
			}
		delete ip;
		CAMsg::printMsg(LOG_DEBUG,"Exiting Thread AcceptUser\n");
		THREAD_RETURN_SUCCESS;
	}


SINT32 CAFirstMix::loop()
	{
//		CAFirstMixChannelList  oChannelList;

		//CASocketGroup osocketgroupAccept;
		//CASocketGroup osocketgroupUsersRead;
		CASocketGroup osocketgroupUsersWrite;
		CASingleSocketGroup osocketgroupMixOut;
//		CAMuxSocket* pnewMuxSocket;
		SINT32 countRead;
		//HCHANNEL lastChannelId=1;
		MIXPACKET* pMixPacket=new MIXPACKET;
//		CAInfoService oInfoService(this);
		m_nUser=0;
		SINT32 ret;
		UINT8 rsaBuff[RSA_SIZE];
//		UINT32 maxSocketsIn;
//		osocketgroupAccept.add(m_socketIn);
//    CASocket** socketsIn;
//		bool bProxySupport=false;
/*    if(options.getProxySupport())
    	{
    		osocketgroupAccept.add(m_socketHttpsIn);
      	bProxySupport=true;
				socketsIn=new CASocket*[2];
				maxSocketsIn=2;
				socketsIn[0]=&m_socketIn;
				socketsIn[1]=&m_socketHttpsIn;
      }
		else
			{
				socketsIn=new CASocket*[1];
				maxSocketsIn=1;
				socketsIn[0]=&m_socketIn;
			}
*/
		osocketgroupMixOut.add(muxOut);
		muxOut.setCrypt(true);
		//((CASocket*)muxOut)->setNonBlocking(true);
		
		m_pInfoService->setSignature(&mSignature);
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix InfoService - Signature set\n");
		m_pInfoService->setLevel(0,-1,-1);
		m_pInfoService->sendHelo();
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix Helo sended\n");
		m_pInfoService->start();
		CAMsg::printMsg(LOG_DEBUG,"InfoService Loop started\n");

		UINT8 ip[4];
		UINT8* tmpBuff=new UINT8[MIXPACKET_SIZE];
		CAMsg::printMsg(LOG_DEBUG,"Starting Message Loop... \n");
		bool bAktiv;
		//Starting thread for Step 1
		CAThread threadAcceptUsers;
		threadAcceptUsers.setMainLoop(loopAcceptUsers);
		threadAcceptUsers.start(this);
		
		//Starting thread for Step 4
		CAThread threadSendToMix;
		threadSendToMix.setMainLoop(loopSendToMix);
		threadSendToMix.start(this);
		for(;;)
			{
				bAktiv=false;
//LOOP_START:

//First Step
//Checking for new connections		
// Now in a separat Thread....
				
/*				
				countRead=osocketgroupAccept.select(false,0);
				UINT32 i=0;
				if(countRead>0)
					bAktiv=true;
				while(countRead>0&&i<maxSocketsIn)
					{						
						if(osocketgroupAccept.isSignaled(*socketsIn[i]))
							{
								countRead--;
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"New direct Connection from Browser!\n");
								#endif
								pnewMuxSocket=new CAMuxSocket;
								if(socketsIn[i]->accept(*(CASocket*)pnewMuxSocket)==SOCKET_ERROR)
									{
										CAMsg::printMsg(LOG_ERR,"Accept Error %u - direct Connection from Browser!\n",GET_NET_ERROR);
										delete pnewMuxSocket;
									}
								else
									{
		//Prüfen ob schon vorhanden..	
											ret=((CASocket*)pnewMuxSocket)->getPeerIP(ip);
											if(ret!=E_SUCCESS||m_pIPList->insertIP(ip)<0)
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
													((CASocket*)pnewMuxSocket)->send(mKeyInfoBuff,mKeyInfoSize);
													((CASocket*)pnewMuxSocket)->setNonBlocking(true);
													oChannelList.add(pnewMuxSocket,new CAQueue);
													nUser++;
													oInfoService.setLevel(nUser,-1,-1);
													osocketgroupUsersRead.add(*pnewMuxSocket);
												}
									}
							}
						i++;
					}
*/
				
// Second Step 
// Checking for data from users
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
								if(ret==SOCKET_ERROR)
									{
										((CASocket*)pMuxSocket)->getPeerIP(ip);
										m_pIPList->removeIP(ip);
										m_psocketgroupUsersRead->remove(*(CASocket*)pMuxSocket);
										osocketgroupUsersWrite.remove(*(CASocket*)pMuxSocket);
										fmChannelListEntry* pEntry;
										pEntry=m_pChannelList->getFirstChannelForSocket(pMuxSocket);
										while(pEntry!=NULL)
											{
												muxOut.close(pEntry->channelOut,tmpBuff);
												m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
                        delete pEntry->pCipher;
												pEntry=m_pChannelList->getNextChannel(pEntry);
											}
										ASSERT(pHashEntry->pQueueSend!=NULL,"Send queue is NULL");
										delete pHashEntry->pQueueSend;
										m_pChannelList->remove(pMuxSocket);
										pMuxSocket->close();
										delete pMuxSocket;
										m_nUser--;
										m_pInfoService->setLevel(m_nUser,-1,-1);
									}
								else if(ret==MIXPACKET_SIZE)
									{
										if(pMixPacket->flags==CHANNEL_CLOSE)
											{
												fmChannelListEntry* pEntry;
												pEntry=m_pChannelList->get(pMuxSocket,pMixPacket->channel);
												if(pEntry!=NULL)
													{
														muxOut.close(pEntry->channelOut,tmpBuff);
														m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
                            delete pEntry->pCipher;
														m_pChannelList->remove(pMuxSocket,pMixPacket->channel);
													}
												else
													{
														#ifdef _DEBUG
															CAMsg::printMsg(LOG_DEBUG,"Invalid ID to close from Browser!\n");
														#endif
													}
											}
										else
											{
												CASymCipher* pCipher=NULL;
												fmChannelListEntry* pEntry;
												pEntry=m_pChannelList->get(pMuxSocket,pMixPacket->channel);
												if(pEntry!=NULL)
													{
														pMixPacket->channel=pEntry->channelOut;
														pCipher=pEntry->pCipher;
														pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
													}
												else
													{
														pCipher= new CASymCipher();
														m_pRSA->decrypt(pMixPacket->data,rsaBuff);
														pCipher->setKeyAES(rsaBuff);
														pCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																						 pMixPacket->data+RSA_SIZE-KEY_SIZE,
																						 DATA_SIZE-RSA_SIZE);
														memcpy(pMixPacket->data,rsaBuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);

														m_pChannelList->add(pMuxSocket,pMixPacket->channel,pCipher,&pMixPacket->channel);
														#ifdef _DEBUG
															CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",pMixPacket->channel);
														#endif
														//oMixPacket.channel=lastChannelId++;
													}
												muxOut.send(pMixPacket,tmpBuff);
												m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
												m_MixedPackets++;
											}
									}
							}
						pHashEntry=m_pChannelList->getNext();
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
						ret=muxOut.receive(pMixPacket,0);
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Mux-Out-Channel Receiving Data Error - Exiting!\n");
								goto ERR;
							}
						if(ret!=MIXPACKET_SIZE)
							break;
						if(pMixPacket->flags==CHANNEL_CLOSE) //close event
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ... ",pMixPacket->channel);
								#endif
								fmChannelList* pEntry=m_pChannelList->get(pMixPacket->channel);
								if(pEntry!=NULL)
									{
										pEntry->pHead->pMuxSocket->close(pEntry->channelIn,tmpBuff);
										pEntry->pHead->pQueueSend->add(tmpBuff,MIXPACKET_SIZE);
										osocketgroupUsersWrite.add(*pEntry->pHead->pMuxSocket);
										delete pEntry->pCipher;
										m_pChannelList->remove(pEntry->pHead->pMuxSocket,pEntry->channelIn);
									}
							}
						else
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!");
								#endif
								fmChannelList* pEntry=m_pChannelList->get(pMixPacket->channel);
								if(pEntry!=NULL)
									{
										pMixPacket->channel=pEntry->channelIn;
										pEntry->pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										
										pEntry->pHead->pMuxSocket->send(pMixPacket,tmpBuff);
										pEntry->pHead->pQueueSend->add(tmpBuff,MIXPACKET_SIZE);
										osocketgroupUsersWrite.add(*pEntry->pHead->pMuxSocket);
#define MAX_USER_SEND_QUEUE 100000
										if(pEntry->pHead->pQueueSend->getSize()>MAX_USER_SEND_QUEUE&&
												!pEntry->bIsSuspended)
											{
												pMixPacket->channel=pEntry->channelOut;
												pMixPacket->flags=CHANNEL_SUSPEND;
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_INFO,"Sending suspend for channel: %u\n",pMixPacket->channel);
												#endif												
												muxOut.send(pMixPacket,tmpBuff);
												m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
												
												pEntry->bIsSuspended=true;
												pEntry->pHead->cSuspend++;
											}
										m_MixedPackets++;
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
				countRead=osocketgroupUsersWrite.select(true,0);
				if(countRead>0)
					bAktiv=true;
				while(countRead>0&&pfmHashEntry!=NULL)
					{
						if(osocketgroupUsersWrite.isSignaled(*pfmHashEntry->pMuxSocket))
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
																muxOut.send(pMixPacket,tmpBuff);
																m_pQueueSendToMix->add(tmpBuff,MIXPACKET_SIZE);
															}
														pEntry=m_pChannelList->getNextChannel(pEntry);
													}
												pfmHashEntry->cSuspend=0;
											}
										if(pfmHashEntry->pQueueSend->isEmpty())
											{
												osocketgroupUsersWrite.remove(*pfmHashEntry->pMuxSocket);
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
		m_pInfoService->stop();
		muxOut.close();
		for(UINT32  i=0;i<m_nSocketsIn;i++)
			m_arrSocketsIn[i].close();
		threadAcceptUsers.join();
		threadSendToMix.join();
		fmHashTableEntry* pHashEntry=m_pChannelList->getFirst();
		while(pHashEntry!=NULL)
			{
				pHashEntry->pMuxSocket->close();
				delete pHashEntry->pMuxSocket;
				delete pHashEntry->pQueueSend;

				fmChannelListEntry* pEntry=m_pChannelList->getFirstChannelForSocket(pHashEntry->pMuxSocket);
				while(pEntry!=NULL)
					{
						delete pEntry->pCipher;
						pEntry=m_pChannelList->getNextChannel(pEntry);
					}
				pHashEntry=m_pChannelList->getNext();
			}
		delete pMixPacket;
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
		muxOut.close();
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
		if(m_pRSA!=NULL)
			delete m_pRSA;
		m_pRSA=NULL;
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAFirstMix::clean() finished\n");
		#endif
		return E_SUCCESS;
	}
