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
#include "CAMix.hpp"
#include "CAUtil.hpp"
#include "CAInfoService.hpp"
#include "CACmdLnOptions.hpp"
#include "CAStatusManager.hpp"
#include "CALibProxytest.hpp"

CAMix::CAMix()
{
    m_acceptReconfiguration = CALibProxytest::getOptions()->acceptReconfiguration();
		//m_pSignature=NULL;
		m_pMultiSignature=NULL;
		m_pInfoService=NULL;
		m_pMuxOutControlChannelDispatcher=NULL;
		m_pMuxInControlChannelDispatcher=NULL;
		m_u32KeepAliveSendInterval=0;//zero means --> do not use
		m_u32KeepAliveRecvInterval=0;//zero means --> do not use
		m_bShutDown = false;
		m_docMixCascadeInfo=NULL;
		m_bConnected = false;
#ifdef DYNAMIC_MIX
		/* LERNGRUPPE: Run by default */
		m_bLoop = true;
		m_bReconfiguring = false;
		m_bCascadeEstablished = false;
		m_bReconfigured = false;
#endif
#ifdef DATA_RETENTION_LOG
		m_pDataRetentionLog=NULL;
#endif
	}

CAMix::~CAMix()
	{
#ifdef DATA_RETENTION_LOG
		if(m_pDataRetentionLog!=NULL)
			{
				delete m_pDataRetentionLog;
				m_pDataRetentionLog=NULL;
			}
#endif

	}

SINT32 CAMix::initOnce()
	{
#ifdef DATA_RETENTION_LOG
		m_pDataRetentionLog=new CADataRetentionLog();
		CAASymCipher* pKey=NULL;
		CALibProxytest::getOptions()->getDataRetentionPublicEncryptionKey(&pKey);
		if(m_pDataRetentionLog->setPublicEncryptionKey(pKey)!=E_SUCCESS)
			return E_UNKNOWN;
		UINT8 strDir[4096];
		strDir[0]=0;
		if(CALibProxytest::getOptions()->getDataRetentionLogDir(strDir,4096)!=E_SUCCESS ||
			 m_pDataRetentionLog->setLogDir(strDir)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR,"Data Retention Error: Could not set log dir to %s!\n",strDir);
				return E_UNKNOWN;
			}
		else
			{
				CAMsg::printMsg(LOG_INFO,"Data Retention: Set log dir to: %s\n",strDir);
				CAMsg::printMsg(LOG_INFO,"Data Retention: tQueueEntry size is: %u\n",sizeof(tQueueEntry));
			}

#endif
		return E_SUCCESS;
	}

