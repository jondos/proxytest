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
#include "CAControlChannelDispatcher.hpp"
#include "CAAbstractControlChannel.hpp"

SINT32 CAControlChannelDispatcher::registerControlChannel(CAAbstractControlChannel* pControlChannel)
	{
		if(pControlChannel==NULL||pControlChannel->getID()>255)
			return E_UNKNOWN;
		m_pcsRegisterChannel->lock();
		pControlChannel->setDispatcher(this);
		m_arControlChannels[pControlChannel->getID()]= pControlChannel;
		m_pcsRegisterChannel->unlock();
		return E_SUCCESS;
	}

SINT32 CAControlChannelDispatcher::removeControlChannel(UINT32 id)
	{
		if(id>255)
			return E_UNKNOWN;
		m_pcsRegisterChannel->lock();
		m_arControlChannels[id]=NULL;
		m_pcsRegisterChannel->unlock();
		return E_SUCCESS;
	}

/** Deregisters all control channels and calls delete on every registered control channel object.*/
void CAControlChannelDispatcher::deleteAllControlChannels()
	{
		m_pcsRegisterChannel->lock();
		for(UINT32 i=0;i<256;i++)
			{
				delete m_arControlChannels[i];
				m_arControlChannels[i]=NULL;			
			}
		m_pcsRegisterChannel->unlock();
	}
	
bool CAControlChannelDispatcher::proccessMixPacket(const MIXPACKET* pPacket)
	{
		if(pPacket->channel<256&&pPacket->channel>0)
			{
				CAAbstractControlChannel* pControlChannel=m_arControlChannels[pPacket->channel];
				if(pControlChannel!=NULL)
				{
					if (pControlChannel->proccessMessage(pPacket->data,pPacket->flags) == E_SUCCESS)
					{
						return true;
					}
				}
				else
				{
					return true;		
				}
			}
		return false;
	}

SINT32 CAControlChannelDispatcher::sendMessages(UINT32 id,bool m_bIsEncrypted,const UINT8* msg,UINT32 msglen) const
	{
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"dispatch - sendMesg()\n");
		#endif
		if(msg==NULL)
			return E_UNKNOWN;
		m_pcsSendMsg->lock();
		m_pMixPacket->channel=id;
		UINT32 aktIndex=0;
		while(msglen>0)
			{
				if(msglen>DATA_SIZE)
					{
						m_pMixPacket->flags=DATA_SIZE;
					}
				else
					{
						m_pMixPacket->flags=(UINT16)msglen;
					}
				memcpy(m_pMixPacket->data,msg+aktIndex,m_pMixPacket->flags);
				m_pSendQueue->add(m_pQueueEntry,sizeof(tQueueEntry));
				aktIndex+=m_pMixPacket->flags;
				msglen-=m_pMixPacket->flags;
			}
		m_pcsSendMsg->unlock();	
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
