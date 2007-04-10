/*
Copyright (c) 2000-2007, The JAP-Team All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
 
- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
 
- Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided
with the distribution.
 
- Neither the name of the University of Technology Dresden,
Germany nor the names of its contributors may be used to endorse
or promote products derived from this software without specific
prior written permission.
 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE 
*/

#include "StdAfx.h"

#include "Hashtable.hpp"
#include "CAMsg.hpp"
#include "CAUtil.hpp"

// Every entry in the hashtable is embedded in this structure

struct Entry
{
  struct Entry *e_Next;
  void   *e_Key;
  void   *e_Value;
};


/************************** standard string hash functions **************************/


UINT32 stringHash(char *c)
{
  UINT32 len = strlen(c);
  
  return(*(UINT32 *)(c+len-4));  
}

SINT32 stringCompare(char *a,char *b)
{
  return(!strcmp(a,b));
}


/************************** Hashtable **************************/
// #pragma mark -


CAMutex& Hashtable::getMutex()
{
	return m_mutex;
}


/* Hashtable - a general purpose hash table
 * Erzeugt einen neuen Hashtable nach den Vorgaben des
 *  Benutzers.
 *
 *  @param capacity die Anfangskapazitt des Hashtables. Sie wird zwar bei
 *    Bedarf vergroessert, aber das kann dann etwas laenger dauern, da der Hashtable
 *    dann komplett neu aufgebaut werden muss Voreingestellt sind 100 Eintrge.
 *  @param loadFactor die gewueschte maximale Auslastung des Hashtables.
 *    Bei kleinen Werten muss der Hashtable haeufiger neu aufgebaut werden und belegt
 *    mehr Speicher, ist aber schnell. Bei groessen Werten wird das Beschreiben und
 *    Auslesen langsamer. Voreingestellt ist 0.75f.
 * 
**/
Hashtable::Hashtable(UINT32 (*hashFunc)(void *), SINT32 (*compareFunc)(void *,void *), SINT32 capacity, float loadFactor)
{
	if (capacity < 0 || loadFactor <= 0)
	{
		return;
	}

	if (!capacity)
	{
		capacity = 1;
	}	
	
	m_table = new Entry*[capacity];
	for (UINT32 i = 0; i < capacity; i++)
	{
		m_table[i] = NULL;
	}
	
	m_threshold = (UINT32)(capacity * loadFactor);
	m_modCount = 0;
	m_count = 0;
	m_loadFactor = loadFactor;
	m_capacity = capacity;

	//m_hashFunc = (UINT32 (*)(void *))stringHash;
	m_hashFunc = hashFunc;
	//m_compareFunc = (SINT32 (*)(void *,void *))stringCompare;
	m_compareFunc = compareFunc;
}


Hashtable::~Hashtable()
{
	struct Entry **table = m_table;
	
	for(SINT32 index = m_capacity;--index >= 0;)
	{
		struct Entry *e,*next;

		for(e = table[index];e;e = next)
		{
			next = e->e_Next;
			delete e;
		}
	}
	delete table;
}


UINT32 Hashtable::hashUINT64(UINT64 *a_number)
{
	UINT32 hash = ((*a_number) % 4294967295u);
 	return hash;
}

SINT32 Hashtable::compareUINT64(UINT64 *a_numberA, UINT64 *a_numberB)
{
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
	}
}	


/** 
 *  Tests if the hashtable is empty.
 *  @return true if empty; false otherwise
 */

bool Hashtable::isEmpty()
{
	return m_count == 0;
}


/** Testet, ob ein bestimmter Schlssel im Hashtable enthalten ist.
 *
 *  @param key der gesuchte Schlssel
 *  @return success TRUE, wenn der Schlssel enthalten ist, FALSE, wenn nicht
 */

bool Hashtable::containsKey(void *key)
{
	return getHashEntry(key) ? true : false;
}


/** Sucht den zu einem Schlssel (key) passenden Wert (value)
 *
 *  @param key der gesuchte Schlssel
 *  @return value der zum Schlssel gehrige Wert
 */

void *Hashtable::getValue(void *key)
{
	struct Entry *e = getHashEntry(key);

	return e ? e->e_Value : NULL;
}


/** Mit dieser Funktion wird ein neuer Eintrag bestehend aus einem
 *  Zugriffsschlssel (key) und dessen Wert (value) in den Hashtable
 *  eingefgt.
 *  Gibt es diesen Schlssel bereits im Hashtable, wird der alte
 *  Eintrag berschrieben.
 *
 *  @short einen neuen Eintrag in einen Hashtable einfgen
 *
 *  @param key der Schlssel
 *  @param value der zugehrige Wert
 *  @return the old value if a value has been overwritten, or NULL if the value did not exist before
 */

