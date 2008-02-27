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
#include "CATempIPBlockList.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"


CATempIPBlockList::CATempIPBlockList(UINT64 validTimeMillis)
	{
		m_validTimeMillis = validTimeMillis;

		m_hashTable=new PTEMPIPBLOCKLIST[0x10000];
		memset(m_hashTable,0,0x10000*sizeof(PTEMPIPBLOCKLIST));
	
		m_pMutex = new CAMutex();
	
		// launch cleanup thread
		m_pCleanupThread = new CAThread((UINT8*)"Cleanup Thread");
		m_bRunCleanupThread=true;
		m_pCleanupThread->setMainLoop(cleanupThreadMainLoop);
		m_pCleanupThread->start(this);
	}



CATempIPBlockList::~CATempIPBlockList()
	{
		CAMsg::printMsg(LOG_DEBUG, "CATmpIPBlockList terminating..\n");
		//Now stop the cleanup thread...
		m_bRunCleanupThread=false;
		m_pCleanupThread->join(); //wait for cleanupthread to wakeup and exit
		m_pMutex->lock();
		//its safe to delete it because we have the lock...
		for(UINT32 i=0;i<=0xFFFF;i++) 
			{
				PTEMPIPBLOCKLIST entry=m_hashTable[i];
				PTEMPIPBLOCKLIST tmpEntry;
				while(entry!=NULL)
					{
						tmpEntry=entry;
						entry=entry->next;
						delete tmpEntry;
					}
			}
		delete [] m_hashTable;
		m_pMutex->unlock();
		delete m_pMutex;
	}



/** 
	* inserts an IP into the blocklist 
	*
	* @retval E_SUCCESS if successful
	* @retval E_UNKNOWN if IP was already in blocklist
	*/
SINT32 CATempIPBlockList::insertIP(const UINT8 ip[4])
{
	UINT64 now;
	getcurrentTimeMillis(now);
	
	PTEMPIPBLOCKLIST newEntry;
	newEntry = new TEMPIPBLOCKLISTENTRY;
	memcpy(newEntry->ip,ip,2);
	newEntry->validTimeMillis = now + m_validTimeMillis;
	newEntry->next=NULL;	
	
	UINT16 hashvalue=(ip[2]<<8)|ip[3];
	m_pMutex->lock();
	
	if(m_hashTable[hashvalue]==NULL) {
		m_hashTable[hashvalue] = newEntry;
	}
	else {
		PTEMPIPBLOCKLIST temp = m_hashTable[hashvalue];
		do {
			if(memcmp(temp->ip,ip,2)==0) {
				// we have found the entry
				m_pMutex->unlock();
				return E_UNKNOWN;
			}
			temp = temp->next;
		}
		while(temp->next);
		temp->next = newEntry;
	}
	m_pMutex->unlock();	
	return E_SUCCESS;
}



/** 
 * check whether an IP is blocked 
 *
 * @retval E_SUCCESS, if the IP is not blocked
 * @retval E_UNKNOWN, if the IP is blocked
 */
SINT32 CATempIPBlockList::checkIP(const UINT8 ip[4])
{
	UINT16 hashvalue=(ip[2]<<8)|ip[3];
	m_pMutex->lock();
	PTEMPIPBLOCKLIST entry = m_hashTable[hashvalue];
	PTEMPIPBLOCKLIST previous = NULL;
	while(entry) {
		if(memcmp(entry->ip,ip,2)==0) {
			// we have found the entry
			// additional check: is it still valid?
			UINT64 now;
			getcurrentTimeMillis(now);
			if(entry->validTimeMillis <= now) {
				// entry can be removed
				if(previous==NULL) {
					m_hashTable[hashvalue] = entry->next;
				}
				else {
					previous->next = entry->next;
				}
				delete entry;
				m_pMutex->unlock();
				return E_SUCCESS;
			}
			else {
				m_pMutex->unlock();
				return E_UNKNOWN;
			}
		}
		previous = entry;
		entry = entry->next;
	}
	m_pMutex->unlock();
	return E_SUCCESS;
}



/** 
 * the cleanup thread main loop 
 */
THREAD_RETURN CATempIPBlockList::cleanupThreadMainLoop(void *param)
	{
		INIT_STACK;
		BEGIN_STACK("CATempIPBlockList::cleanupThreadMainLoop");
		
		CATempIPBlockList * instance;
		instance = (CATempIPBlockList *)param;
		while(instance->m_bRunCleanupThread) 
			{				
				// do cleanup
				UINT64 now;
				getcurrentTimeMillis(now);
				instance->m_pMutex->lock();
				for(UINT32 i=0;i<=0xFFFF;i++) 
					{
						PTEMPIPBLOCKLIST entry=instance->m_hashTable[i];
						PTEMPIPBLOCKLIST previous = NULL;
						while(entry!=NULL)
							{
								if(entry->validTimeMillis <= now) 
									{
										// entry can be removed
										if(previous==NULL)
											{
												CAMsg::printMsg(LOG_DEBUG, "CATmpIPBlockList: removing entry...\n");
												instance->m_hashTable[i] = entry->next;
												previous=entry->next;
												delete entry; 
												entry=previous;
												previous=NULL;
										}
								else
									{
										previous->next = entry->next;
										delete entry;
										entry = previous->next;
									}
							}
					else {
					// entry is still valid
					previous = entry;
					entry = entry->next;
				}
			}
		}
		instance->m_pMutex->unlock();

		// let the thread sleep for 1 minute
		sSleep(CLEANUP_THREAD_SLEEP_INTERVAL);
	}
	
	FINISH_STACK("CATempIPBlockList::cleanupThreadMainLoop");
	
	THREAD_RETURN_SUCCESS;
}

