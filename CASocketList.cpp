#include "StdAfx.h"
#include "CASocketList.hpp"

CASocketList::CASocketList()
	{
		connections=NULL;
		pool=new CONNECTIONLIST;
		pool->id=1;
		pool->pSocket=NULL;
		CONNECTIONLIST* tmp;
		tmp=pool;
		for(int i=2;i<1000;i++)
			{
				tmp->next=new CONNECTIONLIST;
				tmp=tmp->next;
				tmp->id=i;
				tmp->pSocket=NULL;
			}
		tmp->next=NULL;
		InitializeCriticalSection(&cs);
	}

CASocketList::~CASocketList()
	{
		DeleteCriticalSection(&cs);
	}

int CASocketList::add(CASocket* pSocket)
	{
		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		tmp=pool;
		pool=pool->next;
		tmp->next=connections;
		connections=tmp;
		connections->pSocket=pSocket;
		int ret=connections->id;
		LeaveCriticalSection(&cs);
		return ret;
	}

CASocket* CASocketList::get(int id)
	{
		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		tmp=connections;
		CASocket* ret;
		while(tmp!=NULL)
			{
				if(tmp->id==id)
					{
						ret=tmp->pSocket;
						LeaveCriticalSection(&cs);
						return ret;
					}
				tmp=tmp->next;
			}
		LeaveCriticalSection(&cs);
		return NULL;
	}

CASocket* CASocketList::remove(int id)
	{
		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp,*before;
		CASocket* ret;
		tmp=connections;
		before=NULL;
		while(tmp!=NULL)
			{
				if(tmp->id==id)
					{
						if(before!=NULL)
							before->next=tmp->next;
						else
							connections=tmp->next;
						tmp->next=pool;
						pool=tmp;
						ret=tmp->pSocket;
						LeaveCriticalSection(&cs);
						return ret;
					}
				before=tmp;
				tmp=tmp->next;
			}
		LeaveCriticalSection(&cs);
		return NULL;
	}