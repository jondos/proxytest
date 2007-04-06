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
#include "CAIPList.hpp"
#include "CAMsg.hpp"
#include "CAUtil.hpp"
#include "CACmdLnOptions.hpp"

extern CACmdLnOptions options;

/** Constructs an empty CAIPList. 
	* The default number #MAX_IP_CONNECTIONS of allowed insertions is used*/ 
CAIPList::CAIPList()
	{	
		m_pMutex=new CAMutex();
		m_Random=new UINT8[56];
		m_HashTable=new PIPLIST[0x10000];
		memset((void*)m_HashTable,0,0x10000*sizeof(PIPLIST));
		m_allowedConnections=MAX_IP_CONNECTIONS;
		getRandom(m_Random,56);
	}

/**Constructs a empty CAIPList, there allowedConnections insertions 
are allowed, until an error is returned.
@param allowedConnections number of insertions of the same IP-Address, until an error is returned
*/
CAIPList::CAIPList(UINT32 allowedConnections)
	{
		m_pMutex=new CAMutex();
		m_Random=new UINT8[56];
		m_HashTable=new PIPLIST[0x10000];
		memset((void*)m_HashTable,0,0x10000*sizeof(PIPLIST));
		m_allowedConnections=allowedConnections;
		getRandom(m_Random,56);
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
		delete m_pMutex;
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
		SINT32 ret;
		m_pMutex->lock();
		PIPLIST entry=m_HashTable[hashvalue];
		if(entry==NULL)
			{//Hashkey nicht in der Hashtabelle gefunden --> neuer Eintrag in Hashtabelle
#ifndef PSEUDO_LOG
				UINT8 hash[16];
				memcpy(m_Random,ip,4);
				MD5(m_Random,56,hash);
				CAMsg::printMsg(LOG_DEBUG,"Inserting new IP-Address: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X !\n",hash[0],hash[1],hash[2],hash[3],hash[4],hash[5],hash[6],hash[7],hash[8],hash[9],hash[10],hash[11],hash[12],hash[13],hash[14],hash[15]);
#else
				CAMsg::printMsg(LOG_DEBUG,"Inserting new IP-Address: {%u.%u.%u.%u} !\n",ip[0],ip[1],ip[2],ip[3]);
#endif
				entry=new IPLISTENTRY;
				memcpy(entry->ip,ip,2);
				entry->count=1;
				entry->next=NULL;
				m_HashTable[hashvalue]=entry;
				ret = entry->count;
#ifndef PSEUDO_LOG
				CAMsg::printMsg(LOG_DEBUG,"New IP-Address inserted: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X !\n",hash[0],hash[1],hash[2],hash[3],hash[4],hash[5],hash[6],hash[7],hash[8],hash[9],hash[10],hash[11],hash[12],hash[13],hash[14],hash[15]);
#else
				CAMsg::printMsg(LOG_DEBUG,"New IP-Address inserted: {%u.%u.%u.%u} !\n",ip[0],ip[1],ip[2],ip[3]);
#endif
				m_pMutex->unlock();
				return ret;
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
										//#if !defined(PSEUDO_LOG)&&defined(FIREWALL_SUPPORT)
											CAMsg::printMsg(LOG_CRIT,"Possible Flooding Attack from: %u.%u.%u.%u !\n",ip[0],ip[1],ip[2],ip[3]);
										//#endif
										m_pMutex->unlock();
										return E_UNKNOWN;
									}
								entry->count++;
								ret = entry->count;
								#ifdef PSEUDO_LOG
									CAMsg::printMsg(LOG_DEBUG,"IP-Address inserted: {%u.%u.%u.%u} !\n",ip[0],ip[1],ip[2],ip[3]);
								#endif
								m_pMutex->unlock();
								return ret;
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
				ret = entry->count;
				m_pMutex->unlock();
				return ret;
			}	
	}

/** Removes the IP-Address from the list.
	* @param ip IP-Address to remove
	* @return the remaining count of inserts for this IP-Address. 
	* @retval 0 if IP-Address is delete form the list
	*/
#ifndef LOG_TRAFFIC_PER_USER
	SINT32 CAIPList::removeIP(const UINT8 ip[4])
#else
	SINT32 CAIPList::removeIP(const UINT8 ip[4],UINT32 time,UINT32 trafficIn,UINT32 trafficOut)
#endif
	{
		UINT16 hashvalue=(ip[2]<<8)|ip[3];
		SINT32 ret;
		m_pMutex->lock();
		PIPLIST entry=m_HashTable[hashvalue];
		if(entry==NULL)
			{
				m_pMutex->unlock();
				CAMsg::printMsg(LOG_INFO,"Try to remove IP which is not in the hashtable of the IP-list - possible inconsistences in IPList!\n");
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
						if(entry->count<=0)
						{
							if (entry->count < 0)
							{
								CAMsg::printMsg(LOG_CRIT,"Negative count for IP address!");
								entry->count = 0;
							}
							
							#ifndef PSEUDO_LOG
								UINT8 hash[16];
								memcpy(m_Random,ip,4);
								MD5(m_Random,56,hash);
								#ifdef LOG_TRAFFIC_PER_USER
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
							m_pMutex->unlock();
							return 0;
						}
						ret = entry->count;
						m_pMutex->unlock();
						return ret;
					}
					before=entry;
					entry=entry->next;
				}
				m_pMutex->unlock();
				CAMsg::printMsg(LOG_INFO,"Try to remove IP which is not in list - possible inconsistences in IPList!\n");
				return 0;
			}	
	}
#endif //ONLY_LOCAL_PROXY
