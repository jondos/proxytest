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
//#include "CAMuxChannelList.hpp"
#include "CAFirstMixChannelList.hpp"
#include "CAASymCipher.hpp"
#include "CAInfoService.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocketAddrUnix.hpp"

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

		mRSA.generateKeyPair(1024);
		UINT32 keySize=mRSA.getPublicKeySize();
		mKeyInfoSize=ntohs((*(UINT16*)recvBuff))+2;
		mKeyInfoBuff=new UINT8[mKeyInfoSize+keySize];
		memcpy(mKeyInfoBuff,recvBuff,mKeyInfoSize);
		delete recvBuff;
		mKeyInfoBuff[2]++; //chainlen erhoehen
		mRSA.getPublicKey(mKeyInfoBuff+mKeyInfoSize,&keySize);
		mKeyInfoSize+=keySize;
		(*(UINT16*)mKeyInfoBuff)=htons(mKeyInfoSize-2);

		
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
		m_socketIn.create();
		m_socketIn.setReuseAddr(true);
		if(m_socketIn.listen(socketAddrIn)==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
					return E_UNKNOWN;
		    }
    if(options.getProxySupport())
    	{
    		m_socketHttpsIn.create();
        m_socketHttpsIn.setReuseAddr(true);
				socketAddrIn.setPort(443);
#ifndef _WIN32
        //we have to be a temporaly superuser...
				int old_uid=geteuid();
				if(seteuid(0)==-1) //changing to root
					CAMsg::printMsg(LOG_CRIT,"Setuid failed!\n");
#endif				
				SINT32 ret=m_socketHttpsIn.listen(socketAddrIn);
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
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix init() succeded\n");
		return E_SUCCESS;
	}

