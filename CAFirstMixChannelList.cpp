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
#include "CAFirstMixChannelList.hpp"


#define MAX_HASH_KEY 1025

CAFirstMixChannelList::CAFirstMixChannelList()
	{
		m_HashTable=new LP_fmHashTableEntry[MAX_HASH_KEY];
		memset(m_HashTable,0,sizeof(LP_fmHashTableEntry)*MAX_HASH_KEY);
		for(int i=0;i<MAX_HASH_KEY;i++)
			{
				m_HashTable[i]=new fmHashTableEntry;
				memset(m_HashTable[i],0,sizeof(fmHashTableEntry));
			}
		m_listOutChannelHead=NULL;
		m_listHashTableHead=NULL;
		m_listHashTableCurrent=NULL;
	}

CAFirstMixChannelList::~CAFirstMixChannelList()
	{
	}
		
SINT32 CAFirstMixChannelList::add(CAMuxSocket* pMuxSocket,CAQueue* pQueueSend)
	{
		if(pMuxSocket==NULL)
			return E_UNKNOWN;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		if(pHashTableEntry->pMuxSocket!=NULL)
			return E_UNKNOWN;
		pHashTableEntry->pMuxSocket=pMuxSocket;
		pHashTableEntry->pQueueSend=pQueueSend;
		if(m_listHashTableHead==NULL)
			{
				pHashTableEntry->list_HashEntries.next=NULL;
				pHashTableEntry->list_HashEntries.prev=NULL;
				m_listHashTableHead=pHashTableEntry;				
			}
		else
			{
				pHashTableEntry->list_HashEntries.next=m_listHashTableHead;
				pHashTableEntry->list_HashEntries.prev=NULL;
				m_listHashTableHead->list_HashEntries.prev=pHashTableEntry;
				m_listHashTableHead=pHashTableEntry;				
			}
		return E_SUCCESS;
	}

SINT32 CAFirstMixChannelList::add(CAMuxSocket* pMuxSocket,HCHANNEL channelIn,
																	HCHANNEL channelOut,CASymCipher* pCipher)
	{
		if(pMuxSocket==NULL)
			return E_UNKNOWN;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		if(pHashTableEntry->pMuxSocket==NULL)
			return E_UNKNOWN;
		fmChannelListEntry* pEntry=pHashTableEntry->pChannelList;
		fmChannelListEntry* pNewEntry=new fmChannelListEntry;
		memset(pNewEntry,0,sizeof(fmChannelListEntry));
		pNewEntry->pCipher=pCipher;
		pNewEntry->channelIn=channelIn;
		pNewEntry->channelOut=channelOut;
		pNewEntry->bIsSuspended=false;
		pNewEntry->pHead=pHashTableEntry;
		if(pEntry==NULL) //First Entry to the channel list
			{
				pNewEntry->list_InChannelPerSocket.next=NULL;
				pNewEntry->list_InChannelPerSocket.prev=NULL;				
			}
		else
			{
				pNewEntry->list_InChannelPerSocket.next=pEntry;
				pNewEntry->list_InChannelPerSocket.prev=NULL;
				pEntry->list_InChannelPerSocket.prev=pNewEntry;
			}
		pHashTableEntry->pChannelList=pNewEntry;
		if(m_listOutChannelHead==NULL) //First Entry to OutChannel-List
			{
				pNewEntry->list_OutChannel.prev=NULL;
				pNewEntry->list_OutChannel.next=NULL;
				m_listOutChannelHead=pNewEntry;
			}
		else
			{
				pNewEntry->list_OutChannel.prev=NULL;
				pNewEntry->list_OutChannel.next=m_listOutChannelHead;
				m_listOutChannelHead->list_OutChannel.prev=pNewEntry;
				m_listOutChannelHead=pNewEntry;
			}

		return E_SUCCESS;
	}
			
fmChannelListEntry* CAFirstMixChannelList::get(CAMuxSocket* pMuxSocket,HCHANNEL channelIn)
	{
		if(pMuxSocket==NULL)
			return NULL;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return NULL;
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		fmChannelListEntry* pEntry=pHashTableEntry->pChannelList;
		while(pEntry!=NULL)
			{
				if(pEntry->channelIn==channelIn)
					return pEntry;
				pEntry=pEntry->list_InChannelPerSocket.next;
			}
		return NULL;		
	}

fmChannelListEntry* CAFirstMixChannelList::get(HCHANNEL channelOut)
	{
		fmChannelListEntry* pEntry=m_listOutChannelHead;
		while(pEntry!=NULL)
			{
				if(pEntry->channelOut==channelOut)
					return pEntry;
				pEntry=pEntry->list_OutChannel.next;
			}
		return NULL;
	}
	
