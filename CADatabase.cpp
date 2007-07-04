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
		m_refTime=time(NULL);
		m_currentClock=0;
		m_pThread=NULL;
		m_pMutex=new CAMutex();
	}

t_databaseInfo* CADatabase::createDBInfo()
	{
		t_databaseInfo* pInfo=new t_databaseInfo;
		memset(pInfo,0,sizeof(t_databaseInfo));
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
	}

SINT32 CADatabase::clearDB(t_databaseInfo* pDBInfo)
	{
		while(pDBInfo->m_pHeap!=NULL)
			{
				pDBInfo->m_pLastHeap=pDBInfo->m_pHeap;
				pDBInfo->m_pHeap=pDBInfo->m_pHeap->next;
				delete pDBInfo->m_pLastHeap;
			}
		memset(pDBInfo,0,sizeof(t_databaseInfo));
		return E_SUCCESS;
	}

SINT32 CADatabase::deleteDB(t_databaseInfo*& pDBInfo)
	{
		clearDB(pDBInfo);
		delete pDBInfo;
		pDBInfo=NULL;
		return E_SUCCESS;
	}
	
/** Inserts this key in the replay DB. The last two bytes are the timestamp*/
SINT32 CADatabase::insert(UINT8 key[16])
	{
		m_pMutex->lock();
		UINT16 timestamp=(key[14]<<8)|key[15];
		if(timestamp<m_currentClock-1||timestamp>m_currentClock+1)
			{
				m_pMutex->unlock();
				return E_UNKNOWN;
			}
		t_databaseInfo* aktDB=m_currDatabase;
		if(timestamp>m_currentClock)
			{
				aktDB=m_nextDatabase;
			}
		else if(timestamp<m_currentClock)
			{
				aktDB=m_prevDatabase;
			}
		UINT16 hashKey=(key[8]<<8)|key[9];
		LP_databaseEntry hashList=aktDB->m_pHashTable[hashKey];
		if(hashList==NULL)
			{
				LP_databaseEntry newEntry=getNewDBEntry(aktDB);
				newEntry->left=NULL;
				newEntry->right=NULL;
				newEntry->key=key[0]<<24|key[1]<<16|key[2]<<8|key[3];
				aktDB->m_pHashTable[hashKey]=newEntry;
				aktDB->m_u32Size++;
				m_pMutex->unlock();
				return E_SUCCESS;
			}
		else
			{
				UINT32 ret=key[0]<<24|key[1]<<16|key[2]<<8|key[3];;
				LP_databaseEntry before=NULL;
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
				LP_databaseEntry newEntry=getNewDBEntry(aktDB);
				newEntry->left=newEntry->right=NULL;
				//memcpy(newEntry->key,key,6);				
				newEntry->key=ret;
				if(before->key<ret)
					{
						before->right=newEntry;
					}
				else
						before->left=newEntry;

			}
		aktDB->m_u32Size++;	
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
		tReplayTimestamp rt;
		pDatabase->getCurrentReplayTimestamp(rt);
		pDatabase->m_currentClock=rt.interval;
		while(pDatabase->m_bRun)
			{
				sSleep(10);
				SINT32 secondsTilNextClock=((pDatabase->m_currentClock+1)*SECONDS_PER_INTERVALL)+pDatabase->m_refTime-time(NULL);
				if(secondsTilNextClock<=0&&pDatabase->m_bRun)
					pDatabase->nextClock();
			}
			
		FINISH_STACK("CADatabase::db_loopMaintenance");
			
		THREAD_RETURN_SUCCESS;
	}