/*
SINT32 CAFirstMix::loop()
	{
		CAMuxChannelList  oMuxChannelList;
		REVERSEMUXLISTENTRY* tmpReverseEntry;
		MUXLISTENTRY* tmpMuxListEntry;

		CASocketGroup oSocketGroup;
		CASingleSocketGroup oSocketGroupMuxOut;
		CAMuxSocket* pnewMuxSocket;
		SINT32 countRead;
		HCHANNEL lastChannelId=1;
		MIXPACKET oMixPacket;
		CONNECTION oConnection;
		CAInfoService oInfoService(this);
		UINT32 nUser=0;
		SINT32 ret;
		UINT8 rsaBuff[RSA_SIZE];

		oSocketGroup.add(socketIn);
    bool bProxySupport=false;
    if(options.getProxySupport())
    	{
    		oSocketGroup.add(m_socketHttpsIn);
      	bProxySupport=true;
      }
    oSocketGroup.add(muxOut);
		oSocketGroupMuxOut.add(muxOut);
		muxOut.setCrypt(true);

		oInfoService.setSignature(&mSignature);
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix InfoService - Signature set\n");
		oInfoService.setLevel(0,-1,-1);
		oInfoService.sendHelo();
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix Helo sended\n");
		oInfoService.start();
		CAMsg::printMsg(LOG_DEBUG,"InfoService Loop started\n");

		UINT8 ip[4];
		CAMsg::printMsg(LOG_DEBUG,"Starting Message Loop... \n");
		for(;;)
			{
LOOP_START:
				if((countRead=oSocketGroup.select())==SOCKET_ERROR)
					{
						CAMsg::printMsg(LOG_ERR,"SELECT Error %u - Connection from Browser!\n",errno);
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(socketIn))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New direct Connection from Browser!\n");
						#endif
						pnewMuxSocket=new CAMuxSocket;
						if(socketIn.accept(*(CASocket*)pnewMuxSocket)==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_ERR,"Accept Error %u - direct Connection from Browser!\n",errno);
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
			#ifdef _ASYNC
											((CASocket*)pnewMuxSocket)->setASyncSend(true,MIXPACKET_SIZE,0,0,this);
			#endif
											((CASocket*)pnewMuxSocket)->send(mKeyInfoBuff,mKeyInfoSize);
											oMuxChannelList.add(pnewMuxSocket);
											nUser++;
											oInfoService.setLevel(nUser,-1,-1);
											oSocketGroup.add(*pnewMuxSocket);
										}
							}
					}
			if(bProxySupport&&oSocketGroup.isSignaled(m_socketHttpsIn))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New proxy Connection from Browser!\n");
						#endif
						pnewMuxSocket=new CAMuxSocket;
						if(m_socketHttpsIn.accept(*(CASocket*)pnewMuxSocket)==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_ERR,"Accept Error %u - proxy Connection from Browser!\n",errno);
								delete pnewMuxSocket;
							}
						else
							{
//Prüfen ob schon vorhanden..	
								((CASocket*)pnewMuxSocket)->getPeerIP(ip);
								if(m_pIPList->insertIP(ip)<0)
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
		#ifdef _ASYNC
										((CASocket*)pnewMuxSocket)->setASyncSend(true,MIXPACKET_SIZE,0,0,this);
		#endif
										((CASocket*)pnewMuxSocket)->send(mKeyInfoBuff,mKeyInfoSize);
										oMuxChannelList.add(pnewMuxSocket);
										nUser++;
										oInfoService.setLevel(nUser,-1,-1);
										oSocketGroup.add(*pnewMuxSocket);
									}
							}
					}
			if(oSocketGroup.isSignaled(muxOut))
					{
						countRead--;
						int countMuxOut=nUser;
						do
							{
								ret=muxOut.receive(&oMixPacket);
								if(ret==SOCKET_ERROR)
									{
										CAMsg::printMsg(LOG_CRIT,"Mux-Out-Channel Receiving Data Error - Exiting!\n");
										goto ERR;
									}
								if(oMixPacket.flags==CHANNEL_CLOSE) //close event
									{
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ... ",oMixPacket.channel);
										#endif
										REVERSEMUXLISTENTRY otmpReverseEntry;
										if(oMuxChannelList.remove(oMixPacket.channel,&otmpReverseEntry))
											{
												otmpReverseEntry.pMuxSocket->close(otmpReverseEntry.inChannel);
												delete otmpReverseEntry.pCipher;
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_DEBUG,"closed!\n");
												#endif
												deleteResume(otmpReverseEntry.pMuxSocket,otmpReverseEntry.outChannel);
											}
									}
								else
									{
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!");
										#endif
										tmpReverseEntry=oMuxChannelList.get(oMixPacket.channel);
										if(tmpReverseEntry!=NULL)
											{
												oMixPacket.channel=tmpReverseEntry->inChannel;
												tmpReverseEntry->pCipher->decryptAES2(oMixPacket.data,oMixPacket.data,DATA_SIZE);
												//TODO: Test for SOCKET_ERROR!!!!
												
												if(tmpReverseEntry->pMuxSocket->send(&oMixPacket)==E_QUEUEFULL)
													{
														EnterCriticalSection(&csResume);
														MUXLISTENTRY* pml=oSuspendList.get(tmpReverseEntry->pMuxSocket);
														CONNECTION oCon;
														if(pml==NULL||!pml->pSocketList->get(&oCon,tmpReverseEntry->outChannel)) //Have we not send a suspend message yet ?
															{
																oMixPacket.channel=tmpReverseEntry->outChannel;
																oMixPacket.flags=CHANNEL_SUSPEND;
																#ifdef _DEBUG
																	CAMsg::printMsg(LOG_INFO,"Sending suspend for channel: %u\n",oMixPacket.channel);
																#endif
																if(muxOut.send(&oMixPacket)==SOCKET_ERROR)
                                	{
                                		CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Restarting!\n");
                                    LeaveCriticalSection(&csResume);
                                    goto ERR;
																	}
                                if(pml==NULL)
																	{
																		oSuspendList.add(tmpReverseEntry->pMuxSocket);
																		pml=oSuspendList.get(tmpReverseEntry->pMuxSocket);
																	}
																pml->pSocketList->add(tmpReverseEntry->inChannel,tmpReverseEntry->outChannel,NULL);
															}
														LeaveCriticalSection(&csResume);
													}
												m_MixedPackets++;
											}
										else
											{
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- Channel-Id %u no valid!\n",oMixPacket.channel);
												#endif
											}
							}
						countMuxOut--;
					}while(oSocketGroupMuxOut.select(false,0)==1&&countMuxOut>0);
				}
				if(countRead>0)
					{
						tmpMuxListEntry=oMuxChannelList.getFirst();
						while(tmpMuxListEntry!=NULL&&countRead>0)
							{
								if(oSocketGroup.isSignaled(*tmpMuxListEntry->pMuxSocket))
									{
										if(oSocketGroupMuxOut.select(true,100)!=1)
											goto LOOP_START;
										countRead--;
										ret=tmpMuxListEntry->pMuxSocket->receive(&oMixPacket,0);
										if(ret==SOCKET_ERROR)
											{
												((CASocket*)tmpMuxListEntry->pMuxSocket)->getPeerIP(ip);
												m_pIPList->removeIP(ip);
												deleteResume(tmpMuxListEntry->pMuxSocket);
												MUXLISTENTRY otmpEntry;
												if(oMuxChannelList.remove(tmpMuxListEntry->pMuxSocket,&otmpEntry))
													{
														oSocketGroup.remove(*(CASocket*)otmpEntry.pMuxSocket);
														CONNECTION* tmpCon=otmpEntry.pSocketList->getFirst();
														while(tmpCon!=NULL)
															{
																if(muxOut.close(tmpCon->outChannel)==SOCKET_ERROR)
  																{
																		CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Restarting!\n");
																		goto ERR;
																	}
                                delete tmpCon->pCipher;
																tmpCon=otmpEntry.pSocketList->getNext();
															}
														otmpEntry.pMuxSocket->close();
														delete otmpEntry.pMuxSocket;
														delete otmpEntry.pSocketList;
													}
												nUser--;
												oInfoService.setLevel(nUser,-1,-1);
											}
										else if(ret==E_AGAIN)
											{
												tmpMuxListEntry=oMuxChannelList.getNext();
												continue;
											}
										else
											{
												if(oMixPacket.flags==CHANNEL_CLOSE)
													{
														if(oMuxChannelList.get(tmpMuxListEntry,oMixPacket.channel,&oConnection))
															{
																if(muxOut.close(oConnection.outChannel)==SOCKET_ERROR)
               										{
																		CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Restarting!\n");
																		goto ERR;
																	}
                                oMuxChannelList.remove(oConnection.outChannel,NULL);
																delete oConnection.pCipher;
																deleteResume(tmpMuxListEntry->pMuxSocket,oConnection.outChannel);
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
														if(oMuxChannelList.get(tmpMuxListEntry,oMixPacket.channel,&oConnection))
															{
																oMixPacket.channel=oConnection.outChannel;
																pCipher=oConnection.pCipher;
																pCipher->decryptAES(oMixPacket.data,oMixPacket.data,DATA_SIZE);
															}
														else
															{
																pCipher= new CASymCipher();
																mRSA.decrypt(oMixPacket.data,rsaBuff);
																pCipher->setKeyAES(rsaBuff);
																pCipher->decryptAES(oMixPacket.data+RSA_SIZE,
																								 oMixPacket.data+RSA_SIZE-KEY_SIZE,
																								 DATA_SIZE-RSA_SIZE);
																memcpy(oMixPacket.data,rsaBuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);

																oMuxChannelList.add(tmpMuxListEntry,oMixPacket.channel,lastChannelId,pCipher);
																#ifdef _DEBUG
																	CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",lastChannelId);
																#endif
																oMixPacket.channel=lastChannelId++;
															}
														if(muxOut.send(&oMixPacket)==SOCKET_ERROR)
															{
																CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Restarting!\n");
																goto ERR;
															}
														m_MixedPackets++;
												}
											}
									}
								tmpMuxListEntry=oMuxChannelList.getNext();
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		socketIn.close();
		muxOut.close();
		tmpMuxListEntry=oMuxChannelList.getFirst();
		while(tmpMuxListEntry!=NULL)
			{
				tmpMuxListEntry->pMuxSocket->close();
				delete tmpMuxListEntry->pMuxSocket;
				
				CONNECTION* tmpCon=tmpMuxListEntry->pSocketList->getFirst();
				while(tmpCon!=NULL)
					{
						delete tmpCon->pCipher;
						tmpCon=tmpMuxListEntry->pSocketList->getNext();
					}
				tmpMuxListEntry=oMuxChannelList.getNext();
			}

		return E_UNKNOWN;
	}
*/