SINT32 CAMix::start()
	{
		SINT32 initStatus;

		if(initOnce()!=E_SUCCESS)
			return E_UNKNOWN;
		if(m_pMultiSignature != NULL && CALibProxytest::getOptions()->isInfoServiceEnabled())
		{
			CAMsg::printMsg(LOG_DEBUG, "CAMix start: creating InfoService object\n");
			m_pInfoService=new CAInfoService(this);

			//UINT32 opCertLength;
			//CACertificate* opCert = CALibProxytest::getOptions()->getOpCertificate();
			//CACertificate* pOwnCert=CALibProxytest::getOptions()->getOwnCertificate();
			m_pInfoService->setMultiSignature(m_pMultiSignature);
			//delete pOwnCert;
			//pOwnCert = NULL;
			UINT64 currentMillis;
			if (getcurrentTimeMillis(currentMillis) != E_SUCCESS)
			{
				currentMillis = 0;
			}
			m_pInfoService->setSerial(currentMillis);

			//delete opCert;
			//opCert = NULL;

	        bool allowReconf = CALibProxytest::getOptions()->acceptReconfiguration();
	        bool needReconf = needAutoConfig();

	        m_pInfoService->setConfiguring(allowReconf && needReconf);
			CAMsg::printMsg(LOG_DEBUG, "CAMix start: starting InfoService\n");
			m_pInfoService->start();
		}
		else
		{
			m_pInfoService = NULL;
		}
		bool allowReconf = CALibProxytest::getOptions()->acceptReconfiguration();
#ifdef DYNAMIC_MIX
		/* LERNGRUPPE: We might want to break out of this loop if the mix-type changes */
		while(m_bLoop)
#else
    	for(;;)
#endif
	    {
	    	if (m_pInfoService != NULL)
	    		m_pInfoService->setConfiguring(allowReconf && needAutoConfig());
			while(allowReconf && (needAutoConfig() || m_bReconfiguring))
			{
				CAMsg::printMsg(LOG_DEBUG, "Not configured -> sleeping\n");
	            sSleep(20);
	        }
#ifdef DYNAMIC_MIX
			// if we change the mix type, we must not enter init!
			if(!m_bLoop) goto SKIP;
#endif
			CAMsg::printMsg(LOG_DEBUG, "CAMix main: before init()\n");
			initStatus = init();
	        if(initStatus == E_SUCCESS)
	        {
	        	m_bConnected = true;
	        	CAMsg::printMsg(LOG_DEBUG, "CAMix main: init() returned success\n");
	            if(m_pInfoService != NULL)
	            {
	                m_pInfoService->setConfiguring(false);
	                if( ! m_pInfoService->isRunning())
	                {
	                	m_pInfoService->start();
	                }
	            }

	            CAMsg::printMsg(LOG_INFO, "The mix is now on-line.\n");
#ifdef DYNAMIC_MIX
							m_bReconfiguring = false;
							m_bCascadeEstablished = true;
							m_bReconfigured = false;
							if(CALibProxytest::getOptions()->isFirstMix() && CALibProxytest::getOptions()->isDynamic())
								m_pInfoService->sendCascadeHelo();
#endif
							MONITORING_FIRE_SYS_EVENT(ev_sys_enterMainLoop);
							loop();
							MONITORING_FIRE_SYS_EVENT(ev_sys_leavingMainLoop);
#ifdef DYNAMIC_MIX
							m_bCascadeEstablished = false;
							/** If the cascade breaks down, some mix might have been reconfigured. Let's have a look if there is new information */
							if(!m_bReconfiguring)
								m_pInfoService->dynamicCascadeConfiguration();
#endif
							CAMsg::printMsg(LOG_DEBUG, "CAMix main: loop() returned, maybe connection lost.\n");
	        }
	        else if (initStatus == E_SHUTDOWN)
	        {
	        	CAMsg::printMsg(LOG_DEBUG, "Mix has been stopped. Waiting for shutdown...\n");
				//break;
			}
	        else
	        {
	            CAMsg::printMsg(LOG_DEBUG, "init() failed, maybe no connection.\n");
	        }
#ifdef DYNAMIC_MIX
SKIP:
#endif
	        if(m_pInfoService != NULL)
	        {
#ifndef DYNAMIC_MIX
	            if(CALibProxytest::getOptions()->acceptReconfiguration())
#else
				// Only keep the InfoService alive if the Mix-Type doesn't change
				if(CALibProxytest::getOptions()->acceptReconfiguration() && m_bLoop)
#endif
                	m_pInfoService->setConfiguring(true);
	            /*else
				{
					CAMsg::printMsg(LOG_DEBUG, "CAMix main: stopping InfoService\n");
	                m_pInfoService->stop();
				}*/
	            // maybe Cascade information (e.g. certificate validity) will change on next connection
	            UINT64 currentMillis;
	            if (getcurrentTimeMillis(currentMillis) != E_SUCCESS)
	            {
	            	currentMillis = 0;
	            }
	            m_pInfoService->setSerial(currentMillis);
	        }
	        m_bConnected = false;
			CAMsg::printMsg(LOG_DEBUG, "CAMix main: before clean()\n");
			clean();
			CAMsg::printMsg(LOG_DEBUG, "CAMix main: after clean()\n");
#ifdef DYNAMIC_MIX
			if(m_bLoop)
#endif
			sSleep(10);
		}// Big loop....
		if(m_pInfoService != NULL)
		{
			m_pInfoService->stop();
			delete m_pInfoService;
			m_pInfoService = NULL;
		}
		m_pInfoService=NULL;
		return E_SUCCESS;
	}


