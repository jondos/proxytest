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
#include "CAUtil.hpp"
#include "CAThread.hpp"
#include "CAThreadPool.hpp"
#ifdef PAYMENT
#include "CAAccountingInstance.hpp"
#endif
#include "CALogPacketStats.hpp"
#ifdef HAVE_EPOLL
	#include "CASocketGroupEpoll.hpp"
#endif

class CAInfoService;


class CAFirstMix:public CAMix
	{
		public:
			CAFirstMix()
				{
					m_nMixedPackets=0;
					m_nSocketsIn=0;
					m_pQueueSendToMix=NULL;
					m_pQueueReadFromMix=NULL;
					m_pIPList=NULL;
					m_arrSocketsIn=NULL;
					m_pRSA=NULL;
					m_pInfoService=NULL;
					m_psocketgroupUsersRead=NULL;
					m_psocketgroupUsersWrite=NULL;
					m_pChannelList=NULL;
					m_pMuxOut=NULL;
					m_docMixCascadeInfo=NULL;
					m_xmlKeyInfoBuff=NULL;
					m_pthreadSendToMix=NULL;
					m_pthreadReadFromMix=NULL;
					m_pthreadAcceptUsers=NULL;
					m_pthreadsLogin=NULL;
					#ifdef LOG_PACKET_TIMES
						if(sizeof(CAFirstMix::tQueueEntry)!=MIXPACKET_SIZE+sizeof(UINT64))
							{
								CAMsg::printMsg(LOG_CRIT,"sizeof(CAFirstMix::tQueueEntry) [%u] !=MIXPACKETSIZE+sizeof(UINT64) [%u]\n",sizeof(CAFirstMix::tQueueEntry),MIXPACKET_SIZE+sizeof(UINT64));
								//exit(0);
							}
						m_pLogPacketStats=NULL;
					#endif	
				}
			virtual ~CAFirstMix(){}
		protected:
			virtual SINT32 loop()=0;
			SINT32 init();
			SINT32 clean();
			SINT32 initOnce();
			SINT32 initMixCascadeInfo(UINT8* recvBuff,UINT32 len);
		public:
			SINT32 getMixedPackets(UINT64& ppackets)
				{
					set64(ppackets,m_nMixedPackets);
					return E_SUCCESS;
				}

			SINT32 getLevel(SINT32* puser,SINT32* prisk,SINT32* ptraffic)
				{
					*puser=(SINT32)m_nUser;
					*prisk=-1;
					*ptraffic=-1;
					return E_SUCCESS;
				}
			
			/** Returns the Mix-Cascade info which should be send to the InfoService.
				* This is NOT a copy!
				*
				* @param docMixCascadeInfo where the XML struct would be stored
				* @retval E_SUCCESS
				*/
			SINT32 getMixCascadeInfo(DOM_Document& docMixCascadeInfo)
				{
					docMixCascadeInfo=m_docMixCascadeInfo;
					return E_SUCCESS;
				}
					
			
		friend THREAD_RETURN fm_loopSendToMix(void*);
		friend THREAD_RETURN fm_loopReadFromMix(void*);
		friend THREAD_RETURN fm_loopAcceptUsers(void*);
		friend THREAD_RETURN fm_loopReadFromUsers(void*);
		friend THREAD_RETURN fm_loopDoUserLogin(void* param);

		protected:
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
					inc64(m_nMixedPackets);
					m_mutexMixedPackets.unlock();
					return E_SUCCESS;
				}

			bool getRestart()
				{
					return m_bRestart;
				}
			SINT32 doUserLogin(CAMuxSocket* pNewUSer,UINT8 perrIP[4]);
		protected:	
			CAIPList* m_pIPList;
			struct t_queue_entry
				{
					MIXPACKET packet;
					#ifdef LOG_PACKET_TIMES
						UINT64 timestamp;
					#endif
				};
			typedef struct t_queue_entry tQueueEntry;
			CAQueue* m_pQueueSendToMix;
			CAQueue* m_pQueueReadFromMix;
			#ifdef LOG_PACKET_TIMES
				CALogPacketStats* m_pLogPacketStats;
			#endif
			CAFirstMixChannelList* m_pChannelList;
			volatile UINT32 m_nUser;
			UINT32 m_nSocketsIn; //number of usable ListenerInterface (non 'virtual')
			volatile bool m_bRestart;
			CASocket* m_arrSocketsIn;
#ifdef HAVE_EPOLL
			CASocketGroupEpoll* m_psocketgroupUsersRead;
			CASocketGroupEpoll* m_psocketgroupUsersWrite;
#else
			CASocketGroup* m_psocketgroupUsersRead;
			CASocketGroup* m_psocketgroupUsersWrite;
#endif
			CAInfoService* m_pInfoService;
			CAMuxSocket* m_pMuxOut;
	
			UINT8* m_xmlKeyInfoBuff;
			UINT16 m_xmlKeyInfoSize;

			DOM_Document m_docMixCascadeInfo;
			UINT64 m_nMixedPackets;
			CAASymCipher* m_pRSA;
			CASignature* m_pSignature;
			CAMutex m_mutexUser;
			CAMutex m_mutexMixedPackets;
			CAMutex m_mutexLoginThreads;

			CAThread* m_pthreadAcceptUsers;
			CAThreadPool* m_pthreadsLogin;
			CAThread* m_pthreadSendToMix;
			CAThread* m_pthreadReadFromMix;
			#ifdef PAYMENT
				CAAccountingInstance * m_pAccountingInstance;
			#endif
	};

#endif
