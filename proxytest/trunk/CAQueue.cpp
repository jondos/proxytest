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
#include "CAThread.hpp"

CAQueue::~CAQueue()
	{
		while(m_Queue!=NULL)
			{
				delete m_Queue->pBuff;
				m_lastElem=m_Queue;
				m_Queue=m_Queue->next;
				delete m_lastElem;
			}
	}

/** Adds data to the Queue.
	* @param buff pointer to the data buffer
	* @param size size of data to add
	* @retval E_UNKNOWN, in case of an error
	* @retval E_SUCCESS, if succesful
	*/
SINT32 CAQueue::add(const UINT8* buff,UINT32 size)
	{
		if(buff==NULL)
			return E_UNKNOWN;
		m_csQueue.lock();
		if(m_Queue==NULL)
			{
				m_Queue=new QUEUE;
				if(m_Queue==NULL)
					{
						m_csQueue.unlock();
						return E_UNKNOWN;
					}
				m_Queue->pBuff=new UINT8[size];
				if(m_Queue->pBuff==NULL)
					{
						delete m_Queue;
						m_Queue=NULL;
						m_csQueue.unlock();
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
						m_csQueue.unlock();
						return E_UNKNOWN;
					}
				m_lastElem->next->pBuff=new UINT8[size];
				if(m_lastElem->next->pBuff==NULL)
					{
						delete m_lastElem->next;
						m_lastElem->next=NULL;
						m_csQueue.unlock();
						return E_UNKNOWN;
					}
				m_lastElem=m_lastElem->next;
				m_lastElem->next=NULL;
				m_lastElem->size=size;
				memcpy(m_lastElem->pBuff,buff,size);
			}
		m_nQueueSize+=size;
	//	m_convarSize.unlock();
		m_csQueue.unlock();
		//m_convarSize.signal();
		return E_SUCCESS;
	}

/** Gets data from the Queue. The data is removed from the Queue
  * @param pbuff, pointer to a buffer, there the data should be stored
	* @param psize, on call contains the size of pbuff, on return contains 
	*								the size of returned data
	* @retval E_SUCCESS, if succesful
	* @retval E_UNKNOWN, in case of an error
	*/
SINT32 CAQueue::get(UINT8* pbuff,UINT32* psize)
	{
		if(pbuff==NULL||psize==NULL)
			return E_UNKNOWN;
		if(*psize==0)
			return E_SUCCESS;
		if(m_Queue==NULL)
			{
				*psize=0;
				return E_SUCCESS;
			}
		m_csQueue.lock();
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
						m_csQueue.unlock();
						return E_SUCCESS;
					}
			}
		memcpy(pbuff,m_Queue->pBuff,space);
		*psize+=space;
		m_Queue->size-=space;
		m_nQueueSize-=space;
		memmove(m_Queue->pBuff,m_Queue->pBuff+space,m_Queue->size);
		m_csQueue.unlock();
		return E_SUCCESS;
	}

/** Gets data from the Queue or waits until some data is available, if the Queue is empty.
	* The data is removed from the Queue
  * @param pbuff, pointer to a buffer, there the data should be stored
	* @param psize, on call contains the size of pbuff, on return contains 
	*								the size of returned data
	* @retval E_SUCCESS, if succesful
	* @retval E_UNKNOWN, in case of an error
	*/
SINT32 CAQueue::getOrWait(UINT8* pbuff,UINT32* psize)
	{
		m_convarSize.lock();
		while(m_Queue==NULL)
			m_convarSize.wait();
		SINT32 ret=get(pbuff,psize);
		m_convarSize.unlock();
		return ret;
	}

	/** Peeks data from the Queue. The data is NOT removed from the Queue.
  * @param pbuff, pointer to a buffer, there the data should be stored
	* @param psize, on call contains the size of pbuff, 
	*								on return contains the size of returned data
	* @retval E_SUCCESS, if succesful
	* @retval E_UNKNOWN, in case of an error
	*/
