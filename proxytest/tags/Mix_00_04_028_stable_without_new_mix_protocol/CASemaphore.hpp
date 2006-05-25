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
#ifndef __CASEMAPHORE__
#define __CASEMAPHORE__
#undef USE_SEMAPHORE
#ifdef USE_SEMAPHORE
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
#endif
