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
#ifndef __CAMUTEX__
#define __CAMUTEX__
class CAConditionVariable;
#ifndef HAVE_PTHREAD_MUTEXES
	#include "CASemaphore.hpp"
#endif

class CAMutex
	{
		public:
			CAMutex();
			virtual ~CAMutex();

			SINT32 lock()
				{
					#ifdef	HAVE_PTHREAD_MUTEXES
						if(pthread_mutex_lock(m_pMutex)==0)
							return E_SUCCESS;
						return E_UNKNOWN;
					#else
						return m_pMutex->down();
					#endif
				}

			SINT32 unlock()
				{
					#ifdef HAVE_PTHREAD_MUTEXES
						if(pthread_mutex_unlock(m_pMutex)==0)
							return E_SUCCESS;
						return E_UNKNOWN;
					#else
						return m_pMutex->up();
					#endif
				}
		
		friend class CAConditionVariable;
		protected:
			#ifdef HAVE_PTHREAD_MUTEXES
				pthread_mutex_t* m_pMutex;
				pthread_mutexattr_t* m_pMutexAttributes;
			#else
				CASemaphore* m_pMutex;
			#endif
	};
#endif