// NEw Version..................

SINT32 CAFirstMix::loop()
	{
		CAFirstMixChannelList  oChannelList;

		CASocketGroup osocketgroupAccept;
		CASocketGroup osocketgroupUsersRead;
		CASocketGroup osocketgroupUsersWrite;
		CASingleSocketGroup osocketgroupMixOut;
		CAQueue oqueueMixOut;
		CAMuxSocket* pnewMuxSocket;
		SINT32 countRead;
		HCHANNEL lastChannelId=1;
		MIXPACKET oMixPacket;
		CAInfoService oInfoService(this);
		UINT32 nUser=0;
		SINT32 ret;
		UINT8 rsaBuff[RSA_SIZE];
		UINT32 maxSocketsIn;
		osocketgroupAccept.add(m_socketIn);
    CASocket** socketsIn;
		bool bProxySupport=false;
    if(options.getProxySupport())
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
		osocketgroupMixOut.add(muxOut);
		muxOut.setCrypt(true);
		((CASocket*)muxOut)->setNonBlocking(true);
		
		oInfoService.setSignature(&mSignature);
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix InfoService - Signature set\n");
		oInfoService.setLevel(0,-1,-1);
		oInfoService.sendHelo();
		CAMsg::printMsg(LOG_DEBUG,"CAFirstMix Helo sended\n");
		oInfoService.start();
		CAMsg::printMsg(LOG_DEBUG,"InfoService Loop started\n");

		UINT8 ip[4];
		UINT8* tmpBuff=new UINT8[MIXPACKET_SIZE];
		CAMsg::printMsg(LOG_DEBUG,"Starting Message Loop... \n");
		bool bAktiv;
		for(;;)
			{
				bAktiv=false;
//LOOP_START:

//First Step
//Checking for new connections		
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
										CAMsg::printMsg(LOG_ERR,"Accept Error %u - direct Connection from Browser!\n",errno);
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

				
// Second Step 
// Checking for data from users
				fmHashTableEntry* pHashEntry=oChannelList.getFirst();
				countRead=osocketgroupUsersRead.select(false,0);
				if(countRead>0)
					bAktiv=true;
				while(pHashEntry!=NULL&&countRead>0)
					{
						CAMuxSocket* pMuxSocket=pHashEntry->pMuxSocket;
						if(osocketgroupUsersRead.isSignaled(*pMuxSocket))
							{
								countRead--;
								ret=pMuxSocket->receive(&oMixPacket,0);
								if(ret==SOCKET_ERROR)
									{
										((CASocket*)pMuxSocket)->getPeerIP(ip);
										m_pIPList->removeIP(ip);
										osocketgroupUsersRead.remove(*(CASocket*)pMuxSocket);
										osocketgroupUsersWrite.remove(*(CASocket*)pMuxSocket);
										fmChannelListEntry* pEntry;
										pEntry=oChannelList.getFirstChannelForSocket(pMuxSocket);
										while(pEntry!=NULL)
											{
												muxOut.close(pEntry->channelOut,tmpBuff);
												oqueueMixOut.add(tmpBuff,MIXPACKET_SIZE);
                        delete pEntry->pCipher;
												pEntry=oChannelList.getNextChannel(pEntry);
											}
										pMuxSocket->close();
										delete pEntry->pHead->pQueueSend;
										oChannelList.remove(pMuxSocket);
										delete pMuxSocket;
										nUser--;
										oInfoService.setLevel(nUser,-1,-1);
									}
								else if(ret==MIXPACKET_SIZE)
									{
										if(oMixPacket.flags==CHANNEL_CLOSE)
											{
												fmChannelListEntry* pEntry;
												pEntry=oChannelList.get(pMuxSocket,oMixPacket.channel);
												if(pEntry!=NULL)
													{
														muxOut.close(pEntry->channelOut,tmpBuff);
														oqueueMixOut.add(tmpBuff,MIXPACKET_SIZE);
                            delete pEntry->pCipher;
														oChannelList.remove(pMuxSocket,oMixPacket.channel);
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
												pEntry=oChannelList.get(pMuxSocket,oMixPacket.channel);
												if(pEntry!=NULL)
													{
														oMixPacket.channel=pEntry->channelOut;
														pCipher=pEntry->pCipher;
														pCipher->decryptAES(oMixPacket.data,oMixPacket.data,DATA_SIZE);
													}
												else
													{
														pCipher= new CASymCipher();
														mRSA.decrypt(oMixPacket.data,rsaBuff);
														pCipher->setKeyAES(rsaBuff);
														pCipher->decryptAES(oMixPacket.data+RSA_SIZE,
																						 oMixPacket.data+RSA_SIZE-KEY_SIZE,
																						 DATA_SIZE-RSA_SIZE);
														memcpy(oMixPacket.data,rsaBuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);

														oChannelList.add(pMuxSocket,oMixPacket.channel,lastChannelId,pCipher);
														#ifdef _DEBUG
															CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",lastChannelId);
														#endif
														oMixPacket.channel=lastChannelId++;
													}
												muxOut.send(&oMixPacket,tmpBuff);
												oqueueMixOut.add(tmpBuff,MIXPACKET_SIZE);
												m_MixedPackets++;
											}
									}
							}
						pHashEntry=oChannelList.getNext();
					}

//Third step
//Sending to next mix
				countRead=nUser+1;
				while(countRead>0&&!oqueueMixOut.isEmpty()&&osocketgroupMixOut.select(true,0)==1)
					{
						bAktiv=true;
						countRead--;
						UINT32 len=MIXPACKET_SIZE;
						oqueueMixOut.peek(tmpBuff,&len);
						ret=((CASocket*)muxOut)->send(tmpBuff,len);
						if(ret>0)
							{
								oqueueMixOut.remove((UINT32*)&ret);
							}
						else if(ret==E_AGAIN)
							break;
						else
							goto ERR;
					}

//Step 4
//Reading from Mix				
				countRead=nUser+1;
				while(countRead>0&&osocketgroupMixOut.select(false,0)==1)
					{
						bAktiv=true;
						countRead--;
						ret=muxOut.receive(&oMixPacket,0);
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Mux-Out-Channel Receiving Data Error - Exiting!\n");
								goto ERR;
							}
						if(ret!=MIXPACKET_SIZE)
							break;
						if(oMixPacket.flags==CHANNEL_CLOSE) //close event
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ... ",oMixPacket.channel);
								#endif
								fmChannelList* pEntry=oChannelList.get(oMixPacket.channel);
								if(pEntry!=NULL)
									{
										pEntry->pHead->pMuxSocket->close(pEntry->channelIn,tmpBuff);
										pEntry->pHead->pQueueSend->add(tmpBuff,MIXPACKET_SIZE);
										osocketgroupUsersWrite.add(*pEntry->pHead->pMuxSocket);
										delete pEntry->pCipher;
										oChannelList.remove(pEntry->pHead->pMuxSocket,pEntry->channelIn);
									}
							}
						else
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!");
								#endif
								fmChannelList* pEntry=oChannelList.get(oMixPacket.channel);
								if(pEntry!=NULL)
									{
										oMixPacket.channel=pEntry->channelIn;
										pEntry->pCipher->decryptAES2(oMixPacket.data,oMixPacket.data,DATA_SIZE);
										
										pEntry->pHead->pMuxSocket->send(&oMixPacket,tmpBuff);
										pEntry->pHead->pQueueSend->add(tmpBuff,MIXPACKET_SIZE);
										osocketgroupUsersWrite.add(*pEntry->pHead->pMuxSocket);
#define MAX_USER_SEND_QUEUE 100000
										if(pEntry->pHead->pQueueSend->getSize()>MAX_USER_SEND_QUEUE&&
												!pEntry->bIsSuspended)
											{
												oMixPacket.channel=pEntry->channelOut;
												oMixPacket.flags=CHANNEL_SUSPEND;
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_INFO,"Sending suspend for channel: %u\n",oMixPacket.channel);
												#endif
												muxOut.send(&oMixPacket,tmpBuff);
												oqueueMixOut.add(tmpBuff,MIXPACKET_SIZE);
												
												pEntry->bIsSuspended=true;
												pEntry->pHead->cSuspend++;
											}
										m_MixedPackets++;
									}
								else
									{
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- Channel-Id %u no valid!\n",oMixPacket.channel);
										#endif
									}
							}
					}