SINT32 CAFirstMixChannelList::remove(CAMuxSocket* pMuxSocket)
	{
		if(pMuxSocket==NULL)
			return E_UNKNOWN;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		if(pHashTableEntry->pMuxSocket==NULL)
			return E_UNKNOWN;
		if(pHashTableEntry->list_HashEntries.prev==NULL) //head
			{
				if(pHashTableEntry->list_HashEntries.next==NULL)
					{
						m_listHashTableHead=NULL;
					}
				else
					{
						pHashTableEntry->list_HashEntries.next->list_HashEntries.prev=NULL;
						m_listHashTableHead=pHashTableEntry->list_HashEntries.next;
					}
			}
		else
			{
				if(pHashTableEntry->list_HashEntries.next==NULL)
					{
						pHashTableEntry->list_HashEntries.prev->list_HashEntries.next=NULL;
					}
				else
					{
						pHashTableEntry->list_HashEntries.prev->list_HashEntries.next=pHashTableEntry->list_HashEntries.next;
						pHashTableEntry->list_HashEntries.next->list_HashEntries.prev=pHashTableEntry->list_HashEntries.prev;
					}
			}
		fmChannelListEntry* pEntry=pHashTableEntry->pChannelList;
		fmChannelListEntry* pTmpEntry;
		while(pEntry!=NULL)
			{
				pTmpEntry=pEntry->list_InChannelPerSocket.next;
				if(pEntry->list_OutChannel.prev==NULL) //head
					{
						if(pEntry->list_OutChannel.next==NULL)
							{
								m_listOutChannelHead=NULL;
							}
						else
							{
								pEntry->list_OutChannel.next->list_OutChannel.prev=NULL;
								m_listOutChannelHead=pEntry->list_OutChannel.next;
							}
					}
				else
					{
						if(pEntry->list_OutChannel.next==NULL)
							{
								pEntry->list_OutChannel.prev->list_OutChannel.next=NULL;
							}
						else
							{
								pEntry->list_OutChannel.prev->list_OutChannel.next=pEntry->list_OutChannel.next;
								pEntry->list_OutChannel.next->list_OutChannel.prev=pEntry->list_OutChannel.prev;
							}
					}
				delete pEntry;
				pEntry=pTmpEntry;
			}
		memset(pHashTableEntry,0,sizeof(fmHashTableEntry));
		return E_SUCCESS;
	}

SINT32 CAFirstMixChannelList::remove(CAMuxSocket* pMuxSocket,HCHANNEL channelIn)
	{
		if(pMuxSocket==NULL)
			return E_UNKNOWN;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		if(pHashTableEntry->pMuxSocket==NULL)
			return E_UNKNOWN;
		fmChannelListEntry* pEntry=pHashTableEntry->pChannelList;
		while(pEntry!=NULL)
			{
				if(pEntry->channelIn==channelIn)
					{
						if(pEntry->list_OutChannel.prev==NULL) //head
							{
								if(pEntry->list_OutChannel.next==NULL)
									{
										m_listOutChannelHead=NULL;
									}
								else
									{
										pEntry->list_OutChannel.next->list_OutChannel.prev=NULL;
										m_listOutChannelHead=pEntry->list_OutChannel.next;
									}
							}
						else
							{
								if(pEntry->list_OutChannel.next==NULL)
									{
										pEntry->list_OutChannel.prev->list_OutChannel.next=NULL;
									}
								else
									{
										pEntry->list_OutChannel.prev->list_OutChannel.next=pEntry->list_OutChannel.next;
										pEntry->list_OutChannel.next->list_OutChannel.prev=pEntry->list_OutChannel.prev;
									}
							}
						
						if(pEntry->list_InChannelPerSocket.prev==NULL) //head
							{
								if(pEntry->list_InChannelPerSocket.next==NULL)
									{
										pHashTableEntry->pChannelList=NULL;
									}
								else
									{
										pEntry->list_InChannelPerSocket.next->list_InChannelPerSocket.prev=NULL;
										pHashTableEntry->pChannelList=pEntry->list_InChannelPerSocket.next;
									}
							}
						else
							{
								if(pEntry->list_InChannelPerSocket.next==NULL)
									{
										pEntry->list_InChannelPerSocket.prev->list_InChannelPerSocket.next=NULL;
									}
								else
									{
										pEntry->list_InChannelPerSocket.prev->list_InChannelPerSocket.next=pEntry->list_InChannelPerSocket.next;
										pEntry->list_InChannelPerSocket.next->list_InChannelPerSocket.prev=pEntry->list_InChannelPerSocket.prev;
									}
							}
						delete pEntry;
						return E_SUCCESS;
					}
				pEntry=pEntry->list_InChannelPerSocket.next;
			}
		return E_UNKNOWN;
	}


fmHashTableEntry* CAFirstMixChannelList::getFirst()
	{
		m_listHashTableCurrent=m_listHashTableHead;
		return m_listHashTableHead;
	}

fmHashTableEntry* CAFirstMixChannelList::getNext()
	{
		if(m_listHashTableCurrent!=NULL)
			m_listHashTableCurrent=m_listHashTableCurrent->list_HashEntries.next;
		return m_listHashTableCurrent;
	}

fmChannelListEntry* CAFirstMixChannelList::getFirstChannelForSocket(CAMuxSocket* pMuxSocket)
	{
		if(pMuxSocket==NULL)
			return NULL;
		SINT32 hashkey=pMuxSocket->getSocket();
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return NULL;
		fmHashTableEntry* pHashTableEntry=m_HashTable[hashkey];
		return pHashTableEntry->pChannelList;
	}

fmChannelListEntry* CAFirstMixChannelList::getNextChannel(fmChannelListEntry* pEntry)
	{
		return pEntry->list_InChannelPerSocket.next;
	}
