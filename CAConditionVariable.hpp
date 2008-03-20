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
#ifndef __CACONVAR__
#define __CACONVAR__
#ifndef ONLY_LOCAL_PROXY

#include "CAMutex.hpp"
#include "CAUtil.hpp"
#include "CASemaphore.hpp"

class CAConditionVariable:public CAMutex
	{
		public:
			CAConditionVariable()
				{
					#ifdef HAVE_PTHREAD_CV
						m_pCondVar=new pthread_cond_t;
						pthread_cond_init(m_pCondVar,NULL);
					#else
						m_pMutex=new CAMutex();
						m_pSemaphore=new CASemaphore(0);
						m_iSleepers=0;
					#endif
				}

			~CAConditionVariable()
				{
					#ifdef HAVE_PTHREAD_CV
						pthread_cond_destroy(m_pCondVar);
						delete m_pCondVar;
					#else
						delete m_pMutex;
						delete m_pSemaphore;
					#endif
				}
			
				/** Waits for a signal or for a timeout.
				* Note: lock() must be called before wait() and unlock() 
				* must be called if proccessing ends.
				* @retval E_SUCCESS if signaled
				* @retval E_UNKNOWN if an error occured
				*/
			SINT32 wait()
				{
					#ifdef HAVE_PTHREAD_CV
						if(pthread_cond_wait(m_pCondVar,m_pMutex)==0)
							return E_SUCCESS;
						return E_UNKNOWN;
					#else
						m_pMutex->lock();
						unlock(); // Release the lock that is associated with our cv
						m_iSleepers++;
						m_pMutex->unlock();
						m_pSemaphore->down();
						return lock();
					#endif
				}

			/** Very ugly shortly to be deleted, uncommented function!
				*/
			SINT32 wait(CAMutex& oMutex)
				{
					#ifdef HAVE_PTHREAD_CV
						if(pthread_cond_wait(m_pCondVar,oMutex.m_pMutex)==0)
							return E_SUCCESS;
						return E_UNKNOWN;
					#else
						m_pMutex->lock();
						oMutex.unlock(); // Release the lock that is associated with our cv
						m_iSleepers++;
						m_pMutex->unlock();
						m_pSemaphore->down();
						return oMutex.lock();
					#endif
				}

			/** Very ugly shortly to be deleted, uncommented function!
				*/
			SINT32 wait(CAMutex* pMutex)
				{
					#ifdef HAVE_PTHREAD_CV
						if(pthread_cond_wait(m_pCondVar,pMutex->m_pMutex)==0)
							return E_SUCCESS;
						return E_UNKNOWN;
					#else
						m_pMutex->lock();
						pMutex->unlock(); // Release the lock that is associated with our cv
						m_iSleepers++;
						m_pMutex->unlock();
						m_pSemaphore->down();
						return pMutex->lock();
					#endif
				}
	
				/** Waits for a signal or for a timeout.
				* Note: lock() must be called before wait() and unlock() 
				* must be called if proccessing ends.
				* @param msTimeout timout value in millis seconds
				* @retval E_SUCCESS if signaled
				* @retval E_TIMEDOUT if timout was reached
				* @retval E_UNKNOWN if an error occured
				*/
			SINT32 wait(UINT32 msTimeout)
				{
					#ifdef HAVE_PTHREAD_CV
						timespec to;
						getcurrentTime(to);
						to.tv_nsec+=(msTimeout%1000)*1000000;
						to.tv_sec+=msTimeout/1000;
						if(to.tv_nsec>999999999)
							{
								to.tv_sec++;
								to.tv_nsec-=1000000000;
							}
						int ret=pthread_cond_timedwait(m_pCondVar,m_pMutex,&to);
						if(ret==0)
							return E_SUCCESS;
						else if(ret==ETIMEDOUT)
							return E_TIMEDOUT;
						return E_UNKNOWN;
					#else
						///@todo add something better here....
						return wait();
					#endif
				}

			/** Signals this object. One of the threads waiting on this object will awake.
				* Note: lock() must be called before signal() and unlock() 
				* must be called if proccessing ends.
				*/
			SINT32 signal()
				{
					#ifdef HAVE_PTHREAD_CV
						if(pthread_cond_signal(m_pCondVar)==0)
							return E_SUCCESS;
						return E_UNKNOWN;
					#else
						m_pMutex->lock();
						if( m_iSleepers > 0 )
							{
								m_pSemaphore->up();
								m_iSleepers--;
							}
						return m_pMutex->unlock();
					#endif
				}

			/** Signals this object. All threads waiting on this object will awake.
				* Note: lock() must be called before broadcast() and unlock() 
				* must be called if proccessing ends.
				*/
			SINT32 broadcast()
				{
					#ifdef HAVE_PTHREAD_CV
						if(pthread_cond_broadcast(m_pCondVar)==0)
							return E_SUCCESS;
						return E_UNKNOWN;
					#else
						m_pMutex->lock();
						while( m_iSleepers > 0 )
							{
								m_pSemaphore->up();
								m_iSleepers--;
							}
						return m_pMutex->unlock();
					#endif
				}

		private:
				#ifdef HAVE_PTHREAD_CV
					pthread_cond_t* m_pCondVar;
				#else
					CAMutex* m_pMutex;
					CASemaphore* m_pSemaphore;
					UINT32 m_iSleepers;
				#endif
	};
#endif
#endif //ONLY_LOCAL_PROXY
