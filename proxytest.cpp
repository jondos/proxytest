// proxytest.cpp : Definiert den Einsprungpunkt für die Konsolenanwendung.
//

#include "StdAfx.h"
#include "CASocket.hpp"
#include "CASocketGroup.hpp"
#include "CASocketAddr.hpp"
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"
#include "CAMuxSocket.hpp"
#include "CASocketList.hpp"
#include "CAMuxChannelList.hpp"
#include "CASymCipher.hpp"
#include "CAASymCipher.hpp"
#include "CAInfoService.hpp"
//#ifdef _WIN32
//HANDLE hEventThreadEnde;
//#endif
CACmdLnOptions options;

//CRITICAL_SECTION csClose;
typedef struct
{
	unsigned short len;
	time_t time;
} log;

CASocketAddr socketAddrSquid;
#ifdef _DEBUG
int sockets;
#endif

#ifndef _WIN32
	#ifdef _DEBUG 
		void signal_broken_pipe( int sig)
			{
				CAMsg::printMsg(LOG_WARNING,"Hm.. Broken Pipe.... How cares!\n");
				signal(SIGPIPE,signal_broken_pipe);
			}
	#endif
#endif

typedef struct t_LPPair
	{
		CASocket socketIn;
		CASocket socketSOCKSIn;
		CAMuxSocket muxOut;
		unsigned char chainlen;
		CAASymCipher* arRSA;
	} LPPair;

