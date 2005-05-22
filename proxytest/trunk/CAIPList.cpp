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
#include "CAIPList.hpp"
#include "CAMsg.hpp"
#include "CAUtil.hpp"
#include "CACmdLnOptions.hpp"

extern CACmdLnOptions options;

/** Constructs an empty CAIPList. 
	* The default number #MAX_IP_CONNECTIONS of allowed insertions is used*/ 
CAIPList::CAIPList()
	{	
		m_Random=new UINT8[56];
		m_HashTable=new PIPLIST[0x10000];
		memset((void*)m_HashTable,0,0x10000*sizeof(PIPLIST));
		m_allowedConnections=MAX_IP_CONNECTIONS;
		getRandom(m_Random,56);
#ifdef COUNTRY_STATS
		initCountryStats();
#endif
	}

/**Constructs a empty CAIPList, there allowedConnections insertions 
are allowed, until an error is returned.
@param allowedConnections number of insertions of the same IP-Address, until an error is returned
*/
CAIPList::CAIPList(UINT32 allowedConnections)
	{
		m_Random=new UINT8[56];
		m_HashTable=new PIPLIST[0x10000];
		memset((void*)m_HashTable,0,0x10000*sizeof(PIPLIST));
		m_allowedConnections=allowedConnections;
		getRandom(m_Random,56);
#ifdef COUNTRY_STATS
		initCountryStats();
#endif
	}

/** Deletes the IPList and frees all used resources*/
CAIPList::~CAIPList()
	{
		for(UINT32 i=0;i<=0xFFFF;i++)
			{
				VOLATILE_PIPLIST entry=m_HashTable[i];
				PIPLIST tmpEntry;
				while(entry!=NULL)
					{	
						tmpEntry=entry;
						entry=entry->next;
						delete tmpEntry;
					}
			}
		delete [] m_Random;
		delete [] m_HashTable;
#ifdef COUNTRY_STATS		
		deleteCountryStats();
#endif		
	}

/** Inserts the IP-Address into the list. 
  * If the IP-Address is already in the list then the number of insert()
	* called for this IP-Adress is returned. If this number is larger than m_allowedConnections
	* an error is returned.
	* Intern handelt es sich um eine Hashtabelle mit Ueberlaufliste. Die letzten 16 Bit der IP-Adresse bilden dabei den
	* Hashkey. Die Hashtabelle hat 16^2 Eintraege. In den Ueberlauflisten der einzelnen Hasheintraege sind die ersten 16 Bit der 
	* IP-Adresse gespeichert.
	* @param ip the IP-Address to insert
	* @return number of inserts for this IP-Address
  * @retval E_UNKNOWN if an error occured or an IP is inserted more than m_allowedConnections times
	*/
SINT32 CAIPList::insertIP(const UINT8 ip[4])
	{
		UINT16 hashvalue=(ip[2]<<8)|ip[3];
		m_Mutex.lock();
		PIPLIST entry=m_HashTable[hashvalue];
		if(entry==NULL)
			{//Hashkey nicht in der Hashtabelle gefunden --> neuer Eintrag in Hashtabelle
#ifndef PSEUDO_LOG
				UINT8 hash[16];
				memcpy(m_Random,ip,4);
				MD5(m_Random,56,hash);
				CAMsg::printMsg(LOG_DEBUG,"Inserting IP-Address: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X !\n",hash[0],hash[1],hash[2],hash[3],hash[4],hash[5],hash[6],hash[7],hash[8],hash[9],hash[10],hash[11],hash[12],hash[13],hash[14],hash[15]);
#else
				CAMsg::printMsg(LOG_DEBUG,"Inserting IP-Address: {%u.%u.%u.%u} !\n",ip[0],ip[1],ip[2],ip[3]);
#endif
				entry=new IPLISTENTRY;
				memcpy(entry->ip,ip,2);
				entry->count=1;
				entry->next=NULL;
#ifdef COUNTRY_STATS
				entry->countryID=updateCountryStats(ip,0,false);
#endif				
				m_HashTable[hashvalue]=entry;
				m_Mutex.unlock();
				return entry->count;
			}
		else
			{//Hashkey in Hashtabelle gefunden --> suche in Ueberlaufliste nach Eintrag bzw. lege neuen Eitnrag an
				PIPLIST last;
				do
					{
						if(memcmp(entry->ip,ip,2)==0) //we have found the entry
							{
								#ifdef PSEUDO_LOG
									CAMsg::printMsg(LOG_DEBUG,"Inserting IP-Address: {%u.%u.%u.%u} !\n",ip[0],ip[1],ip[2],ip[3]);
								#endif
								if(entry->count>=m_allowedConnections) //an Attack...
									{
										#if !defined(PSEUDO_LOG)&&defined(FIREWALL_SUPPORT)
											CAMsg::printMsg(LOG_CRIT,"possible Flooding Attack from: %u.%u.%u.%u !\n",ip[0],ip[1],ip[2],ip[3]);
										#endif
										m_Mutex.unlock();
										return E_UNKNOWN;
									}
								entry->count++;
#ifdef COUNTRY_STATS
								updateCountryStats(NULL,entry->countryID,false);
#endif
								m_Mutex.unlock();
								return entry->count;
							}
						last=entry;
						entry=entry->next;
					} while(entry!=NULL);
//Nicht in der Ueberlaufliste gefunden
				last->next=new IPLISTENTRY;
				entry=last->next;
				memcpy(entry->ip,ip,2);
				entry->count=1;
				entry->next=NULL;
#ifdef COUNTRY_STATS
				entry->countryID=updateCountryStats(ip,0,false);
#endif				
				m_Mutex.unlock();
				return entry->count;
			}	
	}

