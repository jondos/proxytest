#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "CAMutex.hpp"

#define HASH_EMPTY_NONE (SINT8)0
#define HASH_EMPTY_FREE (SINT8)1
#define HASH_EMPTY_DELETE (SINT8)2




class Hashtable
{
	public:
		Hashtable(UINT32 (*func)(void *), SINT32 (*func)(void *,void *), SINT32 capacity = 1000,float loadFactor = 0.75);
		~Hashtable();

		/************************** standard string hash functions **************************/
		static UINT32 stringHash(UINT8* c)
		{
		  UINT32 len = strlen((char*)c);
		  
		  return(*(UINT32 *)(c+len-4));  
		}
		
		static SINT32 stringCompare(UINT8* a,UINT8* *b)
		{
		  return strcmp((char*)a,(char*)b);
		}

		static UINT32 hashUINT64(UINT64 *a_number);
		static SINT32 compareUINT64(UINT64 *a_numberA, UINT64 *a_numberB);
		static UINT32 hashUINT32(UINT32 *a_number);
		static SINT32 compareUINT32(UINT32 *a_numberA, UINT32 *a_numberB);

		CAMutex& getMutex();

		bool isEmpty();
		bool containsKey(void *key);
		void *getValue(void *key);
		UINT32 getSize();
		UINT32 getCapacity();

		void* put(void *key,void *value);
		void *remove(void *key);

		void clear(SINT8 keyMode = HASH_EMPTY_NONE,SINT8 valueMode = HASH_EMPTY_NONE);
		
	protected:
		bool rehash();
		struct Entry *getHashEntry(void *key);

		SINT32	m_capacity, m_count, m_threshold, m_modCount;
		float	m_loadFactor;
		struct Entry **m_table;
		UINT32	(*m_hashFunc)(void *);
		SINT32	(*m_compareFunc)(void *,void *);
		
	private:
		CAMutex m_mutex;
};

#endif  // HASHTABLE_H