THREAD_RETURN lpIO(void *v)
	{
		LPPair* lpIOPair=(LPPair*)v;
		CASocketList  oSocketList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(lpIOPair->socketIn);
		if(options.getSOCKSServerPort()!=-1)
			oSocketGroup.add(lpIOPair->socketSOCKSIn);
		oSocketGroup.add(lpIOPair->muxOut);
		HCHANNEL lastChannelId=1;
		MUXPACKET oMuxPacket;
		memset(&oMuxPacket,0,sizeof(oMuxPacket));
		int len,ret;
		CASocket* newSocket;//,*tmpSocket;
		CASymCipher* newCipher;
		int countRead;
		CONNECTION oConnection;		
		unsigned char chainlen=lpIOPair->chainlen;
		unsigned short socksPort=options.getSOCKSServerPort();
		bool bHaveSocks=(socksPort!=0xFFFF);
		for(;;)
			{
				if((countRead=oSocketGroup.select())==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(lpIOPair->socketIn))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New Connection from Browser!\n");
						#endif
						newSocket=new CASocket;
						if(lpIOPair->socketIn.accept(*newSocket)==SOCKET_ERROR)
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Accept Error - Connection from Browser!\n");
								#endif
								delete newSocket;
							}
						else
							{
								newCipher=new CASymCipher();
								oSocketList.add(lastChannelId++,newSocket,newCipher);
								oSocketGroup.add(*newSocket);
							}
					}
				if(bHaveSocks&&oSocketGroup.isSignaled(lpIOPair->socketSOCKSIn))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New Connection from SOCKS!\n");
						#endif
						newSocket=new CASocket;
						if(lpIOPair->socketSOCKSIn.accept(*newSocket)==SOCKET_ERROR)
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Accept Error - Connection from SOCKS!\n");
								#endif
								delete newSocket;
							}
						else
							{
								newCipher=new CASymCipher();
								oSocketList.add(lastChannelId++,newSocket,newCipher);
								oSocketGroup.add(*newSocket);
							}
					}
				if(oSocketGroup.isSignaled(lpIOPair->muxOut))
						{
							countRead--;	
							ret=lpIOPair->muxOut.receive(&oMuxPacket);
							if(ret==SOCKET_ERROR)
								{
									CAMsg::printMsg(LOG_CRIT,"Mux-Channel Receiving Data Error - Exiting!\n");									
									exit(-1);
								}

							if(!oSocketList.get(oMuxPacket.channel,&oConnection))
								{
									CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- Channel-Id no valid!\n");										
								}
							else
								{
									for(int c=0;c<chainlen;c++)
										oConnection.pCipher->encrypt((unsigned char*)oMuxPacket.data,DATA_SIZE);
									len=oMuxPacket.len;
									if(len==0)
										{
											#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ... ",oMuxPacket.channel);
											#endif
											//TODO: deleting cipher...
											/*tmpSocket=*/oSocketList.remove(oMuxPacket.channel);
											if(oConnection.pSocket!=NULL)
												{
													oSocketGroup.remove(*oConnection.pSocket);
													oConnection.pSocket->close();
													#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"closed!\n");
													#endif
													delete oConnection.pSocket;
													delete oConnection.pCipher;
												}
										}
									else
										{
											#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!");
											#endif
											oConnection.pSocket->send(oMuxPacket.data,len);
										}
								}
						}
				if(countRead>0)
					{
						CONNECTION* tmpCon;
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL&&countRead>0)
							{
								if(oSocketGroup.isSignaled(*tmpCon->pSocket))
									{
										countRead--;
										if(!tmpCon->pCipher->isEncyptionKeyValid())
											len=tmpCon->pSocket->receive(oMuxPacket.data,DATA_SIZE-chainlen*16);
										else
											len=tmpCon->pSocket->receive(oMuxPacket.data,DATA_SIZE);
										if(len==SOCKET_ERROR||len==0)
											{
												//TODO delete cipher..
												CASocket* tmpSocket=oSocketList.remove(tmpCon->id);
												if(tmpSocket!=NULL)
													{
														oSocketGroup.remove(*tmpSocket);
														lpIOPair->muxOut.close(tmpCon->id);
														tmpSocket->close();
														delete tmpSocket;
													}
											}
										else 
											{
												oMuxPacket.channel=tmpCon->id;
												oMuxPacket.len=(unsigned short)len;
												if(bHaveSocks&&tmpCon->pSocket->getLocalPort()==socksPort)
													{
														oMuxPacket.type=MUX_SOCKS;
													}
												else
													{
														oMuxPacket.type=MUX_HTTP;
													}
												if(!tmpCon->pCipher->isEncyptionKeyValid()) //First time --> rsa key
													{
														//Has to bee optimized!!!!
														unsigned char buff[DATA_SIZE];
														int size=DATA_SIZE-16;
														tmpCon->pCipher->generateEncryptionKey(); //generate Key
														for(int c=0;c<chainlen;c++)
															{
																tmpCon->pCipher->getEncryptionKey(buff); // get key...
																memcpy(buff+KEY_SIZE,oMuxPacket.data,size);
																lpIOPair->arRSA[c].encrypt(buff,buff);
																tmpCon->pCipher->encrypt(buff+RSA_SIZE,DATA_SIZE-RSA_SIZE);
																memcpy(oMuxPacket.data,buff,DATA_SIZE);
																size-=KEY_SIZE;
																len+=KEY_SIZE;
															}
														oMuxPacket.len=len;
													}
												else //sonst
													{
														for(int c=0;c<chainlen;c++)
															tmpCon->pCipher->encrypt((unsigned char*)oMuxPacket.data,DATA_SIZE);
													}
												if(lpIOPair->muxOut.send(&oMuxPacket)==SOCKET_ERROR)
													{
														CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Exiting!\n");									
														exit(-1);
													}
											}
										break;
									}
								tmpCon=oSocketList.getNext();
							}
					}
			}
		THREAD_RETURN_SUCCESS;
	}

