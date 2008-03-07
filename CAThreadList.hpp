#ifndef CATHREADLIST_H_
#define CATHREADLIST_H_

#if defined (DEBUG) && !defined(ONLY_LOCAL_PROXY)
struct thread_list_entry
{
	CAThread* tle_thread;
	struct thread_list_entry *tle_next;
};

typedef struct thread_list_entry thread_list_entry_t;

/* Just a simple list implementation to get hold of all created threads */
class CAThreadList
{

public:
	CAThreadList();
	virtual ~CAThreadList();
	
	SINT32	put(const CAThread* const thread);
	SINT32	remove(const CAThread* const thread);
	//CAThread* get(CAThread *thread);
	void showAll() const;
	UINT32 getSize() const
		{
			return m_Size;
		}
	
private:
	
	void			removeAll();
	
	UINT32 m_Size;
	thread_list_entry_t* m_pHead;
	CAMutex* m_pListLock;
};

#endif /*CATHREADLIST_H_*/
#endif //DEBUG