// proxytest.cpp : Definiert den Einsprungpunkt für die Konsolenanwendung.
//

#include "StdAfx.h"
#include "CASocket.hpp"
#include "CAMixSocket.hpp"
#include "CASocketGroup.hpp"
#include "CASocketAddr.hpp"
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"
#include "CAMuxSocket.hpp"
#include "CASocketList.hpp"
#include "CAMuxChannelList.hpp"
#include "CASymCipher.hpp"
#ifdef _WIN32
HANDLE hEventThreadEnde;
#endif
CACmdLnOptions options;

CRITICAL_SECTION csClose;
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
	} LPPair;

THREAD_RETURN lpIO(void *v)
	{
		LPPair* lpIOPair=(LPPair*)v;
		CASocketList  oSocketList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(lpIOPair->socketIn);
		if(options.getSOCKSServerPort()!=-1)
			oSocketGroup.add(lpIOPair->socketSOCKSIn);
		oSocketGroup.add(*((CASocket*)lpIOPair->muxOut));
		HCHANNEL lastChannelId=1;
		MUXPACKET oMuxPacket;
		int len;
		CASocket* newSocket,*tmpSocket;
		CASymCipher oSymCipher;
		unsigned char key[16];
		memset(key,0,sizeof(key));
		oSymCipher.setEncryptionKey(key);
		oSymCipher.setDecryptionKey(key);
		int countRead;
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
								oSocketList.add(lastChannelId++,newSocket);
								oSocketGroup.add(*newSocket);
							}
					}
				if(options.getSOCKSServerPort()!=-1&&oSocketGroup.isSignaled(lpIOPair->socketSOCKSIn))
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
								oSocketList.add(lastChannelId++,newSocket);
								oSocketGroup.add(*newSocket);
							}
					}
				if(oSocketGroup.isSignaled((*(CASocket*)lpIOPair->muxOut)))
						{
							countRead--;	
							len=lpIOPair->muxOut.receive(&oMuxPacket);
							if(len==0)
								{
									#ifdef _DEBUG
										CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ... ",oMuxPacket.channel);
									#endif
									tmpSocket=oSocketList.remove(oMuxPacket.channel);
									if(tmpSocket!=NULL)
										{
											oSocketGroup.remove(*tmpSocket);
											tmpSocket->close();
											#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"closed!\n");
											#endif
											delete tmpSocket;
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
									CASocket* tmpSocket=oSocketList.get(oMuxPacket.channel);
									if(tmpSocket!=NULL)
										{
											tmpSocket->send(oMuxPacket.data,len);
										}
									else
										{
											CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- Channel-Id no valid!\n");										
										}
								}
						}
				if(countRead>0)
					{
						CONNECTION* tmpCon;
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL)
							{
								if(oSocketGroup.isSignaled(*tmpCon->pSocket)&&countRead>0)
									{
										countRead--;
										len=tmpCon->pSocket->receive(oMuxPacket.data,1000);
										if(len==SOCKET_ERROR||len==0)
											{
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
												if(options.getSOCKSServerPort()!=-1&&tmpCon->pSocket->getLocalPort()==options.getSOCKSServerPort())
													{
														oMuxPacket.type=MUX_SOCKS;
													}
												else
													{
														oMuxPacket.type=MUX_HTTP;
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
		if(lpIOPair->socketIn.listen(&socketAddrIn)==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
					return -1;
		    }
		if(options.getSOCKSServerPort()!=-1)
			{
				socketAddrIn.setAddr("127.0.0.1",options.getSOCKSServerPort());
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
		if(lpIOPair->muxOut.connect(&addrNext)!=SOCKET_ERROR)
			{
				CAMsg::printMsg(LOG_INFO," connected!\n");
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
	};

THREAD_RETURN mmIO(void* v)
	{
		MMPair* mmIOPair=(MMPair*)v;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(mmIOPair->muxIn);
		oSocketGroup.add(mmIOPair->muxOut);
		CASocketList oSocketList;
//		char buff[1001];
		HCHANNEL inChannel,outChannel,lastId;
		lastId=1;
		int len;
		MUXPACKET oMuxPacket;
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
								exit(-1);
							}
						if(!oSocketList.get(oMuxPacket.channel,&outChannel))
							{
								if(len!=0)
									{
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										oSocketList.add(oMuxPacket.channel,lastId);
										oMuxPacket.channel=lastId;
										mmIOPair->muxOut.send(&oMuxPacket);
										lastId++;
									}
							}
						else
							{
								if(len==0)
									{
										mmIOPair->muxOut.close(outChannel);
										oSocketList.remove(oMuxPacket.channel);
									}
								else
									{
										oMuxPacket.channel=outChannel;
										mmIOPair->muxOut.send(&oMuxPacket);
									}
							}
					}
				
				if(oSocketGroup.isSignaled(mmIOPair->muxOut))
					{
						len=mmIOPair->muxOut.receive(&oMuxPacket);
						if(len==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								exit(-1);
							}
						if(oSocketList.get(&inChannel,oMuxPacket.channel))
							{
								if(len!=0)
									{
										oMuxPacket.channel=inChannel;
										mmIOPair->muxIn.send(&oMuxPacket);
									}
								else
									{
										mmIOPair->muxIn.close(inChannel);
										oSocketList.remove(inChannel);
									}
							}
						else
							mmIOPair->muxOut.close(outChannel);
					}
			}
		THREAD_RETURN_SUCCESS;
	}

int doMiddleMix()
	{
		CASocketAddr nextMix;
		MMPair* mmIOPair=new MMPair;
		char strTarget[255];
		options.getTargetHost(strTarget,255);
		nextMix.setAddr(strTarget,options.getTargetPort());
		if(mmIOPair->muxOut.connect(&nextMix)==SOCKET_ERROR)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix -- Exiting!\n");
				delete mmIOPair;
				return SOCKET_ERROR;
			}
		if(mmIOPair->muxIn.accept(options.getServerPort())==SOCKET_ERROR)
			{
				CAMsg::printMsg(LOG_CRIT,"Error waiting for previous Mix... -- Exiting!\n");
				delete mmIOPair;
				return SOCKET_ERROR;
			}
		mmIO(mmIOPair);
		delete mmIOPair;
		return 0;
	}

typedef struct t_FMPair
	{
		CASocket socketIn;
		CAMuxSocket muxOut;
	} FMPair;

THREAD_RETURN fmIO(void *v)
	{
		FMPair* fmIOPair=(FMPair*)v;
		CAMuxChannelList  oMuxChannelList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(fmIOPair->socketIn);
		oSocketGroup.add(*((CASocket*)fmIOPair->muxOut));
		HCHANNEL lastChannelId=1;
		HCHANNEL outChannel;
		MUXPACKET oMuxPacket;
//		char buff[1001];
		int len;
		CAMuxSocket* newMuxSocket;
		MUXLISTENTRY* tmpEntry;
		REVERSEMUXLISTENTRY* tmpReverseEntry;
		CASymCipher oSymCipher;
		unsigned char key[16];
		memset(key,0,sizeof(key));
		oSymCipher.setEncryptionKey(key);
		oSymCipher.setDecryptionKey(key);
		int countRead=0;
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
								oMuxChannelList.add(newMuxSocket);
								oSocketGroup.add(*newMuxSocket);
							}
					}
				if(oSocketGroup.isSignaled((*(CASocket*)fmIOPair->muxOut)))
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
											tmpReverseEntry->pMuxSocket->send(&oMuxPacket);
										}
									else
										{
											CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- Channel-Id %u no valid!\n",oMuxPacket.channel);										
										}
								}
						}
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
		if(fmIOPair->socketIn.listen(&socketAddrIn)==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
					return -1;
		    }
		CASocketAddr addrNext;
		char strTarget[255];
		options.getTargetHost(strTarget,255);
		addrNext.setAddr(strTarget,options.getTargetPort());
		CAMsg::printMsg(LOG_INFO,"Try connectiong to next Mix...");
		if(fmIOPair->muxOut.connect(&addrNext)!=SOCKET_ERROR)
			{
				CAMsg::printMsg(LOG_INFO," connected!\n");
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
	};

