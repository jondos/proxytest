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
#include "StdAfx.h"
#ifndef ONLY_LOCAL_PROXY
#include "CADatabase.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

CADatabase::CADatabase()
	{
		m_currDatabase=createDBInfo();
		m_prevDatabase=createDBInfo();
		m_nextDatabase=createDBInfo();
		m_lastSwitch=time(NULL);
		m_currentClock=0;
		m_pThread=NULL;
		m_pMutex=new CAMutex();
	}

t_databaseInfo* CADatabase::createDBInfo()
	{
		t_databaseInfo* pInfo=new t_databaseInfo;
		memset(pInfo,NULL,sizeof(t_databaseInfo));
		return pInfo;
	}
	
CADatabase::~CADatabase()
	{
		m_pMutex->lock();
		stop();
		deleteDB(m_currDatabase);
		deleteDB(m_nextDatabase);
		deleteDB(m_prevDatabase);
		m_pMutex->unlock();
		delete m_pMutex;
		m_pMutex = NULL;
	}

SINT32 CADatabase::clearDB(t_databaseInfo* pDBInfo)
	{
		UINT16 tmp,tmp2;
		for(tmp=0;tmp<256;tmp++){
			for(tmp2=0;tmp2<256;tmp2++){
				while (pDBInfo->m_pHashTable[tmp][tmp2]!=NULL){
					t_databaseEntry* anker=pDBInfo->m_pHashTable[tmp][tmp2];
					pDBInfo->m_pHashTable[tmp][tmp2]=anker->next;
					delete anker;
					anker = NULL;
					}
				}
			}
		pDBInfo->m_u32Size=0;
		return E_SUCCESS;
	}

SINT32 CADatabase::deleteDB(t_databaseInfo*& pDBInfo)
	{
		clearDB(pDBInfo);
		delete pDBInfo;
		pDBInfo=NULL;
		return E_SUCCESS;
	}
	
/** Inserts this key in the replay DB.*/
SINT32 CADatabase::insert(UINT8 key[16],UINT64 timestamp)
	{
	// insert hash if timestamp valid in prevDB currDB nextDB
		m_pMutex->lock();

		
		// return E_UNKNOWN if the timestamp is too old or is too far in the future
		if ((timestamp<(m_lastSwitch-SECONDS_PER_INTERVALL))||(timestamp>(time(NULL)+FUTURE_TOLERANCE))) {
			// timestamp not valid!!
			m_pMutex->unlock();
			return E_UNKNOWN;
			}

		t_databaseInfo* aktDB=NULL;
		t_databaseInfo* prevDB=NULL;
		t_databaseInfo* nextDB=NULL;

		if(timestamp<m_lastSwitch){
			aktDB=m_prevDatabase;
			prevDB=NULL;
			nextDB=m_currDatabase;
			}
		else if(timestamp<(m_lastSwitch+SECONDS_PER_INTERVALL)){
			aktDB=m_currDatabase;
			prevDB=m_prevDatabase;
			nextDB=m_nextDatabase;
			}
		else {
			aktDB=m_nextDatabase;
			prevDB=m_currDatabase;
			nextDB=NULL;
			}

		UINT64 hashkey=(key[2]<<56)+(key[3]<<48)+(key[4]<<40)+(key[5]<<32)+(key[6]<<24)+(key[7]<<16)+(key[8]<<8)+key[9];

// insert
		if (prevDB!=NULL){
			t_databaseEntry* tmp=prevDB->m_pHashTable[key[0]][key[1]];

			while(tmp!=NULL){
				if (tmp->key!=hashkey) {
					tmp=tmp->next;
					}
				else {
					// duplicate found!!
					m_pMutex->unlock();
					return E_UNKNOWN;
					}
				}

			// inserting in DB
			tmp=new t_databaseEntry;
			tmp->next=prevDB->m_pHashTable[key[0]][key[1]];
			tmp->key=hashkey;
			prevDB->m_pHashTable[key[0]][key[1]]=tmp;
			prevDB->m_u32Size++;
			}

		t_databaseEntry* tmp=aktDB->m_pHashTable[key[0]][key[1]];

		while(tmp!=NULL){
			if (tmp->key!=hashkey) {
				tmp=tmp->next;
				}
			else {
				// duplicate found!!
				m_pMutex->unlock();
				return E_UNKNOWN;
				}
			}

		// inserting in DB
		tmp=new t_databaseEntry;
		tmp->next=aktDB->m_pHashTable[key[0]][key[1]];
		tmp->key=hashkey;
		aktDB->m_pHashTable[key[0]][key[1]]=tmp;
		aktDB->m_u32Size++;

		if (nextDB!=NULL){
			t_databaseEntry* tmp=nextDB->m_pHashTable[key[0]][key[1]];

			while(tmp!=NULL){
				if (tmp->key!=hashkey) {
					tmp=tmp->next;
					}
				else {
					// duplicate found!!
					m_pMutex->unlock();
					return E_UNKNOWN;
					}
				}

			// inserting in DB
			tmp=new t_databaseEntry;
			tmp->next=nextDB->m_pHashTable[key[0]][key[1]];
			tmp->key=hashkey;
			nextDB->m_pHashTable[key[0]][key[1]]=tmp;
			nextDB->m_u32Size++;
			}

		m_pMutex->unlock();
		return E_SUCCESS;

	}