/**
* This method checks if target interfaces (network adress and port of next mix)
* have been specified in the config file. A return value of true means that necessary
* information to bring the cascade online is missing and must be downloaded from the
* InfoService (if available).
* @retval false if the target interfaces are configured one way or another
* @retval true if config file contains no target interface info and InfoService is unavailable
*/
bool CAMix::needAutoConfig()
{
    bool ret = false;

    if(!CALibProxytest::getOptions()->isLastMix())
    {
        ret = true;

        // look for usable target interfaces
        for(UINT32 i=0;i<CALibProxytest::getOptions()->getTargetInterfaceCount();i++)
        {
            TargetInterface oNextMix;
            CALibProxytest::getOptions()->getTargetInterface(oNextMix,i+1);
            if(oNextMix.target_type==TARGET_MIX)
            {
                ret = false;
            }
						delete oNextMix.addr;
						oNextMix.addr = NULL;
				}

        if(!CALibProxytest::getOptions()->hasNextMixTestCertificate())
            ret = true;
    }

    if(!CALibProxytest::getOptions()->isFirstMix() && !CALibProxytest::getOptions()->hasPrevMixTestCertificate())
        ret = true;

    return ret;
}

/** This will initialize the XML Cascade Info struct @ref XMLFirstMixToInfoService that is sent to the InfoService
* in \c CAInfoService::sendCascadeHelo()
* @param mixes the \<Mixes\> element of the XML struct we received from the succeeding mix.
* @retval E_UNKNOWN if processing produces an error
* @retval E_SUCCESS otherwise
*/
SINT32 CAMix::initMixCascadeInfo(DOMElement* mixes)
{
    UINT32 count;
    m_docMixCascadeInfo=createDOMDocument();
    DOMElement* elemRoot=createDOMElement(m_docMixCascadeInfo,"MixCascade");
#ifdef LOG_DIALOG
    setDOMElementAttribute(elemRoot,"study",(UINT8*)"true");
#endif
    if(CALibProxytest::getOptions()->isFirstMix())
    {
    	UINT32 maxUsers = CALibProxytest::getOptions()->getMaxNrOfUsers();
    	if(maxUsers > 0)
    	{
    		setDOMElementAttribute(elemRoot,"maxUsers", maxUsers);
    	}
#ifdef MANIOQ
    	setDOMElementAttribute(elemRoot,"context", (UINT8*) "jondonym.business");
#else

#ifdef PAYMENT
    	setDOMElementAttribute(elemRoot,"context", (UINT8*) "jondonym.premium");
#else
    	setDOMElementAttribute(elemRoot,"context", (UINT8*) "jondonym");
#endif
#endif
    }

    UINT8 id[50];
		UINT8* cascadeID=NULL;
    CALibProxytest::getOptions()->getMixId(id,50);

    UINT8 name[255];
    m_docMixCascadeInfo->appendChild(elemRoot);
    DOMElement* elem = NULL;

    if(CALibProxytest::getOptions()->getCascadeName(name,255) == E_SUCCESS)
    {
    	elem = createDOMElement(m_docMixCascadeInfo,"Name");
    	setDOMElementValue(elem,name);
    	elemRoot->appendChild(elem);
	}
    else
    {
    	CAMsg::printMsg(LOG_ERR,"No cascade name given!\n");
    }

    elem=createDOMElement(m_docMixCascadeInfo,"Network");
    elemRoot->appendChild(elem);
    DOMElement* elemListenerInterfaces=createDOMElement(m_docMixCascadeInfo,"ListenerInterfaces");
    elem->appendChild(elemListenerInterfaces);

    DOMElement *perfTest = createDOMElement(m_docMixCascadeInfo, OPTIONS_NODE_PERFORMANCE_TEST);
    setDOMElementAttribute(perfTest,
    		OPTIONS_ATTRIBUTE_PERFTEST_ENABLED,
    		CALibProxytest::getOptions()->isPerformanceTestEnabled());
    elemRoot->appendChild(perfTest);

    for(UINT32 i=1;i<=CALibProxytest::getOptions()->getListenerInterfaceCount();i++)
    {
        CAListenerInterface* pListener=CALibProxytest::getOptions()->getListenerInterface(i);
        if(pListener->isHidden())
        {//do nothing
        }
        else if(pListener->getType()==RAW_TCP)
        {
            DOMElement* elemTmpLI=NULL;
            pListener->toDOMElement(elemTmpLI,m_docMixCascadeInfo);
            elemListenerInterfaces->appendChild(elemTmpLI);
        }
        delete pListener;
        pListener = NULL;
    }

    DOMNode* elemMixesDocCascade=createDOMElement(m_docMixCascadeInfo,"Mixes");
    DOMElement* elemMix=NULL;
    count=1;
    if(CALibProxytest::getOptions()->isFirstMix())
    {
    	addMixInfo(elemMixesDocCascade, false);
		getDOMChildByName(elemMixesDocCascade, "Mix", elemMix, false);
    	// create signature
		if (signXML(elemMix) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo sent to users...\n");
		}
    	//if(m_pSignature->signXML(docMixInfo,m_pcertstoreOwnCerts)!=E_SUCCESS)
		//m_pSignature, CALibProxytest::getOptions()->getOwnCertificate()
		/*
        elemMixesDocCascade.appendChild(elemThisMix);*/
    }
    elemRoot->appendChild(elemMixesDocCascade);

//    UINT8 cascadeId[255];
//		UINT32 cascadeIdLen=255;

    DOMNode* node=mixes->getFirstChild();
    while(node!=NULL)
    {
        if(node->getNodeType()==DOMNode::ELEMENT_NODE&&equals(node->getNodeName(),"Mix"))
        {
            elemMixesDocCascade->appendChild(m_docMixCascadeInfo->importNode(node,true));
            count++;
 //           cascadeId = static_cast<const DOM_Element&>(node).getAttribute("id").transcode();
        }
        node=node->getNextSibling();
    }

    if(CALibProxytest::getOptions()->isLastMix())
    {
      addMixInfo(elemMixesDocCascade, false);
			getLastDOMChildByName(elemMixesDocCascade, "Mix", elemMix);
    	// create signature
		if (signXML(elemMix) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo sent to users...\n");
		}
        cascadeID = id;
    }
		else if(CALibProxytest::getOptions()->isFirstMix())
    {
        cascadeID = id;
    }

    if(cascadeID != NULL)
				setDOMElementAttribute(elemRoot,"id",cascadeID);
    setDOMElementAttribute(elemMixesDocCascade,"count",count);

  DOMNode* elemPayment=createDOMElement(m_docMixCascadeInfo,"Payment");
	elemRoot->appendChild(elemPayment);
#ifdef PAYMENT
	setDOMElementAttribute(elemPayment,"required",(UINT8*)"true");
	setDOMElementAttribute(elemPayment,"version",(UINT8*)PAYMENT_VERSION);
	setDOMElementAttribute(elemPayment,"prepaidInterval", CALibProxytest::getOptions()->getPrepaidInterval());
	setDOMElementAttribute(elemPayment,"piid", CALibProxytest::getOptions()->getBI()->getID());
#else
	setDOMElementAttribute(elemPayment,"required",(UINT8*)"false");
#endif
		return E_SUCCESS;
}

