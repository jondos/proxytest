// proxytest.cpp : Definiert den Einsprungpunkt für die Konsolenanwendung.
//

#include "stdafx.h"
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
int sockets;
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
		WSADATA wsadata;
		err=WSAStartup(0x0202,&wsadata);
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
	WSACleanup();
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
void proxytomix(void* tmpPair)
	{
		CASocket* inSocket=((CASocketToMix*)tmpPair)->in;	
		CAMixSocket* outSocket=((CASocketToMix*)tmpPair)->out;	
		char buff[1001];
		while(true)
			{
				int len=inSocket->receive(buff,1000);
				if(len==SOCKET_ERROR||len==0)
					{
						break;
					}
				buff[len]=0;
				printf(buff);
				if(outSocket->send(buff,len)==SOCKET_ERROR)
					break;
			}
		EnterCriticalSection(&csClose);
		if(inSocket->close(SD_RECEIVE)==0)
			delete inSocket;
		if(outSocket->close(SD_SEND)==0)
			delete outSocket;
		delete tmpPair;
		LeaveCriticalSection(&csClose);
	}

void mixtoproxy(void* tmpPair)
	{
		CAMixSocket* inSocket=((CAMixToSocket*)tmpPair)->in;	
		CASocket* outSocket=((CAMixToSocket*)tmpPair)->out;	
		char buff[1000];
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
		EnterCriticalSection(&csClose);
		if(inSocket->close(SD_RECEIVE)==0)
			delete inSocket;
		if(outSocket->close(SD_SEND)==0)
			delete outSocket;
		delete tmpPair;
		LeaveCriticalSection(&csClose);
	}

int main(int argc, char* argv[])
	{
		sockets=0;
		int err=0;
		WSADATA wsadata;
		err=WSAStartup(0x0202,&wsadata);
		InitializeCriticalSection(&csClose);
		CASocketAddr socketAddrIn(1999);
		socketAddrSquid.setAddr(argv[1],atol(argv[2]));
		CASocket socketIn;
		socketIn.listen(&socketAddrIn);
//		time_t t=time(NULL);
//		strftime(buff,BUFF_SIZE,"%Y%m%d-%H%M%S",localtime(&t));
//		int handle=open(buff,_O_BINARY|_O_CREAT|_O_RDWR,S_IWRITE);
		while(!_kbhit())
			{
				CAMixToSocket* tmpPair2=new CAMixToSocket();
				CASocketToMix* tmpPair1=new CASocketToMix();
				tmpPair1->in=new CASocket();
				socketIn.accept(*tmpPair1->in);
				tmpPair1->out=new CAMixSocket();
				tmpPair1->out->connect(&socketAddrSquid);
				tmpPair2->in=tmpPair1->out;
				tmpPair2->out=tmpPair1->in;
				_beginthread(proxytomix,0,tmpPair1);
				_beginthread(mixtoproxy,0,tmpPair2);
				printf("%i\n",sockets);
			}
		socketIn.close();
//	close(handle);
		WSACleanup();
		DeleteCriticalSection(&csClose);
		return 0;
	}
