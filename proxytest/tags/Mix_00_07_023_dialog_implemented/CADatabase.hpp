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

typedef struct _t_database_entry
	{
		_t_database_entry* next;
		UINT64 key;
	} t_databaseEntry;

typedef struct _t_database_info
	{
		t_databaseEntry*	m_pHashTable[0x100][0x100];
		UINT32			m_u32Size;
	} t_databaseInfo;


#define SECONDS_PER_INTERVALL 30
#define FUTURE_TOLERANCE 5

THREAD_RETURN db_loopMaintenance(void *param);

class CADatabase
	{
		public:
			CADatabase();
			~CADatabase();
			SINT32 insert(UINT8 key[16],UINT64 timestamp);
			SINT32 start();
			SINT32 stop();

			SINT32 test();
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
			static SINT32 measurePerformance(UINT8* strLogFile, UINT32 lowerBoundEntries, UINT32 upperBoundEntries, UINT32 stepBy, UINT32 meassuresPerStep, UINT32 insertsPerMeasure);

		private:
			friend THREAD_RETURN db_loopMaintenance(void *param);

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

			UINT64 m_lastSwitch;
			t_databaseInfo* m_currDatabase;
			t_databaseInfo* m_nextDatabase;
			t_databaseInfo* m_prevDatabase;
			volatile bool m_bRun;
//			UINT32 m_refTime; //the seconds since epoch for the start of interval 0
			volatile SINT32 m_currentClock; //the current 'interval' since m_refTimer
			CAMutex*	m_pMutex;
			CAThread* m_pThread;
	};
#endif //__CA_DATABASE__
#endif //ONLY_LOCAL_PROXY