int doLocalProxy()
	{
		int ret;	
		CASocketAddr socketAddrIn("127.0.0.1",options.getServerPort());
		LPPair* lpIOPair=new LPPair;
		lpIOPair->socketIn.create();
		lpIOPair->socketIn.setReuseAddr(true);
		if(lpIOPair->socketIn.listen(&socketAddrIn)==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
					return -1;
		    }
		if(options.getSOCKSServerPort()!=-1)
			{
				socketAddrIn.setAddr("127.0.0.1",options.getSOCKSServerPort());
				lpIOPair->socketSOCKSIn.create();
				lpIOPair->socketSOCKSIn.setReuseAddr(true);
				if(lpIOPair->socketSOCKSIn.listen(&socketAddrIn)==SOCKET_ERROR)
						{
							CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
							return -1;
						}
			}
		CASocketAddr addrNext;
		char strTarget[255];
		options.getTargetHost(strTarget,255);
		addrNext.setAddr(strTarget,options.getTargetPort());
		CAMsg::printMsg(LOG_INFO,"Try connectiong to next Mix...");

/*		CAMuxSocket http;
		http.useTunnel("anon.inf.tu-dresden.de",3128);
		http.connect(&addrNext);
		MUXPACKET oMuxPacket;
		oMuxPacket.channel=1;
		oMuxPacket.len=10;
		memcpy(oMuxPacket.data,"Hllo",3);
		http.send(&oMuxPacket);
		http.close();
		sleep(10);
		return -1;
*/		((CASocket*)lpIOPair->muxOut)->create();
		((CASocket*)lpIOPair->muxOut)->setSendBuff(sizeof(MUXPACKET)*50);
		((CASocket*)lpIOPair->muxOut)->setRecvBuff(sizeof(MUXPACKET)*50);
		if(lpIOPair->muxOut.connect(&addrNext)==E_SUCCESS)
			{
				
				CAMsg::printMsg(LOG_INFO," connected!\n");
				unsigned short size;
				((CASocket*)lpIOPair->muxOut)->receive((char*)&size,2);
				((CASocket*)lpIOPair->muxOut)->receive((char*)&lpIOPair->chainlen,1);
				CAMsg::printMsg(LOG_INFO,"Chain-Length: %d\n",lpIOPair->chainlen);
				size=ntohs(size)-1;
				unsigned char* buff=new unsigned char[size];
				((CASocket*)lpIOPair->muxOut)->receive((char*)buff,size);
				lpIOPair->arRSA=new CAASymCipher[lpIOPair->chainlen];
				int aktIndex=0;
				for(int i=0;i<lpIOPair->chainlen;i++)
					{
						int len=size;
						lpIOPair->arRSA[i].setPublicKey(buff+aktIndex,&len);
						size-=len;
						aktIndex+=len;
					}
				
				lpIO(lpIOPair);
				ret=0;
			}
		else
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix!\n");
				ret=-1;
			}
	//	WaitForSingleObject(hEventThreadEnde,INFINITE);
		delete lpIOPair;
		return ret;
	}


struct MMPair
	{
		CAMuxSocket muxIn;
		CAMuxSocket muxOut;
		CAASymCipher oRSA;
	};