/** Removes the IP-Address from the list.
	* @param ip IP-Address to remove
	* @return the remaining count of inserts for this IP-Address. 
	* @retval 0 if IP-Address is delete form the list
	*/
#ifndef LOG_CHANNEL
	SINT32 CAIPList::removeIP(const UINT8 ip[4])
#else
	SINT32 CAIPList::removeIP(const UINT8 ip[4],UINT32 time,UINT32 trafficIn,UINT32 trafficOut)
#endif
	{
		UINT16 hashvalue=(ip[2]<<8)|ip[3];
		m_Mutex.lock();
		PIPLIST entry=m_HashTable[hashvalue];
		if(entry==NULL)
			{
				m_Mutex.unlock();
				CAMsg::printMsg(LOG_INFO,"Try to remove Ip which is not in the hashtable of the IP-list - possible inconsistences in IPList!\n");
				return 0;
			}
		else
			{
				PIPLIST before=NULL;
				while(entry!=NULL)
					{
						if(memcmp(entry->ip,ip,2)==0)
							{
								entry->count--;
#ifdef COUNTRY_STATS
								updateCountryStats(NULL,entry->countryID,true);
#endif								
								if(entry->count==0)
									{
										#ifndef PSEUDO_LOG
											UINT8 hash[16];
											memcpy(m_Random,ip,4);
											MD5(m_Random,56,hash);
											#ifdef LOG_CHANNEL
												CAMsg::printMsg(LOG_DEBUG,"Removing IP-Address: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X -- Time [ms]: %u  Traffic was: IN: %u  --  OUT: %u\n",hash[0],hash[1],hash[2],hash[3],hash[4],hash[5],hash[6],hash[7],hash[8],hash[9],hash[10],hash[11],hash[12],hash[13],hash[14],hash[15],time,trafficIn,trafficOut);
											#else
												CAMsg::printMsg(LOG_DEBUG,"Removing IP-Address: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X !\n",hash[0],hash[1],hash[2],hash[3],hash[4],hash[5],hash[6],hash[7],hash[8],hash[9],hash[10],hash[11],hash[12],hash[13],hash[14],hash[15]);
											#endif
										#else
											CAMsg::printMsg(LOG_DEBUG,"Removing IP-Address: {%u.%u.%u.%u} !\n",ip[0],ip[1],ip[2],ip[3]);
										#endif
										if(before==NULL)
											m_HashTable[hashvalue]=entry->next;
										else
											before->next=entry->next;
										delete entry;
										m_Mutex.unlock();
										return 0;
									}
								m_Mutex.unlock();
								return entry->count;
							}
						before=entry;
						entry=entry->next;
					}
				m_Mutex.unlock();
				CAMsg::printMsg(LOG_INFO,"Try to remove Ip which is not in list - possible inconsistences in IPList!\n");
				return 0;
			}	
	}

#ifdef COUNTRY_STATS
#define COUNTRY_STATS_DB "CountryStats"
#define NR_OF_COUNTRIES 250

