#include "StdAfx.h"
#include "CAMiddleMixChannelList.hpp"
#include "CAUtil.hpp"


CAMiddleMixChannelList::~CAMiddleMixChannelList()
	{
		m_Mutex.lock();
		mmChannelListEntry* pEntry=m_pChannelList;
		while(m_pChannelList!=NULL)
			{
				pEntry=m_pChannelList;
				m_pChannelList=m_pChannelList->next;
				delete pEntry->pCipher;
				delete pEntry;
			}
		m_Mutex.unlock();
	}


SINT32 CAMiddleMixChannelList::add(HCHANNEL channelIn,CASymCipher* pCipher,HCHANNEL* channelOut)
	{
		m_Mutex.lock();
		mmChannelListEntry* pEntry=new mmChannelListEntry;
		pEntry->pCipher=pCipher;
		pEntry->channelIn=channelIn;
		pEntry->prev=NULL;
		pEntry->next=m_pChannelList;
		do
			{
				getRandom(&pEntry->channelOut);
			}while(getOutToIn_intern_without_lock(NULL,pEntry->channelOut,NULL)==E_SUCCESS);
		if(m_pChannelList!=NULL)
			m_pChannelList->prev=pEntry;
		m_pChannelList=pEntry;
		m_Mutex.unlock();
		return E_SUCCESS;
	}
			
SINT32 CAMiddleMixChannelList::getInToOut(HCHANNEL channelIn, HCHANNEL* channelOut,CASymCipher** ppCipher)
	{
		m_Mutex.lock();
		mmChannelListEntry* pEntry=m_pChannelList;
		while(pEntry!=NULL)
			{
				if(pEntry->channelIn==channelIn)
					{
						if(channelOut!=NULL)
							*channelOut=pEntry->channelOut;
						if(ppCipher!=NULL)
							*ppCipher=pEntry->pCipher;
						(*ppCipher)->lock();
						m_Mutex.unlock();
						return E_SUCCESS;
					}
				pEntry=pEntry->next;
			}
		m_Mutex.unlock();
		return E_UNKNOWN;
	}

SINT32 CAMiddleMixChannelList::remove(HCHANNEL channelIn)
	{
		m_Mutex.lock();
		mmChannelListEntry* pEntry=m_pChannelList;
		while(pEntry!=NULL)
			{
				if(pEntry->channelIn==channelIn)
					{
						delete pEntry->pCipher;
						if(pEntry->prev==NULL)
							{
								if(pEntry->next==NULL)
									{
										delete pEntry;
										m_pChannelList=NULL;
									}
								else
									{
										m_pChannelList=pEntry->next;
										m_pChannelList->prev=NULL;
										delete pEntry;
									}
							}
						else
							{
								if(pEntry->next==NULL)
									{
										pEntry->prev->next=NULL;
										delete pEntry;
									}
								else
									{
										pEntry->prev->next=pEntry->next;
										pEntry->next->prev=pEntry->prev;
										delete pEntry;
									}								
							}
						m_Mutex.unlock();
						return E_SUCCESS;
					}
				pEntry=pEntry->next;
			}
		m_Mutex.unlock();
		return E_UNKNOWN;
	}
					
