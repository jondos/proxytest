#include "StdAfx.h"
#ifndef ONLY_LOCAL_PROXY
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

/** Adds a new Channel to the Channellist.
	*
	* @param channelIn incoming Channel-ID
	* @param pCipher Cipher for recoding
	* @param channelOut on return holds a newly created random outgoing Channel-ID
	* @retval E_SUCCESS if Channel was successfully added to the list
	**/
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
			}while(*channelOut<256||getOutToIn_intern_without_lock(NULL,*channelOut,NULL)==E_SUCCESS);
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

/** Removes a channel form the channellist.
	* @param channelIn incoming Channel-ID of the channel which should be removed
	* @retval E_SUCCESS if channel was successfully removed from the list
	* @retval E_UNKNOWN otherwise
	*/
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
					
SINT32 CAMiddleMixChannelList::test()
				{
					CAMiddleMixChannelList oList;
					UINT32 c;
					UINT32 d;
					UINT32 rand;
					int i,j;
					for(i=0;i<1000;i++)
						{
							getRandom(&c);
							oList.add(c,NULL,&d);
						}
					for(i=0;i<100;i++)
						{
							getRandom(&rand);
							if(rand<0x0FFFFFFF)
								for(j=0;j<5;j++)
									{
										getRandom(&c);
										oList.add(c,NULL,&d);
									}
							getRandom(&rand);
							if(rand<0x7FFFFFFF)
								for(int j=0;j<100000;j++)
									{
										getRandom(&c);
										oList.remove(c);
									}
						}
					return 0;
				}
#endif //ONLY_LOCAL_PROXY
