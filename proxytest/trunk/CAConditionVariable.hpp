#ifndef __CACONVAR__
#define __CACONVAR__
#include "CAMutex.hpp"
class CAConditionVariable:public CAMutex
	{
		public:
			CAConditionVariable()
				{
					m_pCondVar=new pthread_cond_t;
					pthread_cond_init(m_pCondVar,NULL);
				}

			~CAConditionVariable()
				{
					pthread_cond_destroy(m_pCondVar);
					delete m_pCondVar;
				}
			
			SINT32 wait()
				{
					if(pthread_cond_wait(m_pCondVar,m_pMutex)==0)
						return E_SUCCESS;
					return E_UNKNOWN;
				}

/*			SINT32 wait(UINT32 msSeconds)
				{
					struct timespec t;
					t
					if(pthread_cond_timedwait(m_pCondVar,m_pMutex,&t)==0)
						return E_SUCCESS;
					return E_UNKNOWN;
				}
*/
			SINT32 signal()
				{
					if(pthread_cond_signal(m_pCondVar)==0)
						return E_SUCCESS;
					return E_UNKNOWN;
				}

		private:
			pthread_cond_t* m_pCondVar;
	};
#endif
