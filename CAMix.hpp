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
#ifndef __CAMIX__
#define __CAMIX__

#ifndef ONLY_LOCAL_PROXY

class CAMultiSignature;
class CAInfoService;
//class DOM_Element;
class CAControlChannelDispatcher;

#ifdef DATA_RETENTION_LOG
	#include "CADataRetentionLog.hpp"
#endif

#define KEYINFO_NODE_TNC_INFOS "TermsAndConditionsInfos"
#define KEYINFO_NODE_TNC_INFO "TermsAndConditionsInfo"

#define KEYINFO_NODE_EXTENSIONS "Extensions"
#define KEYINFO_NODE_TNC_EXTENSION "TermsAndConditionsExtension"

class CAMix
	{
		public:
			enum tMixType
				{
					FIRST_MIX,
					MIDDLE_MIX,
					LAST_MIX,
					JAP
				};

		public:
			CAMix();
			virtual ~CAMix();
			SINT32 start();
			virtual SINT32 reconfigure(){return E_SUCCESS;}
			virtual tMixType getType() const =0;

			virtual void shutDown()
			{
				m_bShutDown = true;
			}

			virtual bool isShutDown()
			{
				return m_bShutDown;
			}

#ifdef DYNAMIC_MIX
			void setReconfiguring(bool a_val)
			{
				m_bReconfiguring = a_val;
			}
			SINT32 dynaReconfigure(bool a_bChangeMixType);
#endif

			/** Returns the Mix-Cascade info which should be send to the InfoService.
    		* This is NOT a copy!
    		*
    		* @param docMixCascadeInfo where the XML struct would be stored
    		* @retval E_SUCCESS
    		*/
			SINT32 getMixCascadeInfo(XERCES_CPP_NAMESPACE::DOMDocument* & docMixCascadeInfo)
			{
					if(m_docMixCascadeInfo != NULL)
					{
							docMixCascadeInfo=m_docMixCascadeInfo;
							return E_SUCCESS;
					}
					else
					{
							return E_UNKNOWN;
					}
			}

			// added by ronin <ronin2@web.de>
			bool acceptsReconfiguration()
			{
					return m_acceptReconfiguration;
			}

			CAControlChannelDispatcher* getDownstreamControlChannelDispatcher() const
				{
					return m_pMuxInControlChannelDispatcher;
				}

			CAControlChannelDispatcher* getUpstreamControlChannelDispatcher() const
				{
					return m_pMuxOutControlChannelDispatcher;
				}
			
			UINT32 getLastConnectionTime()
			{
				m_lLastConnectionTime;
			}
				
			bool isConnected()
			{
				return m_bConnected;
			}
		protected:
#ifdef DYNAMIC_MIX
			virtual void stopCascade() =0;
			bool m_bLoop;
			bool m_bCascadeEstablished;
			bool m_bReconfigured;
#endif
			bool m_bReconfiguring;
			volatile bool m_bShutDown;
			virtual SINT32 clean()=0;
			virtual SINT32 initOnce();
			virtual SINT32 init()=0;
			virtual SINT32 loop()=0;

			SINT32 checkCompatibility(DOMNode* a_parent, const char* a_mixPosition);
			SINT32 appendCompatibilityInfo(DOMNode* a_parent);
			SINT32 addMixInfo(DOMNode* a_element, bool a_bForceFirstNode);

			/**
			 * convenience function: returns an already prepared and signed TermsAndCondition node
			 * which can be appended as a KeyInfoExtension.
			 * if elemTnCExtensions is null it will be created by this function.
			 * returns the TNCExtension root element.
			 */
			DOMNode *appendTermsAndConditionsExtension(XERCES_CPP_NAMESPACE::DOMDocument *doc,
					DOMElement *root);

			/**
			 * convenience function: creates a node for the KeyInfo structure
			 * for indicating the clients which Terms And Conditions are valid.
			 */
			DOMNode *termsAndConditionsInfoNode(XERCES_CPP_NAMESPACE::DOMDocument *ownerDoc);

			// added by ronin <ronin2@web.de>
			virtual SINT32 processKeyExchange()
			{
					return E_SUCCESS;
			}

			// added by ronin <ronin2@web.de>
			virtual SINT32 initMixCascadeInfo(DOMElement* elemMixes);

			SINT32 signXML(DOMNode* a_element);

			//CASignature* m_pSignature;
			CAMultiSignature* m_pMultiSignature;
			CAInfoService* m_pInfoService;

			UINT32 m_u32KeepAliveRecvInterval;
			UINT32 m_u32KeepAliveSendInterval;

			bool m_acceptReconfiguration;
			volatile bool m_bConnected;
			volatile UINT32 m_lLastConnectionTime;
			// added by ronin <ronin2@web.de>
			XERCES_CPP_NAMESPACE::DOMDocument* m_docMixCascadeInfo;

			CAControlChannelDispatcher* m_pMuxOutControlChannelDispatcher;
			CAControlChannelDispatcher* m_pMuxInControlChannelDispatcher;

#ifdef DATA_RETENTION_LOG
			CADataRetentionLog* m_pDataRetentionLog;
#endif

		private:
			// added by ronin <ronin2@web.de>
			bool needAutoConfig();
	};
#endif
#endif //ONLY_LOCAL_PROXY
