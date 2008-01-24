/********************************************************
 * A Thread pool class inspired by:
 * "Using POSIX Threads: Programming with Pthreads"
 *     by Brad nichols, Dick Buttlar, Jackie Farrell
 *     O'Reilly & Associates, Inc.
 */
#include "StdAfx.h"
#ifndef ONLY_LOCAL_PROXY
#include "CAThreadPool.hpp"
#include "CAMsg.hpp"
void *tpool_thread(void *);

CAThreadPool::CAThreadPool(	UINT32 num_worker_threads, 
														UINT32 max_queue_size,
														bool	 b_do_not_block_when_full)
	{
		UINT i;
	  
		/* initialize fields */
		m_NumThreads = num_worker_threads;
		m_MaxQueueSize = max_queue_size;
		m_bDoNotBlockWhenFull = b_do_not_block_when_full;
		m_parThreads=new CAThread*[num_worker_threads];
		m_CurQueueSize = 0;
		m_pQueueHead = NULL; 
		m_pQueueTail = NULL;
		m_bQueueClosed = false;  
		m_bShutdown = false; 
		m_pmutexQueue=new CAMutex();
		m_pcondEmpty=new CAConditionVariable();
		m_pcondNotEmpty=new CAConditionVariable();
		m_pcondNotFull=new CAConditionVariable();

		/* create threads */
		for (i = 0; i != num_worker_threads; i++) 
			{
				m_parThreads[i]=new CAThread();
				m_parThreads[i]->setMainLoop(worker_thread_main_loop);
				m_parThreads[i]->start(this);
			}
	}

/** Adds a new request (task) to this threadpool.
	* @retval E_SPACe if there was  no more space in the waiting queue 
	* and we do not want to wait for an other request to finish
	* @retval E_SUCCESS if this request was added to the working queue
	*/
SINT32 CAThreadPool::addRequest(THREAD_MAIN_TYP routine, void *args)
{
	m_pmutexQueue->lock();
  tpool_work_t *workp;

  // no space and this caller doesn't want to wait 
  if ((m_CurQueueSize == m_MaxQueueSize) && m_bDoNotBlockWhenFull) 
		{
			m_pmutexQueue->unlock();
			return E_SPACE;
		}

  while((m_CurQueueSize == m_MaxQueueSize) && 
				(!(m_bShutdown || m_bQueueClosed))  )
		{
			CAMsg::printMsg(LOG_INFO,"CAThreadPool::addRequest() -the Thread pool is full...waiting!\n");
			m_pcondNotFull->wait(m_pmutexQueue);
		}

  // the pool is in the process of being destroyed 
  if (m_bShutdown || m_bQueueClosed)
		{
			m_pmutexQueue->unlock();
			return E_UNKNOWN;
		}

  // allocate work structure 
  workp = new tpool_work_t;
	workp->routine = routine;
  workp->arg = args;
  workp->next = NULL;

  if (m_CurQueueSize == 0)
		{
			m_pQueueTail = m_pQueueHead = workp;
			m_pcondNotEmpty->broadcast();
		}
	else
		{
			m_pQueueTail->next = workp;
			m_pQueueTail = workp;
		}

  m_CurQueueSize++; 
	m_pmutexQueue->unlock();
  return E_SUCCESS;
}

SINT32 CAThreadPool::destroy(bool bWaitForFinish)
	{
		tpool_work_t *cur_nodep;
		// Is a shutdown already in progress?
		if (m_bQueueClosed || m_bShutdown)
			{
				return E_SUCCESS;
			}

		m_pmutexQueue->lock();
		m_bQueueClosed = true;
		// If the finish flag is set, wait for workers to 
		//   drain queue  
		if (bWaitForFinish)
			{
				while (m_CurQueueSize != 0)
					{
						m_pcondEmpty->wait(m_pmutexQueue);
					}
			}

		m_bShutdown = true;
		m_pmutexQueue->unlock();

		// Wake up any workers so they recheck shutdown flag 
		m_pcondNotEmpty->broadcast();
		m_pcondNotFull->broadcast();

		// Wait for workers to exit 
		for(UINT32 i=0; i < m_NumThreads; i++) 
			{
				m_parThreads[i]->join();
				delete m_parThreads[i];
			}
		// Now free pool structures 
		delete[] m_parThreads;
		while(m_pQueueHead != NULL)
			{
				cur_nodep = m_pQueueHead->next; 
				m_pQueueHead = m_pQueueHead->next;
				delete cur_nodep;
			}
		delete m_pmutexQueue;
		delete m_pcondEmpty;
		delete m_pcondNotEmpty;
		delete m_pcondNotFull;

		return E_SUCCESS;
	}

THREAD_RETURN worker_thread_main_loop(void *arg)
{
  CAThreadPool* pPool = (CAThreadPool*)arg; 
  tpool_work_t	*my_workp;
	
  for(;;)
		{
    // Check queue for work  
			pPool->m_pmutexQueue->lock();
			while ((pPool->m_CurQueueSize == 0) && (!pPool->m_bShutdown))
				{
					pPool->m_pcondNotEmpty->wait(pPool->m_pmutexQueue);
				}
			//sSleep(5); 
			// Has a shutdown started while i was sleeping? 
			if (pPool->m_bShutdown)
				{
					pPool->m_pmutexQueue->unlock();
					THREAD_RETURN_SUCCESS;
				}

			// Get to work, dequeue the next item  
			my_workp = pPool->m_pQueueHead;
			pPool->m_CurQueueSize--;
			if (pPool->m_CurQueueSize == 0)
				pPool->m_pQueueHead = pPool->m_pQueueTail = NULL;
			else
				pPool->m_pQueueHead = my_workp->next;
 
			// Handle waiting add_work threads 
			if ((!pPool->m_bDoNotBlockWhenFull) &&
					(pPool->m_CurQueueSize ==  (pPool->m_MaxQueueSize - 1))) 
					pPool->m_pcondNotFull->broadcast();
			// Handle waiting destroyer threads 
			if (pPool->m_CurQueueSize == 0)
				pPool->m_pcondEmpty->signal();
			pPool->m_pmutexQueue->unlock();
      
			// Do this work item 
			(*(my_workp->routine))(my_workp->arg);
			delete my_workp;
  } 
}
#endif //ONLY_LOCAL_PROXY
