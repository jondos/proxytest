typedef void *(*THREAD_MAIN_TYP)(void *);

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
		private:
			THREAD_MAIN_TYP m_fncMainLoop;
	 		pthread_t* m_pThread;
	};
