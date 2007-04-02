#ifndef HASHTABLE_H
#define HASHTABLE_H
/* Hashtable - a general purpose hash table
**
** Copyright 2001, pinc Software. All Rights Reserved.
*/
#include "CAMutex.hpp"

#define HASH_EMPTY_NONE (SINT8)0
#define HASH_EMPTY_FREE (SINT8)1
#define HASH_EMPTY_DELETE (SINT8)2

class Hashtable
{
	public:
		Hashtable(SINT32 capacity = 1000,float loadFactor = 0.75);
		~Hashtable();

		void SetHashFunction(UINT32 (*func)(void *));
		void SetCompareFunction(SINT32 (*func)(void *,void *));

		CAMutex* getMutex();

		bool IsEmpty();
		bool ContainsKey(void *key);
		void *GetValue(void *key);

		bool Put(void *key,void *value);
		void *Remove(void *key);

		void MakeEmpty(SINT8 keyMode = HASH_EMPTY_NONE,SINT8 valueMode = HASH_EMPTY_NONE);
		
	protected:
		bool Rehash();
		struct Entry *GetHashEntry(void *key);

		SINT32	fCapacity,fCount,fThreshold,fModCount;
		float	fLoadFactor;
		struct Entry **fTable;
		UINT32	(*fHashFunc)(void *);
		SINT32	(*fCompareFunc)(void *,void *);
		
	private:
		CAMutex* m_mutex;
};

#endif  // HASHTABLE_H