THREAD_RETURN mmIO(void* v)
	{
		MMPair* mmIOPair=(MMPair*)v;
		CASocketList oSocketList;
		CASocketGroup oSocketGroup;
		int len;
		unsigned char buff[RSA_SIZE];
		CONNECTION oConnection;
		CONNECTION* pCon;
		MUXPACKET oMuxPacket;
		HCHANNEL lastId;
		lastId=1;
		unsigned char* recvBuff=NULL;
		unsigned char* infoBuff=NULL;
		//CAMuxSocket& muxOut=mmIOPair->muxOut;
		//CAMuxSocket& muxIn=mmIOPair->muxIn;
		
		char strTarget[255];
		options.getTargetHost(strTarget,255);
		CASocketAddr nextMix;
		nextMix.setAddr(strTarget,options.getTargetPort());
		
SET_IN:		
		((CASocket*)mmIOPair->muxOut)->create();
		((CASocket*)mmIOPair->muxOut)->setRecvBuff(50*sizeof(MUXPACKET));
		((CASocket*)mmIOPair->muxOut)->setSendBuff(50*sizeof(MUXPACKET));
		if(mmIOPair->muxOut.connect(&nextMix)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix -- Exiting!\n");
				THREAD_RETURN_ERROR;
			}
		oSocketGroup.add(mmIOPair->muxOut);

		CAMsg::printMsg(LOG_INFO," connected!\n");
		unsigned short keyLen;
		((CASocket*)mmIOPair->muxOut)->receive((char*)&keyLen,2);
		CAMsg::printMsg(LOG_INFO,"Received Key Info lenght %u\n",ntohs(keyLen));
		delete recvBuff;
		recvBuff=new unsigned char[ntohs(keyLen)+2];
		memcpy(recvBuff,&keyLen,2);
		((CASocket*)mmIOPair->muxOut)->receive((char*)recvBuff+2,ntohs(keyLen));
		CAMsg::printMsg(LOG_INFO,"Received Key Info...\n");
		
		
SET_OUT:		
		if(mmIOPair->muxIn.accept(options.getServerPort())==SOCKET_ERROR)
			{
				CAMsg::printMsg(LOG_CRIT,"Error waiting for previous Mix... -- Exiting!\n");
				THREAD_RETURN_ERROR;
			}
		((CASocket*)mmIOPair->muxIn)->setRecvBuff(50*sizeof(MUXPACKET));
		((CASocket*)mmIOPair->muxIn)->setSendBuff(50*sizeof(MUXPACKET));

		oSocketGroup.add(mmIOPair->muxIn);

		int keySize=mmIOPair->oRSA.getPublicKeySize();
		unsigned short infoSize=ntohs((*(unsigned short*)recvBuff))+2;
		delete infoBuff;
		infoBuff=new unsigned char[infoSize+keySize]; 
		memcpy(infoBuff,recvBuff,infoSize);
		infoBuff[2]++; //chainlen erhöhen
		mmIOPair->oRSA.getPublicKey(infoBuff+infoSize,&keySize);
		infoSize+=keySize;
		(*(unsigned short*)infoBuff)=htons(infoSize-2);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n",infoSize);
		#endif
		if(((CASocket*)mmIOPair->muxIn)->send((char*)infoBuff,infoSize)==-1)
			CAMsg::printMsg(LOG_DEBUG,"Error sending new New Key Info\n");
		else
			CAMsg::printMsg(LOG_DEBUG,"Sending new New Key Info succeded\n");
		
		for(;;)
			{
				if(oSocketGroup.select()==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(mmIOPair->muxIn))
					{
						len=mmIOPair->muxIn.receive(&oMuxPacket);
						if(len==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								//exit(-1);
								goto ERR_IN;
							}
						if(!oSocketList.get(oMuxPacket.channel,&oConnection))
							{
								if(len!=0)
									{
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										CASymCipher* newCipher=new CASymCipher();
										mmIOPair->oRSA.decrypt((unsigned char*)oMuxPacket.data,buff);
										newCipher->setDecryptionKey(buff);
										newCipher->setEncryptionKey(buff);
										newCipher->decrypt((unsigned char*)oMuxPacket.data+RSA_SIZE,
																			 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																			 DATA_SIZE-RSA_SIZE);
										memcpy(oMuxPacket.data,buff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
										
										oMuxPacket.len=oMuxPacket.len-16;

										oSocketList.add(oMuxPacket.channel,lastId,newCipher);
										oMuxPacket.channel=lastId;
										lastId++;
										if(mmIOPair->muxOut.send(&oMuxPacket)==SOCKET_ERROR)
											goto ERR_OUT;
									}
							}
						else
							{
								if(len==0)
									{
										mmIOPair->muxOut.close(oConnection.outChannel);
										oSocketList.remove(oMuxPacket.channel);
									}
								else
									{
										oMuxPacket.channel=oConnection.outChannel;
										oConnection.pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
										if(mmIOPair->muxOut.send(&oMuxPacket)==SOCKET_ERROR)
											goto ERR_OUT;
									}
							}
					}
				
				if(oSocketGroup.isSignaled(mmIOPair->muxOut))
					{
						len=mmIOPair->muxOut.receive(&oMuxPacket);
						if(len==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								//exit(-1);
								goto ERR_OUT;
							}
						if(oSocketList.get(&oConnection,oMuxPacket.channel))
							{
								if(len!=0)
									{
										oMuxPacket.channel=oConnection.id;
										oConnection.pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
										if(mmIOPair->muxIn.send(&oMuxPacket)==SOCKET_ERROR)
											goto ERR_IN;
									}
								else
									{
										mmIOPair->muxIn.close(oConnection.id);
										oSocketList.remove(oConnection.id);
									}
							}
						else
							mmIOPair->muxOut.close(oMuxPacket.channel);
					}
			}
ERR_IN:
		oSocketGroup.remove(*(CASocket*)mmIOPair->muxIn);
		mmIOPair->muxIn.close();
		pCon=oSocketList.getFirst();
		while(pCon!=NULL)
			{
				mmIOPair->muxOut.close(pCon->outChannel);
				oSocketList.remove(pCon->id);
				pCon=oSocketList.getNext();
			}
		goto SET_OUT;

ERR_OUT:
		oSocketGroup.remove(*(CASocket*)mmIOPair->muxOut);
		mmIOPair->muxOut.close();
		pCon=oSocketList.getFirst();
		while(pCon!=NULL)
			{
				mmIOPair->muxIn.close(pCon->id);
				oSocketList.remove(pCon->id);
				pCon=oSocketList.getNext();
			}

		oSocketGroup.remove(*(CASocket*)mmIOPair->muxIn);
		mmIOPair->muxIn.close();
		goto SET_IN;

		THREAD_RETURN_SUCCESS;
	}

int doMiddleMix()
	{
		MMPair* mmIOPair=new MMPair;
		
		CAMsg::printMsg(LOG_INFO,"Creating Key...\n");
		mmIOPair->oRSA.generateKeyPair(1024);

		mmIO(mmIOPair);
		delete mmIOPair;
		return 0;
	}

typedef struct t_FMPair
	{
		CASocket socketIn;
//		CAMuxSocket muxHttpIn;
		CAMuxSocket muxOut;
		unsigned char* recvBuff;
	} FMPair;

THREAD_RETURN fmIO(void *v)
	{
		FMPair* fmIOPair=(FMPair*)v;
		CAMuxChannelList  oMuxChannelList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(fmIOPair->socketIn);
		oSocketGroup.add(fmIOPair->muxOut);
		HCHANNEL lastChannelId=1;
		MUXPACKET oMuxPacket;
		CONNECTION oConnection;
		int len;
		CAMuxSocket* newMuxSocket;
		MUXLISTENTRY* tmpEntry;
		REVERSEMUXLISTENTRY* tmpReverseEntry;
		unsigned char buff[RSA_SIZE];
		int countRead=0;
		CAASymCipher oRSA;
		oRSA.generateKeyPair(1024);
		int keySize=oRSA.getPublicKeySize();
		unsigned short infoSize=ntohs((*(unsigned short*)fmIOPair->recvBuff))+2;
		unsigned char* infoBuff=new unsigned char[infoSize+keySize]; 
		memcpy(infoBuff,fmIOPair->recvBuff,infoSize);
		infoBuff[2]++; //chainlen erhöhen
		oRSA.getPublicKey(infoBuff+infoSize,&keySize);
		infoSize+=keySize;
		(*(unsigned short*)infoBuff)=htons(infoSize-2);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n",infoSize);
		#endif
		#ifdef _DEBUG
		 CAMsg::printMsg(LOG_DEBUG,"Size of MuxPacket: %u\n",sizeof(oMuxPacket));
		 CAMsg::printMsg(LOG_DEBUG,"Pointer: %p,%p,%p,%p\n",&oMuxPacket.channel,&oMuxPacket.len,&oMuxPacket.type,&oMuxPacket.data);
		#endif
		CAInfoService oInfoService;
		oInfoService.setLevel(0,0,0);
		oInfoService.start();
		int nUser=0;
		for(;;)
			{
				if((countRead=oSocketGroup.select())==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(fmIOPair->socketIn))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New Connection from Browser!\n");
						#endif
						newMuxSocket=new CAMuxSocket;
						if(fmIOPair->socketIn.accept(*(CASocket*)newMuxSocket)==SOCKET_ERROR)
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Accept Error - Connection from Browser!\n");
								#endif
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
								((CASocket*)newMuxSocket)->send((char*)infoBuff,infoSize);
								oMuxChannelList.add(newMuxSocket);
								nUser++;
								oInfoService.setLevel(nUser,-1,-1);
								oSocketGroup.add(*newMuxSocket);
							}
					}
				if(oSocketGroup.isSignaled(fmIOPair->muxOut))
						{
							len=fmIOPair->muxOut.receive(&oMuxPacket);
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
											tmpReverseEntry->pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
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
																fmIOPair->muxOut.close(tmpCon->outChannel);
																delete tmpCon->pCipher;
																tmpCon=otmpEntry.pSocketList->getNext();
															}
														otmpEntry.pMuxSocket->close();
														delete otmpEntry.pMuxSocket;
														delete otmpEntry.pSocketList;
													}
												nUser--;
												oInfoService.setLevel(nUser,rand()%100,rand()%100);
											}
										else
											{
												if(len==0)
													{
														if(oMuxChannelList.get(tmpEntry,oMuxPacket.channel,&oConnection))
															{
																fmIOPair->muxOut.close(oConnection.outChannel);
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
																pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
															}
														else
															{
																pCipher= new CASymCipher();
																oRSA.decrypt((unsigned char*)oMuxPacket.data,buff);
																pCipher->setDecryptionKey(buff);
																pCipher->setEncryptionKey(buff);
																pCipher->decrypt((unsigned char*)oMuxPacket.data+RSA_SIZE,
																								 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																								 DATA_SIZE-RSA_SIZE);
																memcpy(oMuxPacket.data,buff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
																
																oMuxChannelList.add(tmpEntry,oMuxPacket.channel,lastChannelId,pCipher);
																#ifdef _DEBUG
																	CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",lastChannelId);
																#endif
																oMuxPacket.channel=lastChannelId++;
																oMuxPacket.len=oMuxPacket.len-16;
															}
														if(fmIOPair->muxOut.send(&oMuxPacket)==SOCKET_ERROR)
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
		THREAD_RETURN_SUCCESS;
	}

int doFirstMix()
	{
		int ret;	
		CASocketAddr socketAddrIn(options.getServerPort());
		FMPair* fmIOPair=new FMPair;
		fmIOPair->socketIn.create();
		fmIOPair->socketIn.setReuseAddr(true);
		if(fmIOPair->socketIn.listen(&socketAddrIn)==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
					return -1;
		    }
		
		
		CASocketAddr addrNext;
		char strTarget[255];
		options.getTargetHost(strTarget,255);
		addrNext.setAddr(strTarget,options.getTargetPort());
		CAMsg::printMsg(LOG_INFO,"Try connectiong to next Mix... %s:%u",strTarget,options.getTargetPort());
		((CASocket*)fmIOPair->muxOut)->create();
		((CASocket*)fmIOPair->muxOut)->setSendBuff(50*sizeof(MUXPACKET));
		((CASocket*)fmIOPair->muxOut)->setRecvBuff(50*sizeof(MUXPACKET));
		if(fmIOPair->muxOut.connect(&addrNext,10,10)==E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO," connected!\n");
				unsigned short len;
				((CASocket*)fmIOPair->muxOut)->receive((char*)&len,2);
				CAMsg::printMsg(LOG_CRIT,"Received Key Inof lenght %u\n",ntohs(len));
				fmIOPair->recvBuff=new unsigned char[ntohs(len)+2];
				memcpy(fmIOPair->recvBuff,&len,2);
				((CASocket*)fmIOPair->muxOut)->receive((char*)fmIOPair->recvBuff+2,ntohs(len));
				CAMsg::printMsg(LOG_CRIT,"Received Key Info...\n");
				fmIO(fmIOPair);
				ret=0;
			}
		else
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix!\n");
				ret=-1;
			}
	//	WaitForSingleObject(hEventThreadEnde,INFINITE);
		delete fmIOPair;
		return ret;
	}