//Step 5 
//Writing to users...
				fmHashTableEntry* pfmHashEntry=oChannelList.getFirst();
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
												pEntry=oChannelList.getFirstChannelForSocket(pfmHashEntry->pMuxSocket);
												while(pEntry!=NULL)
													{
														if(pEntry->bIsSuspended)
															{
																oMixPacket.flags=CHANNEL_RESUME;
																oMixPacket.channel=pEntry->channelOut;
																muxOut.send(&oMixPacket,tmpBuff);
																oqueueMixOut.add(tmpBuff,MIXPACKET_SIZE);
															}
														pEntry=oChannelList.getNextChannel(pEntry);
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
						pfmHashEntry=oChannelList.getNext();
					}
				if(!bAktiv)
					msleep(100);
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		muxOut.close();
		fmHashTableEntry* pHashEntry=oChannelList.getFirst();
		while(pHashEntry!=NULL)
			{
				pHashEntry->pMuxSocket->close();
				delete pHashEntry->pMuxSocket;
				delete pHashEntry->pQueueSend;

				fmChannelListEntry* pEntry=oChannelList.getFirstChannelForSocket(pHashEntry->pMuxSocket);
				while(pEntry!=NULL)
					{
						delete pEntry->pCipher;
						pEntry=oChannelList.getNextChannel(pEntry);
					}
				pHashEntry=oChannelList.getNext();
			}

		return E_UNKNOWN;
	}