THREAD_RETURN lmIO(void *v)
	{
		LMPair* lmIOPair=(LMPair*)v;
		CASocketList  oSocketList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(*((CASocket*)lmIOPair->muxIn));
//		char buff[1001];
		MUXPACKET oMuxPacket;
		int len;
		int countRead;
		for(;;)
			{
				if((countRead=oSocketGroup.select())==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(*((CASocket*)lmIOPair->muxIn)))
					{
						countRead--;
						len=lmIOPair->muxIn.receive(&oMuxPacket);
						if(len==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Channel to previous mix closed -- Exiting!\n");
								exit(-1);
							}
						CASocket* tmpSocket=oSocketList.get(oMuxPacket.channel);
						if(tmpSocket==NULL)
							{
								if(len!=0)
									{
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										tmpSocket=new CASocket;
										int ret;
										if(oMuxPacket.type==MUX_SOCKS)
											ret=tmpSocket->connect(&lmIOPair->addrSocks);
										else
											ret=tmpSocket->connect(&lmIOPair->addrSquid);	
										if(ret==SOCKET_ERROR)
										    {
	    										#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"Cannot connect to Squid!\n");
													#endif
													lmIOPair->muxIn.close(oMuxPacket.channel);
													delete tmpSocket;
										    }
										else
										    {    
													if(tmpSocket->send(oMuxPacket.data,len)==SOCKET_ERROR)
														{
															tmpSocket->close();
															lmIOPair->muxIn.close(oMuxPacket.channel);
															delete tmpSocket;
														}
													else
														{
															oSocketList.add(oMuxPacket.channel,tmpSocket);
															oSocketGroup.add(*tmpSocket);
														}
										    }
									}
							}
						else
							{
								if(len==0)
									{
										oSocketGroup.remove(*tmpSocket);
										tmpSocket->close();
										lmIOPair->muxIn.close(oMuxPacket.channel);
										oSocketList.remove(oMuxPacket.channel);
										delete tmpSocket;
									}
								else
									{
										len=tmpSocket->send(oMuxPacket.data,len);
										if(len==SOCKET_ERROR)
											{
												oSocketGroup.remove(*tmpSocket);
												tmpSocket->close();
												lmIOPair->muxIn.close(oMuxPacket.channel);
												oSocketList.remove(oMuxPacket.channel);
												delete tmpSocket;
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
										do
											{
												len=tmpCon->pSocket->receive(oMuxPacket.data,1000);
												if(len==SOCKET_ERROR||len==0)
													{
														#ifdef _DEBUG
																CAMsg::printMsg(LOG_DEBUG,"Closing Connection from Squid!\n");
														#endif
														oSocketGroup.remove(*tmpCon->pSocket);
														tmpCon->pSocket->close();
														lmIOPair->muxIn.close(tmpCon->id);
														oSocketList.remove(tmpCon->id);
														delete tmpCon->pSocket;
														break;
													}
												else 
													{
														oMuxPacket.channel=tmpCon->id;
														oMuxPacket.len=(unsigned short)len;
														if(lmIOPair->muxIn.send(&oMuxPacket)==SOCKET_ERROR)
															{
																CAMsg::printMsg(LOG_CRIT,"Mux Data Sending Error - Exiting!\n");
																exit(-1);
															}
													}
											}
										while(tmpCon->pSocket->available()>=1000);
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
		CAMsg::printMsg(LOG_INFO,"Waiting for Connection from previous Mix...");
		if(lmIOPair->muxIn.accept(options.getServerPort())==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT," failed!\n");
					delete lmIOPair;
					return -1;
		    }
		CAMsg::printMsg(LOG_INFO,"connected!\n");
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
#ifdef _DEBUG
		sockets=0;
#endif
		#ifdef _WIN32
    		int err=0;
		WSADATA wsadata;
		err=WSAStartup(0x0202,&wsadata);
		#endif
		
/*		InitializeCriticalSection(&csClose);
		CASocketAddr socketAddrIn(options.getServerPort());
		socketAddrSquid.setAddr(strTargetHost,options.getTargetPort());
		CASocket socketIn;
		if(socketIn.listen(&socketAddrIn)==SOCKET_ERROR)
		    {
			CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
			exit(-1);
		    }
*/
	//		time_t t=time(NULL);
//		strftime(buff,BUFF_SIZE,"%Y%m%d-%H%M%S",localtime(&t));
//		int handle=open(buff,_O_BINARY|_O_CREAT|_O_RDWR,S_IWRITE);
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
/*		while(
		#ifdef _WIN32
		!_kbhit()
		#else
		true
		#endif
		)
			{
				CAMixToSocket* tmpPair2=new CAMixToSocket();
				CASocketToMix* tmpPair1=new CASocketToMix();
				if(tmpPair2==NULL||tmpPair1==NULL)
				    {
					CAMsg::printMsg(LOG_ERR,"Less memory!\n");
					exit(-1);
				    }
				tmpPair1->in=new CASocket();
				if(socketIn.accept(*tmpPair1->in)==SOCKET_ERROR)
					{
						delete tmpPair1->in;
						delete tmpPair1;
						delete tmpPair2;
						sleep(1);
						continue;
					}
				tmpPair1->out=new CAMixSocket();
				if(tmpPair1->out->connect(&socketAddrSquid)==SOCKET_ERROR)
					{
						delete tmpPair1->in;
						delete tmpPair1->out;
						delete tmpPair1;
						delete tmpPair2;
						sleep(1);
						continue;
					}
				
				tmpPair2->in=tmpPair1->out;
				tmpPair2->out=tmpPair1->in;
				#ifdef _WIN32
				    _beginthread(proxytomix,0,tmpPair1);
				    _beginthread(mixtoproxy,0,tmpPair2);
				#else
				    pthread_t p1,p2;
				    int err;
				    err=pthread_create(&p1,NULL,proxytomix,tmpPair1);
				    if(err!=0)
					{
					    CAMsg::printMsg(LOG_WARNING,"Can't create Thread 1 - Error:%i\n",err);
					    sleep(1);
					    continue;
					    //exit(-2);
					}
				    err=pthread_create(&p2,NULL,mixtoproxy,tmpPair2);
				    if(err!=0)	
					{
					    CAMsg::printMsg(LOG_WARNING,"Can't create Thread 2 - Error:%i\n",err);
					    sleep(1);
					    continue;
					}
				#endif
#ifdef _DEBUG
				CAMsg::printMsg(LOG_DEBUG,"%i\n",sockets);
#endif
			}
		CAMsg::printMsg(LOG_INFO,"Exiting...\n");	
		socketIn.close();
//	close(handle);
*/
	#ifdef _WIN32		
WSACleanup();
#endif
		DeleteCriticalSection(&csClose);
		return 0;
	}
