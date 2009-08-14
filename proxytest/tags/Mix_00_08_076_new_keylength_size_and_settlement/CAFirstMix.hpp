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
#include "doxygen.h"
#ifndef ONLY_LOCAL_PROXY
#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
#include "CAASymCipher.hpp"
//#include "CASignature.hpp"
#include "CAFirstMixChannelList.hpp"
#include "CAIPList.hpp"
#include "CASocketGroup.hpp"
#include "CAQueue.hpp"
#include "CAUtil.hpp"
#include "CAThread.hpp"
#include "CAThreadPool.hpp"
#include "TermsAndConditions.hpp"
#include "CALogPacketStats.hpp"
#include "CAConditionVariable.hpp"
#ifdef HAVE_EPOLL
	#include "CASocketGroupEpoll.hpp"
#endif

#ifdef REPLAY_DETECTION
	#include "CAMixWithReplayDB.hpp"
#endif
#ifdef COUNTRY_STATS
	#include "CAMutex.hpp"
#endif

#define TNC_SREQUEST "TermsAndConditionsRequest"
#define TNC_RESPONSE "TermsAndConditionsResponse"
#define TNC_SINTERRUPT "TermsAndConditionsInterrupt"
#define TNC_REQ_TRANSLATION "Translation"
#define TNC_RESOURCES "Resources"
#define TNC_RESOURCE_TEMPLATE "Template"
#define TNC_TEMPLATE_ROOT_ELEMENT "TermsAndConditionsTemplate"
#define TNC_RESOURCE_CUSTOMIZED_SECT "CustomizedSections"

#define TNC_RESPONSE_INVALID_REQUEST "InvalidTermsAndConditionsRequest"
#define TNC_CONFIRM_REQ "TermsAndConditionsConfirm"

class CAInfoService;

THREAD_RETURN fm_loopSendToMix(void*);
THREAD_RETURN fm_loopReadFromMix(void*);
THREAD_RETURN fm_loopAcceptUsers(void*);
THREAD_RETURN fm_loopReadFromUsers(void*);
THREAD_RETURN fm_loopDoUserLogin(void* param);
#ifdef CH_LOG_STUDY
THREAD_RETURN fm_loopLogChannelsOpened(void* param);
#endif //CH_LOG_STUDY
THREAD_RETURN	fm_loopLog(void*);

#ifdef COUNTRY_STATS
THREAD_RETURN iplist_loopDoLogCountries(void* param);
class tUINT32withLock
	{
	private:
		volatile UINT32 value;
		CAMutex lock;
	public:
		tUINT32withLock()
			{
				value=0;
			}
		void inc()
			{
				lock.lock();
				value++;
				lock.unlock();
			}

		int getAndzero()
			{
				UINT32 tmp;
				lock.lock();
				tmp=value;
				value=0;
				lock.unlock();
				return tmp;
			}
	};
#endif

class CAFirstMix:public
#ifdef REPLAY_DETECTION
	CAMixWithReplayDB
#else
	CAMix
