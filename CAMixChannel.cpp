#include "StdAfx.h"
#include "CAMixChannel.hpp"
#include "CASocket.hpp"

CAMixChannel oMixChannel;

typedef struct receivelist
	{
		char* buff;
		int len;
		receivelist* next;
	}	RECEIVELIST;

CAMixChannel::CAMixChannel()
	{
		InitializeCriticalSection(&csSend);
		InitializeCriticalSection(&csConnect);
		InitializeCriticalSection(&csClose);
	}

CAMixChannel::~CAMixChannel()
	{
		DeleteCriticalSection(&csSend);
		DeleteCriticalSection(&csConnect);
		DeleteCriticalSection(&csClose);
	}

int CAMixChannel::connect(LPSOCKETADDR psa)
	{
		EnterCriticalSection(&csConnect);
		int ret;
		CASocket* pSocket=new CASocket();
		if(pSocket->connect(psa)==SOCKET_ERROR)
		    {
			delete pSocket;
			printf("CAMixChannel - Connection error!\n");
			ret=SOCKET_ERROR;
		    }
		else
		    ret=connections.add(pSocket);
		LeaveCriticalSection(&csConnect);
		return ret;
	}

int CAMixChannel::send(int id,char* buff,int len)
	{
		EnterCriticalSection(&csSend);
		CASocket* pSocket=connections.get(id);
		int ret;
		if(pSocket==NULL)
			ret=SOCKET_ERROR;
		else
			ret=pSocket->send(buff,len);
		LeaveCriticalSection(&csSend);
		return ret;
	}

int CAMixChannel::receive(int id,char* buff,int len)
	{
	/*	EnterCriticalSection(&csReceive);
		RECEIVELIST* tmp=received;
		RECEIVELIST* before=NULL;
		while(tmp!=NULL)
			{
				if(tmp->id==id)
					{
						if(before!=NULL)
							before->next=tmp->next;
						else
							received=tmp->next;
						memcpy(buff,tmp->buff,tmp->len);
						delete tmp->buff;
						int ret=tmp->len;
						delete tmp;
						LeaveCriticalSection(&csReceive);
						return ret;
					}
			}*/
		CASocket* pSocket=connections.get(id);
		if(pSocket==NULL)
		    return SOCKET_ERROR;
		else
		    return pSocket->receive(buff,len);		
	}

int CAMixChannel::close(int id)
	{
		EnterCriticalSection(&csClose);
		CASocket* pSocket=connections.remove(id);
		if(pSocket!=NULL)
			delete pSocket;
		LeaveCriticalSection(&csClose);
		return 0;
	}

int CAMixChannel::close(int id,int mode)
	{
		EnterCriticalSection(&csSend);
		CASocket* pSocket=connections.get(id);
		int ret;
		if(pSocket==NULL)
			ret=SOCKET_ERROR;
		else
		    {
			ret=pSocket->close(mode);
			if(ret==0)
			    close(id);
		    }
		LeaveCriticalSection(&csClose);
		return ret;
	}