struct LMPair
	{
		CAMuxSocket muxIn;
		CASocketAddr addrSquid;
		CASocketAddr addrSocks;
		CAASymCipher* pRSA;
	};

THREAD_RETURN lmIO(void *v)
	{
		LMPair* lmIOPair=(LMPair*)v;
		CASocketList  oSocketList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(lmIOPair->muxIn);
		MUXPACKET oMuxPacket;
		int len;
		int countRead;
		CONNECTION oConnection;
		unsigned char buff[RSA_SIZE];
		CAASymCipher* pRSA=lmIOPair->pRSA;
		for(;;)
			{
				if((countRead=oSocketGroup.select())==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(lmIOPair->muxIn))
					{
						countRead--;
						len=lmIOPair->muxIn.receive(&oMuxPacket);
						if(len==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Channel to previous mix closed -- Exiting!\n");
								exit(-1);
							}
						if(!oSocketList.get(oMuxPacket.channel,&oConnection))
							{
								if(len!=0)
									{
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										CASocket* tmpSocket=new CASocket;
										int ret;
										if(oMuxPacket.type==MUX_SOCKS)
											ret=tmpSocket->connect(&lmIOPair->addrSocks);
										else
											ret=tmpSocket->connect(&lmIOPair->addrSquid);	
										if(ret!=E_SUCCESS)
										    {
	    										#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"Cannot connect to Squid!\n");
													#endif
													lmIOPair->muxIn.close(oMuxPacket.channel);
													delete tmpSocket;
										    }
										else
										    {    
													CASymCipher* newCipher=new CASymCipher();
													pRSA->decrypt((unsigned char*)oMuxPacket.data,buff);
													newCipher->setDecryptionKey(buff);
													newCipher->setEncryptionKey(buff);
													newCipher->decrypt((unsigned char*)oMuxPacket.data+RSA_SIZE,
																						 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																						 DATA_SIZE-RSA_SIZE);
													memcpy(oMuxPacket.data,buff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
													
													#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"%s",oMuxPacket.data);
													#endif
													if(tmpSocket->send(oMuxPacket.data,len-KEY_SIZE)==SOCKET_ERROR)
														{
															#ifdef _DEBUG
																CAMsg::printMsg(LOG_DEBUG,"Error sending Data to Squid!");
															#endif
															tmpSocket->close();
															lmIOPair->muxIn.close(oMuxPacket.channel);
															delete tmpSocket;
															delete newCipher;
														}
													else
														{
															oSocketList.add(oMuxPacket.channel,tmpSocket,newCipher);
															oSocketGroup.add(*tmpSocket);
														}
										    }
									}
							}
						else
							{
								if(len==0)
									{
										oSocketGroup.remove(*(oConnection.pSocket));
										oConnection.pSocket->close();
										lmIOPair->muxIn.close(oMuxPacket.channel);
										oSocketList.remove(oMuxPacket.channel);
										delete oConnection.pSocket;
										delete oConnection.pCipher;
									}
								else
									{
										oConnection.pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
										len=oConnection.pSocket->send(oMuxPacket.data,len);
										if(len==SOCKET_ERROR)
											{
												oSocketGroup.remove(*(oConnection.pSocket));
												oConnection.pSocket->close();
												lmIOPair->muxIn.close(oMuxPacket.channel);
												oSocketList.remove(oMuxPacket.channel);
												delete oConnection.pSocket;
												delete oConnection.pCipher;
											}
									}
							}
					}
				if(countRead>0)
					{
						CONNECTION* tmpCon;
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL&&countRead>0)
							{
								if(oSocketGroup.isSignaled(*(tmpCon->pSocket)))
									{
										countRead--;
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"Receiving Data from Squid!");
										#endif
										len=tmpCon->pSocket->receive(oMuxPacket.data,1000);
										if(len==SOCKET_ERROR||len==0)
											{
												#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"Closing Connection from Squid!\n");
												#endif
												oSocketGroup.remove(*(tmpCon->pSocket));
												tmpCon->pSocket->close();
												lmIOPair->muxIn.close(tmpCon->id);
												oSocketList.remove(tmpCon->id);
												delete tmpCon->pSocket;
												delete tmpCon->pCipher;
												break;
											}
										else 
											{
												oMuxPacket.channel=tmpCon->id;
												oMuxPacket.len=(unsigned short)len;
												tmpCon->pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
												if(lmIOPair->muxIn.send(&oMuxPacket)==SOCKET_ERROR)
													{
														CAMsg::printMsg(LOG_CRIT,"Mux Data Sending Error - Exiting!\n");
														exit(-1);
													}
											}
									}
								tmpCon=oSocketList.getNext();
							}
					}
			}
		THREAD_RETURN_SUCCESS;
	}

