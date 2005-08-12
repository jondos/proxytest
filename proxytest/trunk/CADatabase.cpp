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
#include "CADatabase.hpp"
#include "CAUtil.hpp"

CADatabase::CADatabase()
	{
		m_currDatabase=new LP_databaseEntry[0x10000];
		memset(m_currDatabase,0,sizeof(LP_databaseEntry)*0x10000);
		m_nextDatabase=new LP_databaseEntry[0x10000];
		memset(m_nextDatabase,0,sizeof(LP_databaseEntry)*0x10000);
		m_prevDatabase=new LP_databaseEntry[0x10000];
		memset(m_prevDatabase,0,sizeof(LP_databaseEntry)*0x10000);
		m_refTime=time(NULL);
		m_currentClock=0;
		m_pThread=NULL;
	}

CADatabase::~CADatabase()
	{
		m_oMutex.lock();
		stop();
		deleteDB(m_currDatabase);
		deleteDB(m_nextDatabase);
		deleteDB(m_prevDatabase);
		m_oMutex.unlock();
	}

SINT32 CADatabase::clearDB(LP_databaseEntry*& pHashTable)
	{
		UINT32 deleteCount=0;
		for(UINT32 i=0;i<0x10000;i++)
			{
				LP_databaseEntry tmp,tmp1;
				tmp=pHashTable[i];
				LP_databaseEntry stack[10000];
				SINT32 stackIndex=-1;
				while ( tmp != NULL || stackIndex>=0 )
					{
						for (; tmp != NULL; tmp = tmp->left )
							{
								stack[++stackIndex]=tmp;//add to stack
								if(stackIndex>9998)
									{
										CAMsg::printMsg(LOG_CRIT,"Could not delete the replay database - stack full!\n");
										return E_SPACE;
									}
							}
						if ( stack[stackIndex] != NULL )
							{
								tmp = stack[stackIndex]->right;
								stack[++stackIndex]=NULL;
							}
						else
							{
								stackIndex--;
								#ifdef DEBUG
									memset(stack[stackIndex],0,sizeof(t_databaseEntry));
								#endif
								delete stack[stackIndex];
								deleteCount++;
								stackIndex--;
								tmp = NULL;
							}
					}
			}
			CAMsg::printMsg(LOG_DEBUG,"DeleteCount %u\n",deleteCount);
		memset(pHashTable,0,sizeof(LP_databaseEntry)*0x10000);
		return E_SUCCESS;
	}

SINT32 CADatabase::deleteDB(LP_databaseEntry*& pHashTable)
	{
		clearDB(pHashTable);
		delete [] pHashTable;
		pHashTable=NULL;
		return E_SUCCESS;
	}
	
/** Inserts this key in the replay DB. The last two bytes are the timestamp*/
SINT32 CADatabase::insert(UINT8 key[16])
	{
		m_oMutex.lock();
		UINT16 timestamp=(key[14]<<8)|key[15];
		if(timestamp<m_currentClock-1||timestamp>m_currentClock+1)
			{
				m_oMutex.unlock();
				return E_UNKNOWN;
			}
		LP_databaseEntry* aktDB=m_currDatabase;
		if(timestamp>m_currentClock)
			aktDB=m_nextDatabase;
		else if(timestamp<m_currentClock)
			aktDB=m_prevDatabase;
		UINT16 hashKey=(key[8]<<8)|key[9];
		LP_databaseEntry hashList=aktDB[hashKey];
		if(hashList==NULL)
			{
				LP_databaseEntry newEntry=new t_databaseEntry;
				newEntry->left=NULL;
				newEntry->right=NULL;
				memcpy(newEntry->key,key,6);
				aktDB[hashKey]=newEntry;
				m_oMutex.unlock();
				return E_SUCCESS;
			}
		else
			{
				SINT32 ret;
				LP_databaseEntry before=NULL;
				do
					{
						ret=memcmp(key,hashList->key,6);
						if(ret==0)
							{
								m_oMutex.unlock();
								return E_UNKNOWN;
							}
						before=hashList;	
						if(ret<0)
							{
								hashList=hashList->right;
							}
						else
							{
								hashList=hashList->left;
							}
					} while(hashList!=NULL);
				LP_databaseEntry newEntry=new t_databaseEntry;
				newEntry->left=newEntry->right=NULL;
				memcpy(newEntry->key,key,6);				
				if(ret<0)
					{
						before->right=newEntry;
					}
				else
						before->left=newEntry;

			}
		m_oMutex.unlock();
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
		THREAD_RETURN_SUCCESS;
	}

SINT32 CADatabase::nextClock()
	{
		m_oMutex.lock();
		tReplayTimestamp rt;
		getCurrentReplayTimestamp(rt);
		m_currentClock=rt.interval;
		clearDB(m_prevDatabase);
		LP_databaseEntry* tmpDB=m_prevDatabase;
		m_prevDatabase=m_currDatabase;
		m_currDatabase=m_nextDatabase;
		m_nextDatabase=tmpDB;		
		m_oMutex.unlock();
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
						getcurrentTimeMicros(startTime);
						aktKey=key;
						getRandom(key,insertsPerMeasure*16);
						for(UINT32 j=0;j<insertsPerMeasure;j++)
							{
								pDatabase->simulateInsert(aktKey);
								aktKey+=16;
							}
						getcurrentTimeMicros(endTime);
						sprintf(buff,atemplate,aktNrOfEntries,insertsPerMeasure,diff64(endTime,startTime));
						write(file,buff,strlen(buff));
						delete pDatabase;
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
		m_oMutex.lock();
		UINT16 timestamp=(key[14]<<8)|key[15];
		if(timestamp<m_currentClock-1||timestamp>m_currentClock+1)
			{
			//do nothing
			}
		LP_databaseEntry* aktDB=m_currDatabase;
		LP_databaseEntry* aktDB1=m_currDatabase;
		if(timestamp>m_currentClock)
			aktDB1=m_nextDatabase;
		else if(timestamp<m_currentClock)
			aktDB1=m_prevDatabase;
		UINT16 hashKey=(key[8]<<8)|key[9];
		LP_databaseEntry hashList=aktDB[hashKey];
		t_databaseEntry si;
		if(hashList==NULL)
			{
				LP_databaseEntry newEntry=&si;
				newEntry->left=NULL;
				newEntry->right=NULL;
				memcpy(newEntry->key,key,6);
				//aktDB[hashKey]=newEntry;
				m_oMutex.unlock();
				return E_SUCCESS;
			}
		else
			{
				SINT32 ret;
				LP_databaseEntry before=NULL;
				do
					{
						ret=memcmp(key,hashList->key,6);
						if(ret==0)
							{
								m_oMutex.unlock();
								return E_UNKNOWN;
							}
						before=hashList;	
						if(ret<0)
							{
								hashList=hashList->right;
							}
						else
							{
								hashList=hashList->left;
							}
					} while(hashList!=NULL);
				LP_databaseEntry newEntry=&si;//new t_databaseEntry;
				newEntry->left=newEntry->right=NULL;
				memcpy(newEntry->key,key,6);				
				if(ret<0)
					{
						before->right=before->right;
					}
				else
						before->left=before->left;

			}
		m_oMutex.unlock();
		return E_SUCCESS;
	}