#endif
{
public:
    CAFirstMix()
				{
					m_pmutexUser=new CAMutex();
					m_pmutexMixedPackets=new CAMutex();
					m_pmutexLoginThreads=new CAMutex();
					m_pmutexNewConnections=new CAMutex();
#ifdef CH_LOG_STUDY
					//log nr of opened channels per minute
					nrOfChOpMutex = new CAMutex();
					nrOfChThread = NULL;
					nrOfOpenedChannels = 0;
					currentOpenedChannels = 0;
					lastLogTime = 0;
#endif //CH_LOG_STUDY
					m_nMixedPackets=0;
					m_nUser=0;
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
					m_nrOfTermsAndConditionsDefs = 0;
					m_tnCDefs = NULL;
					m_nrOfTermsAndConditionsTemplates = 0;
					m_templatesOwner = NULL;
					m_tcTemplates = NULL;
					m_bIsShuttingDown=false;
#ifdef LOG_PACKET_TIMES
					m_pLogPacketStats=NULL;
#endif
#ifdef COUNTRY_STATS
					m_PacketsPerCountryIN=m_PacketsPerCountryOUT=NULL;
					m_CountryStats=NULL;
					m_mysqlCon=NULL;
					m_threadLogLoop=NULL;
#endif
					m_arMixParameters=NULL;
#ifdef DYNAMIC_MIX
					m_bBreakNeeded = false;
#endif
					TNC_REQUEST = XMLString::transcode(TNC_SREQUEST);
					TNC_CONFIRM = XMLString::transcode(TNC_CONFIRM_REQ);
					TNC_INTERRUPT =  XMLString::transcode(TNC_SINTERRUPT);
#ifdef PAYMENT
					m_pmutexLogin=new CAMutex();
#endif
				}

    	/*virtual ~CAFirstMix()
			{
				delete m_pmutexNewConnections;
				m_pmutexNewConnections = NULL;
			}*/
    	virtual ~CAFirstMix()
		{
			//clean(); // speeds up shutdown
			delete m_pmutexUser;
			m_pmutexUser = NULL;
			delete m_pmutexMixedPackets;
			m_pmutexMixedPackets = NULL;
			delete m_pmutexLoginThreads;
			m_pmutexLoginThreads = NULL;
#ifdef PAYMENT
			delete m_pmutexLogin;
#endif
		}

		tMixType getType() const
			{
				return CAMix::FIRST_MIX;
			}
#ifdef PAYMENT
		bool forceKickout(fmHashTableEntry* pHashTableEntry, const XERCES_CPP_NAMESPACE::DOMDocument *pErrDoc=NULL);
		CAMutex* getLoginMutex()
			{
				return m_pmutexLogin;
			}
#endif

#ifdef DYNAMIC_MIX
private:
			bool m_bBreakNeeded;
#endif
			SINT32 connectToNextMix(CASocketAddr* a_pAddrNext);
protected:
			virtual SINT32 loop()=0;
			bool isShuttingDown();
			SINT32 init();
			SINT32 clean();
			virtual SINT32 initOnce();
#ifdef DYNAMIC_MIX
			void stopCascade()
			{
				m_bRestart = true;
			}
#endif
    //added by ronin <ronin2@web.de>
    virtual SINT32 processKeyExchange();

		/** Initialises the MixParameters info for each mix form the \<Mixes\> element received from the second mix.*/
    SINT32 initMixParameters(DOMElement* elemMixes);


public:
		SINT32 getMixedPackets(UINT64& ppackets);
		UINT32 getNrOfUsers();
		SINT32 getLevel(SINT32* puser,SINT32* prisk,SINT32* ptraffic);

		TermsAndConditions *getTermsAndConditions(const UINT8 *opSki);
		DOMNode *getTermsAndConditionsTemplate(UINT8 *templateRefID);

		friend THREAD_RETURN fm_loopSendToMix(void*);
		friend THREAD_RETURN fm_loopReadFromMix(void*);
		friend THREAD_RETURN fm_loopAcceptUsers(void*);
		friend THREAD_RETURN fm_loopReadFromUsers(void*);
		friend THREAD_RETURN fm_loopDoUserLogin(void* param);
#ifdef CH_LOG_STUDY
		friend THREAD_RETURN fm_loopLogChannelsOpened(void* param);
#endif //CH_LOG_STUDY


		//How many mixes are in the cascade?
		SINT32 getMixCount()
			{
				return m_u32MixCount;
			}
		///Returns the ordered list of the mix parameters from the first mix to the last mix.
		tMixParameters *getMixParameters()
			{
				return m_arMixParameters;
			}

		/**Sets the parameters for the mix specified in the params.m_strMixID field. Only the values which are set are copied
			* to the stored parameters of the mixes of this cascade.
			*/
		SINT32 setMixParameters(const tMixParameters& params);

		SINT32 handleKeyInfoExtensions(DOMElement *root);
		SINT32 handleTermsAndConditionsExtension(DOMElement *extensionRoot);

#ifdef REPLAY_DETECTION
		UINT64 m_u64LastTimestampReceived;
#endif

protected:
#ifndef COUNTRY_STATS
			SINT32 incUsers()
#else
			SINT32 incUsers(LP_fmHashTableEntry pHashEntry)
#endif
				{
					m_pmutexUser->lock();
					m_nUser++;
					#ifdef COUNTRY_STATS
						pHashEntry->countryID=updateCountryStats(pHashEntry->peerIP,0,false);
					#endif
					m_pmutexUser->unlock();
					return E_SUCCESS;
				}

#ifndef COUNTRY_STATS
			SINT32 decUsers()
#else
			SINT32 decUsers(LP_fmHashTableEntry pHashEntry)
#endif
				{
					m_pmutexUser->lock();
					m_nUser--;
					#ifdef COUNTRY_STATS
						updateCountryStats(NULL,pHashEntry->countryID,true);
					#endif
					m_pmutexUser->unlock();
					return E_SUCCESS;
				}

			SINT32 incMixedPackets()
				{
					m_pmutexMixedPackets->lock();
					inc64(m_nMixedPackets);
					m_pmutexMixedPackets->unlock();
					return E_SUCCESS;
				}

			/*bool getRestart() const
				{
					return m_bRestart;
				}*/
			SINT32 doUserLogin(CAMuxSocket* pNewUSer,UINT8 perrIP[4]);

#ifdef DELAY_USERS
			SINT32 reconfigure();
#endif

protected:
			CAIPList* m_pIPList;
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
			//how many mixes are in the cascade?
			UINT32	m_u32MixCount;
			//stores the mix parameters for each mix
			tMixParameters* m_arMixParameters;

#ifdef HAVE_EPOLL

			CASocketGroupEpoll* m_psocketgroupUsersRead;
			CASocketGroupEpoll* m_psocketgroupUsersWrite;
#else

			CASocketGroup* m_psocketgroupUsersRead;
			CASocketGroup* m_psocketgroupUsersWrite;
#endif
    // moved to CAMix
    //CAInfoService* m_pInfoService;
			CAMuxSocket* m_pMuxOut;

			UINT8* m_xmlKeyInfoBuff;
			UINT16 m_xmlKeyInfoSize;

			XERCES_CPP_NAMESPACE::DOMDocument* m_docMixCascadeInfo;
			UINT64 m_nMixedPackets;
			CAASymCipher* m_pRSA;
    // moved to CAMix
    //CASignature* m_pSignature;
			CAMutex* m_pmutexUser;
			CAMutex* m_pmutexMixedPackets;
			CAMutex* m_pmutexLoginThreads;

			CAThread* m_pthreadAcceptUsers;
			CAThreadPool* m_pthreadsLogin;
			CAThread* m_pthreadSendToMix;
			CAThread* m_pthreadReadFromMix;

			UINT32 m_nrOfTermsAndConditionsDefs;
			TermsAndConditions **m_tnCDefs;
			UINT32 m_nrOfTermsAndConditionsTemplates;
			DOMNode **m_tcTemplates;
			XERCES_CPP_NAMESPACE::DOMDocument *m_templatesOwner; //owner for the TC templates stored in tcTemplates

			/* constants for the XML root tags received from the client */
			const XMLCh *TNC_REQUEST;
			const XMLCh *TNC_CONFIRM;
			const XMLCh *TNC_INTERRUPT;
#ifdef CH_LOG_STUDY
protected:
			CAMutex *nrOfChOpMutex;
			UINT32 nrOfOpenedChannels;
			UINT32 currentOpenedChannels;
			CAThread *nrOfChThread;
			time_t lastLogTime;
#endif //CH_LOG_STUDY

#ifdef COUNTRY_STATS
		private:
			SINT32 initCountryStats(char* db_host,char* db_user,char*db_passwd);
			SINT32 updateCountryStats(const UINT8 ip[4],UINT32 a_countryID,bool bRemove);
			volatile bool m_bRunLogCountries;
			volatile UINT32* m_CountryStats;
		protected:
		    SINT32 deleteCountryStats();
			tUINT32withLock* m_PacketsPerCountryIN;
			tUINT32withLock* m_PacketsPerCountryOUT;
		private:
			CAThread* m_threadLogLoop;
			MYSQL* m_mysqlCon;
			friend THREAD_RETURN iplist_loopDoLogCountries(void* param);
#endif

#ifdef REPLAY_DETECTION
		private:
			SINT32 sendReplayTimestampRequestsToAllMixes();
#endif

protected:
	bool m_bIsShuttingDown;
	friend THREAD_RETURN	fm_loopLog(void*);
	volatile bool					m_bRunLog;

#ifdef PAYMENT
	CAMutex* m_pmutexLogin;
#endif

private:
	SINT32 doUserLogin_internal(CAMuxSocket* pNewUSer,UINT8 perrIP[4]);
	SINT32 isAllowedToPassRestrictions(CASocket* pNewMuxSocket);


	/* handlerFunction for Terms And Conditions invoked during user Login */
	termsAndConditionMixAnswer_t *handleTermsAndConditionsLogin(XERCES_CPP_NAMESPACE::DOMDocument *request);

	static const UINT32 MAX_CONCURRENT_NEW_CONNECTIONS;

	volatile UINT32 m_newConnections;
	CAMutex* m_pmutexNewConnections;

	void incNewConnections()
		{
			m_pmutexNewConnections->lock();
			m_newConnections++;
			m_pmutexNewConnections->unlock();
		}

	void decNewConnections()
		{
			m_pmutexNewConnections->lock();
			m_newConnections--;
			m_pmutexNewConnections->unlock();
		}

};

#endif
#endif //ONLY_LOCAL_PROXY
