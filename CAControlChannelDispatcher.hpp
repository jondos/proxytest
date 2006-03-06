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

#ifndef _CACONTROLCHANNELDISPATCHER_H_HEADER_
#define _CACONTROLCHANNELDISPATCHER_H_HEADER_
#include "CAQueue.hpp"
class CAAbstractControlChannel;

/** This class "dispatches" messages which it receives via proccessMixPacket()
	* to the associated control channel.
	*/
class CAControlChannelDispatcher
{
  public:
		/** Constructs a new dispatcher.
			* @param pSendQueue send queue in which the mix packets will be puted, if
			*  a control channel sends a message
			*/
		CAControlChannelDispatcher(CAQueue* pSendQueue)
			{
				m_pSendQueue=pSendQueue;
				m_pQueueEntry=new tQueueEntry;
				m_pMixPacket=&m_pQueueEntry->packet;
				m_arControlChannels=new CAAbstractControlChannel*[256];
				memset(m_arControlChannels,0,256*sizeof(CAAbstractControlChannel*));
				m_pcsSendMsg=new CAMutex();
				m_pcsRegisterChannel=new CAMutex();
			}

		~CAControlChannelDispatcher()
			{
				delete m_pcsSendMsg;
				delete m_pcsRegisterChannel;
				delete[] m_arControlChannels;
				delete m_pQueueEntry;
			}
			
		void deleteAllControlChannels(void);

		/** Registers a control channel for receiving messages*/		
    SINT32 registerControlChannel(CAAbstractControlChannel* pControlChannel);
    SINT32 removeControlChannel(UINT32 id);

    bool proccessMixPacket(const MIXPACKET* pPacket);
		SINT32 sendMessages(UINT32 id,bool m_bIsEncrypted,const UINT8* msg,UINT32 msglen) const;
  
	private:
		CAQueue* m_pSendQueue;
		MIXPACKET* m_pMixPacket;
    CAAbstractControlChannel** m_arControlChannels;
		tQueueEntry* m_pQueueEntry;
		CAMutex* m_pcsSendMsg;
		CAMutex* m_pcsRegisterChannel;
};
#endif
