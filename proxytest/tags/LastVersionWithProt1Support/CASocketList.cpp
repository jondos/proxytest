/*
Copyright (c) 2000, The JAP-Team 
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.

	- Redistributions in binary form must reproduce the above copyright notice, 
	  this list of conditions and the following disclaimer in the documentation and/or 
		other materials provided with the distribution.

	- Neither the name of the University of Technology Dresden, Germany nor the names of its contributors 
	  may be used to endorse or promote products derived from this software without specific 
		prior written permission. 

	
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS 
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/
#include "StdAfx.h"
#include "CASocketList.hpp"

#define POOL_SIZE 1000

typedef struct t_MEMBLOCK
	{
		CONNECTIONLIST* mem;
		t_MEMBLOCK* next;
	} _MEMBLOCK;

SINT32 CASocketList::increasePool()
	{
		CONNECTIONLIST* tmp=new CONNECTIONLIST[POOL_SIZE];
		if(tmp==NULL)
			return E_UNKNOWN;
		_MEMBLOCK* tmpMem=new _MEMBLOCK;
		if(tmpMem==NULL)
			{
				delete tmp;
				return E_UNKNOWN;
			}
		memset(tmp,0,sizeof(CONNECTIONLIST)*POOL_SIZE);
		for(int i=1;i<POOL_SIZE;i++)
			{
				tmp[i-1].next=&tmp[i];
			}
		tmp[POOL_SIZE-1].next=pool;
		pool=tmp;
		tmpMem->next=memlist;
		tmpMem->mem=tmp;
		memlist=tmpMem;
		return E_SUCCESS;
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
		clear();
//		DeleteCriticalSection(&cs);
	}

SINT32 CASocketList::clear()
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
		connections=NULL;
		pool=NULL;
		memlist=NULL;
		aktEnumPos=NULL;
		return E_SUCCESS;
	}

/** Add a new channel to the channel-list.
*	@param id - channel-id of the new channel
*	@param pSocket - a CASocket assoziated with the channel
*	@param pCipher - a CASymCipher assoziated with the channel
*	@return E_SUCCESS, if no error occurs
*	        E_UNKNOWN, otherwise
*
*/
SINT32 CASocketList::add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher)
	{
//		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		if(pool==NULL)
		    {
			if(increasePool()!=E_SUCCESS)
						{
//							LeaveCriticalSection(&cs);
							return E_UNKNOWN;
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
		return E_SUCCESS;
	}

SINT32 CASocketList::add(HCHANNEL in,HCHANNEL out,CASymCipher* pCipher)
	{
//		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		if(pool==NULL)
		    {
					if(increasePool()!=E_SUCCESS)
						{
//							LeaveCriticalSection(&cs);
							return E_UNKNOWN;
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
		return E_SUCCESS;
	}

/** Gets a copy of an entry form the channel-list.
* @param in - the channel-id for wich the entry is requested
*	@param out - the object, that will hold the copy
*	@return true - if the channel was found
	        false - otherwise
*
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
						memcpy(out,tmp,sizeof(CONNECTION));
//						LeaveCriticalSection(&cs);
						return true;
					}
				tmp=tmp->next;
			}
//		LeaveCriticalSection(&cs);
		return false;
	}

/** Gets a copy of an entry form the channel-list.
* @param in - the object, that will hold the copy
*	@param out - the assoziated(output) channel-id for wich the entry is requested
*	@return true - if the channel was found
	        false - otherwise
*
*/
bool	CASocketList::get(CONNECTION* in,HCHANNEL out)
	{
//		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		tmp=connections;
		while(tmp!=NULL)
			{
				if(tmp->outChannel==out)
					{
						memcpy(in,tmp,sizeof(CONNECTION));
//						LeaveCriticalSection(&cs);
						return true;
					}
				tmp=tmp->next;
			}
//		LeaveCriticalSection(&cs);
		return false;
	}

/** Gets a copy of an entry form the channel-list.
* @param in - the object, that will hold the copy
*	@param pSocket - the assoziated(output) socket for wich the entry is requested
*	@return true - if the channel was found
	        false - otherwise
*
*/
bool	CASocketList::get(CONNECTION* in,CASocket* pSocket)
	{
//		EnterCriticalSection(&cs);
		CONNECTIONLIST* tmp;
		tmp=connections;
		while(tmp!=NULL)
			{
				if(tmp->pSocket==pSocket)
					{
						memcpy(in,tmp,sizeof(CONNECTION));
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

/** Gets the first entry of the channel-list.
*	@return the first entry of the channel list (this is not a copy!!)
*
*/	 
CONNECTION* CASocketList::getFirst()
	{
		aktEnumPos=connections;
		return aktEnumPos;
	}

/** Gets the next entry of the channel-list.
*	@return the next entry of the channel list (this is not a copy!!)
*
*/	 
CONNECTION* CASocketList::getNext()
	{
		if(aktEnumPos!=NULL)
			aktEnumPos=aktEnumPos->next;
		return aktEnumPos;
	}
