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
#include "StdAfx.h"
#include "CACacheLoadBalancing.hpp"

/* Adds a new Address to the pool of Addresses. 
 * This addresses are used for Load Balancing (currently a simple Round Robin).
 * @retval E_UNKNOWN, in case of an error
 * @retval E_SUCCESS, if succesful
 */
SINT32 CACacheLoadBalancing::add(CASocketAddr* const pAddr)
	{
		if(pAddr->getType()!=AF_INET)
			return E_UNKNOWN;
		CACHE_LB_ENTRY* pEntry=new CACHE_LB_ENTRY;
		if(pEntry==NULL)
			return E_UNKNOWN;
		pEntry->pAddr=(CASocketAddrINet*)pAddr->clone();
		m_csLock.lock();
		if(pSelectedEntry==NULL)
			{
				pSelectedEntry=pEntry;
				pSelectedEntry->next=pSelectedEntry;
			}
		else
			{
				pEntry->next=pSelectedEntry->next;
				pSelectedEntry->next=pEntry;						
			}
		m_ElementCount++;
		m_csLock.unlock();
		return E_SUCCESS;
	}
