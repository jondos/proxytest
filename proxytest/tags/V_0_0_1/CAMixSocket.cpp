#include "StdAfx.h"
#include "CAMixChannel.hpp"
#include "CAMixSocket.hpp"

extern CAMixChannel oMixChannel;
#ifdef _DEBUG
extern int sockets;
#endif
CAMixSocket::CAMixSocket()
	{
		id=0;
		InitializeCriticalSection(&csClose);
	}

CAMixSocket::~CAMixSocket()
	{
		DeleteCriticalSection(&csClose);
	}

int CAMixSocket::connect(LPSOCKETADDR psa)
	{
		id=oMixChannel.connect(psa);
		if(id==SOCKET_ERROR)
			{
				id=0;
				return SOCKET_ERROR;
			}
#ifdef _DEBUG
		sockets++;
#endif
		return 0;
	}

int CAMixSocket::send(char* buff,int len)
	{
		return oMixChannel.send(id,buff,len);
	}

int CAMixSocket::receive(char* buff,int len)
	{
		return oMixChannel.receive(id,buff,len);
	}
/*
int CAMixSocket::close()
	{
//		EnterCriticalSection(&csClose);
		int ret;
		if(id!=0)
			{
				oMixChannel.close(id);
				sockets--;
				id=0;
				ret=0;
			}
		else
			ret=SOCKET_ERROR;
//		LeaveCriticalSection(&csClose);
		return ret;
	}
*/
int CAMixSocket::close(int mode)
	{
		EnterCriticalSection(&csClose);
		int ret;
		if(id!=0)
			{
				if(oMixChannel.close(id,mode)==0)
					{
#ifdef _DEBUG
						sockets--;
#endif
						id=0;
						ret=0;
					}
				else
					ret=1;
			}
		else
			ret=SOCKET_ERROR;
		LeaveCriticalSection(&csClose);
		return ret;
	}