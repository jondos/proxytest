#include "stdafx.h"
#include "camixchannel.hpp"
#include "camixsocket.hpp"

extern CAMixChannel oMixChannel;
extern int sockets;
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
		sockets++;
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

int CAMixSocket::close()
	{
		EnterCriticalSection(&csClose);
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
		LeaveCriticalSection(&csClose);
		return ret;
	}

int CAMixSocket::close(int mode)
	{
		EnterCriticalSection(&csClose);
		int ret;
		if(id!=0)
			{
				if(oMixChannel.close(id,mode)==0)
					{
						sockets--;
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