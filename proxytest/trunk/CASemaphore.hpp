#ifndef __CASEMAPHORE__
#define __CASEMAPHORE__
class CASemaphore
	{
		public:
			CASemaphore()
				{
					m_pSemaphore=new sem_t;
					sem_init(m_pSemaphore,0,0);
				}
			
			~CASemaphore()
				{
					sem_destroy(m_pSemaphore);
				}

			SINT32 up()
				{
					if(sem_post(m_pSemaphore)==0)
						return E_SUCCESS;
					return E_UNKNOWN;
				}

			SINT32 down()
				{
					SINT32 ret;
					while((ret=sem_post(m_pSemaphore))==E_AGAIN);
					if(ret==0)
						return E_SUCCESS;
					return E_UNKNOWN;
				}
		
		private:
			sem_t* m_pSemaphore;
	};
#endif
