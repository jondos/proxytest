#include "StdAfx.h"
#include "CAFirstMixChannelList.hpp"


#define MAX_HASH_KEY 1025

CAFirstMixChannelList::CAFirstMixChannelList()
	{
		m_HashTable=new fmHashTableEntry[MAX_HASH_KEY];
		memset(m_HashTable,0,sizeof(fmHashTableEntry)*MAX_HASH_KEY);
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
		SINT32 hashkey=(SOCKET)pMuxSocket;
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		fmHashTableEntry oHashTableEntry=m_HashTable[hashkey];
		if(oHashTableEntry.pMuxSocket!=NULL)
			return E_UNKNOWN;
		oHashTableEntry.pMuxSocket=pMuxSocket;
		oHashTableEntry.pQueueSend=pQueueSend;
		if(m_listHashTableHead==NULL)
			{
				oHashTableEntry.list_HashEntries.next=NULL;
				oHashTableEntry.list_HashEntries.prev=NULL;
				m_listHashTableHead=&oHashTableEntry;				
			}
		else
			{
				oHashTableEntry.list_HashEntries.next=m_listHashTableHead;
				oHashTableEntry.list_HashEntries.prev=NULL;
				m_listHashTableHead->list_HashEntries.prev=&oHashTableEntry;
				m_listHashTableHead=&oHashTableEntry;				
			}
		return E_SUCCESS;
	}

SINT32 CAFirstMixChannelList::add(CAMuxSocket* pMuxSocket,HCHANNEL channelIn,
																	HCHANNEL channelOut,CASymCipher* pCipher)
	{
		if(pMuxSocket==NULL)
			return E_UNKNOWN;
		SINT32 hashkey=(SOCKET)pMuxSocket;
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		fmHashTableEntry oHashTableEntry=m_HashTable[hashkey];
		if(oHashTableEntry.pMuxSocket==NULL)
			return E_UNKNOWN;
		fmChannelListEntry* pEntry=oHashTableEntry.pChannelList;
		fmChannelListEntry* pNewEntry=new fmChannelListEntry;
		memset(pNewEntry,0,sizeof(fmChannelListEntry));
		pNewEntry->pCipher=pCipher;
		pNewEntry->channelIn=channelIn;
		pNewEntry->channelOut=channelOut;
		pNewEntry->bIsSuspended=false;
		pNewEntry->pHead=&oHashTableEntry;
		if(pEntry==NULL)
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
		oHashTableEntry.pChannelList=pNewEntry;
		if(m_listOutChannelHead==NULL)
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
		SINT32 hashkey=(SOCKET)pMuxSocket;
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return NULL;
		fmHashTableEntry oHashTableEntry=m_HashTable[hashkey];
		fmChannelListEntry* pEntry=oHashTableEntry.pChannelList;
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
		SINT32 hashkey=(SOCKET)pMuxSocket;
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		fmHashTableEntry oHashTableEntry=m_HashTable[hashkey];
		if(oHashTableEntry.pMuxSocket==NULL)
			return E_UNKNOWN;
		if(oHashTableEntry.list_HashEntries.prev==NULL) //head
			{
				if(oHashTableEntry.list_HashEntries.next==NULL)
					{
						m_listHashTableHead=NULL;
					}
				else
					{
						oHashTableEntry.list_HashEntries.next->list_HashEntries.prev=NULL;
						m_listHashTableHead=oHashTableEntry.list_HashEntries.next;
					}
			}
		else
			{
				if(oHashTableEntry.list_HashEntries.next==NULL)
					{
						oHashTableEntry.list_HashEntries.prev->list_HashEntries.next=NULL;
					}
				else
					{
						oHashTableEntry.list_HashEntries.prev->list_HashEntries.next=oHashTableEntry.list_HashEntries.next;
						oHashTableEntry.list_HashEntries.next->list_HashEntries.prev=oHashTableEntry.list_HashEntries.prev;
					}
			}
		fmChannelListEntry* pEntry=oHashTableEntry.pChannelList;
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
		memset(&oHashTableEntry,0,sizeof(fmHashTableEntry));
		return E_SUCCESS;
	}

SINT32 CAFirstMixChannelList::remove(CAMuxSocket* pMuxSocket,HCHANNEL channelIn)
	{
		if(pMuxSocket==NULL)
			return E_UNKNOWN;
		SINT32 hashkey=(SOCKET)pMuxSocket;
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return E_UNKNOWN;
		fmHashTableEntry oHashTableEntry=m_HashTable[hashkey];
		if(oHashTableEntry.pMuxSocket==NULL)
			return E_UNKNOWN;
		fmChannelListEntry* pEntry=oHashTableEntry.pChannelList;
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
										oHashTableEntry.pChannelList=NULL;
									}
								else
									{
										pEntry->list_InChannelPerSocket.next->list_InChannelPerSocket.prev=NULL;
										oHashTableEntry.pChannelList=pEntry->list_InChannelPerSocket.next;
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
		SINT32 hashkey=(SOCKET)pMuxSocket;
		if(hashkey>MAX_HASH_KEY-1||hashkey<0)
			return NULL;
		fmHashTableEntry oHashTableEntry=m_HashTable[hashkey];
		return oHashTableEntry.pChannelList;
	}

fmChannelListEntry* CAFirstMixChannelList::getNextChannel(fmChannelListEntry* pEntry)
	{
		return pEntry->list_InChannelPerSocket.next;
	}
