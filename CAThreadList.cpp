#include "StdAfx.h"

#ifdef _DEBUG

#include "CAThread.hpp"
#include "CAThreadList.hpp"
#include "CAMsg.hpp"

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
	thread_list_entry_t *iterator;
	
	if(m_pHead == NULL)
	{
		CAMsg::printMsg(LOG_INFO, "CAThreadList::showAll: list is empty, no threads found\n");
		return;
	}
	
	iterator = m_pHead;
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
	thread_list_entry_t *iterator;
	
	while(m_pHead != NULL)
	{
		iterator=m_pHead->tle_next;

		CAMsg::printMsg(LOG_INFO, "CAThreadList::removeAll: Thread %s, id %x\n", 
										m_pHead->tle_thread->getName(),
										m_pHead->tle_thread->getID());
			delete m_pHead;
			m_pHead=iterator;
		}
		
		m_pHead = NULL;
		m_Size=0;
	}

#endif
