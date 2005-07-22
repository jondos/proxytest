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

#define SECONDS_PER_INTERVALL 600

CADatabase::CADatabase()
	{
		m_currDatabase=new LP_databaseEntry[0x10000];
		memset(m_currDatabase,0,sizeof(LP_databaseEntry)*0x10000);
		m_nextDatabase=new LP_databaseEntry[0x10000];
		memset(m_nextDatabase,0,sizeof(LP_databaseEntry)*0x10000);
		m_prevDatabase=new LP_databaseEntry[0x10000];
		memset(m_prevDatabase,0,sizeof(LP_databaseEntry)*0x10000);
		m_refTime=time(NULL);
		m_pThread=NULL;
	}

CADatabase::~CADatabase()
	{
		m_oMutex.lock();
		stop();
		for(UINT32 i=0;i<0x10000;i++)
			{
				LP_databaseEntry tmp,tmp1;
				tmp=m_currDatabase[i];
				while(tmp!=NULL)
					{
						tmp1=tmp;
						tmp=tmp->next;
						delete tmp1;
					}
			}
		delete [] m_currDatabase;
		for(UINT32 i=0;i<0x10000;i++)
			{
				LP_databaseEntry tmp,tmp1;
				tmp=m_nextDatabase[i];
				while(tmp!=NULL)
					{
						tmp1=tmp;
						tmp=tmp->next;
						delete tmp1;
					}
			}
		delete [] m_nextDatabase;
		for(UINT32 i=0;i<0x10000;i++)
			{
				LP_databaseEntry tmp,tmp1;
				tmp=m_prevDatabase[i];
				while(tmp!=NULL)
					{
						tmp1=tmp;
						tmp=tmp->next;
						delete tmp1;
					}
			}
		delete [] m_prevDatabase;
		m_oMutex.unlock();
	}

/** Inserts this key in the replay DB. The last two bytes are the timestamp*/
SINT32 CADatabase::insert(UINT8 key[16])
	{
		m_oMutex.lock();
		UINT16 timestamp=(key[14]<<8)|key[15];
		if(timestamp<m_currentClock-1||timestamp>m_currentClock+1)
			return E_UNKNOWN;
		LP_databaseEntry* aktDB=m_currDatabase;
		if(timestamp>m_currentClock)
			aktDB=m_nextDatabase;
		else if(timestamp>m_currentClock)
			aktDB=m_prevDatabase;
		UINT16 hashKey=(key[8]<<8)|key[9];
		LP_databaseEntry hashList=aktDB[hashKey];
		if(hashList==NULL)
			{
				LP_databaseEntry newEntry=new t_databaseEntry;
				newEntry->next=NULL;
				memcpy(newEntry->key,key,6);
				aktDB[hashKey]=newEntry;
				m_oMutex.unlock();
				return E_SUCCESS;
			}
		else
			{
				LP_databaseEntry before=NULL;
				do
					{
						int ret=memcmp(key,hashList->key,6);
						if(ret==0)
							{
								m_oMutex.unlock();
								return E_UNKNOWN;
							}
						if(ret<0)
							break;
						before=hashList;
						hashList=hashList->next;
					} while(hashList!=NULL);
				LP_databaseEntry newEntry=new t_databaseEntry;
				newEntry->next=hashList;
				memcpy(newEntry->key,key,6);				
				if(before==NULL)
					{
						aktDB[hashKey]=newEntry;
					}
				else
					{
						before->next=newEntry;
					}

			}
		m_oMutex.unlock();
		return E_SUCCESS;
	}

SINT32 CADatabase::start()
	{
		m_pThread=new CAThread();
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
		pDatabase->m_bRun=true;
		pDatabase->m_currentClock=pDatabase->getClockForTime(time(NULL));
		while(pDatabase->m_bRun)
			{
				sSleep(10);
				UINT32 secondsTilNextClock=((pDatabase->m_currentClock+1)*SECONDS_PER_INTERVALL)+pDatabase->m_refTime-time(NULL);
				if(secondsTilNextClock<=0&&pDatabase->m_bRun)
					pDatabase->nextClock();
			}
		THREAD_RETURN_SUCCESS;
	}

SINT32 CADatabase::nextClock()
	{
		m_oMutex.lock();
		m_currentClock=getClockForTime(time(NULL));
		for(UINT32 i=0;i<0x10000;i++)
			{
				LP_databaseEntry tmp,tmp1;
				tmp=m_prevDatabase[i];
				while(tmp!=NULL)
					{
						tmp1=tmp;
						tmp=tmp->next;
						delete tmp1;
					}
			}
		memset(m_prevDatabase,0,0x10000*sizeof(LP_databaseEntry));
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
		for(i=0;i<0x10000;i++)
			{
				LP_databaseEntry tmp=oDatabase.m_currDatabase[i];
				while(tmp!=NULL&&tmp->next!=NULL)
					{
						if(memcmp(tmp->key,tmp->next->key,14)>=0)
							return E_UNKNOWN;
						tmp=tmp->next;
					}
			}
		return E_SUCCESS;
	}

SINT32 CADatabase::getCurrentReplayTimestamp(tReplayTimestamp& replayTimestamp)
	{
		time_t aktTime=time(NULL);
		replayTimestamp.interval=getClockForTime(aktTime);
		replayTimestamp.offset=(aktTime-m_refTime)%SECONDS_PER_INTERVALL;
		return E_SUCCESS;
	}


SINT32 CADatabase::getClockForTime(UINT32 time)
	{
		return (time-m_refTime)/SECONDS_PER_INTERVALL;
	}