SINT32 CAIPList::initCountryStats()
	{
		m_CountryStats=NULL;
		m_mysqlCon=new MYSQL;
		mysql_init(m_mysqlCon);
		MYSQL* tmp=NULL;
		tmp=mysql_real_connect(m_mysqlCon,NULL,"root",NULL,COUNTRY_STATS_DB,0,NULL,0);
		if(tmp==NULL)
			{
				CAMsg::printMsg(LOG_DEBUG,"Could not connet to CountryStats DB!\n");
				my_thread_end();
				mysql_close(m_mysqlCon);
				delete m_mysqlCon;
				m_mysqlCon=NULL;
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_DEBUG,"Connected to CountryStats DB!\n");
		char query[1024];
		UINT8 buff[255];
		options.getCascadeName(buff,255);
		sprintf(query,"CREATE TABLE IF NOT EXISTS `stats_%s` (date timestamp,id int,count int)",buff);
		mysql_query(m_mysqlCon,query);
		m_CountryStats=new UINT32[NR_OF_COUNTRIES+1];
		memset((void*)m_CountryStats,0,sizeof(UINT32)*(NR_OF_COUNTRIES+1));
		m_threadLogLoop=new CAThread();
		m_threadLogLoop->setMainLoop(iplist_loopDoLogCountries);
		m_bRunLogCountries=true;
		m_threadLogLoop->start(this,true);
		return E_SUCCESS;
	}

SINT32 CAIPList::deleteCountryStats()
	{
		m_bRunLogCountries=false;
		m_threadLogLoop->join();
		delete m_threadLogLoop;
		if(m_mysqlCon!=NULL)
			{
				my_thread_end();
				mysql_close(m_mysqlCon);
				delete m_mysqlCon;
				m_mysqlCon=NULL;
			}
		if(m_CountryStats!=NULL)
			delete[] m_CountryStats;
		m_CountryStats=NULL;
		return E_SUCCESS;
	}

/** Update the statisitics of the countries users come from. The dependency between the argumenst is as follow:
	* @param bRemove if true the number of users of a given country is decreased, if false it is increased
	* @param a_countryID the country the user comes from. Must be set if bRemove==true. If bRemove==false and ip==NULL, than
	*        if also must be set to the country the user comes from. In case ip!=NULL if holdes the default country id, if no country for the ip could be found
	* @param ip the ip the user comes from. this ip is looked up in the databse to find the corresponding country. it is only used if bRemove==false. If no country for
	*         that ip could be found a_countryID is used as default value 
  * @return the countryID which was asigned  to the user. This may be the default value a_countryID, if no country could be found.
**/  
SINT32 CAIPList::updateCountryStats(const UINT8 ip[4],UINT32 a_countryID,bool bRemove)
	{
		if(!bRemove)
			{
				UINT32 countryID=a_countryID;
				if(ip!=NULL)
					{
						UINT32 u32ip=ip[0]<<24|ip[1]<<16|ip[2]<<8|ip[3];
						char query[1024];
						sprintf(query,"SELECT id FROM ip2c WHERE ip_lo<=\"%u\" and ip_hi>=\"%u\" LIMIT 1",u32ip,u32ip);
						int ret=mysql_query(m_mysqlCon,query);
						if(ret!=0)
							goto RET;
						MYSQL_RES* result=mysql_store_result(m_mysqlCon);
						if(result==NULL)
							goto RET;
						MYSQL_ROW row=mysql_fetch_row(result);
						if(row!=NULL)
							{
								countryID=atoi(row[0]);
							}
						else
							{
								CAMsg::printMsg(LOG_DEBUG,"DO country stats query result no result for ip %u)\n",u32ip);														
							}	
						mysql_free_result(result);
					}
RET:
				m_CountryStats[countryID]++;
				return countryID;
			}
		else//bRemove
			{
				m_CountryStats[a_countryID]--;
			}
		return a_countryID;
	}

THREAD_RETURN iplist_loopDoLogCountries(void* param)
	{
		CAIPList* pIPList=(CAIPList*)param;
		UINT32 s=0;
		UINT8 buff[255];
		options.getCascadeName(buff,255);

		while(pIPList->m_bRunLogCountries)
			{
				if(s==LOG_COUNTRIES_INTERVALL)
					{
						UINT8 aktDate[255];
						time_t aktTime=time(NULL);
						strftime((char*)aktDate,255,"%Y%m%d%H%M%S",gmtime(&aktTime));
						char query[1024];
						sprintf(query,"INSERT into `stats_%s` (date,id,count) VALUES (\"%s\",\"%%u\",\"%%u\")",buff,aktDate);
						pIPList->m_Mutex.lock();
						for(UINT32 i=0;i<NR_OF_COUNTRIES+1;i++)
							{
								if(pIPList->m_CountryStats[i]>0)
									{
										char aktQuery[1024];
										sprintf(aktQuery,query,i,pIPList->m_CountryStats[i]);
										int ret=mysql_query(pIPList->m_mysqlCon,aktQuery);
									}
							}
						pIPList->m_Mutex.unlock();
						s=0;
					}
				sSleep(10);
				s++;
			}
		THREAD_RETURN_SUCCESS;	
	}
#endif
