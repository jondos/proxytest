// proxytest.cpp : Definiert den Einsprungpunkt f�r die Konsolenanwendung.
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
typedef struct 
	{
		CASocket* in;
		CAMixSocket* out;
	}CASocketToMix;
typedef struct 
	{
		CAMixSocket* in;
		CASocket* out;
	}CAMixToSocket;

#ifndef _WIN32
	#ifdef _DEBUG 
		void signal_broken_pipe( int sig)
			{
				CAMsg::printMsg(LOG_WARNING,"Hm.. Broken Pipe.... How cares!\n");
				signal(SIGPIPE,signal_broken_pipe);
			}
	#endif
#endif

		/*
THREAD_RETURN proxytomix(void* tmpPair)
	{
		CASocket* inSocket=((CASocketToMix*)tmpPair)->in;	
		CAMixSocket* outSocket=((CASocketToMix*)tmpPair)->out;	
		char* buff=new char[1001];
		if(buff==NULL)
		    {
					CAMsg::printMsg(LOG_ERR,"No more Memory!\n");
					THREAD_RETURN_ERROR;
		    }    
		while(true)
			{
				int len=inSocket->receive(buff,1000);
				if(len==SOCKET_ERROR||len==0)
					{
						break;
					}
				if(outSocket->send(buff,len)==SOCKET_ERROR)
					break;
			}
		delete buff;
		EnterCriticalSection(&csClose);
		if(inSocket->close(SD_RECEIVE)==0)
			delete inSocket;
		if(outSocket->close(SD_SEND)==0)
			delete outSocket;
		delete (CASocketToMix*)tmpPair;
		LeaveCriticalSection(&csClose);
#ifdef _DEBUG
		CAMsg::printMsg(LOG_DEBUG,"Thread terminated\n");
#endif
		THREAD_RETURN_SUCCESS;
	}

THREAD_RETURN mixtoproxy(void* tmpPair)
	{
		CAMixSocket* inSocket=((CAMixToSocket*)tmpPair)->in;	
		CASocket* outSocket=((CAMixToSocket*)tmpPair)->out;	
		char* buff=new char[1001];
		if(buff==NULL)
		    {
					CAMsg::printMsg(LOG_ERR,"Out of Memory!\n");
					THREAD_RETURN_ERROR;
		    }
		while(true)
			{
				int len=inSocket->receive(buff,1000);
				if(len==SOCKET_ERROR||len==0)
					{
						break;
					}
				if(outSocket->send(buff,len)==SOCKET_ERROR)
					break;
			}
		delete buff;
		EnterCriticalSection(&csClose);
		if(inSocket->close(SD_RECEIVE)==0)
			delete inSocket;
		if(outSocket->close(SD_SEND)==0)
			delete outSocket;
		delete (CAMixToSocket*)tmpPair;
		LeaveCriticalSection(&csClose);
#ifdef _DEBUG
		CAMsg::printMsg(LOG_DEBUG,"Thread terminated\n");
#endif
		THREAD_RETURN_SUCCESS;
	}
*/

struct MMPair
	{
		CAMuxSocket muxIn;
		CAMuxSocket muxOut;
	};

THREAD_RETURN mmInToOut(void* v)
	{
		MMPair* mmIOPair=(MMPair*)v;
		char* buff=new char[1001];
		if(buff==NULL)
		    {
					CAMsg::printMsg(LOG_ERR,"Out of Memory!\n");
					THREAD_RETURN_ERROR;
		    }
		while(true)
			{
				HCHANNEL channel;
				int len=mmIOPair->muxIn.receive(&channel,buff,1000);
				if(len==SOCKET_ERROR||len==0)
					{
						break;
					}
				if(mmIOPair->muxOut.send(channel,buff,len)==SOCKET_ERROR)
					break;
			}
		delete buff;		
		THREAD_RETURN_SUCCESS;
	}

THREAD_RETURN mmOutToIn(void* v)
	{
		MMPair* mmIOPair=(MMPair*)v;
		char* buff=new char[1001];
		if(buff==NULL)
		    {
					CAMsg::printMsg(LOG_ERR,"Out of Memory!\n");
					THREAD_RETURN_ERROR;
		    }
		while(true)
			{
				HCHANNEL channel;
				int len=mmIOPair->muxOut.receive(&channel,buff,1000);
				if(len==SOCKET_ERROR||len==0)
					{
						break;
					}
				if(mmIOPair->muxIn.send(channel,buff,len)==SOCKET_ERROR)
					break;
			}
		delete buff;		
		THREAD_RETURN_SUCCESS;
	}