SINT32 CAQueue::peek(UINT8* pbuff,UINT32* psize)
	{
		if(m_Queue==NULL||pbuff==NULL||psize==NULL)
			return E_UNKNOWN;
		if(*psize==0)
			return E_SUCCESS;
		m_csQueue.lock();
		UINT32 space=*psize;
		*psize=0;
		QUEUE* tmpQueue=m_Queue;
		while(space>=tmpQueue->size)
			{
				memcpy(pbuff,tmpQueue->pBuff,tmpQueue->size);
				*psize+=tmpQueue->size;
				pbuff+=tmpQueue->size;
				space-=tmpQueue->size;
				tmpQueue=tmpQueue->next;
				if(tmpQueue==NULL)
					{
						m_csQueue.unlock();
						return E_SUCCESS;
					}
			}
		memcpy(pbuff,tmpQueue->pBuff,space);
		*psize+=space;
		m_csQueue.unlock();
		return E_SUCCESS;
	}	
	
	
/** Removes data from the Queue.
	* @param psize, on call contains the size of data to remove, 
	*								on return contains the size of removed data
	* @retval E_SUCCESS, if succesful
	* @retval E_UNKNOWN, in case of an error
	*/
SINT32 CAQueue::remove(UINT32* psize)
	{
		if(m_Queue==NULL||psize==NULL)
			return E_UNKNOWN;
		if(*psize==0)
			return E_SUCCESS;
		m_csQueue.lock();
		UINT32 space=*psize;
		*psize=0;
		while(space>=m_Queue->size)
			{
				*psize+=m_Queue->size;
				space-=m_Queue->size;
				m_nQueueSize-=m_Queue->size;
				delete m_Queue->pBuff;
				QUEUE* tmp=m_Queue;
				m_Queue=m_Queue->next;
				delete tmp;
				if(m_Queue==NULL)
					{
						m_csQueue.unlock();
						return E_SUCCESS;
					}
			}
		*psize+=space;
		m_Queue->size-=space;
		m_nQueueSize-=space;
		memmove(m_Queue->pBuff,m_Queue->pBuff+space,m_Queue->size);
		m_csQueue.unlock();
		return E_SUCCESS;
	}

struct __queue_test
	{
		CAQueue* pQueue;
		UINT8* buff;
		UINT32 len;
	};

THREAD_RETURN producer(void* param)
	{
		struct __queue_test* pTest=(struct __queue_test *)param;
		UINT32 count=0;
		UINT32 aktSize;
		while(pTest->len>10000)
				{
					aktSize=rand();
					aktSize%=0xFFFF;
					aktSize%=pTest->len;
					if(pTest->pQueue->add(pTest->buff+count,aktSize)!=E_SUCCESS)
						THREAD_RETURN_ERROR;
					count+=aktSize;
					pTest->len-=aktSize;
				}
		if(pTest->pQueue->add(pTest->buff+count,pTest->len)!=E_SUCCESS)
			THREAD_RETURN_ERROR;
		THREAD_RETURN_SUCCESS;
	}

THREAD_RETURN consumer(void* param)
	{
		struct __queue_test* pTest=(struct __queue_test *)param;
		UINT32 count=0;
		UINT32 aktSize;
		do
			{
				aktSize=rand();
				aktSize%=0xFFFF;
				if(pTest->pQueue->getOrWait(pTest->buff+count,&aktSize)!=E_SUCCESS)
					THREAD_RETURN_ERROR;
				count+=aktSize;
				pTest->len-=aktSize;
			}while(pTest->len>0);
		THREAD_RETURN_SUCCESS;
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
		
		//Single Thread.....
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
		
		//Multiple Threads....
		CAThread othreadProducer;
		CAThread othreadConsumer;
		othreadProducer.setMainLoop(producer);
		othreadConsumer.setMainLoop(consumer);
		struct __queue_test t1,t2;
		t1.buff=source;
		t2.buff=target;
		t2.len=t1.len=TEST_SIZE;
		t2.pQueue=t1.pQueue=&oQueue;
		othreadConsumer.start(&t2);
		othreadProducer.start(&t1);
		othreadProducer.join();
		othreadConsumer.join();
		if(memcmp(source,target,TEST_SIZE)!=0)
			return E_UNKNOWN;
		
		return E_SUCCESS;
	}
