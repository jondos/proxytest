#include "StdAfx.h"
#include "CAMiddleMixChannelList.hpp"
#include "CAUtil.hpp"


CAMiddleMixChannelList::~CAMiddleMixChannelList()
	{
		m_Mutex.lock();
		for(UINT32 i=0;i<0x10000;i++)
			{			
				mmChannelListEntry* pEntry=m_pHashTableIn[i];
				mmChannelListEntry* pTmpEntry;
				while(pEntry!=NULL)
					{
						pTmpEntry=pEntry;
						pEntry=pEntry->list_HashTableIn.next;
						delete pTmpEntry->pCipher;
						delete pTmpEntry;
					}
			}
		delete m_pHashTableIn;
		delete m_pHashTableOut;
		m_Mutex.unlock();
	}


SINT32 CAMiddleMixChannelList::add(HCHANNEL channelIn,CASymCipher* pCipher,HCHANNEL* channelOut)
	{
		m_Mutex.lock();
		mmChannelListEntry* pEntry=new mmChannelListEntry;
		pEntry->pCipher=pCipher;
		pEntry->channelIn=channelIn;
		pEntry->list_HashTableIn.prev=NULL;
		pEntry->list_HashTableOut.prev=NULL;
		do
			{
				getRandom(channelOut);
			}while(getOutToIn_intern_without_lock(NULL,*channelOut,NULL)==E_SUCCESS);
		pEntry->channelOut=*channelOut;

		mmChannelListEntry* pTmpEntry=m_pHashTableIn[channelIn&0x0000FFFF];
		pEntry->list_HashTableIn.next=pTmpEntry;
		if(pTmpEntry!=NULL)
			pTmpEntry->list_HashTableIn.prev=pEntry;
		m_pHashTableIn[channelIn&0x0000FFFF]=pEntry;

		pTmpEntry=m_pHashTableOut[(*channelOut)&0x0000FFFF];
		pEntry->list_HashTableOut.next=pTmpEntry;
		if(pTmpEntry!=NULL)
			pTmpEntry->list_HashTableOut.prev=pEntry;
		m_pHashTableOut[(*channelOut)&0x0000FFFF]=pEntry;

		m_Mutex.unlock();
		return E_SUCCESS;
	}
			
SINT32 CAMiddleMixChannelList::getInToOut(HCHANNEL channelIn, HCHANNEL* channelOut,CASymCipher** ppCipher)
	{
		m_Mutex.lock();
		mmChannelListEntry* pEntry=m_pHashTableIn[channelIn&0x0000FFFF];
		while(pEntry!=NULL)
			{
				if(pEntry->channelIn==channelIn)
					{
						if(channelOut!=NULL)
							*channelOut=pEntry->channelOut;
						if(ppCipher!=NULL)
							{
								*ppCipher=pEntry->pCipher;
								(*ppCipher)->lock();
							}
						m_Mutex.unlock();
						return E_SUCCESS;
					}
				pEntry=pEntry->list_HashTableIn.next;
			}
		m_Mutex.unlock();
		return E_UNKNOWN;
	}

SINT32 CAMiddleMixChannelList::remove(HCHANNEL channelIn)
	{
		m_Mutex.lock();
		mmChannelListEntry* pEntry=m_pHashTableIn[channelIn&0x0000FFFF];
		while(pEntry!=NULL)
			{
				if(pEntry->channelIn==channelIn)
					{
						delete pEntry->pCipher;
						if(pEntry->list_HashTableIn.prev==NULL)
							{
								if(pEntry->list_HashTableIn.next==NULL)
									{
										m_pHashTableIn[channelIn&0x0000FFFF]=NULL;
									}
								else
									{
										m_pHashTableIn[channelIn&0x0000FFFF]=pEntry->list_HashTableIn.next;
										pEntry->list_HashTableIn.next->list_HashTableIn.prev=NULL;
									}
							}
						else
							{
								if(pEntry->list_HashTableIn.next==NULL)
									{
										pEntry->list_HashTableIn.prev->list_HashTableIn.next=NULL;
									}
								else
									{
										pEntry->list_HashTableIn.prev->list_HashTableIn.next=pEntry->list_HashTableIn.next;
										pEntry->list_HashTableIn.next->list_HashTableIn.prev=pEntry->list_HashTableIn.prev;
									}								
							}
						if(pEntry->list_HashTableOut.prev==NULL)
							{
								if(pEntry->list_HashTableOut.next==NULL)
									{
										m_pHashTableOut[pEntry->channelOut&0x0000FFFF]=NULL;
									}
								else
									{
										m_pHashTableOut[pEntry->channelOut&0x0000FFFF]=pEntry->list_HashTableOut.next;
										pEntry->list_HashTableOut.next->list_HashTableOut.prev=NULL;
									}
							}
						else
							{
								if(pEntry->list_HashTableOut.next==NULL)
									{
										pEntry->list_HashTableOut.prev->list_HashTableOut.next=NULL;
									}
								else
									{
										pEntry->list_HashTableOut.prev->list_HashTableOut.next=pEntry->list_HashTableOut.next;
										pEntry->list_HashTableOut.next->list_HashTableOut.prev=pEntry->list_HashTableOut.prev;
									}								
							}
						delete pEntry;
						m_Mutex.unlock();
						return E_SUCCESS;
					}
				pEntry=pEntry->list_HashTableIn.next;
			}
		m_Mutex.unlock();
		return E_UNKNOWN;
	}
					
