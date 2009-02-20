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
#include "CAQueue.hpp"
#include "CAMsg.hpp"

CAQueue::~CAQueue()
	{
		while(m_Queue!=NULL)
			{
				delete m_Queue->pBuff;
				m_lastElem=m_Queue;
				m_Queue=m_Queue->next;
				delete m_lastElem;
			}
		DeleteCriticalSection(&m_csQueue);
	}

/** Adds data to the Queue.
	* @param buff pointer to the data buffer
	* @param size size of data to add
	* @returnval E_UNKOWN, in case of an error
	* @retval E_SUCCESS if succesful
	*/
SINT32 CAQueue::add(const UINT8* buff,UINT32 size)
	{
		if(buff==NULL)
			return E_UNKNOWN;
		EnterCriticalSection(&m_csQueue);
		if(m_Queue==NULL)
			{
				m_Queue=new QUEUE;
				if(m_Queue==NULL)
					{
						LeaveCriticalSection(&m_csQueue);
						return E_UNKNOWN;
					}
				m_Queue->pBuff=new UINT8[size];
				if(m_Queue->pBuff==NULL)
					{
						delete m_Queue;
						m_Queue=NULL;
						LeaveCriticalSection(&m_csQueue);
						return E_UNKNOWN;
					}
				m_Queue->next=NULL;
				m_Queue->size=size;
				memcpy(m_Queue->pBuff,buff,size);
				m_lastElem=m_Queue;
			}
		else
			{
				m_lastElem->next=new QUEUE;
				if(m_lastElem->next==NULL)
					{
						LeaveCriticalSection(&m_csQueue);
						return E_UNKNOWN;
					}
				m_lastElem->next->pBuff=new UINT8[size];
				if(m_lastElem->next->pBuff==NULL)
					{
						delete m_lastElem->next;
						m_lastElem->next=NULL;
						LeaveCriticalSection(&m_csQueue);
						return E_UNKNOWN;
					}
				m_lastElem=m_lastElem->next;
				m_lastElem->next=NULL;
				m_lastElem->size=size;
				memcpy(m_lastElem->pBuff,buff,size);
			}
		m_nQueueSize+=size;
		LeaveCriticalSection(&m_csQueue);
		return E_SUCCESS;
	}

/** Gets data from the Queue.
  * @param pbuff, pointer to a buffer, there the data should be stored
	* @param psize, on call contains the size of pbuff, on return contains the size of returned data
	* @retval E_SUCCESS, if succesful
	* @retval E_UNKNOWN, in case of an error
	*/
SINT32 CAQueue::get(UINT8* pbuff,UINT32* psize)
	{
		if(m_Queue==NULL||pbuff==NULL||psize==NULL)
			return E_UNKNOWN;
		if(*psize==0)
			return E_SUCCESS;
		EnterCriticalSection(&m_csQueue);
		SINT32 ret;
		UINT32 space=*psize;
		*psize=0;
		while(space>=m_Queue->size)
			{
				memcpy(pbuff,m_Queue->pBuff,m_Queue->size);
				*psize+=m_Queue->size;
				pbuff+=m_Queue->size;
				space-=m_Queue->size;
				m_nQueueSize-=m_Queue->size;
				delete m_Queue->pBuff;
				QUEUE* tmp=m_Queue;
				m_Queue=m_Queue->next;
				delete tmp;
				if(m_Queue==NULL)
					{
						LeaveCriticalSection(&m_csQueue);
						return E_SUCCESS;
					}
			}
		memcpy(pbuff,m_Queue->pBuff,space);
		*psize+=space;
		m_Queue->size-=space;
		m_nQueueSize-=space;
		memmove(m_Queue->pBuff,m_Queue->pBuff+space,m_Queue->size);
		ret=E_SUCCESS;
		LeaveCriticalSection(&m_csQueue);
		return ret;
	}
			
SINT32 CAQueue::test()
	{
		CAQueue oQueue;
		#define TEST_SIZE 1000000
		UINT8* source=new UINT8[TEST_SIZE];
		UINT8* target=new UINT8[TEST_SIZE];
		RAND_pseudo_bytes(source,TEST_SIZE);
		UINT32 count=0;
		UINT32 aktSize;
		
		srand((unsigned)time( NULL ));
		//adding
		
		while(TEST_SIZE-count>10000)
				{
					aktSize=rand();
					aktSize%=0xFFFF;
					aktSize%=(TEST_SIZE-count);
					if(oQueue.add(source+count,aktSize)!=E_SUCCESS)
						return E_UNKNOWN;
					count+=aktSize;
					if(oQueue.getSize()!=count)
						return E_UNKNOWN;
				}
		if(oQueue.add(source+count,TEST_SIZE-count)!=E_SUCCESS)
			return E_UNKNOWN;
		if(oQueue.getSize()!=TEST_SIZE)
			return E_UNKNOWN;
		
		//getting
		count=0;
		while(!oQueue.isEmpty())
			{
				aktSize=rand();
				aktSize%=0xFFFF;
				if(oQueue.get(target+count,&aktSize)!=E_SUCCESS)
					return E_UNKNOWN;
				count+=aktSize;
			}
		if(count!=TEST_SIZE)
			return E_UNKNOWN;
		if(memcmp(source,target,TEST_SIZE)!=0)
			return E_UNKNOWN;
		return E_SUCCESS;
	}