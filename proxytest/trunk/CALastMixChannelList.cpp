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
#include "CALastMixChannelList.hpp"

CALastMixChannelList::CALastMixChannelList()
	{
		m_HashTable=new LP_lmChannelListEntry[0x10000];
		m_listSockets=NULL;
		m_listSocketsNext=NULL;
	}

CALastMixChannelList::~CALastMixChannelList()
	{
		for(UINT32 i=0;i<0x10000;i++)
			{
				lmChannelListEntry* akt=m_HashTable[i];
				lmChannelListEntry* tmp;
				while(akt!=NULL)
					{
						tmp=akt;
						delete akt;
						akt=tmp->list_Channels.next;
					}
			}
		delete[] m_HashTable;
	}

SINT32 CALastMixChannelList::add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher,CAQueue* pQueue)
	{
		UINT32 hash=id&0x0000FFFF;
		lmChannelListEntry* pEntry=m_HashTable[hash];
		lmChannelListEntry* pNewEntry=new lmChannelListEntry;
		pNewEntry->channelIn=id;
		pNewEntry->pCipher=pCipher;
		pNewEntry->pSocket=pSocket;
		pNewEntry->pQueueSend=pQueue;
		pNewEntry->trafficIn=0;
		pNewEntry->trafficOut=0;
		if(pEntry==NULL) //First Entry for Hash in HashTable
			{
				m_HashTable[hash]=pNewEntry;
				pNewEntry->list_Channels.next=NULL;
				pNewEntry->list_Channels.prev=NULL;
			}
		else //insert in Hashlist for Hashtableentry
			{
				pNewEntry->list_Channels.prev=NULL;
				pNewEntry->list_Channels.next=pEntry;
				pEntry->list_Channels.prev=pNewEntry;
			}
		//Insert in SocketList
		if(m_listSockets==NULL)
			{
				m_listSockets=pNewEntry;
				pNewEntry->list_Sockets.next=NULL;
				pNewEntry->list_Sockets.prev=NULL;
			}
		else
			{
				pNewEntry->list_Sockets.prev=NULL;
				pNewEntry->list_Sockets.next=m_listSockets;
				m_listSockets->list_Sockets.prev=pNewEntry;
				m_listSockets=pNewEntry;				
			}
		if(m_listSocketsNext==NULL)
			m_listSocketsNext=m_listSockets;
		return E_SUCCESS;
	}

SINT32 CALastMixChannelList::removeChannel(HCHANNEL channel)
	{
		UINT32 hash=channel&0x0000FFFF;
		lmChannelListEntry* pEntry=m_HashTable[hash];
		while(pEntry!=NULL)
			{
				if(pEntry->channelIn==channel)
					{//remove from HashTable
						if(pEntry->list_Channels.prev==NULL)
							m_HashTable[hash]=pEntry->list_Channels.next;
						else
							{
								pEntry->list_Channels.prev->list_Channels.next=pEntry->list_Channels.next;
							}
						if(pEntry->list_Channels.next!=NULL)
							{
								pEntry->list_Channels.next->list_Channels.prev=pEntry->list_Channels.prev;
							}
						//remove from SocketList
						if(pEntry->list_Sockets.prev==NULL)
							m_listSockets=pEntry->list_Sockets.next;
						else
							pEntry->list_Sockets.prev->list_Sockets.next=pEntry->list_Sockets.next;
						if(pEntry->list_Sockets.next!=NULL)
							pEntry->list_Sockets.next->list_Sockets.prev=pEntry->list_Sockets.prev;
						//TODO: Adjust m_nextListsocket
						return E_SUCCESS;
					}
			}
		return E_SUCCESS;
	}