SINT32 CADatabase::start()
	{
		m_pThread=new CAThread();
		m_bRun=true;
		m_pThread->setMainLoop(db_loopMaintenance);
		return m_pThread->start(this);
	}

SINT32 CADatabase::stop()
	{
		m_bRun=false;
		SINT32 ret=E_SUCCESS;
		if(m_pThread!=NULL)
			{
				ret=m_pThread->join();
				delete m_pThread;
				m_pThread=NULL;
			}
		return ret;
	}

THREAD_RETURN db_loopMaintenance(void *param)
	{
		INIT_STACK;
		BEGIN_STACK("CADatabase::db_loopMaintenance");
		
		CADatabase* pDatabase=(CADatabase*)param;
		while(pDatabase->m_bRun)
			{
				sSleep(10);
				if (pDatabase->m_lastSwitch+SECONDS_PER_INTERVALL<=time(NULL)) {
					pDatabase->nextClock();
					}
			}
			
		FINISH_STACK("CADatabase::db_loopMaintenance");
			
		THREAD_RETURN_SUCCESS;
	}

SINT32 CADatabase::nextClock()
	{
		m_pMutex->lock();
		m_lastSwitch+=SECONDS_PER_INTERVALL;
		CAMsg::printMsg(LOG_DEBUG,"Replay DB Size was: %u\n",m_prevDatabase->m_u32Size);
		clearDB(m_prevDatabase);
		t_databaseInfo* tmpDB=m_prevDatabase;
		m_prevDatabase=m_currDatabase;
		m_currDatabase=m_nextDatabase;
		m_nextDatabase=tmpDB;
		m_pMutex->unlock();
		return E_SUCCESS;
	}

// ?????????
SINT32 CADatabase::test()
	{
/*		CADatabase oDatabase;
		oDatabase.start();
		UINT8 key[16];
		memset(key,0,16);
		UINT32 i;
		for(i=0;i<20;i++)
			{
				getRandom(key,4);
				oDatabase.insert(key,time(NULL));///TODO WRONG - fixme
			}
		for(i=0;i<200000;i++)
			{
				getRandom(key,16);
				oDatabase.insert(key,time(NULL));//TODO WRONG - Fixme
			}
		oDatabase.stop();
*/
		UINT32 entries=10000;
		fill(entries);
		return E_SUCCESS;
//		return CADatabase::fill(entries);
	}

