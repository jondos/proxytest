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
#include "CAMutex.hpp"

struct t_cachelb_list
	{
		CASocketAddrINet* pAddr;
		struct t_cachelb_list* next;
	};

typedef t_cachelb_list CACHE_LB_ENTRY; 

/** This class stores Addresses off different Cache-Proxies. It can be used
  * for Load Balancing between them. Currently a simple Ropund Robin is implemented.
	* 
	*/
class CACacheLoadBalancing
	{
		public:
			CACacheLoadBalancing()
				{
					m_ElementCount=0;
					pSelectedEntry=NULL;
				}
				~CACacheLoadBalancing(){clean();}

			/** Deletes all information*/
			SINT32 clean()
				{
					m_csLock.lock();
					CACHE_LB_ENTRY* pEntry;
					CACHE_LB_ENTRY* pFirst=pSelectedEntry;
					while(pSelectedEntry!=NULL)
						{
							if(pSelectedEntry->next==pFirst)
								pEntry=NULL;
							else 
								pEntry=pSelectedEntry->next;
							delete pSelectedEntry->pAddr;
							pSelectedEntry->pAddr = NULL;
							delete pSelectedEntry;
							pSelectedEntry = NULL;
							pSelectedEntry=pEntry;
						}
					m_ElementCount=0;
					m_csLock.unlock();
					return E_SUCCESS;
				}

			SINT32 add(CASocketAddr* const pAddr);
			
			/** Gets the 'next' Address according to the Load-Balancing algorithm. 
			  * This is the Address which should be used for a connection to a cache proxy.
				* @return next Address of a Cache-Proxy
				*/
			CASocketAddrINet* get()
				{
					m_csLock.lock();
					if(pSelectedEntry==NULL)
						{
							m_csLock.unlock();
							return NULL;
						}
					pSelectedEntry=pSelectedEntry->next;
					m_csLock.unlock();
					return pSelectedEntry->pAddr;
				}

			/* Gets the number of Addresses added.
			 * @return number of added Addresses
			 */
			UINT32 getElementCount()
				{
					return m_ElementCount;
				}

		private:
			CACHE_LB_ENTRY* pSelectedEntry;
			UINT32 m_ElementCount;
			CAMutex m_csLock;
	};
#endif