int doMiddleMix()
	{
		CASocketAddr nextMix;
		MMPair* mmIOPair=new MMPair;
		char strTarget[255];
		options.getTargetHost(strTarget,255);
		nextMix.setAddr(strTarget,options.getTargetPort());
		mmIOPair->muxOut.connect(&nextMix);
		mmIOPair->muxIn.accept(options.getServerPort());
		#ifdef _WIN32
		    _beginthread(mmInToOut,0,mmIOPair);
		    _beginthread(mmOutToIn,0,mmIOPair);
		    WaitForSingleObject(hEventThreadEnde,INFINITE);
		#else
		    pthread_t p1,p2;
		    int err;
		    err=pthread_create(&p1,NULL,mmInToOut,mmIOPair);
		    err=pthread_create(&p2,NULL,mmOutToIn,mmIOPair);
		#endif
		delete mmIOPair;
		return 0;
	}

struct FMPair
	{
		CASocket socketIn;
		CAMuxSocket muxOut;
	};

THREAD_RETURN fmIO(void *v)
	{
		FMPair* fmIOPair=(FMPair*)v;
		CASocketList  oSocketList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(fmIOPair->socketIn);
		oSocketGroup.add(*((CASocket*)fmIOPair->muxOut));
		
		char buff[1001];
		while(true)
			{
				oSocketGroup.select();
				if(oSocketGroup.isSignaled(fmIOPair->socketIn))
					{
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New Connection from Browser!\n");
						#endif
						CASocket* newSocket=new CASocket;
						fmIOPair->socketIn.accept(*newSocket);
						oSocketList.add(newSocket);
						oSocketGroup.add(*newSocket);
					}
				else
					if(oSocketGroup.isSignaled((*(CASocket*)fmIOPair->muxOut)))
						{
							HCHANNEL channel;
							int len;
							len=fmIOPair->muxOut.receive(&channel,buff,1000);
							if(len==0)
								{
									CASocket* tmpSocket=oSocketList.remove(channel);
									oSocketGroup.remove(*tmpSocket);
									tmpSocket->close();
									delete tmpSocket;
								}
							else
								{
									#ifdef _DEBUG
										CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!\n");
									#endif
									CASocket* tmpSocket=oSocketList.get(channel);
									tmpSocket->send(buff,len);
								}
						}
				else
					{
						CONNECTION* tmpCon;
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL)
							{
								if(oSocketGroup.isSignaled(*tmpCon->pSocket))
									{
										int len=tmpCon->pSocket->receive(buff,1000);
										if(len==SOCKET_ERROR||len==0)
											{
												CASocket* tmpSocket=oSocketList.remove(tmpCon->id);
												oSocketGroup.remove(*tmpSocket);
												tmpSocket->close();
												delete tmpSocket;
												break;
											}
										if(fmIOPair->muxOut.send(tmpCon->id,buff,len)==SOCKET_ERROR)
											break;
										break;
									}
								tmpCon=oSocketList.getNext();
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
	};

THREAD_RETURN lmIO(void *v)
	{
		LMPair* lmIOPair=(LMPair*)v;
		CASocketList  oSocketList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(*((CASocket*)lmIOPair->muxIn));
		char buff[1001];
		while(true)
			{
				if(oSocketGroup.select()==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(*((CASocket*)lmIOPair->muxIn)))
					{
						HCHANNEL channel;
						int len;
						len=lmIOPair->muxIn.receive(&channel,buff,1000);
						CASocket* tmpSocket=oSocketList.get(channel);
						if(tmpSocket==NULL)
							{
								if(len!=0)
									{
										tmpSocket=new CASocket;
										tmpSocket->connect(&lmIOPair->addrSquid);
										oSocketList.add(channel,tmpSocket);
										oSocketGroup.add(*tmpSocket);
										tmpSocket->send(buff,len);
									}
							}
						else
							{
								if(len==0)
									{
										oSocketGroup.remove(*tmpSocket);
										tmpSocket->close();
										lmIOPair->muxIn.send(channel,buff,0);
										oSocketList.remove(channel);
										delete tmpSocket;
									}
								else
									{
										len=tmpSocket->send(buff,len);
										if(len==SOCKET_ERROR)
											{
												oSocketGroup.remove(*tmpSocket);
												tmpSocket->close();
												lmIOPair->muxIn.send(channel,buff,0);
												oSocketList.remove(channel);
												delete tmpSocket;
											}
									}
							}
					}
				else
					{
						CONNECTION* tmpCon;
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL)
							{
								if(oSocketGroup.isSignaled(*(tmpCon->pSocket)))
									{
										int len=tmpCon->pSocket->receive(buff,1000);
										if(len==SOCKET_ERROR||len==0)
											{
												oSocketGroup.remove(*tmpCon->pSocket);
												tmpCon->pSocket->close();
												lmIOPair->muxIn.send(tmpCon->id,buff,0);
												oSocketList.remove(tmpCon->id);
												delete tmpCon->pSocket;
												break;
											}
										if(lmIOPair->muxIn.send(tmpCon->id,buff,len)==SOCKET_ERROR)
											{
												break;
											}
										break;
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
		if(lmIOPair->muxIn.accept(options.getServerPort())==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
					delete lmIOPair;
					return -1;
		    }
		char strTarget[255];
		options.getTargetHost(strTarget,255);
		lmIOPair->addrSquid.setAddr(strTarget,options.getTargetPort());
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
		if(options.isFirstMix())
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
