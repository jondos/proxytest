// proxytest.cpp : Definiert den Einsprungpunkt für die Konsolenanwendung.
//

#include "StdAfx.h"
#include "CASocket.hpp"
#include "CAMixSocket.hpp"
#include "CASocketGroup.hpp"
#include "CASocketAddr.hpp"
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
/*
int main(int argc, char* argv[])
	{
		int err=0;
		#ifdef _WIN32
		WSADATA wsadata;
		err=WSAStartup(0x0202,&wsadata);
		#endif
	fd_set read_set;
	FD_ZERO(&read_set);
	SOCKET socketIn=socket(AF_INET,SOCK_STREAM,0);
	sockaddr_in addrIn;
	char hostname[50];
	err=gethostname(hostname,50);
	HOSTENT* hostent=gethostbyname(hostname);
	addrIn.sin_family=AF_INET;
	addrIn.sin_port=htons(1999);
	memcpy(&addrIn.sin_addr,hostent->h_addr_list[0],hostent->h_length);

	hostent=gethostbyname("anon.inf.tu-dresden.de");
	sockaddr_in addrSquid;
	addrSquid.sin_family=AF_INET;
	addrSquid.sin_port=htons(3128);
	memcpy(&addrSquid.sin_addr,hostent->h_addr_list[0],hostent->h_length);
	err=bind(socketIn,(sockaddr*)&addrIn,sizeof(addrIn));
	err=listen(socketIn,SOMAXCONN);
	FD_SET(socketIn,&read_set);
	fd_set tmp_set;
	SOCKET intoout[10000];
	memset(intoout,0,sizeof(intoout));
#define BUFF_SIZE 65000
	char buff[BUFF_SIZE];
	time_t t=time(NULL);
	strftime(buff,BUFF_SIZE,"%Y%m%d-%H%M%S",localtime(&t));
	int handle=open(buff,_O_BINARY|_O_CREAT|_O_RDWR,S_IWRITE);
	while(!_kbhit())
		{
			memcpy(&tmp_set,&read_set,sizeof(FD_SET));
			int waiting=select(0,&tmp_set,NULL,NULL,NULL);
			if(FD_ISSET(socketIn,&tmp_set))
				{
					SOCKET newConn=accept(socketIn,NULL,NULL);
					SOCKET newOut=socket(AF_INET,SOCK_STREAM,0);
					err=connect(newOut,(sockaddr*)&addrSquid,sizeof(addrSquid));
					intoout[newConn]=newOut;
					intoout[newOut]=newConn;
					FD_SET(newConn,&read_set);
					FD_SET(newOut,&read_set);
					waiting--;
				}
			SOCKET i=10;
			while(waiting>0&&i<4096)
				{
					i++;
					if(FD_ISSET(i,&tmp_set)&&i!=socketIn)
						{
							long len=recv(i,buff,BUFF_SIZE,0);
							if(len==SOCKET_ERROR)
								{
									err=WSAGetLastError();
									if(err==WSAECONNRESET||err==WSAESHUTDOWN)
										{
											shutdown(i,SD_BOTH);
											shutdown(intoout[i],SD_BOTH);
											closesocket(i);
											closesocket(intoout[i]);
											FD_CLR(i,&read_set);
											FD_CLR(intoout[i],&read_set);
											intoout[intoout[i]]=0;
											intoout[i]=0;
										}
									else{	
									printf("Receive Error: %u",err);
									LPVOID lpMsgBuf;
									FormatMessage( 
												FORMAT_MESSAGE_ALLOCATE_BUFFER | 
												FORMAT_MESSAGE_FROM_SYSTEM | 
												FORMAT_MESSAGE_IGNORE_INSERTS,
												NULL,
												err,
												MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
												(LPTSTR) &lpMsgBuf,
												0,
												NULL 
										);
									printf("%s\n", (LPCTSTR)lpMsgBuf);
									LocalFree( lpMsgBuf );
									}
 								}
							else if(len==0)
								{
									shutdown(i,SD_BOTH);
									shutdown(intoout[i],SD_BOTH);
									closesocket(i);
									closesocket(intoout[i]);
									FD_CLR(i,&read_set);
									FD_CLR(intoout[i],&read_set);
									intoout[intoout[i]]=0;
									intoout[i]=0;
								}
							else
								{
									log proto;
									proto.len=len;
									proto.time=time(NULL);
									write(handle,&proto,sizeof(proto));
									long index=0;
									while(len>1000)
										{
											long s=send(intoout[i],buff+index,1000,0);
											len-=s;
											index+=s;
										}
									send(intoout[i],buff+index,len,0);
								}

							waiting--;
						}
				}
		}
	shutdown(socketIn,SD_BOTH);
	closesocket(socketIn);
	for(SOCKET i=0;i<10000;i++)
		{
			if(intoout[i]!=0)
				{
					shutdown(i,SD_BOTH);
					closesocket(i);
				}
		}
	close(handle);
	#ifdef _WIN32
	WSACleanup();
	#endif
	return 0;
}
*/
/*
void proxy(void* tmpSocket)
	{
		CASocket* inSocket=(CASocket*)tmpSocket;	
		CASocket outSocket;
		if(outSocket.connect(&socketAddrSquid)==SOCKET_ERROR)
			{
				delete inSocket;
				return;
			}
		CASocketGroup socketGroup;
		socketGroup.add(*inSocket);
		socketGroup.add(outSocket);
		char buff[1000];
		while(true)
			{
				socketGroup.select();
				if(socketGroup.isSignaled(*inSocket))
					{
						int len=inSocket->receive(buff,1000);
						if(len==SOCKET_ERROR||len==0)
							{
								if(len==SOCKET_ERROR)printf("Socket Error...\n");
								inSocket->close();
								outSocket.close();
								break;
							}
						outSocket.send(buff,len);
					}
				if(socketGroup.isSignaled(outSocket))
					{
						int len=outSocket.receive(buff,1000);
						if(len==SOCKET_ERROR||len==0)
							{
								if(len==SOCKET_ERROR)printf("Socket Error...\n");
								inSocket->close();
								outSocket.close();
								break;
							}
						inSocket->send(buff,len);
					}
			}
		delete inSocket;
	}
*/