// i don't know if this still works
SINT32 CADatabase::measurePerformance(	UINT8* strLogFile,
																				UINT32 lowerBoundEntries,
																				UINT32 upperBoundEntries,
																				UINT32 stepBy,
																				UINT32 meassuresPerStep,
																				UINT32 insertsPerMeasure)
	{
		initRandom();
		CADatabase* pDatabase=NULL;
		UINT32 aktNrOfEntries=lowerBoundEntries;
		UINT8* key=new UINT8[insertsPerMeasure*16];
		UINT8* aktKey;
		SINT32 file=open((char*)strLogFile,O_CREAT|O_WRONLY|O_LARGEFILE|O_TRUNC,S_IREAD|S_IWRITE);
		char buff[255];
		const char* atemplate="%u,%u,%u\n";
		const char* header="The format is as follows: Number of Entries in DB, Number of Inserts done, Total time for Inserts (in micro seconds)\n";
		write(file,header,strlen(header));
		while(aktNrOfEntries<=upperBoundEntries)
			{
				CAMsg::printMsg(LOG_DEBUG,"Starting measurement with %u entries in the replay database\n",aktNrOfEntries);
				for(UINT32 i=0;i<meassuresPerStep;i++)
					{
						pDatabase=new CADatabase();
						pDatabase->fill(aktNrOfEntries);
						UINT64 startTime,endTime;
						getRandom(key,insertsPerMeasure*16);
						aktKey=key;
						getcurrentTimeMicros(startTime);
						for(UINT32 j=0;j<insertsPerMeasure;j++)
							{
								pDatabase->simulateInsert(aktKey);
								aktKey+=16;
							}
						getcurrentTimeMicros(endTime);
						sprintf(buff,atemplate,aktNrOfEntries,insertsPerMeasure,diff64(endTime,startTime));
						write(file,buff,strlen(buff));
						printf("Start delete \n");
						getcurrentTimeMicros(startTime);
						delete pDatabase;
						pDatabase = NULL;
						getcurrentTimeMicros(endTime);
						printf("delete takes %u microsecs\n",diff64(endTime,startTime));
					}
				aktNrOfEntries+=stepBy;
			}
		delete[] key;
		key = NULL;
		return E_SUCCESS;
	}


SINT32 CADatabase::fill(UINT32 nrOfEntries)
	{
		UINT32 i=0;
		UINT8 key[16];
		while(i<nrOfEntries)
			{
				getRandom(key,16);
				if(insert(key,time(NULL))==E_SUCCESS)
					i++;
			}
		return E_SUCCESS;
	}

// this won't work anymore
SINT32 CADatabase::simulateInsert(UINT8 key[16])
	{
		m_pMutex->lock();
		UINT16 timestamp=(key[14]<<8)|key[15];
		if(timestamp<m_currentClock-1||timestamp>m_currentClock+1)
			{
				//m_pMutex->unlock();
				//return E_UNKNOWN;
			}
		t_databaseInfo* aktDB=m_currDatabase;
		if(timestamp>m_currentClock)
			{
				//aktDB=m_nextDatabase;
			}
		else if(timestamp<m_currentClock)
			{
				//aktDB=m_prevDatabase;
			}
//		UINT16 hashKey=(key[8]<<8)|key[9];
//		t_databaseEntry* hashList=aktDB->m_pHashTable[hashKey];
//		if(hashList==NULL)
//			{
//				LP_databaseEntry newEntry=getNewDBEntry(aktDB);
//				newEntry->left=NULL;
//				newEntry->right=NULL;
//				newEntry->key=key[0]<<24|key[1]<<16|key[2]<<8|key[3];
				//aktDB->m_pHashTable[hashKey]=newEntry;
				aktDB->m_u32Size++;
				m_pMutex->unlock();
				return E_SUCCESS;
/*			}
		else
			{
//				UINT32 ret=key[0]<<24|key[1]<<16|key[2]<<8|key[3];
//				LP_databaseEntry before=NULL;
				do
					{
						//newEntry->keymemcmp(key,hashList->key,6);
						if(ret==hashList->key)
							{
								m_pMutex->unlock();
								return E_UNKNOWN;
							}
						before=hashList;	
						if(hashList->key<ret)
							{
								hashList=hashList->right;
							}
						else
							{
								hashList=hashList->left;
							}
					} while(hashList!=NULL);
//				LP_databaseEntry newEntry=getNewDBEntry(aktDB);
//				newEntry->left=newEntry->right=NULL;
				//memcpy(newEntry->key,key,6);				
//				newEntry->key=ret;
//				if(before->key<ret)
//					{
						//before->right=newEntry;
//					}
//				else
//					{
						//before->left=newEntry;
//					}
			}
		aktDB->m_u32Size++;	
		m_pMutex->unlock();
		return E_SUCCESS;	*/
	}
#endif //Only_LOCAL_PROXY