void* Hashtable::put(void *key, void *value)
{
	struct Entry *oldEntry = getHashEntry(key);
	struct Entry *e = NULL;
	UINT32 hash = m_hashFunc(key);
	UINT32 index;
	
	if (oldEntry)
	{
		value = oldEntry->e_Value;
	}
	
	m_modCount++;
	if (m_modCount >= m_threshold)
	{
		
		rehash();
	}
	
	index = hash % m_capacity;

	e = new Entry;
	e->e_Key = key;
	e->e_Value = value;
	
	if (m_table[index])
	{		
		e->e_Next = m_table[index];
	}
	else
	{
		e->e_Next = NULL;
	}
	m_table[index] = e; 
	m_count++;
	
	return value;
}


/** @short entfernt einen Eintrag aus einem Hashtable
 *
 *  Mit dieser Funktion entfernt man den zum Schlssel (key)
 *  gehrigen Eintrag aus dem Hashtable.
 *
 *  @param key der Schlssel
 *
 *  @return value der zum Schlssel gehrende Wert
 */

void *Hashtable::remove(void *key)
{
	struct Entry **table,*e,*prev;
	UINT32 hash,(*func)(void *);
	SINT32 index;
	
	table = m_table;
	hash = (func = m_hashFunc)(key);
	index = hash % m_capacity;
	
	for(e = table[index],prev = NULL;e;e = e->e_Next)
	{
		if (e == NULL)
		{
			return NULL;
		}
		
		if ((func(e->e_Key) == hash) && m_compareFunc(e->e_Key,key))
		{
			void *value;

			m_modCount++;
			if (prev)
			{
				prev->e_Next = e->e_Next;
			}
			else
			{
				table[index] = e->e_Next;
			}
			
			m_count--;
			value = e->e_Value;
			delete e;

			return value;
		}
		prev = e;
	}
	
	return NULL;
}


void Hashtable::makeEmpty(SINT8 keyMode,SINT8 valueMode)
{
	m_modCount++;

	for(SINT32 index = m_capacity;--index >= 0;)
	{
		struct Entry *e,*next;

		if (!m_table[index])
		{
			continue;
		}
		for(e = m_table[index];e;e = next)
		{
			switch(keyMode)
			{
				case HASH_EMPTY_DELETE:
					delete e->e_Key;
					break;
				case HASH_EMPTY_FREE:
					free(e->e_Key);
					break;
			}
			switch(valueMode)
			{
				case HASH_EMPTY_DELETE:
					delete e->e_Value;
					break;
				case HASH_EMPTY_FREE:
					free(e->e_Value);
					break;
			}
			next = e->e_Next;
			delete e;
		}
		m_table[index] = NULL;
	}
	m_count = 0;
}

UINT32 Hashtable::getSize()
{
	return m_count;
}

UINT32 Hashtable::getCapacity()
{
	return m_capacity;
}


/** Der Hashtable wird in der Kapazitt verdoppelt und neu
 *  aufgebaut.
 *
 *  @return success true, wenn die Kapazitt verdoppelt werden konnte, false, wenn nicht
 */
 
bool Hashtable::rehash()
{
	UINT32 (*hashCode)(void *) = m_hashFunc;
	struct Entry **oldtable = m_table,**newtable;
	UINT32 oldCapacity = m_capacity;
	UINT32 newCapacity,i;

	newCapacity = oldCapacity * 2 + 1;
	//if (!(newtable = (struct Entry **)malloc(newCapacity * sizeof(struct Entry *))))
	{
		//return false;
	}
	newtable = new Entry*[newCapacity];
	for (UINT32 i = 0; i < newCapacity; i++)
	{
		newtable[i] = NULL;
	}

	m_modCount++;
	if (m_modCount > 10)
	{
		CAMsg::printMsg(LOG_INFO, "Hashtable: Too many rehash operations!\n");
	}
	
	m_threshold = (UINT32)(newCapacity * m_loadFactor);
	m_table = newtable;
	m_capacity = newCapacity;

	for(i = oldCapacity;i-- > 0;)
	{
		struct Entry *old,*e = NULL;
		UINT32 index;
		
		for (old = oldtable[i];old;)
		{
			e = old;  
			old = old->e_Next;
			
			index = hashCode(e->e_Key) % newCapacity;
			e->e_Next = newtable[index];
			newtable[index] = e;
		}
	}
	delete oldtable;
	
	return true;
}


/** Gibt den zu einem Schlssel passenden Eintrag eines Hashtables
 *  zurck.
 *
 *  @param key der zu suchende Schlssel
 *  @return entry der gefundene Eintrag oder NULL
 */

struct Entry *Hashtable::getHashEntry(void *key)
{	
	struct Entry **table,*e;
	UINT32 hash,(*func)(void *);
	
	table = m_table;
	hash = (func = m_hashFunc)(key);
	UINT32 index = hash % m_capacity;
	
	for(e = table[index];e;e = e->e_Next)
	{
		if (e == NULL)
		{
			return NULL;
		}
		
		
		if ((func(e->e_Key) == hash) && !m_compareFunc(e->e_Key,key))
		{
			return e;
		}
	}
	
	return NULL;
}

