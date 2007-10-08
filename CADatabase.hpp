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
#ifndef ONLY_LOCAL_PROXY
#include "CAMutex.hpp"
#include "CAThread.hpp"
#ifndef __CA_DATABASE__
#define __CA_DATABASE__
typedef struct __t_database_entry
	{
		__t_database_entry* left;
		__t_database_entry* right;
		UINT32 key;
	} t_databaseEntry; 

typedef t_databaseEntry* LP_databaseEntry;

#define DB_ENTRIES_PER_HEAP 1000

typedef struct __t_database_heap
	{
		t_databaseEntry m_pEntries[DB_ENTRIES_PER_HEAP];
		__t_database_heap* next;
	} t_databaseHeap;
	
//Stores management info for this database
typedef struct __t_database_info
	{
		LP_databaseEntry	m_pHashTable[0x10000];
		UINT32						m_u32Size;
		t_databaseHeap*		m_pHeap;
		t_databaseHeap*		m_pLastHeap;
		SINT32						m_s32FreeEntries;
	} t_databaseInfo;
	
#define SECONDS_PER_INTERVALL 600

THREAD_RETURN db_loopMaintenance(void *param);

class CADatabase
	{
		public:
			CADatabase();
			~CADatabase();
			SINT32 insert(UINT8 key[16]);
			SINT32 start();
			SINT32 stop();
			/** Returns the current Replay timestamp for this database.*/
			SINT32 getCurrentReplayTimestamp(tReplayTimestamp& replayTimestamp) const;

			/** Returns the local time in seconds since epoch for replay timestamp='0' for this database*/
			UINT32 getRefTime() const
				{
					return m_refTime;
				}
			
			/** Returns the replay timestamp for this reference time (seconds since epoch) and time*/
			static SINT32 getReplayTimestampForTime(tReplayTimestamp& replayTimestamp,UINT32 aktTime,UINT32 refTime);

			/** Returns the refernce time (seconds since epoch) for the given replay timestamp*/
			static SINT32 getTimeForReplayTimestamp(UINT32& refTime,tReplayTimestamp replayTimestamp)
				{
					time_t now=time(NULL);
					refTime=(UINT32)(now-replayTimestamp.interval*SECONDS_PER_INTERVALL-replayTimestamp.offset);
					return E_SUCCESS;
				}

			static SINT32 test();
			/** This mehtod can be used to measure the performance of the Replay database. The results are stored in a file in csv format.
				* Ths method will do several measures with different numbers of elements in the database. These number could be specified
				* using owerBoundEntries,upperBoundEntries and stepBy.
				*
				* @param strLogFile the log file name
				* @param lowerBoundEntries the number of entries in the database (at beginn)
				* @param upperBoundEntries the number of entries in the database (at end)
				* @param stepBy how many entries should be added for each new measurement
				* @param meassuresPerStep how many measure values should be generate per step. That means that the experiement is repeated this many times.
				* @param insertsPerMeasure one measure value will be the time: (Total Insertion Time)/insertsPerMeasure
				*/
			static SINT32 measurePerformance(	UINT8* strLogFile,
																				UINT32 lowerBoundEntries,
																				UINT32 upperBoundEntries,
																				UINT32 stepBy,
																				UINT32 meassuresPerStep,
																				UINT32 insertsPerMeasure);
		private:
			friend THREAD_RETURN db_loopMaintenance(void *param);
			LP_databaseEntry getNewDBEntry(t_databaseInfo* pDB)
				{
					if(pDB->m_pHeap==NULL)
						{
							pDB->m_pHeap=new t_databaseHeap;
							pDB->m_pHeap->next=NULL;
							pDB->m_pLastHeap=pDB->m_pHeap;
							pDB->m_s32FreeEntries=DB_ENTRIES_PER_HEAP;
						}
					else if(pDB->m_s32FreeEntries==0)
						{
							pDB->m_pLastHeap->next=new t_databaseHeap;
							pDB->m_pLastHeap=pDB->m_pLastHeap->next;
							pDB->m_pLastHeap->next=NULL;
							pDB->m_s32FreeEntries=DB_ENTRIES_PER_HEAP;
						}
					return &pDB->m_pLastHeap->m_pEntries[--pDB->m_s32FreeEntries];	
				}
			 
			/** Creates and initialises a dbinfo struct*/
			t_databaseInfo* createDBInfo();
			
			/** clears the whole database pDB - but does not delete the hashtable pDB
				* @param pDB database to delete
				*/
			SINT32 clearDB(t_databaseInfo* pDB);
			
			/** Deletes the whole database pDB.
				* @param pDB database to delete
				*/
			SINT32 deleteDB(t_databaseInfo*& pDB);

			SINT32 nextClock();
			/** Pre fills the database with nrOfEntries random entries
				* @param nrOfEntries number of entries to put in the database
				*/
			SINT32 fill(UINT32 nrOfEntries);

			/** This is a modified copy of insert() which simulates the insert() function as close as possible without actually changing
			  * the replay database
				*/
			SINT32 simulateInsert(UINT8 key[16]);

			t_databaseInfo* m_currDatabase;
			t_databaseInfo* m_nextDatabase;
			t_databaseInfo* m_prevDatabase;
			volatile bool m_bRun;
			UINT32 m_refTime; //the seconds since epoch for the start of interval 0
			volatile SINT32 m_currentClock; //the current 'interval' since m_refTimer
			CAMutex*	m_pMutex;
			CAThread* m_pThread;
	};
#endif //__CA_DATABASE__
#endif //ONLY_LOCAL_PROXY
