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
	for (SINT32 i = 0; i < capacity; i++)
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
	m_mutex.lock();
	
	for(SINT32 index = 0; index < m_capacity; index++)
	{
		struct Entry *e,*next;

		for(e = m_table[index]; e; e = next)
		{
			next = e->e_Next;
			delete e;
		}
		m_table[index] = NULL;
	}
	delete m_table;
	m_table = NULL;
	m_capacity = 0;
	m_count = 0;
	m_threshold = 0;
	m_loadFactor = 0;
	m_modCount = 0;
	m_hashFunc = NULL;
	m_compareFunc = NULL;
	
	m_mutex.unlock();
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
	//CAMsg::printMsg(LOG_INFO, "Hashtable: Putting key.\n");
	
	if (!key || !value || !m_table)
	{
		return NULL;
	}
	
	struct Entry *oldEntry = NULL;
	struct Entry *newEntry = NULL;
	UINT32 hash = m_hashFunc(key);
	UINT32 index = hash % m_capacity;
	
	m_count++;
	if (m_count >= m_threshold)
	{		
		rehash();
	}
	
	index = hash % m_capacity;
	
	newEntry = new Entry;
	newEntry->e_Key = key;
	newEntry->e_Value = value;
	newEntry->e_Next = NULL;
	
	oldEntry = m_table[index];
	if (!oldEntry)
	{
		m_table[index] = newEntry;
	}
	else
	{
		struct Entry *prevEntry = NULL;
		for(; oldEntry; oldEntry = oldEntry->e_Next)
		{			
			if (m_hashFunc(oldEntry->e_Key) == hash && !m_compareFunc(oldEntry->e_Key,key))
			{
				// found the same entry
				newEntry->e_Next = oldEntry->e_Next;
				break;
			}
			prevEntry = oldEntry;
		}
		if (prevEntry)
		{
			prevEntry->e_Next = newEntry;
		}
		else
		{
			m_table[index] = newEntry;
		}
	}
	
	return oldEntry;
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
	//CAMsg::printMsg(LOG_INFO, "Hashtable: Removing key.\n");
	
	if (!key || !m_table)
	{
		return NULL;
	}
	
	struct Entry *e,*prev;
	UINT32 hash = m_hashFunc(key);
	
	for(e = m_table[hash % m_capacity], prev = NULL; e; e = e->e_Next)
	{  		
		if (m_hashFunc(e->e_Key) == hash && !m_compareFunc(e->e_Key,key))
		{
			void *value;

			//m_modCount++;
			if (prev)
			{
				prev->e_Next = e->e_Next;
			}
			else
			{
				m_table[hash % m_capacity] = e->e_Next;
			}
			
			m_count--;
			value = e->e_Value;
			e->e_Value = NULL;
			e->e_Key = NULL;
			e->e_Next = NULL;
			delete e;

			return value;
		}
		prev = e;
	}
	
	return NULL;
}


void Hashtable::clear(SINT8 keyMode,SINT8 valueMode)
{	
	if (!m_table)
	{
		return;
	}
	
	//CAMsg::printMsg(LOG_INFO, "Hashtable: Clearing...\n");

	for(SINT32 index = 0; index < m_capacity; index++)
	{
		struct Entry *e,*next;

		for(e = m_table[index]; e; e = next)
		{
			switch(keyMode)
			{
				case HASH_EMPTY_DELETE:
					if (e->e_Key)
					{
						delete e->e_Key;
						e->e_Key = NULL;
					}
					break;
//				case HASH_EMPTY_FREE:
//					free(e->e_Key);
				default:
					e->e_Key = NULL;		
			}
			switch(valueMode)
			{
				case HASH_EMPTY_DELETE:
					if (e->e_Value)
					{
						delete e->e_Value;
						e->e_Value = NULL;
					}
					break;
//				case HASH_EMPTY_FREE:
//					free(e->e_Value);
				default:
					e->e_Value = NULL;
			}
			next = e->e_Next;
			delete e;
		}
		m_table[index] = NULL;
	}
	m_count = 0;
	
	//CAMsg::printMsg(LOG_INFO, "Hashtable: Has been cleared!\n");
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
	if (!m_table)
	{
		return false;
	}
	
	CAMsg::printMsg(LOG_INFO, "Hashtable: Rehashing.\n");
	
	UINT32 (*hashCode)(void *) = m_hashFunc;
	struct Entry **oldtable = m_table,**newtable;
	SINT32 oldCapacity = m_capacity;
	UINT32 newCapacity;

	newCapacity = oldCapacity * 2 + 1;
	newtable = new Entry*[newCapacity];
	for (UINT32 i = 0; i < newCapacity; i++)
	{
		newtable[i] = NULL;
	}

	m_modCount++;
	if (m_modCount > 10)
	{
		CAMsg::printMsg(LOG_INFO, "Hashtable: Too many rehash operations! Please adapt hashtable capacity to at least %d.\n", newCapacity);
	}
	
	m_threshold = (UINT32)(newCapacity * m_loadFactor);
	m_table = newtable;
	m_capacity = newCapacity;

	for(SINT32 i = 0; i < oldCapacity; i++)
	{
		struct Entry *nextEntry, *e = NULL;
		UINT32 index;
		
		for (nextEntry = oldtable[i]; nextEntry;)
		{
			e = nextEntry;  
			// save the previous next entry, as it will be deleted later
			nextEntry = nextEntry->e_Next;
						
			index = hashCode(e->e_Key) % newCapacity;
			/* If there has been another entry before, replace it.
			 * Delete the previous next entry in the same step.
			 */
			e->e_Next = newtable[index]; 
			newtable[index] = e;
		}
	}
	delete oldtable;
	
	CAMsg::printMsg(LOG_INFO, "Hashtable: Rehashing complete.\n");
	
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
	if (!key || !m_table)
	{
		return NULL;
	}
	
	struct Entry *e;
	UINT32 hash = m_hashFunc(key);
	
	for(e = m_table[hash % m_capacity]; e; e = e->e_Next)
	{		
		if (m_hashFunc(e->e_Key) == hash && !m_compareFunc(e->e_Key,key))
		{
			return e;
		}
	}
	
	return NULL;
}

