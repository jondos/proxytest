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
#ifndef _CA_CACHE_LOAD_BALANCING
#define _CA_CACHE_LOAD_BALANCING
#include "CASocketAddrINet.hpp"

struct t_cachelb_list
	{
		CASocketAddrINet* pAddr;
		t_cachelb_list* next;
	};

typedef t_cachelb_list CACHE_LB_ENTRY; 

class CACacheLoadBalancing
	{
		public:
			CACacheLoadBalancing(){m_ElementCount=0;paktEntry=NULL;}
			~CACacheLoadBalancing();
			SINT32 add(CASocketAddrINet* pAddr);
			CASocketAddrINet const * get()
				{
					if(paktEntry==NULL)
						return NULL;
					paktEntry=paktEntry->next;
					return paktEntry->pAddr;
				}
			UINT32 getElementCount(){return m_ElementCount;}
		private:
			CACHE_LB_ENTRY* paktEntry;
			UINT32 m_ElementCount;
	};
#endif