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
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CAMuxChannelList.hpp"
#include "CAASymCipher.hpp"
#include "CAInfoService.hpp"

extern CACmdLnOptions options;


#ifndef PROT2
SINT32 CAFirstMix::init()
	{
		CASocketAddr socketAddrIn(options.getServerPort());
		socketIn.create();
		socketIn.setReuseAddr(true);
		if(socketIn.listen(&socketAddrIn)==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
					return E_UNKNOWN;
		    }
		
		
		CASocketAddr addrNext;
		UINT8 strTarget[255];
		options.getTargetHost(strTarget,255);
		addrNext.setAddr((char*)strTarget,options.getTargetPort());
		CAMsg::printMsg(LOG_INFO,"Try connectiong to next Mix... %s:%u\n",strTarget,options.getTargetPort());
		((CASocket*)muxOut)->create();
		((CASocket*)muxOut)->setSendBuff(50*sizeof(MUXPACKET));
		((CASocket*)muxOut)->setRecvBuff(50*sizeof(MUXPACKET));
		if(muxOut.connect(&addrNext,10,10)==E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO," connected!\n");
				UINT16 len;
				((CASocket*)muxOut)->receive((UINT8*)&len,2);
				CAMsg::printMsg(LOG_CRIT,"Received Key Info lenght %u\n",ntohs(len));
				recvBuff=new unsigned char[ntohs(len)+2];
				memcpy(recvBuff,&len,2);
				CAMsg::printMsg(LOG_DEBUG,"Try to receive the rest..\n");
				((CASocket*)muxOut)->receive(recvBuff+2,ntohs(len));
				CAMsg::printMsg(LOG_CRIT,"Received Key Info!\n");
				return E_SUCCESS;
			}
		else
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix!\n");
				return E_UNKNOWN;
			}
	}

