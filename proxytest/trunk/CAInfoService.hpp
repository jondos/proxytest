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
#ifndef __CAINFOSERVICE__
#define __CAINFOSERVICE__
#if !defined ONLY_LOCAL_PROXY
//#include "CAMultiSignature.hpp"

#include "CAFirstMix.hpp"
#include "CAXMLBI.hpp"

#endif

#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX
#include "CAThread.hpp"
#include "CAMutex.hpp"
#include "CACmdLnOptions.hpp"

#define NR_REQUEST_TYPES 2
#define REQUEST_TYPE_POST 0
#define REQUEST_TYPE_GET	1

#define NR_REQUEST_COMMANDS 7
#define REQUEST_COMMAND_CONFIGURE	0
#define REQUEST_COMMAND_HELO		1
#define REQUEST_COMMAND_MIXINFO		2
#define REQUEST_COMMAND_DYNACASCADE	3
#define REQUEST_COMMAND_CASCADE		4
#define REQUEST_COMMAND_STATUS		5
#define REQUEST_COMMAND_TNC_DATA	6

#define ATTRIBUTE_SERIAL "serial"

#define LOCALE_DEFAULT LOCALE_EN
#define LOCALE_EN "en"

class CAInfoService
	{
		#ifdef PAYMENT
			SINT32 getPaymentInstance(const UINT8* a_pstrPIID,CAXMLBI** pXMLBI,
										CASocketAddrINet* a_socketAddress);
		#endif

		public:
			CAInfoService();
			CAInfoService(CAMix* pMix);
			~CAInfoService();
			SINT32 sendMixHelo(SINT32 requestCommand=-1,const UINT8* param=NULL);
			SINT32 sendStatus(bool bIncludeCerts);
			bool isRunning()
				{
					return m_bRun;
				}

			void setConfiguring(bool a_configuring)
			{
					m_bConfiguring = a_configuring;
			}
		// added by ronin <ronin2@web.de>
			bool isConfiguring()
			{
					return m_bConfiguring;
			}
			void setSerial(UINT64 a_serial)
			{
				m_serial = a_serial;
			}
			SINT32 start();
			SINT32 stop();
			SINT32 signal();
			SINT32 setMultiSignature(CAMultiSignature* pMultiSignature);

#if !defined ONLY_LOCAL_PROXY
			//SINT32 sendMixInfo(const UINT8* pMixID);
			SINT32 sendCascadeHelo();
#ifdef PAYMENT
			SINT32 getPaymentInstance(const UINT8* a_pstrPIID,CAXMLBI** pXMLBI);
#endif
	
			//remove me
			/*SINT32 setSignature(CASignature* pSignature, CACertificate* a_ownCert,
								CACertificate* a_opCert);*/

	




#ifdef DYNAMIC_MIX
			/** LERNGRUPPE */
			SINT32 dynamicCascadeConfiguration();
			bool newCascadeAvailable();
#endif

		private:
			static THREAD_RETURN TCascadeHelo(void *p);
			UINT8* getCascadeHeloXMLAsString(UINT32& len);
			SINT32 sendCascadeHelo(const UINT8* xml,UINT32 len,const CASocketAddrINet* a_socketAddress) const;

			UINT8 **getOperatorTnCsAsStrings(UINT32 **lengths, UINT32 *nrOfTnCs);
			SINT32 sendOperatorTnCData();


			// added by ronin <ronin2@web.de>
			SINT32 handleConfigEvent(XERCES_CPP_NAMESPACE::DOMDocument* doc) const;

			//CASignature*		m_pSignature;
#ifdef DYNAMIC_MIX
			bool m_bReconfig;
#endif
#endif
			private:
				SINT32 sendMixHelo(const UINT8* strMixHeloXML,UINT32 len,SINT32 requestCommand,const UINT8* param,
								const CASocketAddrINet* a_socketAddress);
				UINT8 *getMixHeloXMLAsString(UINT32& len);
				UINT8 *xmlDocToStringWithSignature(DOMNode *a_node, UINT32& a_len, bool bIncludeCerts);
				SINT32 sendHelo(UINT8* a_strXML, UINT32 a_len, THREAD_RETURN (*a_thread)(void *), UINT8* a_strThreadName, SINT32 requestCommand, const UINT8* param = NULL);
				SINT32 getLevel(SINT32* puser,SINT32* prisk,SINT32* ptraffic);
				SINT32 getMixedPackets(UINT64& ppackets);
				UINT8* getStatusXMLAsString(bool bIncludeCerts,UINT32& len);
				SINT32 sendStatus(const UINT8* strStatusXML,UINT32 len,const CASocketAddrINet* a_socketAddress) const;
				static THREAD_RETURN TCascadeStatus(void *p);
				static THREAD_RETURN TMixHelo(void *p);
				static THREAD_RETURN InfoLoop(void *p);
				volatile bool m_bRun;
				UINT64				m_serial;
				bool					m_bConfiguring;
				CAMix*				m_pMix;
				CAConditionVariable *	m_pLoopCV;
				CAMultiSignature* 	m_pMultiSignature;
				UINT64				m_lastMixedPackets;
				UINT32				m_minuts;
				SINT32				m_expectedMixRelPos;
				CAThread*			m_pthreadRunLoop;
				struct InfoServiceHeloMsg;

		public:
			static const UINT64 MINUTE;
			static const UINT64 SEND_LOOP_SLEEP;
			static const UINT64 SEND_CASCADE_INFO_WAIT;
			static const UINT64 SEND_MIX_INFO_WAIT;
			static const UINT64 SEND_STATUS_INFO_WAIT;
			static const UINT32 SEND_INFO_TIMEOUT_MS;
			static const UINT32 REPEAT_ON_STATUS_SENT_ERROR;	
};
#endif
#endif //ONLY_LOCAL_PROXY
