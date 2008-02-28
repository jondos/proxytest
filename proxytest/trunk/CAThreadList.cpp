#include "StdAfx.h"

#ifdef _DEBUG

#include "CAThread.hpp"
#include "CAThreadList.hpp"
#include "CAMsg.hpp"

CAThreadList::CAThreadList()
{
	m_pHead = NULL;
	m_pListLock = new CAMutex();
}

CAThreadList::~CAThreadList()
{
	m_pListLock->lock();
	__removeAll();
	m_pListLock->unlock();
	
	delete m_pListLock;
	m_pListLock = NULL;
}


/* safe functions */
UINT32 CAThreadList::put(CAThread *thread, pthread_t thread_id)
{
	UINT32 ret = 0;
	
	m_pListLock->lock();
	ret = __put(thread, thread_id);
	m_pListLock->unlock();
	
	return ret;
}

CAThread *CAThreadList::remove(pthread_t thread_id)
{
	CAThread *ret;
	
	m_pListLock->lock();
	ret = __remove(thread_id);
	m_pListLock->unlock();
		
	return ret;
}

CAThread *CAThreadList::get(CAThread *thread, pthread_t thread_id)
{
	CAThread *ret;
	
	m_pListLock->lock();
	ret = __get(thread_id);
	m_pListLock->unlock();
		
	return ret;
}

void CAThreadList::showAll()
{
	m_pListLock->lock();
	__showAll();
	m_pListLock->unlock();
}

UINT32 CAThreadList::getSize()
{
	UINT32 ret = 0;
	
	m_pListLock->lock();
	ret = __getSize();
	m_pListLock->unlock();
	
	return ret;
}

/* unsafe functions */
UINT32 CAThreadList::__put(CAThread *thread, pthread_t thread_id)
{
	thread_list_entry_t *new_entry = new thread_list_entry_t();
	//thread_list_entry_t *iterator;
	
	new_entry->tle_thread = thread;
	new_entry->tle_thread_id = thread_id;
	new_entry->tle_next = m_pHead;
	
	m_pHead = new_entry;	
		
	return 0;
}

CAThread *CAThreadList::__remove(pthread_t thread_id)
{
	thread_list_entry_t *iterator;
	thread_list_entry_t *prev;
	CAThread *match = NULL;
	
	//CAMsg::printMsg(LOG_INFO, "CAThreadList::remove: try to remove thread with id %x\n", thread_id);
	
	if(m_pHead == NULL)
	{
		return NULL;
	}
	
	iterator = m_pHead;
	prev = NULL;
	do 
	{
		if(iterator->tle_thread_id == thread_id)
		{
			match = iterator->tle_thread;
			//CAMsg::printMsg(LOG_INFO, "CAThreadList::remove: found Thread %s, id %x to remove\n",
			//		match->getName(), thread_id);
			if(prev != NULL)
			{
				//unchain
				prev->tle_next = iterator->tle_next;
			}
			else
			{
				//Head holds the corresponding thread;
				m_pHead = iterator->tle_next;
				//if(m_pHead==NULL)
				//	CAMsg::printMsg(LOG_INFO, "CAThreadList::remove: Thread %s, id %x emptied list\n",
				//								match->getName(), thread_id);
			}
			//dispose the entry
			iterator->tle_thread = NULL;
			iterator->tle_next = NULL;
			
			delete iterator;
			iterator = NULL;
			//CAMsg::printMsg(LOG_INFO, "CAThreadList::remove: Thread %s, id %x successfully removed\n",
			//					match->getName(), thread_id);
			return match;
		}
		prev = iterator;
		iterator = iterator->tle_next;
	} while(iterator != NULL);
	
	
	//CAMsg::printMsg(LOG_INFO, "CAThreadList::remove: no thread found with id %x\n", thread_id);
	
	return NULL;
}

CAThread *CAThreadList::__get(pthread_t thread_id)
{
	thread_list_entry_t *iterator;
	
	if(m_pHead == NULL)
	{
		return NULL;
	}
	
	iterator = m_pHead;
	do 
	{
		if(iterator->tle_thread_id == thread_id)
		{
			return iterator->tle_thread;
		}
		iterator = iterator->tle_next;
	} while(iterator != NULL);
	
	return NULL;
}

void CAThreadList::__showAll()
{
	thread_list_entry_t *iterator;
	
	if(m_pHead == NULL)
	{
		CAMsg::printMsg(LOG_INFO, "CAThreadList::showAll: list is empty, no threads found\n");
		return;
	}
	
	iterator = m_pHead;
	do 
	{
		CAMsg::printMsg(LOG_INFO, "CAThreadList::showAll: Thread %s, id %x\n", 
											iterator->tle_thread->getName(),
											iterator->tle_thread_id);
		iterator = iterator->tle_next;
	} while(iterator != NULL);
}

// This only deletes the list entries not the corresponding threads!
void CAThreadList::__removeAll()
{
	thread_list_entry_t *iterator;
	
	if(m_pHead != NULL)
	{
		while(m_pHead->tle_next != NULL)
		{
			iterator = m_pHead->tle_next;
			m_pHead->tle_next = iterator->tle_next;
			
			CAMsg::printMsg(LOG_INFO, "CAThreadList::removeAll: Thread %s, id %x\n", 
										iterator->tle_thread->getName(),
										iterator->tle_thread_id);
			iterator->tle_thread = NULL;
			iterator->tle_next = NULL;
			delete iterator;
			iterator = NULL;
		}
		
		CAMsg::printMsg(LOG_INFO, "CAThreadList::removeAll: Thread %s, id %x\n", 
									m_pHead->tle_thread->getName(),
									m_pHead->tle_thread_id);
		m_pHead->tle_thread = NULL;
		delete m_pHead;
		m_pHead = NULL;
	}
	/*else 
	{
		CAMsg::printMsg(LOG_INFO, "CAThreadList::removeAll: list is already empty\n");
	}*/
}

UINT32 CAThreadList::__getSize()
{
	thread_list_entry_t *iterator;
	UINT32 count = 0; 
		
	if(m_pHead == NULL)
	{
		return count;
	}
	
	iterator = m_pHead;
	
	do 
	{
		count++;
		iterator = iterator->tle_next;
	} while(iterator != NULL);
		
	return count;
}

#endif