/********************************************************
 * A Thread pool class inspired by:
 * "Using POSIX Threads: Programming with Pthreads"
 *     by Brad nichols, Dick Buttlar, Jackie Farrell
 *     O'Reilly & Associates, Inc.
 */
#include "StdAfx.h"
#include "CAThreadPool.hpp"
void *tpool_thread(void *);

CAThreadPool::CAThreadPool(	UINT32 num_worker_threads, 
														UINT32 max_queue_size,
														bool	 b_do_not_block_when_full)
	{
  int i, rtn;
  
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

  /* create threads */
  for (i = 0; i != num_worker_threads; i++) 
		{
			m_parThreads[i]=new CAThread();
			m_parThreads[i]->setMainLoop(worker_thread_main_loop);
			m_parThreads[i]->start(this);
		}
}

/** Adds a new reuest (task) to this threadpool.
	* @retval E_SPACe if there was  no more space in the waiting queue 
	* and we do not want to wait for an other request to finish
	* @retval E_SUCCESS if this request was added to the working queue
	*/
SINT32 CAThreadPool::addRequest(THREAD_MAIN_TYP routine, void *args)
{
  int rtn;
	m_mutexQueue.lock();
  tpool_work_t *workp;

  // no space and this caller doesn't want to wait 
  if ((m_CurQueueSize == m_MaxQueueSize) && m_bDoNotBlockWhenFull) 
		{
			m_mutexQueue.unlock();
			return E_SPACE;
		}

  while((m_CurQueueSize == m_MaxQueueSize) && 
				(!(m_bShutdown || m_bQueueClosed))  )
		{
			m_condNotFull.wait(m_mutexQueue);
  }

  // the pool is in the process of being destroyed 
  if (m_bShutdown || m_bQueueClosed)
		{
			m_mutexQueue.unlock();
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
			m_condNotEmpty.broadcast();
		}
	else
		{
			m_pQueueTail->next = workp;
			m_pQueueTail = workp;
		}

  m_CurQueueSize++; 
	m_mutexQueue.unlock();
  return E_SUCCESS;
}

SINT32 CAThreadPool::destroy(bool bWaitForFinish)
	{
		int i,rtn;
		tpool_work_t *cur_nodep;
		m_mutexQueue.lock();
		// Is a shutdown already in progress?
		if (m_bQueueClosed || m_bShutdown)
			{
				m_mutexQueue.unlock();
				return E_SUCCESS;
			}

		m_bQueueClosed = true;
		// If the finish flag is set, wait for workers to 
		//   drain queue  
		if (bWaitForFinish)
			{
				while (m_CurQueueSize != 0)
					{
						m_condEmpty.wait(m_mutexQueue);
					}
			}

		m_bShutdown = true;
		m_mutexQueue.unlock();

		// Wake up any workers so they recheck shutdown flag 
		m_condNotEmpty.broadcast();
		m_condNotFull.broadcast();

		// Wait for workers to exit 
		for(i=0; i < m_NumThreads; i++) 
			{
				m_parThreads[i]->join();
			}
		// Now free pool structures 
		delete[] m_parThreads;
		while(m_pQueueHead != NULL)
			{
				cur_nodep = m_pQueueHead->next; 
				m_pQueueHead = m_pQueueHead->next;
				delete cur_nodep;
			}
		return E_SUCCESS;
	}

THREAD_RETURN worker_thread_main_loop(void *arg)
{
  CAThreadPool* pPool = (CAThreadPool*)arg; 
  int rtn;
  tpool_work_t	*my_workp;
	
  for(;;)
		{
    // Check queue for work  
			pPool->m_mutexQueue.lock();
			while ((pPool->m_CurQueueSize == 0) && (!pPool->m_bShutdown))
				{
					pPool->m_condNotEmpty.wait(pPool->m_mutexQueue);
				}
			//sSleep(5); 
			// Has a shutdown started while i was sleeping? 
			if (pPool->m_bShutdown)
				{
					pPool->m_mutexQueue.unlock();
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
					pPool->m_condNotFull.broadcast();
			// Handle waiting destroyer threads 
			if (pPool->m_CurQueueSize == 0)
				pPool->m_condEmpty.signal();
			pPool->m_mutexQueue.unlock();
      
			// Do this work item 
			(*(my_workp->routine))(my_workp->arg);
			delete my_workp;
  } 
  THREAD_RETURN_SUCCESS;            
}
