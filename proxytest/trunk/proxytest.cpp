// proxytest.cpp : Definiert den Einsprungpunkt für die Konsolenanwendung.
//

#include "StdAfx.h"
#include "CASocket.hpp"
#include "CAMixSocket.hpp"
#include "CASocketGroup.hpp"
#include "CASocketAddr.hpp"
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"
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

int main(int argc, const char* argv[])
	{
	
	    CACmdLnOptions options;
	    options.parse(argc,argv);
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
	    CAMsg::printMsg(LOG_INFO,"Anon proxy started!\n");
	    char strTargetHost[255];
	    options.getTargetHost(strTargetHost,255);
#ifdef _DEBUG
		sockets=0;
#endif
		#ifdef _WIN32
    		int err=0;
		WSADATA wsadata;
		err=WSAStartup(0x0202,&wsadata);
		#endif
		
		InitializeCriticalSection(&csClose);
		CASocketAddr socketAddrIn(options.getServerPort());
		socketAddrSquid.setAddr(strTargetHost,options.getTargetPort());
		CASocket socketIn;
		if(socketIn.listen(&socketAddrIn)==SOCKET_ERROR)
		    {
			CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
			exit(-1);
		    }
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
		while(
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
#ifdef _WIN32		
WSACleanup();
#endif
		DeleteCriticalSection(&csClose);
		return 0;
	}