SINT32 CAMix::addMixInfo(DOMNode* a_element, bool a_bForceFirstNode)
{
	// this is a complete mixinfo node to be sent to the InfoService
	XERCES_CPP_NAMESPACE::DOMDocument* docMixInfo=NULL;
	if(CALibProxytest::getOptions()->getMixXml(docMixInfo)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	DOMNode* nodeMixInfo = a_element->getOwnerDocument()->importNode(
		docMixInfo->getDocumentElement(), true);
	if (a_bForceFirstNode && a_element->hasChildNodes())
	{
		a_element->insertBefore(nodeMixInfo, a_element->getFirstChild());
	}
	else
	{
		a_element->appendChild(nodeMixInfo);
	}
	return E_SUCCESS;
}

DOMNode *CAMix::appendTermsAndConditionsExtension(XERCES_CPP_NAMESPACE::DOMDocument *ownerDoc,
		DOMElement *root)
{
	if(CALibProxytest::getOptions()->getTermsAndConditions() != NULL)
	{
		if( (root == NULL) || (ownerDoc == NULL) )
		{
			return NULL;
		}

		DOMElement *elemTnCExtension = NULL, *elemExtensions = NULL,
					*elemTemplates = NULL;
		/* Tag for Nodes that can be removed without destroying the signature.
		 * To be appended to the "mixes"-node so older mix versions won't get confused.
		 */
		getDOMChildByName(root, KEYINFO_NODE_EXTENSIONS, elemExtensions);
		if(elemExtensions == NULL)
		{
			elemExtensions = createDOMElement(ownerDoc, KEYINFO_NODE_EXTENSIONS);
			root->appendChild(elemExtensions);
		}
		else
		{
			getDOMChildByName(elemExtensions, KEYINFO_NODE_TNC_EXTENSION, elemTnCExtension);
		}

		if(elemTnCExtension == NULL)
		{
			elemTnCExtension = createDOMElement(ownerDoc, KEYINFO_NODE_TNC_EXTENSION);
			elemExtensions->appendChild(elemTnCExtension);
		}

		//First add templates if there are any
		UINT32 nrOfTemplates = CALibProxytest::getOptions()->getNumberOfTermsAndConditionsTemplates();

		if(nrOfTemplates > 0)
		{
			UINT32 nrOfSentTemplates = 0;
			UINT8 **sentTemplatesRefIds = NULL;
			getDOMChildByName(elemTnCExtension, OPTIONS_NODE_TNCS_TEMPLATES, elemTemplates);
			if(elemTemplates == NULL)
			{
				elemTemplates = createDOMElement(ownerDoc, OPTIONS_NODE_TNCS_TEMPLATES);
				elemTnCExtension->appendChild(elemTemplates);
			}
			else
			{
				DOMNodeList *nl = getElementsByTagName(elemTemplates, "TermsAndConditionsTemplate");
				nrOfSentTemplates = nl->getLength();
				if(nrOfSentTemplates > 0)
				{
					sentTemplatesRefIds = new UINT8*[nrOfSentTemplates];
					for (UINT32 i = 0; i < nrOfSentTemplates; i++)
					{
						sentTemplatesRefIds[i] = getTermsAndConditionsTemplateRefId(nl->item(i));
					}
				}
			}

			XERCES_CPP_NAMESPACE::DOMDocument **allTemplates = CALibProxytest::getOptions()->getAllTermsAndConditionsTemplates();
			UINT8 *currentTemplateRefId = NULL;
			bool duplicate = false;
			for(UINT32 i = 0; i < nrOfTemplates; i++)
			{
				currentTemplateRefId =
					getTermsAndConditionsTemplateRefId(allTemplates[i]->getDocumentElement());
				duplicate = false;
				if(currentTemplateRefId != NULL)
				{
					for(UINT32 j=0; j < nrOfSentTemplates; j++)
					{
						if(strncmp((char *)currentTemplateRefId, (char *)sentTemplatesRefIds[j], TEMPLATE_REFID_MAXLEN) == 0)
						{
							duplicate = true;
							break;
						}
					}
					if(!duplicate)
					{
						//TODO: avoid duplicates.
						elemTemplates->appendChild(ownerDoc->importNode(
								allTemplates[i]->getDocumentElement(), true));
						CAMsg::printMsg(LOG_DEBUG,"appended a tc template node!\n");
					}
					else
					{
						CAMsg::printMsg(LOG_DEBUG,"template '%s' already sent.\n", currentTemplateRefId);
					}
					delete [] currentTemplateRefId;
					currentTemplateRefId = NULL;
				}
			}

			for(UINT32 i = 0; i < nrOfSentTemplates; i++)
			{
				delete [] sentTemplatesRefIds[i];
				sentTemplatesRefIds[i] = NULL;
			}
			delete [] sentTemplatesRefIds;
			sentTemplatesRefIds = NULL;
		}

		DOMNode* elemTnCs = ownerDoc->importNode(CALibProxytest::getOptions()->getTermsAndConditions(), true);
		UINT8 tmpOpSKIBuff[TMP_BUFF_SIZE];
		UINT32 tmpOpSKILen = TMP_BUFF_SIZE;
		memset(tmpOpSKIBuff, 0, tmpOpSKILen);
		CALibProxytest::getOptions()->getOperatorSubjectKeyIdentifier(tmpOpSKIBuff, &tmpOpSKILen);
		setDOMElementAttribute(elemTnCs, OPTIONS_ATTRIBUTE_TNC_ID, tmpOpSKIBuff);

		DOMNodeList *tncDefEntryList = getElementsByTagName((DOMElement *)elemTnCs, OPTIONS_NODE_TNCS_TRANSLATION);
		for (XMLSize_t i = 0; i < tncDefEntryList->getLength(); i++)
		{
			//TODO don't know if to include certs here
			m_pMultiSignature->signXML((DOMElement *)tncDefEntryList->item(i), false);
		}
		elemTnCExtension->appendChild(elemTnCs);
		return elemTnCExtension;
	}
	else
	{
		return NULL;
	}
}

DOMNode *CAMix::termsAndConditionsInfoNode(XERCES_CPP_NAMESPACE::DOMDocument *ownerDoc)
{
	if(CALibProxytest::getOptions()->getTermsAndConditions() != NULL)
	{
		DOMElement *elemTnCInfos = createDOMElement(ownerDoc, KEYINFO_NODE_TNC_INFOS);
		UINT8 tmpBuff[TMP_BUFF_SIZE];
		UINT32 tmpLen = TMP_BUFF_SIZE;
		memset(tmpBuff, 0, tmpLen);

		DOMNodeList *list = getElementsByTagName(CALibProxytest::getOptions()->getTermsAndConditions(), OPTIONS_NODE_TNCS_TRANSLATION);
		DOMElement *iterator = NULL;
		DOMElement *currentInfoNode = NULL;
		bool defaultLangDefined = false;
		for (XMLSize_t i = 0; i < list->getLength(); i++)
		{
			iterator = (DOMElement *) list->item(i);
			currentInfoNode = createDOMElement(ownerDoc, KEYINFO_NODE_TNC_INFO);

			getDOMElementAttribute(iterator, OPTIONS_ATTRIBUTE_TNC_LOCALE, tmpBuff, &tmpLen);
			setDOMElementAttribute(currentInfoNode, OPTIONS_ATTRIBUTE_TNC_LOCALE, tmpBuff);
			getDOMElementAttribute(iterator, OPTIONS_ATTRIBUTE_TNC_DEFAULT_LANG_DEFINED, defaultLangDefined);
			if(defaultLangDefined)
			{
				setDOMElementAttribute(elemTnCInfos, OPTIONS_ATTRIBUTE_TNC_DEFAULT_LANG, tmpBuff);
			}
			defaultLangDefined = false;
			tmpLen = TMP_BUFF_SIZE;

			getDOMElementAttribute(iterator, OPTIONS_ATTRIBUTE_TNC_TEMPLATE_REFID, tmpBuff, &tmpLen);
			setDOMElementAttribute(currentInfoNode, OPTIONS_ATTRIBUTE_TNC_TEMPLATE_REFID, tmpBuff);
			tmpLen = TMP_BUFF_SIZE;

			elemTnCInfos->appendChild(currentInfoNode);
		}

		getDOMElementAttribute(CALibProxytest::getOptions()->getTermsAndConditions(), OPTIONS_ATTRIBUTE_TNC_DATE, tmpBuff, &tmpLen);
		setDOMElementAttribute(elemTnCInfos, OPTIONS_ATTRIBUTE_TNC_DATE, tmpBuff);

		tmpLen = TMP_BUFF_SIZE;
		//memset(tmpBuff, 0, tmpLen);

		CALibProxytest::getOptions()->getOperatorSubjectKeyIdentifier(tmpBuff, &tmpLen);
		setDOMElementAttribute(elemTnCInfos, OPTIONS_ATTRIBUTE_TNC_ID, tmpBuff);
		return elemTnCInfos;
	}
	else
	{
		return NULL;
	}

}

SINT32 CAMix::signXML(DOMNode* a_element)
	{
		return m_pMultiSignature->signXML(a_element, true);
    /*CACertStore* tmpCertStore=new CACertStore();

    CACertificate* ownCert=CALibProxytest::getOptions()->getOwnCertificate();
    if(ownCert==NULL)
			{
        CAMsg::printMsg(LOG_DEBUG,"Own Test Cert is NULL!\n");
			}

    // Operator Certificates
    CACertificate* opCert = CALibProxytest::getOptions()->getOpCertificate();
    if(opCert==NULL)
	{
        CAMsg::printMsg(LOG_DEBUG,"Op Test Cert is NULL!\n");
	}
	else
	{
		// Own  Mix Certificates first, then Operator Certificates
		tmpCertStore->add(opCert);
	}
    tmpCertStore->add(ownCert);

    if(m_pSignature->signXML(a_element, tmpCertStore)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}

	CASignature* test = new CASignature();
	test->setVerifyKey(ownCert);

	if(test->verifyXML(a_element) != E_SUCCESS)
	{
		CAMsg::printMsg(LOG_DEBUG, "Error verifying own Signature!!\n");
		m_pSignature->signXML(a_element, tmpCertStore);
	}
	else
	{
		CAMsg::printMsg(LOG_DEBUG, "Own Signature looks ok!\n");
	}


    delete ownCert;
    ownCert = NULL;

	delete opCert;
	opCert = NULL;

    delete tmpCertStore;
    tmpCertStore = NULL;

    return E_SUCCESS;*/
}
#ifdef DYNAMIC_MIX
/**
 * LERNGRUPPE
  * This method does the following:
 * 1) Set m_bLoop = false to break the main loop, if a_bChangeMixType is true
 * 2) Disconnect the cascade
 * 3) Break the mix out of init if it is stuck there
 * The new configuration options should already be set in the CACmdLnOptions of this mix
 *
 * @param a_bChangeMixType Indicates if this mix becomes another mix type (eg. MiddleMix -> FirstMix)
 * @retval E_SUCCESS
 */
SINT32 CAMix::dynaReconfigure(bool a_bChangeMixType)
{
	SINT32 ret = E_UNKNOWN;
	CASocket tmpSock;
	const CASocketAddr* pAddr=NULL;

	m_bReconfigured = true;
	m_bLoop = !a_bChangeMixType;

	if(m_bCascadeEstablished)
		stopCascade();
	m_docMixCascadeInfo = NULL;

	/** @todo Break a middle mix out of its init. That doesn't look really nice, maybe there is a better way to do this?
   	*/
	CAListenerInterface* pListener=NULL;
	UINT32 interfaces=CALibProxytest::getOptions()->getListenerInterfaceCount();
	for(UINT32 i=1;i<=interfaces;i++)
	{
		pListener=CALibProxytest::getOptions()->getListenerInterface(i);
		if(!pListener->isVirtual())
			break;
		delete pListener;
		pListener=NULL;
	}
	if(pListener==NULL)
	{
		CAMsg::printMsg(LOG_CRIT," failed!\n");
		CAMsg::printMsg(LOG_CRIT,"Reason: no useable (non virtual) interface found!\n");
		goto EXIT;
	}

	pAddr=pListener->getAddr();
	delete pListener;
	pListener = NULL;

	tmpSock.connect(*pAddr);
	delete pAddr;
	pAddr = NULL;
	tmpSock.close();

	ret = E_SUCCESS;
EXIT:
	CAMsg::printMsg(LOG_DEBUG, "Mix has been reconfigured!\n");
	m_bReconfiguring = false;
	return ret;
}
#endif //DYNAMIC_MIX
#endif //ONLY_LOCAL_PROXY