int doLastMix()
	{
		LMPair* lmIOPair=new LMPair;
		lmIOPair->pRSA=new CAASymCipher();
		lmIOPair->pRSA->generateKeyPair(1024);
		CAMsg::printMsg(LOG_INFO,"Waiting for Connection from previous Mix...");
		if(lmIOPair->muxIn.accept(options.getServerPort())==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT," failed!\n");
					delete lmIOPair;
					return -1;
		    }
		((CASocket*)lmIOPair->muxIn)->setRecvBuff(50*sizeof(MUXPACKET));
		((CASocket*)lmIOPair->muxIn)->setSendBuff(50*sizeof(MUXPACKET));
		
		CAMsg::printMsg(LOG_INFO,"connected!\n");
		CAMsg::printMsg(LOG_INFO,"Sending Infos (chain length and RSA-Key)\n");
		int keySize=lmIOPair->pRSA->getPublicKeySize();
		unsigned short messageSize=keySize+1;
		unsigned char* buff=new unsigned char[messageSize+2];
		(*(unsigned short*)buff)=htons(messageSize);
		buff[2]=1; //chainlen
		lmIOPair->pRSA->getPublicKey(buff+3,&keySize);
		((CASocket*)lmIOPair->muxIn)->send((char*)buff,messageSize+2);
		
		char strTarget[255];
		options.getTargetHost(strTarget,255);
		lmIOPair->addrSquid.setAddr(strTarget,options.getTargetPort());

		options.getSOCKSHost(strTarget,255);
		lmIOPair->addrSocks.setAddr(strTarget,options.getSOCKSPort());
		
		//	_beginthread(lmIO,0,lmIOPair);
		lmIO(lmIOPair);
		//WaitForSingleObject(hEventThreadEnde,INFINITE);
		delete lmIOPair;
		return 0;
	}


