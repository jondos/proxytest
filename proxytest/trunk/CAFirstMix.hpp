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

#ifndef __CAFIRSTMIX__
#define __CAFIRSTMIX__

#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
#include "CAASymCipher.hpp"
#include "CASignature.hpp"
#include "CAFirstMixChannelList.hpp"
#include "CAIPList.hpp" 
#include "CASocketGroup.hpp"
#include "CAQueue.hpp"

THREAD_RETURN loopSendToMix(void *param);
THREAD_RETURN loopAcceptUsers(void *param);
THREAD_RETURN loopReadFromUsers(void *param);

class CAInfoService;
class CAFirstMix:public CAMix
	{
		public:
			CAFirstMix()
				{
					m_nMixedPackets=0;
					m_nSocketsIn=0;
					m_pQueueSendToMix=NULL;
					m_pIPList=NULL;
					m_arrSocketsIn=NULL;
					m_KeyInfoBuff=NULL;
					m_pRSA=NULL;
					m_pInfoService=NULL;
					m_psocketgroupUsersRead=m_psocketgroupUsersWrite=NULL;
					m_pChannelList=NULL;
					m_pMuxOut=NULL;
					m_strXmlMixCascadeInfo=NULL;
					m_xmlKeyInfoBuff=NULL;
				}
			virtual ~CAFirstMix(){}
		private:
			SINT32 loop();
			SINT32 init();
			SINT32 clean();
			SINT32 initOnce();
			SINT32 initMixCascadeInfo(UINT8* recvBuff,UINT32 len);
		public:
			SINT32 getMixedPackets(UINT32* ppackets)
				{
					if(ppackets!=NULL)
						{
							*ppackets=m_nMixedPackets;
							return E_SUCCESS;
						}
					return E_UNKNOWN;
				}

			SINT32 getLevel(SINT32* puser,SINT32* prisk,SINT32* ptraffic)
				{
					*puser=(SINT32)m_nUser;
					*prisk=-1;
					*ptraffic=-1;
					return E_SUCCESS;
				}
			
			/** Returns the Mix-Cascade info which should be send to the InfoService.
				* The status message ist an XML struct retruned in buff. There is not \0 after
				* the XML (so it is not a string!*/
			SINT32 getMixCascadeInfo(UINT8* buff,UINT32*len);
					
			
		friend THREAD_RETURN loopSendToMix(void*);
		friend THREAD_RETURN loopAcceptUsers(void*);
		friend THREAD_RETURN loopReadFromUsers(void*);
		private:
			SINT32 incUsers()
				{
					m_mutexUser.lock();
					m_nUser++;
					m_mutexUser.unlock();
					return E_SUCCESS;
				}
			
			SINT32 decUsers()
				{
					m_mutexUser.lock();
					m_nUser--;
					m_mutexUser.unlock();
					return E_SUCCESS;
				}

			SINT32 incMixedPackets()
				{
					m_mutexMixedPackets.lock();
					m_nMixedPackets++;
					m_mutexMixedPackets.unlock();
					return E_SUCCESS;
				}

				
			bool getRestart()
				{
					return m_bRestart;
				}
			
		private:	
			CAIPList* m_pIPList;
			CAQueue* m_pQueueSendToMix;
			CAFirstMixChannelList* m_pChannelList;
			UINT32 m_nUser;
			UINT32 m_nSocketsIn;
			volatile bool m_bRestart;
			CASocket* m_arrSocketsIn;
			CASocketGroup* m_psocketgroupUsersRead;
			CASocketGroup* m_psocketgroupUsersWrite;
			CAInfoService* m_pInfoService;
			CAMuxSocket* m_pMuxOut;
			UINT8* m_KeyInfoBuff;
			UINT16 m_KeyInfoSize;

			UINT8* m_xmlKeyInfoBuff;
			UINT16 m_xmlKeyInfoSize;

			UINT8* m_strXmlMixCascadeInfo;
			UINT32 m_nMixedPackets;
			CAASymCipher* m_pRSA;
			CASignature* m_pSignature;
			CAMutex m_mutexUser;
			CAMutex m_mutexMixedPackets;
	};

#endif