SINT32 CAFirstMix::loop()
	{
		CAMuxChannelList  oMuxChannelList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(socketIn);
		oSocketGroup.add(muxOut);
		HCHANNEL lastChannelId=1;
		MUXPACKET oMuxPacket;
		CONNECTION oConnection;
		int len;
		CAMuxSocket* newMuxSocket;
		MUXLISTENTRY* tmpEntry;
		REVERSEMUXLISTENTRY* tmpReverseEntry;
		UINT8 buff[RSA_SIZE];
		int countRead=0;
		CAASymCipher oRSA;
		oRSA.generateKeyPair(1024);
		UINT32 keySize=oRSA.getPublicKeySize();
		UINT16 infoSize=ntohs((*(UINT16*)recvBuff))+2;
		UINT8* infoBuff=new UINT8[infoSize+keySize]; 
		memcpy(infoBuff,recvBuff,infoSize);
		infoBuff[2]++; //chainlen erhoehen
		oRSA.getPublicKey(infoBuff+infoSize,&keySize);
		infoSize+=keySize;
		(*(UINT16*)infoBuff)=htons(infoSize-2);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n",infoSize);
		#endif
		#ifdef _DEBUG
		 CAMsg::printMsg(LOG_DEBUG,"Size of MuxPacket: %u\n",sizeof(oMuxPacket));
		 CAMsg::printMsg(LOG_DEBUG,"Pointer: %p,%p,%p,%p\n",&oMuxPacket.channel,&oMuxPacket.len,&oMuxPacket.type,&oMuxPacket.data);
		#endif
		CAInfoService oInfoService;
		// reading SingKey....
		UINT8* fileBuff=new UINT8[2048];
		options.getKeyFileName(fileBuff,2048);
		int handle=open((char*)fileBuff,O_BINARY|O_RDONLY);
		if(handle==-1)
			return E_UNKNOWN;
		len=read(handle,fileBuff,2048);
		close(handle);
		CASignature oSignature;
		if(oSignature.setSignKey(fileBuff,len,SIGKEY_XML)==-1)
			{
				delete fileBuff;
				return E_UNKNOWN;
			}
		delete fileBuff;
		oInfoService.setSignature(&oSignature);
		oInfoService.setLevel(0,-1,-1);
		oInfoService.sendHelo();
		oInfoService.start();
		int nUser=0;
		for(;;)
			{
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
							CAMsg::printMsg(LOG_DEBUG,"New Connection from Browser!\n");
						#endif
						newMuxSocket=new CAMuxSocket;
						if(socketIn.accept(*(CASocket*)newMuxSocket)==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_ERR,"Accept Error %u - Connection from Browser!\n",errno);
								delete newMuxSocket;
							}
						else
							{
								#ifdef _DEBUG
									int ret=((CASocket*)newMuxSocket)->setKeepAlive(true);
									if(ret==SOCKET_ERROR)
										CAMsg::printMsg(LOG_DEBUG,"Fehler bei KeepAlive!");
								#else
									((CASocket*)newMuxSocket)->setKeepAlive(true);
								#endif
								((CASocket*)newMuxSocket)->send(infoBuff,infoSize);
								oMuxChannelList.add(newMuxSocket);
								nUser++;
								oInfoService.setLevel(nUser,-1,-1);
								oSocketGroup.add(*newMuxSocket);
							}
					}
				if(oSocketGroup.isSignaled(muxOut))
						{
							len=muxOut.receive(&oMuxPacket);
							if(len==0)
								{
									#ifdef _DEBUG
										CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ... ",oMuxPacket.channel);
									#endif
									REVERSEMUXLISTENTRY otmpReverseEntry;
									if(oMuxChannelList.remove(oMuxPacket.channel,&otmpReverseEntry))
										{
											otmpReverseEntry.pMuxSocket->close(otmpReverseEntry.inChannel);
											delete otmpReverseEntry.pCipher;
											#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"closed!\n");
											#endif
										}
								}
							else if(len==SOCKET_ERROR)
								{
									CAMsg::printMsg(LOG_CRIT,"Mux-Channel Receiving Data Error - Exiting!\n");									
									exit(-1);
								}
							else
								{
									#ifdef _DEBUG
										CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!");
									#endif
									tmpReverseEntry=oMuxChannelList.get(oMuxPacket.channel);
									if(tmpReverseEntry!=NULL)
										{
											oMuxPacket.channel=tmpReverseEntry->inChannel;
#ifndef AES
											tmpReverseEntry->pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#else
											tmpReverseEntry->pCipher->decryptAES((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#endif
											tmpReverseEntry->pMuxSocket->send(&oMuxPacket);
										}
									else
										{
											CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- Channel-Id %u no valid!\n",oMuxPacket.channel);										
										}
								}
						}
			//	if(oSocketGroup.isSignaled(fmIOPair->muxHttpIn))
			//		{
			//			countRead--;
			//			len=fmIOPair->muxHttpIn.receive(&oMuxPacket);
			//			printf("Receivde Htpp-Packet - Len: %u Content %s",len,oMuxPacket.data); 
			/*			if(len==SOCKET_ERROR)
							{
								MUXLISTENTRY otmpEntry;
								if(oMuxChannelList.remove(tmpEntry->pMuxSocket,&otmpEntry))
									{
										oSocketGroup.remove(*(CASocket*)otmpEntry.pMuxSocket);
										CONNECTION* tmpCon=otmpEntry.pSocketList->getFirst();
										while(tmpCon!=NULL)
											{
												fmIOPair->muxOut.close(tmpCon->outChannel);
												tmpCon=otmpEntry.pSocketList->getNext();
											}
										otmpEntry.pMuxSocket->close();
										delete otmpEntry.pMuxSocket;
										delete otmpEntry.pSocketList;
									}
							}
						else
							{
								if(len==0)
									{
										if(oMuxChannelList.get(tmpEntry,oMuxPacket.channel,&outChannel))
											{
												fmIOPair->muxOut.close(outChannel);
												oMuxChannelList.remove(outChannel,NULL);
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
										if(oMuxChannelList.get(tmpEntry,oMuxPacket.channel,&outChannel))
											{
												oMuxPacket.channel=outChannel;
											}
										else
											{
												oMuxChannelList.add(tmpEntry,oMuxPacket.channel,lastChannelId);
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",lastChannelId);
												#endif
												oMuxPacket.channel=lastChannelId++;
											}
										if(fmIOPair->muxOut.send(&oMuxPacket)==SOCKET_ERROR)
											{
												CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Exiting!\n");									
												exit(-1);
											}
								}
						}
						*/
				//	}
				if(countRead>0)
					{
						tmpEntry=oMuxChannelList.getFirst();
						while(tmpEntry!=NULL&&countRead>0)
							{
								if(oSocketGroup.isSignaled(*tmpEntry->pMuxSocket))
									{
										countRead--;
										len=tmpEntry->pMuxSocket->receive(&oMuxPacket);
										if(len==SOCKET_ERROR)
											{
												MUXLISTENTRY otmpEntry;
												if(oMuxChannelList.remove(tmpEntry->pMuxSocket,&otmpEntry))
													{
														oSocketGroup.remove(*(CASocket*)otmpEntry.pMuxSocket);
														CONNECTION* tmpCon=otmpEntry.pSocketList->getFirst();
														while(tmpCon!=NULL)
															{
																muxOut.close(tmpCon->outChannel);
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
										else
											{
												if(len==0)
													{
														if(oMuxChannelList.get(tmpEntry,oMuxPacket.channel,&oConnection))
															{
																muxOut.close(oConnection.outChannel);
																oMuxChannelList.remove(oConnection.outChannel,NULL);
																delete oConnection.pCipher;
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
														if(oMuxChannelList.get(tmpEntry,oMuxPacket.channel,&oConnection))
															{
																oMuxPacket.channel=oConnection.outChannel;
																pCipher=oConnection.pCipher;
#ifndef AES
																pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#else
																pCipher->decryptAES((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#endif
														}
														else
															{
																pCipher= new CASymCipher();
																oRSA.decrypt((unsigned char*)oMuxPacket.data,buff);
#ifndef AES
																pCipher->setDecryptionKey(buff);
//																pCipher->setEncryptionKey(buff);
																pCipher->decrypt((unsigned char*)oMuxPacket.data+RSA_SIZE,
																								 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																								 DATA_SIZE-RSA_SIZE);
#else
																pCipher->setDecryptionKeyAES(buff);
//																pCipher->setEncryptionKeyAES(buff);
																pCipher->decryptAES((unsigned char*)oMuxPacket.data+RSA_SIZE,
																								 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																								 DATA_SIZE-RSA_SIZE);
#endif
																memcpy(oMuxPacket.data,buff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
																
																oMuxChannelList.add(tmpEntry,oMuxPacket.channel,lastChannelId,pCipher);
																#ifdef _DEBUG
																	CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",lastChannelId);
																#endif
																oMuxPacket.channel=lastChannelId++;
																oMuxPacket.len=oMuxPacket.len-16;
															}
														if(muxOut.send(&oMuxPacket)==SOCKET_ERROR)
															{
																CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Exiting!\n");									
																exit(-1);
															}
												}
											}
									}
								tmpEntry=oMuxChannelList.getNext();
							}
					}
			}
		return E_SUCCESS;
	}

#else
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
		CASocketAddr addrNext;
		UINT8 strTarget[255];
		options.getTargetHost(strTarget,255);
		addrNext.setAddr((char*)strTarget,options.getTargetPort());
		CAMsg::printMsg(LOG_INFO,"Try connecting to next Mix: %s:%u ...\n",strTarget,options.getTargetPort());
		if(((CASocket*)muxOut)->create()!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot create SOCKET for connection to next Mix!\n");
				return E_UNKNOWN;
			}
		((CASocket*)muxOut)->setSendBuff(50*MUXPACKET_SIZE);
		((CASocket*)muxOut)->setRecvBuff(50*MUXPACKET_SIZE);
		if(muxOut.connect(&addrNext,10,10)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix!\n");
				return E_UNKNOWN;
			}
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
		
		CASocketAddr socketAddrIn(options.getServerPort());
		socketIn.create();
		socketIn.setReuseAddr(true);
		if(socketIn.listen(&socketAddrIn)==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
					return E_UNKNOWN;
		    }
		return E_SUCCESS;
	}


SINT32 CAFirstMix::loop()
	{
		CAMuxChannelList  oMuxChannelList;
		REVERSEMUXLISTENTRY* tmpReverseEntry;
		MUXLISTENTRY* tmpMuxListEntry;
	
		CASocketGroup oSocketGroup;
		CASocketGroup oSocketGroupMuxOut;
		CAMuxSocket* pnewMuxSocket;
		SINT32 countRead;
		HCHANNEL lastChannelId=1;
		MUXPACKET oMuxPacket;
		CONNECTION oConnection;
		CAInfoService oInfoService;
		UINT32 nUser=0;
		SINT32 ret;
		UINT8 rsaBuff[RSA_SIZE];

		oSocketGroup.add(socketIn);
		oSocketGroup.add(muxOut);
		oSocketGroupMuxOut.add(muxOut);

		oInfoService.setSignature(&mSignature);
		oInfoService.setLevel(0,-1,-1);
		oInfoService.sendHelo();
		oInfoService.start();

		for(;;)
			{
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
							CAMsg::printMsg(LOG_DEBUG,"New Connection from Browser!\n");
						#endif
						pnewMuxSocket=new CAMuxSocket;
						if(socketIn.accept(*(CASocket*)pnewMuxSocket)==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_ERR,"Accept Error %u - Connection from Browser!\n",errno);
								delete pnewMuxSocket;
							}
						else
							{
								#ifdef _DEBUG
									int ret=((CASocket*)pnewMuxSocket)->setKeepAlive(true);
									if(ret!=E_SUCCESS)
										CAMsg::printMsg(LOG_DEBUG,"Fehler bei KeepAlive!");
								#else
									((CASocket*)pnewMuxSocket)->setKeepAlive(true);
								#endif
#ifdef _ASYNC
								((CASocket*)pnewMuxSocket)->setASyncSend(true,MUXPACKET_SIZE,this);
#endif
								((CASocket*)pnewMuxSocket)->send(mKeyInfoBuff,mKeyInfoSize);
								oMuxChannelList.add(pnewMuxSocket);
								nUser++;
								oInfoService.setLevel(nUser,-1,-1);
								oSocketGroup.add(*pnewMuxSocket);
							}
					}
				if(oSocketGroup.isSignaled(muxOut))
					{
						int countMuxOut=nUser;
						do
							{
								ret=muxOut.receive(&oMuxPacket);
								if(ret==SOCKET_ERROR)
									{
										CAMsg::printMsg(LOG_CRIT,"Mux-Out-Channel Receiving Data Error - Exiting!\n");									
										goto ERR;
									}
								if(oMuxPacket.flags==CHANNEL_CLOSE) //close event
									{
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ... ",oMuxPacket.channel);
										#endif
										REVERSEMUXLISTENTRY otmpReverseEntry;
										if(oMuxChannelList.remove(oMuxPacket.channel,&otmpReverseEntry))
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
										tmpReverseEntry=oMuxChannelList.get(oMuxPacket.channel);
										if(tmpReverseEntry!=NULL)
											{
												oMuxPacket.channel=tmpReverseEntry->inChannel;
												tmpReverseEntry->pCipher->decryptAES(oMuxPacket.data,oMuxPacket.data,DATA_SIZE);
												if(tmpReverseEntry->pMuxSocket->send(&oMuxPacket)==E_QUEUEFULL)
													{
														EnterCriticalSection(&csResume);
														MUXLISTENTRY* pml=oSuspendList.get(tmpReverseEntry->pMuxSocket);
														CONNECTION oCon;
														if(pml==NULL||!pml->pSocketList->get(&oCon,tmpReverseEntry->outChannel))
															{
																oMuxPacket.channel=tmpReverseEntry->outChannel;
																oMuxPacket.flags=CHANNEL_SUSPEND;
																CAMsg::printMsg(LOG_INFO,"Sending suspend for channel: %u\n",oMuxPacket.channel);
																muxOut.send(&oMuxPacket);
																if(pml==NULL)
																	{
																		oSuspendList.add(tmpReverseEntry->pMuxSocket);
																		pml=oSuspendList.get(tmpReverseEntry->pMuxSocket);
																	}
																pml->pSocketList->add(tmpReverseEntry->inChannel,tmpReverseEntry->outChannel,NULL);
															}
														LeaveCriticalSection(&csResume);
													}
											}
										else
											{
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- Channel-Id %u no valid!\n",oMuxPacket.channel);										
												#endif
											}
							}
						countMuxOut--;
					}while(oSocketGroupMuxOut.select(false,0)==1&&countMuxOut>0);
				}
			//	if(oSocketGroup.isSignaled(fmIOPair->muxHttpIn))
			//		{
			//			countRead--;
			//			len=fmIOPair->muxHttpIn.receive(&oMuxPacket);
			//			printf("Receivde Htpp-Packet - Len: %u Content %s",len,oMuxPacket.data); 
			/*			if(len==SOCKET_ERROR)
							{
								MUXLISTENTRY otmpEntry;
								if(oMuxChannelList.remove(tmpEntry->pMuxSocket,&otmpEntry))
									{
										oSocketGroup.remove(*(CASocket*)otmpEntry.pMuxSocket);
										CONNECTION* tmpCon=otmpEntry.pSocketList->getFirst();
										while(tmpCon!=NULL)
											{
												fmIOPair->muxOut.close(tmpCon->outChannel);
												tmpCon=otmpEntry.pSocketList->getNext();
											}
										otmpEntry.pMuxSocket->close();
										delete otmpEntry.pMuxSocket;
										delete otmpEntry.pSocketList;
									}
							}
						else
							{
								if(len==0)
									{
										if(oMuxChannelList.get(tmpEntry,oMuxPacket.channel,&outChannel))
											{
												fmIOPair->muxOut.close(outChannel);
												oMuxChannelList.remove(outChannel,NULL);
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
										if(oMuxChannelList.get(tmpEntry,oMuxPacket.channel,&outChannel))
											{
												oMuxPacket.channel=outChannel;
											}
										else
											{
												oMuxChannelList.add(tmpEntry,oMuxPacket.channel,lastChannelId);
												#ifdef _DEBUG
													CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",lastChannelId);
												#endif
												oMuxPacket.channel=lastChannelId++;
											}
										if(fmIOPair->muxOut.send(&oMuxPacket)==SOCKET_ERROR)
											{
												CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Exiting!\n");									
												exit(-1);
											}
								}
						}
						*/
				//	}
				if(countRead>0)
					{
						tmpMuxListEntry=oMuxChannelList.getFirst();
						while(tmpMuxListEntry!=NULL&&countRead>0)
							{
								if(oSocketGroup.isSignaled(*tmpMuxListEntry->pMuxSocket))
									{
										countRead--;
										ret=tmpMuxListEntry->pMuxSocket->receive(&oMuxPacket,0);
										if(ret==SOCKET_ERROR)
											{
												deleteResume(tmpMuxListEntry->pMuxSocket);
												MUXLISTENTRY otmpEntry;
												if(oMuxChannelList.remove(tmpMuxListEntry->pMuxSocket,&otmpEntry))
													{
														oSocketGroup.remove(*(CASocket*)otmpEntry.pMuxSocket);
														CONNECTION* tmpCon=otmpEntry.pSocketList->getFirst();
														while(tmpCon!=NULL)
															{
																muxOut.close(tmpCon->outChannel);
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
												if(oMuxPacket.flags==CHANNEL_CLOSE)
													{
														if(oMuxChannelList.get(tmpMuxListEntry,oMuxPacket.channel,&oConnection))
															{
																muxOut.close(oConnection.outChannel);
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
														if(oMuxChannelList.get(tmpMuxListEntry,oMuxPacket.channel,&oConnection))
															{
																oMuxPacket.channel=oConnection.outChannel;
																pCipher=oConnection.pCipher;
																pCipher->decryptAES((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
															}
														else
															{
																pCipher= new CASymCipher();
																mRSA.decrypt(oMuxPacket.data,rsaBuff);
																pCipher->setDecryptionKeyAES(rsaBuff);
																pCipher->decryptAES(oMuxPacket.data+RSA_SIZE,
																								 oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																								 DATA_SIZE-RSA_SIZE);
																memcpy(oMuxPacket.data,rsaBuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
																
																oMuxChannelList.add(tmpMuxListEntry,oMuxPacket.channel,lastChannelId,pCipher);
																#ifdef _DEBUG
																	CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",lastChannelId);
																#endif
																oMuxPacket.channel=lastChannelId++;
															}
														if(muxOut.send(&oMuxPacket)==SOCKET_ERROR)
															{
																CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Restarting!\n");									
																goto ERR;
															}
												}
											}
									}
								tmpMuxListEntry=oMuxChannelList.getNext();
							}
					}
			}
ERR:
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

SINT32 CAFirstMix::clean()
	{
		socketIn.close();
		muxOut.close();
		mRSA.destroy();
		oSuspendList.clear();
		return E_SUCCESS;
	}

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
								MUXPACKET oMuxPacket;
								oMuxPacket.flags=CHANNEL_RESUME;
								oMuxPacket.channel=pcon->outChannel;
								muxOut.send(&oMuxPacket);
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
#endif