int main(int argc, const char* argv[])
	{

// Test!!!
/*		CAASymCipher oRSA;
		int handle=open("g:\\jap\\classes\\plain.bytes",O_BINARY|O_RDWR,S_IWRITE);
		int MAX=filelength(handle);
		unsigned char * buff=new unsigned char[MAX];
		read(handle,buff,MAX);
		close(handle);
		handle=open("g:\\jap\\classes\\crypt.bytes",O_BINARY|O_RDWR,S_IWRITE);
		unsigned char * crypt=new unsigned char[MAX];
		read(handle,crypt,MAX);
		close(handle);
		unsigned char* decrypt=new unsigned char[MAX];
		printf("Decrypting..\n");
	//	MAX=128*450;
		int start=0*128;
		long s=clock();
		for (int i=start;i<MAX;i+=128)
			if(oRSA.decrypt(crypt+i,decrypt+i)==-1)
				printf("Fehler..!\n");
		long r=clock();
		printf("done...Comparing%u\n",r-s);
		int c=memcmp(buff,decrypt,MAX);
		if(c!=0)
			for (int z=start;z<MAX;z++)
				if(buff[z]!=decrypt[z])
					printf("%u ",z);
	*//*		printf("Plain: \n");
			for (int z=start;z<MAX;z++)
				printf("%X:",buff[z]);
			printf("\nEncrypted by JAVA: \n");
			for (z=start;z<MAX;z++)
				printf("%X:",crypt[z]);
			printf("\nEncrypted by C: \n");
			memset(crypt+start,0,128);
			oRSA.encrypt(buff+start,crypt+start);
			for (z=start;z<MAX;z++)
				printf("%X:",crypt[z]);
			printf("\nDecrypted by C: \n");
			oRSA.decrypt(crypt+start,buff+start);
			for (z=start;z<MAX;z++)
				printf("%X:",buff[z]);
		*/
	//		BIGNUM* bn=NULL;
		//	BN_dec2bn(&bn,"146045156752988119086694783791784827226235382817403930968569889520448117142515762490154404168568789906602128114569640745056455078919081535135223786488790643345745133490238858425068609186364886282528002310113020992003131292706048279603244985126945363695371250073851319256901415103802627246986865697725280735339");
			
			/*			BIGNUM bn1;
			BN_init(&bn1);
			BN_bin2bn(buff+start-128,128,&bn1);
			printf("%d",BN_cmp(bn,&bn1));
			exit(0);
´*/
		// End TEst...

		//initalize Random..
#ifdef _WIN32
		RAND_screen();
#else 
 #ifndef __linux
                 unsigned char randbuff[255];
		RAND_seed(randbuff,sizeof(randbuff));
 #endif
#endif
		options.parse(argc,argv);
#ifndef _WIN32
			if(options.getDaemon())
				{
					CAMsg::setOptions(MSG_LOG);
					pid_t pid;
					pid=fork();
					if(pid!=0)
						exit(0);
					setsid();
					chdir("/");
					umask(0);		    
				}
#endif
	    CAMsg::printMsg(LOG_INFO,"Anon proxy started!\n");
	    CAMsg::printMsg(LOG_INFO,"Using: %s!\n",OPENSSL_VERSION_TEXT);
#ifdef _DEBUG
		sockets=0;
#endif
		#ifdef _WIN32
    		int err=0;
		WSADATA wsadata;
		err=WSAStartup(0x0202,&wsadata);
		#endif
		
#ifndef _WIN32
	#ifdef _DEBUG
			signal(SIGPIPE,signal_broken_pipe);
	#else
			signal(SIGPIPE,SIG_IGN);
	#endif
#endif
		if(options.isLocalProxy())
			{
				doLocalProxy();
			}
		else if(options.isFirstMix())
			{
				doFirstMix();
			}
		else if(options.isMiddleMix())
			{
				doMiddleMix();
			}
		else
			doLastMix();

		#ifdef _WIN32		
			WSACleanup();
		#endif
		return 0;
	}
