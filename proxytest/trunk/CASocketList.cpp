#include "StdAfx.h"
#include "CASocketList.hpp"

#define POOL_SIZE 1000

typedef struct t_MEMBLOCK
	{
		void * mem;
		t_MEMBLOCK* next;
	} _MEMBLOCK;

int CASocketList::increasePool()
	{
		CONNECTIONLIST* tmp=new CONNECTIONLIST[POOL_SIZE];
		memset(tmp,0,sizeof(CONNECTIONLIST)*POOL_SIZE);
		for(int i=1;i<POOL_SIZE;i++)
			{
				tmp[i-1].next=&tmp[i];
			}
		tmp[POOL_SIZE-1].next=pool;
		pool=tmp;
		_MEMBLOCK* tmpMem=new _MEMBLOCK;
		tmpMem->next=memlist;
		tmpMem->mem=tmp;
		memlist=tmpMem;
		return 0;
	}

CASocketList::CASocketList()
	{
		connections=NULL;
		pool=NULL;
		memlist=NULL;
		aktEnumPos=NULL;
//		InitializeCriticalSection(&cs);
		increasePool();
	}

CASocketList::~CASocketList()
	{
		_MEMBLOCK* tmp;
		tmp=memlist;
		while(tmp!=NULL)
			{
				delete tmp->mem;
				memlist=tmp;
				tmp=tmp->next;
				delete memlist;
			}
//		DeleteCriticalSection(&cs);
	}

int CASocketList::add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher)
	{
//		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		if(pool==NULL)
		    {
					if(increasePool()==SOCKET_ERROR)
						{
//							LeaveCriticalSection(&cs);
							return SOCKET_ERROR;
						}
		    }
		tmp=pool;
		pool=pool->next;
		tmp->next=connections;
		connections=tmp;
		connections->pSocket=pSocket;
		connections->pCipher=pCipher;
		connections->id=id;
//		LeaveCriticalSection(&cs);
		return id;
	}

int CASocketList::add(HCHANNEL in,HCHANNEL out,CASymCipher* pCipher)
	{
//		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		if(pool==NULL)
		    {
					if(increasePool()==SOCKET_ERROR)
						{
//							LeaveCriticalSection(&cs);
							return SOCKET_ERROR;
						}
		    }
		tmp=pool;
		pool=pool->next;
		tmp->next=connections;
		connections=tmp;
		connections->outChannel=out;
		connections->pCipher=pCipher;
		connections->id=in;
//		LeaveCriticalSection(&cs);
		return 0;
	}
/*
CASocket* CASocketList::get(HCHANNEL id)
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
*/
bool	CASocketList::get(HCHANNEL in,CONNECTION* out)
	{
//		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		tmp=connections;
		while(tmp!=NULL)
			{
				if(tmp->id==in)
					{
						out->outChannel=tmp->outChannel;
						out->pCipher=tmp->pCipher;
//						LeaveCriticalSection(&cs);
						return true;
					}
				tmp=tmp->next;
			}
//		LeaveCriticalSection(&cs);
		return false;
	}

bool	CASocketList::get(CONNECTION* in,HCHANNEL out)
	{
//		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		tmp=connections;
		while(tmp!=NULL)
			{
				if(tmp->outChannel==out)
					{
						in->id=tmp->id;
						in->pCipher=tmp->pCipher;
//						LeaveCriticalSection(&cs);
						return true;
					}
				tmp=tmp->next;
			}
//		LeaveCriticalSection(&cs);
		return false;
	}

CASocket* CASocketList::remove(HCHANNEL id)
	{
//		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp,*before;
		CASocket* ret;
		tmp=connections;
		before=NULL;
		while(tmp!=NULL)
			{
				if(tmp->id==id)
					{
						if(aktEnumPos==tmp)
							aktEnumPos=tmp->next;
						if(before!=NULL)
							before->next=tmp->next;
						else
							connections=tmp->next;
						tmp->next=pool;
						pool=tmp;
						ret=tmp->pSocket;
//						LeaveCriticalSection(&cs);
						return ret;
					}
				before=tmp;
				tmp=tmp->next;
			}
//		LeaveCriticalSection(&cs);
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