SINT32 CADatabase::nextClock()
	{
		m_pMutex->lock();
		tReplayTimestamp rt;
		getCurrentReplayTimestamp(rt);
		m_currentClock=rt.interval;
		CAMsg::printMsg(LOG_DEBUG,"Replay DB Size was: %u\n",m_prevDatabase->m_u32Size);
		clearDB(m_prevDatabase);
		t_databaseInfo* tmpDB=m_prevDatabase;
		m_prevDatabase=m_currDatabase;
		m_currDatabase=m_nextDatabase;
		m_nextDatabase=tmpDB;
		m_pMutex->unlock();
		return E_SUCCESS;
	}

SINT32 CADatabase::test()
	{
		CADatabase oDatabase;
		oDatabase.start();
		UINT8 key[16];
		memset(key,0,16);
		UINT32 i;
		for(i=0;i<20;i++)
			{
				getRandom(key,1);
				oDatabase.insert(key);///TODO WRONG - fixme
			}
		for(i=0;i<200000;i++)
			{
				getRandom(key,16);
				oDatabase.insert(key);//TODO WRONG - Fixme
			}
		oDatabase.stop();
//check it
//TODO fixme!
/*		for(i=0;i<0x10000;i++)
			{
				LP_databaseEntry tmp=oDatabase.m_currDatabase[i];
				while(tmp!=NULL&&tmp->next!=NULL)
					{
						if(memcmp(tmp->key,tmp->next->key,14)>=0)
							return E_UNKNOWN;
						tmp=tmp->next;
					}
			}*/
		return E_SUCCESS;
	}

/** Returns the current replay timestamp for this database.
	* @param replayTimestamp stores the current replay timestamp
	*/
SINT32 CADatabase::getCurrentReplayTimestamp(tReplayTimestamp& replayTimestamp) const
	{
		return getReplayTimestampForTime(replayTimestamp,time(NULL),m_refTime);
	}


SINT32 CADatabase::getReplayTimestampForTime(tReplayTimestamp& replayTimestamp,UINT32 aktTime,UINT32 refTime)
	{
		UINT32 timeDiff=aktTime-refTime;
		replayTimestamp.interval=timeDiff/SECONDS_PER_INTERVALL;
		replayTimestamp.offset=timeDiff%SECONDS_PER_INTERVALL;
		return E_SUCCESS;
	}


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
						getcurrentTimeMicros(endTime);
						printf("delete takes %u microsecs\n",diff64(endTime,startTime));
					}
				aktNrOfEntries+=stepBy;
			}
		delete[] key;
		return E_SUCCESS;
	}

SINT32 CADatabase::fill(UINT32 nrOfEntries)
	{
		UINT32 i=0;
		UINT8 key[16];
		key[14]=0;
		key[15]=0;
		while(i<nrOfEntries)
			{
				getRandom(key,14);
				if(insert(key)==E_SUCCESS)
					i++;
			}
		return E_SUCCESS;
	}

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
		UINT16 hashKey=(key[8]<<8)|key[9];
		LP_databaseEntry hashList=aktDB->m_pHashTable[hashKey];
		if(hashList==NULL)
			{
				LP_databaseEntry newEntry=getNewDBEntry(aktDB);
				newEntry->left=NULL;
				newEntry->right=NULL;
				newEntry->key=key[0]<<24|key[1]<<16|key[2]<<8|key[3];
				//aktDB->m_pHashTable[hashKey]=newEntry;
				aktDB->m_u32Size++;
				m_pMutex->unlock();
				return E_SUCCESS;
			}
		else
			{
				UINT32 ret=key[0]<<24|key[1]<<16|key[2]<<8|key[3];
				LP_databaseEntry before=NULL;
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
				LP_databaseEntry newEntry=getNewDBEntry(aktDB);
				newEntry->left=newEntry->right=NULL;
				//memcpy(newEntry->key,key,6);				
				newEntry->key=ret;
				if(before->key<ret)
					{
						//before->right=newEntry;
					}
				else
					{
						//before->left=newEntry;
					}
			}
		aktDB->m_u32Size++;	
		m_pMutex->unlock();
		return E_SUCCESS;	
	}
#endif //Only_LOCAL_PROXY
