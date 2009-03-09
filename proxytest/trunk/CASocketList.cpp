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
#include "CAUtil.hpp"
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
				delete[] tmp;
				tmp = NULL;
				return E_UNKNOWN;
			}
		memset(tmp,0,sizeof(CONNECTIONLIST)*POOL_SIZE);
		for(int i=1;i<POOL_SIZE;i++)
			{
				tmp[i-1].next=&tmp[i];
			}
		tmp[POOL_SIZE-1].next=m_Pool;
		m_Pool=tmp;
		tmpMem->next=m_Memlist;
		tmpMem->mem=tmp;
		m_Memlist=tmpMem;
		return E_SUCCESS;
	}

CASocketList::CASocketList()
	{
		m_Connections=NULL;
		m_Pool=NULL;
		m_Memlist=NULL;
		m_AktEnumPos=NULL;
		m_bThreadSafe=false;
		setThreadSafe(false);
		increasePool();
		m_Size=0;
	}

CASocketList::CASocketList(bool bThreadSafe)
	{
		m_Connections=NULL;
		m_Pool=NULL;
		m_Memlist=NULL;
		m_AktEnumPos=NULL;
		m_bThreadSafe=false;
		setThreadSafe(bThreadSafe);
		increasePool();
	}

CASocketList::~CASocketList()
	{
		clear();
	}

SINT32 CASocketList::clear()
	{
		_MEMBLOCK* tmp;
		tmp=m_Memlist;
		while(tmp!=NULL)
			{
				delete []tmp->mem;
				tmp->mem = NULL;
				m_Memlist=tmp;
				tmp=tmp->next;
				delete m_Memlist;
				m_Memlist = NULL;
			}
		m_Connections=NULL;
		m_Pool=NULL;
		m_Memlist=NULL;
		m_AktEnumPos=NULL;
		return E_SUCCESS;
	}

SINT32 CASocketList::setThreadSafe(bool b)
	{
		m_bThreadSafe=b;
		return E_SUCCESS;
	}

/** Add a new channel to the channel-list. The id for that channel is randomly choosen.
*	@param pSocket a CASocket assoziated with the channel
*	@param pCiphers an array of CASymCipher assoziated with the channel
*	@return E_SUCCESS if no error occurs
*	        E_UNKNOWN otherwise
*
*/
SINT32 CASocketList::add(CASocket* pSocket,CASymCipher* pCiphers)
	{
		if(m_bThreadSafe)
			cs.lock();
		CONNECTIONLIST* tmp;
		if(m_Pool==NULL)
		  {
				if(increasePool()!=E_SUCCESS)
					{
						if(m_bThreadSafe)
							cs.unlock();
						return E_UNKNOWN;
					}
		   }
		tmp=m_Pool;
		m_Pool=m_Pool->next;
		tmp->next=m_Connections;
		m_Connections=tmp;
		m_Connections->pSocket=pSocket;
		m_Connections->pCiphers=pCiphers;
		m_Connections->currentSendMeCounter=0;
		m_Connections->upstreamBytes=0;
		for(;;)
			{
SELECT_RANDOM_CHANNEL_ID:
				getRandom(&m_Connections->outChannel);				
				tmp=m_Connections->next;
				while(tmp!=NULL)
					{
						if(tmp->outChannel==m_Connections->outChannel)
							goto SELECT_RANDOM_CHANNEL_ID;
						tmp=tmp->next;
					}
				break;
			}
		m_Size++;
		if(m_bThreadSafe)
			cs.unlock();
		return E_SUCCESS;
	}

/** Gets a copy of an entry form the channel-list.
* @param in - the channel-id for wich the entry is requested
*	@param out - the object, that will hold the copy
*	@return true - if the channel was found
	        false - otherwise
*
*/
SINT32 CASocketList::get(HCHANNEL in,CONNECTION* out)
	{
		if(m_bThreadSafe)
			cs.lock();
		CONNECTIONLIST* tmp;
		tmp=m_Connections;
		while(tmp!=NULL)
			{
				if(tmp->outChannel==in)
					{
						memcpy(out,tmp,sizeof(CONNECTION));
						if(m_bThreadSafe)
							cs.unlock();
						return E_SUCCESS;
					}
				tmp=tmp->next;
			}
		if(m_bThreadSafe)
			cs.unlock();
		return E_UNKNOWN;
	}

SINT32 CASocketList::addSendMeCounter(HCHANNEL in,SINT32 value)
	{
		if(m_bThreadSafe)
			cs.lock();
		CONNECTIONLIST* tmp;
		tmp=m_Connections;
		while(tmp!=NULL)
			{
				if(tmp->outChannel==in)
					{
						tmp->currentSendMeCounter+=value;
						if(m_bThreadSafe)
							cs.unlock();
						return E_SUCCESS;
					}
				tmp=tmp->next;
			}
		if(m_bThreadSafe)
			cs.unlock();
		return E_UNKNOWN;
	}

CASocket* CASocketList::remove(HCHANNEL id)
	{
		if(m_bThreadSafe)
			cs.lock();
		CONNECTIONLIST* tmp,*before;
		CASocket* ret;
		tmp=m_Connections;
		before=NULL;
		while(tmp!=NULL)
			{
				if(tmp->outChannel==id)
					{
						if(m_AktEnumPos==tmp)
							m_AktEnumPos=tmp->next;
						if(before!=NULL)
							before->next=tmp->next;
						else
							m_Connections=tmp->next;
						tmp->next=m_Pool;
						m_Pool=tmp;
						ret=tmp->pSocket;
						m_Size--;
						if(m_bThreadSafe)
							cs.unlock();
						return ret;
					}
				before=tmp;
				tmp=tmp->next;
			}
		if(m_bThreadSafe)
			cs.unlock();
		return NULL;
	}

