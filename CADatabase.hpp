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

#include "CAMutex.hpp"
#include "CAThread.hpp"

typedef struct __t_database_entry
	{
		__t_database_entry* next;
		UINT8 key[6];
	} t_databaseEntry; 

typedef t_databaseEntry* LP_databaseEntry;

class CADatabase
	{
		public:
			CADatabase();
			~CADatabase();
			SINT32 insert(UINT8 key[16]);
			SINT32 start();
			SINT32 stop();
			/** Returns the current Replay timestamp for this database*/
			SINT32 getCurrentReplayTimestamp(tReplayTimestamp& replayTimestamp);
			static SINT32 test();
		private:
			friend THREAD_RETURN db_loopMaintenance(void *param);

			SINT32 nextClock();
			LP_databaseEntry* m_currDatabase;
			LP_databaseEntry* m_nextDatabase;
			LP_databaseEntry* m_prevDatabase;
			bool m_bRun;
			UINT32 m_refTime; //the seconds since epoch for the start of interval 0
			UINT32 m_currentClock; //the current 'interval' since m_refTimer
			SINT32 getClockForTime(UINT32); //returns the intervall for the seconds since epoch
			CAMutex m_oMutex;
			CAThread* m_pThread;
	};
