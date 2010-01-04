/********************************************************
 * A thread pool class inspired by:
 * "Using POSIX Threads: Programming with Pthreads"
 *     by Brad nichols, Dick Buttlar, Jackie Farrell
 *     O'Reilly & Associates, Inc.
 *
 ********************************************************/

#ifndef __CATHREADPOOL__
#define __CATHREADPOOL__

#include "CAMutex.hpp"
#ifndef ONLY_LOCAL_PROXY
#include "CAThread.hpp"
#include "CAConditionVariable.hpp"

struct tpool_work 
	{
		THREAD_MAIN_TYP			routine;
		void                *arg;
		struct tpool_work   *next;
	};

typedef struct tpool_work tpool_work_t;

THREAD_RETURN worker_thread_main_loop(void *args);

/** This class bla bla
 * 
 */ 
class CAThreadPool 
	{
		public:
			CAThreadPool(	UINT32 num_worker_threads, 
																	UINT32 max_queue_size,
																	bool	 b_do_not_block_when_full);
			~CAThreadPool()
				{
					destroy(true);
				}
				/**
				 * 
				 */ 
			SINT32 destroy(bool bWaitForFinish);
			SINT32 addRequest(THREAD_MAIN_TYP, void *args);
			UINT32 countRequests()
			{
				return m_CurQueueSize;
			}
			friend THREAD_RETURN worker_thread_main_loop(void *args);
		private:
			/* pool characteristics */
			UINT32				m_NumThreads;
      UINT32				m_MaxQueueSize;
			bool					m_bDoNotBlockWhenFull;
      /* pool state */
			CAThread**			m_parThreads;
      volatile UINT32	m_CurQueueSize;
			tpool_work_t*		m_pQueueHead;
			tpool_work_t*		m_pQueueTail;
			volatile bool		m_bQueueClosed;
      volatile bool		m_bShutdown;
			/* pool synchronization */
      CAMutex*							m_pmutexQueue;
      CAConditionVariable*	m_pcondNotEmpty;
      CAConditionVariable*	m_pcondNotFull;
      CAConditionVariable*	m_pcondEmpty;
	};
#endif //ONLY_LOCAL_PROXY

#endif 
