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
		CAControlChannelDispatcher(CAQueue* pSendQueue,UINT8* keyRecv,UINT8* keySent)
			{
				m_pSendQueue=pSendQueue;
				m_pQueueEntry=new tQueueEntry;
				m_pMixPacket=&m_pQueueEntry->packet;
				m_arControlChannels=new CAAbstractControlChannel*[256];
				memset(m_arControlChannels,0,256*sizeof(CAAbstractControlChannel*));
				m_pcsSendMsg=new CAMutex();
				m_pcsRegisterChannel=new CAMutex();
				m_pcsEnc=new CAMutex();
				m_pcsDec=new CAMutex();
				m_nEncMsgCounter=0;
				m_pEncMsgIV=new UINT8[12];
				memset(m_pEncMsgIV,0,12);
				m_nDecMsgCounter=0;
				m_pDecMsgIV=new UINT8[12];
				memset(m_pDecMsgIV,0,12);
				if(keySent!=NULL)
				{
					m_pGCMCtxEnc=new gcm_ctx_64k;
					m_pGCMCtxDec=new gcm_ctx_64k;
					gcm_init_64k(m_pGCMCtxEnc,keySent,128);
					gcm_init_64k(m_pGCMCtxDec,keyRecv,128);
				}
				else
				{
					m_pGCMCtxEnc=NULL;
					m_pGCMCtxDec=NULL;
				}
			}

		~CAControlChannelDispatcher()
			{
				delete m_pcsSendMsg;
				m_pcsSendMsg = NULL;
				delete m_pcsRegisterChannel;
				m_pcsRegisterChannel = NULL;
				delete[] m_arControlChannels;
				m_arControlChannels = NULL;
				delete m_pQueueEntry;
				m_pQueueEntry = NULL;
				if(m_pGCMCtxEnc!=NULL)
					delete m_pGCMCtxEnc;
				if(m_pGCMCtxDec!=NULL)
					delete m_pGCMCtxDec;
				delete []m_pEncMsgIV;
				delete m_pcsEnc;
				delete []m_pDecMsgIV;
				delete m_pcsDec;
			}
			
		void deleteAllControlChannels(void);

		/** Registers a control channel for receiving messages*/		
    SINT32 registerControlChannel(CAAbstractControlChannel* pControlChannel);
    SINT32 removeControlChannel(UINT32 id);

    bool proccessMixPacket(const MIXPACKET* pPacket);
		SINT32 sendMessages(UINT32 id,const UINT8* msg,UINT32 msglen);
  
		/** Encrypts a control channel message. The output format is:
			cipher text
			auth tag - 16 bytes
			*/
		SINT32 encryptMessage(const UINT8* in,UINT32 inlen, UINT8* out,UINT32* outlen);
		/** Decrypts a control channel message, which has to be of form:
			cipher text
			auth tag - 16 bytes
			*/
		SINT32 decryptMessage(const UINT8* in,UINT32 inlen, UINT8* out,UINT32* outlen);
		///Temp workaorund function - to be removed soon...
		bool isKeySet()
			{
				return m_pGCMCtxEnc!=NULL;
			}

	private:
		CAQueue* m_pSendQueue;
		MIXPACKET* m_pMixPacket;
    CAAbstractControlChannel** m_arControlChannels;
		tQueueEntry* m_pQueueEntry;
		CAMutex* m_pcsSendMsg;
		CAMutex* m_pcsRegisterChannel;
		CAMutex* m_pcsEnc;
		CAMutex* m_pcsDec;

		gcm_ctx_64k* m_pGCMCtxEnc;
		gcm_ctx_64k* m_pGCMCtxDec;
		UINT32 m_nEncMsgCounter;
		UINT8* m_pEncMsgIV;
		UINT32 m_nDecMsgCounter;
		UINT8* m_pDecMsgIV;
};
#endif
