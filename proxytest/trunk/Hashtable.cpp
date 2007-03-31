/* Hashtable - a general purpose hash table
**
** Copyright 2001, pinc Software. All Rights Reserved.
*/

#include "StdAfx.h"
//#include <stdlib.h>
//#include <stdarg.h>
//#include <string.h>

#include "Hashtable.hpp"

// Every entry in the hashtable is embedded in this structure

struct Entry
{
  struct Entry *e_Next;
  void   *e_Key;
  void   *e_Value;
};


/************************** standard string hash functions **************************/

unsigned int stringHash(char *c)
{
  //unsigned int hash = 0;
  int len = strlen(c);
  
  return(*(int *)(c+len-4));  // erstmal zum Testen
}


int stringCompare(char *a,char *b)
{
  return(!strcmp(a,b));
}


/************************** Hashtable **************************/
// #pragma mark -


/** Erzeugt einen neuen Hashtable nach den Vorgaben des
 *  Benutzers.
 *
 *  @param capacity die Anfangskapazitt des Hashtables. Sie wird zwar bei
 *    Bedarf vergrert, aber das kann dann etwas lnger dauern, da der Hashtable
 *    dann komplett neu aufgebaut werden mu. Voreingestellt sind 100 Eintrge.
 *  @param loadFactor die gewnschte maximale Auslastung des Hashtables.
 *    Bei kleinen Werten mu der Hashtable hufiger neu aufgebaut werden und belegt
 *    mehr Speicher, ist aber schnell. Bei groen Werten wird das Beschreiben und
 *    Auslesen langsamer. Voreingestellt ist 0.75f.
 */

Hashtable::Hashtable(SINT32 capacity, float loadFactor)
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

	fHashFunc = (UINT32 (*)(void *))stringHash;
	fCompareFunc = (int (*)(void *,void *))stringCompare;
}


Hashtable::~Hashtable()
{
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
}


void Hashtable::SetHashFunction(UINT32 (*func)(void *))
{
	fHashFunc = func;
}


void Hashtable::SetCompareFunction(int (*func)(void *,void *))
{
	fCompareFunc = func;
}


/** Testet, ob der Hashtable leer ist.
 *
 *  @return isEmpty true, wenn der Hashtable leer ist, false, wenn nicht
 */

bool Hashtable::IsEmpty()
{
	return fCount == 0;
}


/** Testet, ob ein bestimmter Schlssel im Hashtable enthalten ist.
 *
 *  @param key der gesuchte Schlssel
 *  @return success TRUE, wenn der Schlssel enthalten ist, FALSE, wenn nicht
 */

bool Hashtable::ContainsKey(void *key)
{
	return GetHashEntry(key) ? true : false;
}


/** Sucht den zu einem Schlssel (key) passenden Wert (value)
 *
 *  @param key der gesuchte Schlssel
 *  @return value der zum Schlssel gehrige Wert
 */

void *Hashtable::GetValue(void *key)
{
	struct Entry *e = GetHashEntry(key);

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

bool Hashtable::Put(void *key, void *value)
{
	struct Entry *e = GetHashEntry(key);
	int hash = fHashFunc(key);
	int index;
	
	if (e)
		return true;
	
	fModCount++;
	if (fCount >= fThreshold)
		Rehash();
	
	index = hash % fCapacity;
	
	if (!(e = (struct Entry *)malloc(sizeof(struct Entry))))
		return false;
	
	e->e_Key = key;
	e->e_Value = value;
	e->e_Next = fTable[index];
	fTable[index] = e;  
	fCount++;
	
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

void *Hashtable::Remove(void *key)
{
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

			return value;
		}
		prev = e;
	}
	return NULL;
}


void Hashtable::MakeEmpty(SINT8 keyMode,SINT8 valueMode)
{
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
}


/** Der Hashtable wird in der Kapazitt verdoppelt und neu
 *  aufgebaut.
 *
 *  @return success true, wenn die Kapazitt verdoppelt werden konnte, false, wenn nicht
 */
 
bool Hashtable::Rehash()
{
	UINT32 (*hashCode)(void *) = fHashFunc;
	struct Entry **oldtable = fTable,**newtable;
	int oldCapacity = fCapacity;
	int newCapacity,i;

	newCapacity = oldCapacity * 2 + 1;
	if (!(newtable = (struct Entry **)malloc(newCapacity * sizeof(struct Entry *))))
		return false;
	memset(newtable,0,newCapacity*sizeof(struct Entry *));

	fModCount++;
	fThreshold = (int)(newCapacity * fLoadFactor);
	fTable = newtable;
	fCapacity = newCapacity;

	for(i = oldCapacity;i-- > 0;)
	{
		struct Entry *old,*e = NULL;
		int index;
		
		for (old = oldtable[i];old;)
		{
			e = old;  old = old->e_Next;
			
			index = hashCode(e->e_Key) % newCapacity;
			e->e_Next = newtable[index];
			newtable[index] = e;
		}
	}
	free(oldtable);

	return true;
}


/** Gibt den zu einem Schlssel passenden Eintrag eines Hashtables
 *  zurck.
 *
 *  @param key der zu suchende Schlssel
 *  @return entry der gefundene Eintrag oder NULL
 */

struct Entry *Hashtable::GetHashEntry(void *key)
{
	struct Entry **table,*e;
	UINT32 hash,(*func)(void *);
	
	table = fTable;
	hash = (func = fHashFunc)(key);
	
	for(e = table[hash % fCapacity];e;e = e->e_Next)
	{
		if ((func(e->e_Key) == hash) && fCompareFunc(e->e_Key,key))
			return e;
	}
	return NULL;
}