void signal_broken_pipe( int sig)
	{
		printf("Hm.. Broken Pipe.... How cares!\n");
		signal(SIGPIPE,signal_broken_pipe);
	}
	
THREAD_RETURN proxytomix(void* tmpPair)
	{
		CASocket* inSocket=((CASocketToMix*)tmpPair)->in;	
		CAMixSocket* outSocket=((CASocketToMix*)tmpPair)->out;	
		char* buff=new char[1001];
		if(buff==NULL)
		    {
					printf("No more Memory!\n");
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
		printf("Thread terminated\n");
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
					printf("Out of Memory!\n");
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
		printf("Thread terminated\n");
#endif
		THREAD_RETURN_SUCCESS;
	}

int main(int argc, char* argv[])
	{
#ifdef _DEBUG
		sockets=0;
#endif
		#ifdef _WIN32
    		int err=0;
		WSADATA wsadata;
		err=WSAStartup(0x0202,&wsadata);
		#endif
		
		InitializeCriticalSection(&csClose);
		CASocketAddr socketAddrIn(atol(argv[3]));
		socketAddrSquid.setAddr(argv[1],atol(argv[2]));
		CASocket socketIn;
		if(socketIn.listen(&socketAddrIn)==SOCKET_ERROR)
		    {
			printf("Cannot listen\n");
			exit(-1);
		    }
//		time_t t=time(NULL);
//		strftime(buff,BUFF_SIZE,"%Y%m%d-%H%M%S",localtime(&t));
//		int handle=open(buff,_O_BINARY|_O_CREAT|_O_RDWR,S_IWRITE);
		signal(SIGPIPE,signal_broken_pipe);
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
					printf("Less memory!\n");
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
					break;
				
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
					    printf("Can't create Thread 1 - Error:%i\n",err);
//					    printf("EAGAIN:%i\n",EAGAIN);
//					    printf("MAxThreads:%u\n",PTHREAD_THREADS_MAX);
					    exit(-2);
					}
				    err=pthread_create(&p2,NULL,mixtoproxy,tmpPair2);
				    if(err!=0)	
					{
					    printf("Can't create Thread 2 - Error:%i\n",err);
//					    printf("EAGAIN:%i\n",EAGAIN);
//					    printf("MAxThreads:%u\n",PTHREAD_THREADS_MAX);
					    exit(-2);
					}
				#endif
#ifdef _DEBUG
				printf("%i\n",sockets);
#endif
			}
		printf("Exiting...\n");	
		socketIn.close();
//	close(handle);
#ifdef _WIN32		
WSACleanup();
#endif
		DeleteCriticalSection(&csClose);
		return 0;
	}
