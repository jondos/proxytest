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
