#ifndef __CAMUTEX__
#define __CAMUTEX__

class CAMutex
	{
		public:
			CAMutex()
				{
					m_pMutex=new pthread_mutex_t;
					pthread_mutex_init(m_pMutex,NULL);
				}

			~CAMutex()
				{
					pthread_mutex_destroy(m_pMutex);
					delete m_pMutex;
				}

			SINT32 lock()
				{
					if(pthread_mutex_lock(m_pMutex)==0)
						return E_SUCCESS;
					return E_UNKNOWN;
				}

			SINT32 unlock()
				{
					if(pthread_mutex_unlock(m_pMutex)==0)
						return E_SUCCESS;
					return E_UNKNOWN;
				}

		protected:
			pthread_mutex_t* m_pMutex;
	};
#endif
