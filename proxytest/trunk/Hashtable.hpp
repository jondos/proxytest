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
		Hashtable(UINT32 (*func)(void *), SINT32 (*func)(void *,void *), SINT32 capacity = 1000,float loadFactor = 0.75);
		~Hashtable();

		void SetHashFunction(UINT32 (*func)(void *));
		void SetCompareFunction(SINT32 (*func)(void *,void *));

		CAMutex& getMutex();

		bool isEmpty();
		bool containsKey(void *key);
		void *getValue(void *key);

		bool put(void *key,void *value);
		void *remove(void *key);

		void makeEmpty(SINT8 keyMode = HASH_EMPTY_NONE,SINT8 valueMode = HASH_EMPTY_NONE);
		
	protected:
		bool rehash();
		struct Entry *getHashEntry(void *key);

		SINT32	fCapacity,fCount,fThreshold,fModCount;
		float	fLoadFactor;
		struct Entry **fTable;
		UINT32	(*fHashFunc)(void *);
		SINT32	(*fCompareFunc)(void *,void *);
		
	private:
		CAMutex m_mutex;
};

#endif  // HASHTABLE_H
