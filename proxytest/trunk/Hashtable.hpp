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

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "CAMutex.hpp"

#define HASH_EMPTY_NONE (SINT8)0
#define HASH_EMPTY_FREE (SINT8)1
#define HASH_EMPTY_DELETE (SINT8)2




class Hashtable
{
	public:
		Hashtable(UINT32 (*func1)(void *), SINT32 (*func2)(void *,void *), SINT32 capacity = 1000,float loadFactor = 0.75);
		~Hashtable();

		/************************** standard string hash functions **************************/
		static UINT32 stringHash(UINT8* str)
		{
			// djb2 algorithm
			
			UINT32 hash = 5381; 
			SINT32 c; 
			while ((c = *str++)) 
			{
				hash = ((hash << 5) + hash) + c; /* hash * 33 + c */ 
			}
			return hash;			
		}
		
		static SINT32 stringCompare(UINT8* a,UINT8* *b)
		{
		  return strcmp((char*)a,(char*)b);
		}

		static UINT32 hashUINT64(UINT64 *a_number)
		{
			return ((*a_number) % 4294967295u);
		}
		static SINT32 compareUINT64(UINT64 *a_numberA, UINT64 *a_numberB)
		{
			return (*a_numberA == *a_numberB)? 0 : ((*a_numberA > *a_numberB)? 1 : -1);
			/*
			if (*a_numberA == *a_numberB)
			{
				return 0;
			}
			else if (*a_numberA > *a_numberB)
			{
				return 1;
			}
			else
			{
				return -1;
			}*/
		}	
		static UINT32 hashUINT32(UINT32 *a_number)
		{			
		 	return *a_number;
		}
		static SINT32 compareUINT32(UINT32 *a_numberA, UINT32 *a_numberB)
		{
			return (*a_numberA == *a_numberB)? 0 : ((*a_numberA > *a_numberB)? 1 : -1);
		}

		CAMutex *getMutex();

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
		CAMutex *m_pMutex;
};

#endif  // HASHTABLE_H
