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
		aktEnumPos=NULL;
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
		int ret;
		if(pool==NULL)
		    {
			ret=SOCKET_ERROR;
		    }
		else
		    {
			tmp=pool;
			pool=pool->next;
			tmp->next=connections;
			connections=tmp;
			connections->pSocket=pSocket;
			ret=connections->id;
		    }
		LeaveCriticalSection(&cs);
		return ret;
	}

int CASocketList::add(int id,CASocket* pSocket)
	{
		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		int ret;
		if(pool==NULL)
		    {
					ret=SOCKET_ERROR;
		    }
		else
		    {
					CONNECTIONLIST* before=NULL;
					tmp=pool;
					while(tmp!=NULL)
						{
							if(tmp->id==id)
								{
									tmp->pSocket=pSocket;
									if(before!=NULL)
										{
											before->next=tmp->next;
										}
									else
										pool=tmp->next;
									tmp->next=connections;
									connections=tmp;
									ret=connections->id;
									goto ende;
								}
							before=tmp;
							tmp=tmp->next;
						}
					ret=-1;
			}
ende:
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

CONNECTION* CASocketList::getFirst()
	{
		aktEnumPos=connections;
		return aktEnumPos;
	}

CONNECTION* CASocketList::getNext()
	{
		if(aktEnumPos!=NULL)
			aktEnumPos=aktEnumPos->next;
		return aktEnumPos;
	}