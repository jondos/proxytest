#ifndef CATHREADLIST_H_
#define CATHREADLIST_H_

//#include "StdAfx.hpp"
//#include "CAMsg.hpp"
//#include "CAThread.hpp"

struct thread_list_entry
{
	CAThread *tle_thread;
	pthread_t tle_thread_id;
	struct thread_list_entry *tle_next;
};

typedef struct thread_list_entry thread_list_entry_t;

/* Just a simple list implementation to get hold of all created threads */
class CAThreadList
{

public:
	CAThreadList();
	virtual ~CAThreadList();
	
	UINT32 put(CAThread *thread, pthread_t thread_id);
	CAThread *remove(pthread_t thread_id);
	CAThread *get(CAThread *thread, pthread_t thread_id);
	void showAll();
	UINT32 getSize();
	
private:
	
	UINT32 __put(CAThread *thread, pthread_t thread_id);
	CAThread *__remove(pthread_t thread_id);
	CAThread *__get(pthread_t thread_id);
	void __showAll();
	void __removeAll();
	UINT32 __getSize();
	
	thread_list_entry_t *m_pHead;
	CAMutex *m_pListLock;
};

#endif /*CATHREADLIST_H_*/
