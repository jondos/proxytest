/* Hashtable - a general purpose hash table
**
** Copyright 2001, pinc Software. All Rights Reserved.
*/

#include "StdAfx.h"

#include "Hashtable.hpp"
#include "CAMsg.hpp"

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
  
  return(*(UINT32 *)(c+len-4));  // erstmal zum Testen
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

/** Erzeugt einen neuen Hashtable nach den Vorgaben des
 *  Benutzers.
 *
 *  @param capacity die Anfangskapazitt des Hashtables. Sie wird zwar bei
 *    Bedarf vergroessert, aber das kann dann etwas laenger dauern, da der Hashtable
 *    dann komplett neu aufgebaut werden muss Voreingestellt sind 100 Eintrge.
 *  @param loadFactor die gewueschte maximale Auslastung des Hashtables.
 *    Bei kleinen Werten muss der Hashtable haeufiger neu aufgebaut werden und belegt
 *    mehr Speicher, ist aber schnell. Bei groessen Werten wird das Beschreiben und
 *    Auslesen langsamer. Voreingestellt ist 0.75f.
 */

Hashtable::Hashtable(UINT32 (*hashFunc)(void *), SINT32 (*compareFunc)(void *,void *), SINT32 capacity, float loadFactor)
{
	if (capacity < 0 || loadFactor <= 0)
		return;

	if (!capacity)
		capacity = 1;
	
	if (!(fTable = (struct Entry **)malloc(capacity * sizeof(struct Entry *))))
		return;
	memset(fTable,0,capacity * sizeof(struct Entry *));
	
	fThreshold = (int)(capacity * loadFactor);
	fModCount = 0;
	fLoadFactor = loadFactor;
	fCapacity = capacity;

	//fHashFunc = (UINT32 (*)(void *))stringHash;
	fHashFunc = hashFunc;
	//fCompareFunc = (SINT32 (*)(void *,void *))stringCompare;
	fCompareFunc = compareFunc;
}


Hashtable::~Hashtable()
{
	m_mutex.lock();
	struct Entry **table = fTable;
	
	for(SINT32 index = fCapacity;--index >= 0;)
	{
		struct Entry *e,*next;

		for(e = table[index];e;e = next)
		{
			next = e->e_Next;
			free(e);
		}
	}
	free(table);
	m_mutex.unlock();
}


UINT32 Hashtable::hashUINT64(UINT64 *a_number)
{
	UINT32 temp = 4294967295;
	CAMsg::printMsg( LOG_DEBUG, "Hash modulo: %u", temp);
	UINT32 hash = (UINT32)((*a_number) % temp);
	CAMsg::printMsg( LOG_DEBUG, "Hashed account number: %u", hash);
  
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


/** Testet, ob der Hashtable leer ist.
 *
 *  @return isEmpty true, wenn der Hashtable leer ist, false, wenn nicht
 */

bool Hashtable::isEmpty()
{
	return fCount == 0;
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
 *  @return succes TRUE, wenn der Eintrag hinzugefgt wurde, FALSE, wenn nicht
 */

bool Hashtable::put(void *key, void *value)
{
	m_mutex.lock();
	
	struct Entry *e = getHashEntry(key);
	int hash = fHashFunc(key);
	int index;
	
	if (e)
	{
		m_mutex.unlock();
		return true;
	}
	
	fModCount++;
	if (fCount >= fThreshold)
	{
		rehash();
	}
	
	index = hash % fCapacity;
	
	if (!(e = (struct Entry *)malloc(sizeof(struct Entry))))
	{
		m_mutex.unlock();
		return false;
	}
	
	e->e_Key = key;
	e->e_Value = value;
	e->e_Next = fTable[index];
	fTable[index] = e;  
	fCount++;
	
	m_mutex.unlock();
	return true;
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
	m_mutex.lock();
	
	struct Entry **table,*e,*prev;
	UINT32 hash,(*func)(void *);
	SINT32 index;
	
	table = fTable;
	hash = (func = fHashFunc)(key);
	index = hash % fCapacity;
	
	for(e = table[index],prev = NULL;e;e = e->e_Next)
	{
		if ((func(e->e_Key) == hash) && fCompareFunc(e->e_Key,key))
		{
			void *value;

			fModCount++;
			if (prev)
				prev->e_Next = e->e_Next;
			else
				table[index] = e->e_Next;
			
			fCount--;
			value = e->e_Value;
			free(e);

			m_mutex.unlock();
			return value;
		}
		prev = e;
	}
	
	m_mutex.unlock();
	return NULL;
}


void Hashtable::makeEmpty(SINT8 keyMode,SINT8 valueMode)
{
	m_mutex.lock();
	
	fModCount++;

	for(SINT32 index = fCapacity;--index >= 0;)
	{
		struct Entry *e,*next;

		for(e = fTable[index];e;e = next)
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
			free(e);
		}
		fTable[index] = NULL;
	}
	fCount = 0;
	
	m_mutex.unlock();
}


/** Der Hashtable wird in der Kapazitt verdoppelt und neu
 *  aufgebaut.
 *
 *  @return success true, wenn die Kapazitt verdoppelt werden konnte, false, wenn nicht
 */
 
bool Hashtable::rehash()
{
	m_mutex.lock();
	
	UINT32 (*hashCode)(void *) = fHashFunc;
	struct Entry **oldtable = fTable,**newtable;
	UINT32 oldCapacity = fCapacity;
	UINT32 newCapacity,i;

	newCapacity = oldCapacity * 2 + 1;
	if (!(newtable = (struct Entry **)malloc(newCapacity * sizeof(struct Entry *))))
	{
		m_mutex.unlock();
		return false;
	}
	memset(newtable,0,newCapacity*sizeof(struct Entry *));

	fModCount++;
	fThreshold = (UINT32)(newCapacity * fLoadFactor);
	fTable = newtable;
	fCapacity = newCapacity;

	for(i = oldCapacity;i-- > 0;)
	{
		struct Entry *old,*e = NULL;
		UINT32 index;
		
		for (old = oldtable[i];old;)
		{
			e = old;  old = old->e_Next;
			
			index = hashCode(e->e_Key) % newCapacity;
			e->e_Next = newtable[index];
			newtable[index] = e;
		}
	}
	free(oldtable);
	
	m_mutex.unlock();
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
	m_mutex.lock();
	
	struct Entry **table,*e;
	UINT32 hash,(*func)(void *);
	
	table = fTable;
	hash = (func = fHashFunc)(key);
	
	for(e = table[hash % fCapacity];e;e = e->e_Next)
	{
		if ((func(e->e_Key) == hash) && fCompareFunc(e->e_Key,key))
		{
			m_mutex.unlock();
			return e;
		}
	}
	
	m_mutex.unlock();
	return NULL;
}

