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

CAIPList::CAIPList()
	{
		m_HashTable=new PIPLIST[0xFFFF];
		memset(m_HashTable,0,0xFFFF*sizeof(PIPLIST));

	}

CAIPList::~CAIPList()
	{
	}

SINT32 CAIPList::insertIP(UINT8 ip[4])
	{
		UINT16 hashvalue=ip[2]<<8|ip[3];
		PIPLIST entry=m_HashTable[hashvalue];
		if(entry==NULL)
			{
				entry=new IPLISTENTRY;
				memcpy(entry->ip,ip,4);
				entry->count=1;
				entry->next=NULL;
				m_HashTable[hashvalue]=entry;
				return entry->count;
			}
		else
			{
				PIPLIST last;
				do
					{
						if(memcmp(entry->ip,ip,4)==0)
							{
								if(entry->count>=MAX_IP_CONNECTIONS)
									{
										CAMsg::printMsg(LOG_CRIT,"possible Flooding Attack from: %u.%u.%u.%u !\n",ip[0],ip[1],ip[2],ip[3]);
										return E_UNKNOWN;
									}
								entry->count++;
								return entry->count;
							}
						last=entry;
						entry=entry->next;
					} while(entry!=NULL);
				last->next=new IPLISTENTRY;
				entry=last->next;
				memcpy(entry->ip,ip,4);
				entry->count=1;
				entry->next=NULL;
				return entry->count;
			}	
	}

SINT32 CAIPList::removeIP(UINT8 ip[4])
	{
		UINT16 hashvalue=ip[2]<<8|ip[3];
		PIPLIST entry=m_HashTable[hashvalue];
		if(entry==NULL)
			{
				return 0;
			}
		else
			{
				PIPLIST before=NULL;
				while(entry!=NULL)
					{
						if(memcmp(entry->ip,ip,4)==0)
							{
								entry->count--;
								if(entry->count==0)
									{
										if(before==NULL)
											{
												m_HashTable[hashvalue]=NULL;
												delete entry;
												return 0;
											}
										else
											{
												before->next=entry->next;
												delete entry;
												return 0;
											}
									}
								return entry->count;
							}
						before=entry;
						entry=entry->next;
					}
				return 0;
			}	
	}