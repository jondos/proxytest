#ifndef __CATHREAD__
#define __CATHREAD__
typedef void *(*THREAD_MAIN_TYP)(void *);
#include "CAConditionVariable.hpp"
class CAThread
	{
		public:
			CAThread();
			SINT32 setMainLoop(THREAD_MAIN_TYP fnc)
				{
					m_fncMainLoop=fnc;
					return E_SUCCESS;
				}

			SINT32 start(void* param);
			SINT32 join()
				{
					if(pthread_join(*m_pThread,NULL)==0)
						return E_SUCCESS;
					else
						return E_UNKNOWN;
				}

/*			SINT32 sleep(UINT32 msSeconds)
				{
					m_CondVar.lock();
					m_CondVar.wait(msSeconds);
					m_CondVar.unlock();
					return E_SUCCESS;
				}

			SINT32 wakeup()
				{
					m_CondVar.lock();
					m_CondVar.unlock();
					m_CondVar.signal();
					return E_SUCCESS;
				}
*/
		private:
			THREAD_MAIN_TYP m_fncMainLoop;
	 		pthread_t* m_pThread;
			CAConditionVariable m_CondVar;
	};
#endif

