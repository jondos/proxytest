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

#define SECONDS_PER_CLOCK 10
CADatabase::CADatabase(UINT32 refTime)
	{
		m_currDatabase=new LP_databaseEntry[0x10000];
		memset(m_currDatabase,0,sizeof(LP_databaseEntry)*0x10000);
		m_prevDatabase=new LP_databaseEntry[0x10000];
		memset(m_prevDatabase,0,sizeof(LP_databaseEntry)*0x10000);
		m_nextDatabase=new LP_databaseEntry[0x10000];
		memset(m_nextDatabase,0,sizeof(LP_databaseEntry)*0x10000);
		m_refTime=refTime;
	}

CADatabase::~CADatabase()
	{
		m_oMutex.lock();
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
		m_oMutex.unlock();
	}

SINT32 CADatabase::insert(UINT8 timestamp[2],UINT8 key[16])
	{
		m_oMutex.lock();
		UINT16 hashKey=(key[14]<<8)|key[15];
		LP_databaseEntry hashList=m_currDatabase[hashKey];
		if(hashList==NULL)
			{
				LP_databaseEntry newEntry=new t_databaseEntry;
				newEntry->next=NULL;
				memcpy(newEntry->key,key,6);
				m_currDatabase[hashKey]=newEntry;
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
						m_currDatabase[hashKey]=newEntry;
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
		m_oThread.setMainLoop(db_loopMaintenance);
		m_Clock=(time(NULL)-m_refTime)/SECONDS_PER_CLOCK;
		return m_oThread.start(this);
	}

SINT32 CADatabase::stop()
	{
		m_bRun=false;
		return m_oThread.join();
	}

THREAD_RETURN db_loopMaintenance(void *param)
	{
		CADatabase* pDatabase=(CADatabase*)param;
		pDatabase->m_bRun=true;
		while(pDatabase->m_bRun)
			{
				UINT32 secondsTilNextClock=((pDatabase->m_Clock+1)*SECONDS_PER_CLOCK)+pDatabase->m_refTime-time(NULL);
				sSleep(secondsTilNextClock);
				if(pDatabase->m_bRun)
					pDatabase->nextClock();
			}
		THREAD_RETURN_SUCCESS;
	}

SINT32 CADatabase::nextClock()
	{
		m_oMutex.lock();
		m_Clock++;
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
		CADatabase oDatabase(time(NULL));
		oDatabase.start();
		UINT8 key[16];
		memset(key,0,16);
		UINT32 i;
		for(i=0;i<20;i++)
			{
				getRandom(key,1);
				oDatabase.insert(key,key);///TODO WRONG - fixme
			}
		for(i=0;i<200000;i++)
			{
				getRandom(key,16);
				oDatabase.insert(key,key);//TODO WRONG - Fixme
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
