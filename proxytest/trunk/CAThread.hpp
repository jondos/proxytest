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
#ifndef __CATHREAD__
#define __CATHREAD__

typedef void *(*THREAD_MAIN_TYP)(void *);
/**
	Some example on CAThread:

	THREAD_RETURN_TYPE doSomeThing(void* param)
		{
			THREAD_RETURN_SUCCESS
		}
	

	CAThread* pThread=new CAThread();
	pThread->setMainLoop(doSomeThing);
	pThread->start(theParams);
	pThread->join();
	delete pThread;

	*/
class CAThread
	{
		public:
			CAThread();
			~CAThread()
				{
					if(m_pThread!=NULL)
						delete m_pThread;
				}

			SINT32 setMainLoop(THREAD_MAIN_TYP fnc)
				{
					m_fncMainLoop=fnc;
					return E_SUCCESS;
				}
			
			SINT32 start(void* param,bool bDaemon=false);
			SINT32 join()
				{
					if(m_pThread==NULL)
						return E_SUCCESS;
					if(pthread_join(*m_pThread,NULL)==0)
						{
							delete m_pThread;
							m_pThread=NULL;
							return E_SUCCESS;
						}
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
			//CAConditionVariable m_CondVar;
	};
#endif

