#include "StdAfx.h"
#include "CAMuxChannelList.hpp"

CAMuxChannelList::CAMuxChannelList()
	{
		list=NULL;
		reverselist=NULL;
		aktEnumPos=NULL;
	}

int CAMuxChannelList::add(CAMuxSocket* pMuxSocket)
	{
		if(list==NULL)
			{
				list=new MUXLISTENTRY;
				list->next=NULL;
			}
		else
			{
				MUXLISTENTRY* tmpEntry=new MUXLISTENTRY;
				tmpEntry->next=list;
				list=tmpEntry;
			}
		list->pSocketList=new CASocketList();
		list->pMuxSocket=pMuxSocket;
		return 0;
	}
			
MUXLISTENTRY* CAMuxChannelList::get(CAMuxSocket* pMuxSocket)
	{
		MUXLISTENTRY* tmpEntry=list;
		while(tmpEntry!=NULL)
			{
				if(tmpEntry->pMuxSocket==pMuxSocket)
					return tmpEntry;
				tmpEntry=tmpEntry->next;
			}
		return NULL;
	}

bool CAMuxChannelList::remove(CAMuxSocket* pMuxSocket,MUXLISTENTRY* pEntry)
	{
		MUXLISTENTRY* tmpEntry=list;
		MUXLISTENTRY* before=NULL;
		while(tmpEntry!=NULL)
			{
				if(tmpEntry->pMuxSocket==pMuxSocket)
					{
						if(aktEnumPos==tmpEntry)
							aktEnumPos=tmpEntry->next;
						if(before==NULL)
							{
								list=tmpEntry->next;
							}
						else
							{
								before->next=tmpEntry->next;
							}
						CONNECTION* tmpCon=tmpEntry->pSocketList->getFirst();
						while(tmpCon!=NULL)
							{
								//Out-Channel aus Reverselist entfernen....
								REVERSEMUXLISTENTRY* rbefore=NULL;
								REVERSEMUXLISTENTRY* tmpReverseEntry=reverselist;
								while(tmpReverseEntry!=NULL)
									{
										if(tmpReverseEntry->outChannel==tmpCon->outChannel)
											{
												if(rbefore!=NULL)
													rbefore->next=tmpReverseEntry->next;
												else
													reverselist=tmpReverseEntry->next;
												delete tmpReverseEntry;
												break;			
											}
										rbefore=tmpReverseEntry;
										tmpReverseEntry=tmpReverseEntry->next;
									}
								tmpCon=tmpEntry->pSocketList->getNext();
							}
						if(pEntry!=NULL)
							memcpy(pEntry,tmpEntry,sizeof(MUXLISTENTRY));
						else
							{
								delete tmpEntry->pSocketList;
								delete tmpEntry->pMuxSocket;
							}
						delete tmpEntry;
						return true;
					}
				before=tmpEntry;
				tmpEntry=tmpEntry->next;
			}
		return false;
	}

			
int CAMuxChannelList::add(MUXLISTENTRY* pEntry,HCHANNEL in,HCHANNEL out,CASymCipher* pCipher)
	{
		pEntry->pSocketList->add(in,out,pCipher);
		if(reverselist==NULL)
			{
				reverselist=new REVERSEMUXLISTENTRY;
				reverselist->next=NULL;
			}
		else
			{
				REVERSEMUXLISTENTRY* tmpEntry=new REVERSEMUXLISTENTRY;
				tmpEntry->next=reverselist;
				reverselist=tmpEntry;
			}
		reverselist->pMuxSocket=pEntry->pMuxSocket;
		reverselist->inChannel=in;
		reverselist->outChannel=out;
		reverselist->pCipher=pCipher;
		return 0;
	}

bool CAMuxChannelList::get(MUXLISTENTRY* pEntry, HCHANNEL in, CONNECTION* out)
	{
		return pEntry->pSocketList->get(in,out);
	}
			
REVERSEMUXLISTENTRY* CAMuxChannelList::get(HCHANNEL out)
	{	
		REVERSEMUXLISTENTRY* tmpReverseEntry=reverselist;
		while(tmpReverseEntry!=NULL)
			{
				if(tmpReverseEntry->outChannel==out)
					return tmpReverseEntry;
				tmpReverseEntry=tmpReverseEntry->next;
			}
		return NULL;
	}
	
bool CAMuxChannelList::remove(HCHANNEL out,REVERSEMUXLISTENTRY* retReverseEntry)
	{
		REVERSEMUXLISTENTRY* before=NULL;
		REVERSEMUXLISTENTRY* tmpReverseEntry=reverselist;
		while(tmpReverseEntry!=NULL)
			{
				if(tmpReverseEntry->outChannel==out)
					{
						MUXLISTENTRY* tmpEntry=get(tmpReverseEntry->pMuxSocket);
						
						//Error korrektion!!!!!
						tmpEntry->pSocketList->remove(tmpReverseEntry->inChannel);
						if(before!=NULL)
							{
								before->next=tmpReverseEntry->next;
							}
						else
							{
								reverselist=tmpReverseEntry->next;
							}
						if(retReverseEntry!=NULL)
							memcpy(retReverseEntry,tmpReverseEntry,sizeof(REVERSEMUXLISTENTRY));
						delete tmpReverseEntry;
						return true;			
					}
				before=tmpReverseEntry;
				tmpReverseEntry=tmpReverseEntry->next;
			}
		return false;
	}
	
MUXLISTENTRY* CAMuxChannelList::getFirst()
	{
		aktEnumPos=list;
		return aktEnumPos;
	}

MUXLISTENTRY* CAMuxChannelList::getNext()
	{
		if(aktEnumPos==NULL)
			return NULL;
		aktEnumPos=aktEnumPos->next;
		return aktEnumPos;
	}

	