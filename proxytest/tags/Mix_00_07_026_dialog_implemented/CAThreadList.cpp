/*
Copyright (c) The JAP-Team, JonDos GmbH

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation and/or
       other materials provided with the distribution.
    * Neither the name of the University of Technology Dresden, Germany, nor the name of
       the JonDos GmbH, nor the names of their contributors may be used to endorse or
       promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "StdAfx.h"

#if defined (_DEBUG) && ! defined (ONLY_LOCAL_PROXY)

#include "CAThread.hpp"
#include "CAThreadList.hpp"
#include "CAMsg.hpp"

/**
 * @author Simon Pecher, JonDos GmbH
 */

CAThreadList::CAThreadList()
{
	m_pHead = NULL;
	m_Size=0;
	m_pListLock = new CAMutex();
}

CAThreadList::~CAThreadList()
{
	m_pListLock->lock();
	removeAll();
	m_pListLock->unlock();
	
	delete m_pListLock;
	m_pListLock = NULL;
}


/* safe functions */
SINT32 CAThreadList::put(const CAThread* const thread)
{
	m_pListLock->lock();
	thread_list_entry_t *new_entry = new thread_list_entry_t;
	
	new_entry->tle_thread = (CAThread* const)thread;
	new_entry->tle_next = m_pHead;
	m_pHead = new_entry;	
	m_Size++;		
	m_pListLock->unlock();
	
	return E_SUCCESS;
}

SINT32 CAThreadList::remove(const CAThread* const thread)
{
	SINT32 ret=E_SUCCESS;
	
	m_pListLock->lock();
	thread_list_entry_t* iterator=m_pHead;
	thread_list_entry_t* prev=NULL;
		
	while (iterator!=NULL) 
	{
		if(iterator->tle_thread->getID() == thread->getID())
			{
				if(prev != NULL)
					{
						//unchain
						prev->tle_next = iterator->tle_next;
					}
				else
					{
						//Head holds the corresponding thread;
						m_pHead = iterator->tle_next;
					}
				//dispose the entry			
				delete iterator;
				m_Size--;
				break;
			}
		prev = iterator;
		iterator = iterator->tle_next;
	};

	m_pListLock->unlock();
	return ret;
}

/*
CAThread *CAThreadList::get(CAThread *thread)
{
	CAThread *ret=NULL;
	m_pListLock->lock();
	thread_list_entry_t *iterator=m_pHead;
	while(iterator!=NULL) 
		{
			if(thread->getID()==iterator->tle_thread->getID())
				{
					ret=iterator->tle_thread;
					break;
				}
			iterator = iterator->tle_next;
		} 
	m_pListLock->unlock();
	return ret;
}
*/

void CAThreadList::showAll() const
{
	m_pListLock->lock();
	
	if(m_pHead == NULL)
	{
		CAMsg::printMsg(LOG_INFO, "CAThreadList::showAll: list is empty, no threads found\n");
		m_pListLock->unlock();
		return;
	}
	
	thread_list_entry_t *iterator = m_pHead;
	do 
	{
		CAMsg::printMsg(LOG_INFO, "CAThreadList::showAll: Thread %s, id %u\n", 
											iterator->tle_thread->getName(),
											iterator->tle_thread->getID());
		iterator = iterator->tle_next;
	} while(iterator != NULL);
	m_pListLock->unlock();
}

// This only deletes the list entries not the corresponding threads!
void CAThreadList::removeAll()
{
	thread_list_entry_t* iterator=NULL;
	
	while(m_pHead != NULL)
		{
			iterator=m_pHead->tle_next;

			CAMsg::printMsg(LOG_INFO, "CAThreadList::removeAll: Thread %s, id %u\n", 
											m_pHead->tle_thread->getName(),
											m_pHead->tle_thread->getID());
			delete m_pHead;
			m_pHead=iterator;
		}
		
		m_pHead = NULL;
		m_Size=0;
	}

#endif