SINT32 CAFirstMix::clean()
	{
		m_socketIn.close();
    m_socketHttpsIn.close();
    muxOut.close();
		mRSA.destroy();
		delete m_pIPList;
		m_pIPList=NULL;
		return E_SUCCESS;
	}
/*
void CAFirstMix::resume(CASocket* pSocket)
	{
		EnterCriticalSection(&csResume);
		MUXLISTENTRY* pml=oSuspendList.getFirst();
		while(pml!=NULL)
			{
				if(((SOCKET)*(pml->pMuxSocket))==((SOCKET)*pSocket))
					{
						CONNECTION* pcon=pml->pSocketList->getFirst();
						while(pcon!=NULL)
							{
								MIXPACKET oMixPacket;
								oMixPacket.flags=CHANNEL_RESUME;
								oMixPacket.channel=pcon->outChannel;
								muxOut.send(&oMixPacket);
								pcon=pml->pSocketList->getNext();
							}
						MUXLISTENTRY oEntry;
						oSuspendList.remove(pml->pMuxSocket,&oEntry);
						delete oEntry.pSocketList;
						LeaveCriticalSection(&csResume);
						return;
					}
				pml=oSuspendList.getNext();
			}
		LeaveCriticalSection(&csResume);
	}

void CAFirstMix::deleteResume(CAMuxSocket* pSocket)
	{
		EnterCriticalSection(&csResume);
		MUXLISTENTRY oEntry;
		if(oSuspendList.remove(pSocket,&oEntry))
			delete oEntry.pSocketList;
		LeaveCriticalSection(&csResume);
	}

void CAFirstMix::deleteResume(CAMuxSocket* pSocket,HCHANNEL outChannel)
	{
		EnterCriticalSection(&csResume);
		oSuspendList.remove(outChannel,NULL);
		LeaveCriticalSection(&csResume);
	}
*/
