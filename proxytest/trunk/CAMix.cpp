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

extern CACmdLnOptions* pglobalOptions;

 
CAMix::CAMix()
{
    m_acceptReconfiguration = pglobalOptions->acceptReconfiguration();
		m_pSignature=NULL;
		m_pInfoService=NULL;
		#ifdef REPLAY_DETECTION
			m_pReplayMsgProc=NULL;
		#endif
		m_pMuxOutControlChannelDispatcher=NULL;
		m_pMuxInControlChannelDispatcher=NULL;
		m_u32KeepAliveSendInterval=0;//zero means --> do not use
		m_u32KeepAliveRecvInterval=0;//zero means --> do not use
		m_bShutDown = false; 
#ifdef DYNAMIC_MIX
		/* LERNGRUPPE: Run by default */
		m_bLoop = true;
		m_bReconfiguring = false;
		m_bCascadeEstablished = false;
		m_bReconfigured = false;
#endif
	}

SINT32 CAMix::start()
	{
		SINT32 initStatus;
		
		if(initOnce()!=E_SUCCESS)
			return E_UNKNOWN;
		if(m_pSignature != NULL && pglobalOptions->isInfoServiceEnabled())
		{
			CAMsg::printMsg(LOG_DEBUG, "CAMix start: creating InfoService object\n");
			m_pInfoService=new CAInfoService(this);
        
			UINT32 opCertLength;
			CACertificate** opCerts = pglobalOptions->getOpCertificates(opCertLength);
			CACertificate* pOwnCert=pglobalOptions->getOwnCertificate();
			m_pInfoService->setSignature(m_pSignature, pOwnCert, opCerts, opCertLength);
			delete pOwnCert;
			UINT64 currentMillis;
			if (getcurrentTimeMillis(currentMillis) != E_SUCCESS)
			{
				currentMillis = 0;
			}
			m_pInfoService->setSerial(currentMillis);
			
			if(opCerts!=NULL)
				{
					for(UINT32 i=0;i<opCertLength;i++)
						delete opCerts[i];
					delete[] opCerts;
				}
	
	        bool allowReconf = pglobalOptions->acceptReconfiguration();
	        bool needReconf = needAutoConfig();
	
	        m_pInfoService->setConfiguring(allowReconf && needReconf);
					CAMsg::printMsg(LOG_DEBUG, "CAMix start: starting InfoService\n");
	        m_pInfoService->start();
// 	LERNGRUPPE: Moved this loop to the beginning of the main loop to allow reconfiguration
//         while(allowReconf && needReconf)
//         {
//             CAMsg::printMsg(LOG_INFO, "No next mix or no prev. mix certificate specified. Waiting for auto-config from InfoService.\n");
// 
//             sSleep(20);
//             needReconf = needAutoConfig();
//         }
        }
		else
		{
			m_pInfoService = NULL;
		}
		bool allowReconf = pglobalOptions->acceptReconfiguration();
//     bool needReconf = needAutoConfig();
#ifdef DYNAMIC_MIX
		/* LERNGRUPPE: We might want to break out of this loop if the mix-type changes */
		while(m_bLoop)
#else
    while(true)
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
					CAMsg::printMsg(LOG_DEBUG, "CAMix main: init() returned success\n");
            if(m_pInfoService != NULL)
            {
                m_pInfoService->setConfiguring(false);
                if( ! m_pInfoService->isRunning())
                    m_pInfoService->start();
            }

            CAMsg::printMsg(LOG_INFO, "The mix is now on-line.\n");
#ifdef DYNAMIC_MIX
						m_bReconfiguring = false;
						m_bCascadeEstablished = true;
						m_bReconfigured = false;
						if(pglobalOptions->isFirstMix() && pglobalOptions->isDynamic())
							m_pInfoService->sendCascadeHelo();
#endif
						loop();
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
            if(pglobalOptions->acceptReconfiguration())
#else
			// Only keep the InfoService alive if the Mix-Type doesn't change
			if(pglobalOptions->acceptReconfiguration() && m_bLoop)
#endif
                m_pInfoService->setConfiguring(true);
            else
			{
				CAMsg::printMsg(LOG_DEBUG, "CAMix main: stopping InfoService\n");
                m_pInfoService->stop();
			}
            // maybe Cascade information (e.g. certificate validity) will change on next connection
            UINT64 currentMillis;
            if (getcurrentTimeMillis(currentMillis) != E_SUCCESS)
            {
            	currentMillis = 0;
            }
            m_pInfoService->setSerial(currentMillis);            
        }
		CAMsg::printMsg(LOG_DEBUG, "CAMix main: before clean()\n");
		clean();
		CAMsg::printMsg(LOG_DEBUG, "CAMix main: after clean()\n");
#ifdef DYNAMIC_MIX
		if(m_bLoop)
#endif
		sSleep(10);
    }
#ifdef DYNAMIC_MIX
	if(m_pInfoService != NULL && m_pInfoService->isRunning())
	{
		m_pInfoService->stop();
		delete m_pInfoService;
    }
	return E_SUCCESS;
#endif
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

    if(!pglobalOptions->isLastMix())
    {
        ret = true;

        // look for usable target interfaces
        for(UINT32 i=0;i<pglobalOptions->getTargetInterfaceCount();i++)
        {
            TargetInterface oNextMix;
            pglobalOptions->getTargetInterface(oNextMix,i+1);
            if(oNextMix.target_type==TARGET_MIX)
            {
                ret = false;
            }
						delete oNextMix.addr;
				}

        if(!pglobalOptions->hasNextMixTestCertificate())
            ret = true;
    }

    if(!pglobalOptions->isFirstMix() && !pglobalOptions->hasPrevMixTestCertificate())
        ret = true;

    return ret;
}

/** This will initialize the XML Cascade Info struct @ref XMLFirstMixToInfoService that is sent to the InfoService
* in \c CAInfoService::sendCascadeHelo()
* @param mixes the \<Mixes\> element of the XML struct we received from the succeeding mix.
* @retval E_UNKNOWN if processing produces an error
* @retval E_SUCCESS otherwise
*/
SINT32 CAMix::initMixCascadeInfo(DOM_Element& mixes)
{
    int count;
    m_docMixCascadeInfo=DOM_Document::createDocument();
    DOM_Element elemRoot=m_docMixCascadeInfo.createElement("MixCascade");

    UINT8 id[50];
		UINT8* cascadeID=NULL;
    pglobalOptions->getMixId(id,50);

    UINT8 name[255];
    if(pglobalOptions->getCascadeName(name,255)!=E_SUCCESS)
    {
    	CAMsg::printMsg(LOG_ERR,"No cascade name given!\n");
			return E_UNKNOWN;
		}
    m_docMixCascadeInfo.appendChild(elemRoot);
    DOM_Element elem=m_docMixCascadeInfo.createElement("Name");
		setDOMElementValue(elem,name);
    elemRoot.appendChild(elem);

    elem=m_docMixCascadeInfo.createElement("Network");
    elemRoot.appendChild(elem);
    DOM_Element elemListenerInterfaces=m_docMixCascadeInfo.createElement("ListenerInterfaces");
    elem.appendChild(elemListenerInterfaces);

    for(UINT32 i=1;i<=pglobalOptions->getListenerInterfaceCount();i++)
    {
        CAListenerInterface* pListener=pglobalOptions->getListenerInterface(i);
        if(pListener->isHidden())
        {//do nothing
        }
        else if(pListener->getType()==RAW_TCP)
        {
            DOM_DocumentFragment docFrag;
            pListener->toDOMFragment(docFrag,m_docMixCascadeInfo);
            elemListenerInterfaces.appendChild(docFrag);
        }
        delete pListener;
    }	
    
    DOM_Node elemMixesDocCascade=m_docMixCascadeInfo.createElement("Mixes");
    DOM_Element elemMix;
    count=1;
    if(pglobalOptions->isFirstMix())
    {
    	addMixInfo(elemMixesDocCascade, false);
		getDOMChildByName(elemMixesDocCascade, (UINT8*)"Mix", elemMix, false);
    	// create signature
		if (signXML(elemMix) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo sent to users...\n");
		}
    	//if(m_pSignature->signXML(docMixInfo,m_pcertstoreOwnCerts)!=E_SUCCESS)
		//m_pSignature, pglobalOptions->getOwnCertificate()    	
		/*
        elemMixesDocCascade.appendChild(elemThisMix);*/
    }
    elemRoot.appendChild(elemMixesDocCascade);

//    UINT8 cascadeId[255];
//		UINT32 cascadeIdLen=255;

    DOM_Node node=mixes.getFirstChild();
    while(node!=NULL)
    {
        if(node.getNodeType()==DOM_Node::ELEMENT_NODE&&node.getNodeName().equals("Mix"))
        {
            elemMixesDocCascade.appendChild(m_docMixCascadeInfo.importNode(node,true));
            count++;
 //           cascadeId = static_cast<const DOM_Element&>(node).getAttribute("id").transcode();
        }
        node=node.getNextSibling();
    }

    if(pglobalOptions->isLastMix())
    {
        addMixInfo(elemMixesDocCascade, false);
		getLastDOMChildByName(elemMixesDocCascade, (UINT8*)"Mix", elemMix);
    	// create signature
		if (signXML(elemMix) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG,"Could not sign KeyInfo sent to users...\n");
		}
        cascadeID = id;
    }
		else if(pglobalOptions->isFirstMix())
    {
        cascadeID = id;
    }

    if(cascadeID != NULL)
				setDOMElementAttribute(elemRoot,"id",cascadeID);
    setDOMElementAttribute(elemMixesDocCascade,"count",count);
    
		
  DOM_Node elemPayment=m_docMixCascadeInfo.createElement("Payment");
	elemRoot.appendChild(elemPayment);
#ifdef PAYMENT
	setDOMElementAttribute(elemPayment,"required",(UINT8*)"true");
	setDOMElementAttribute(elemPayment,"version",(UINT8*)PAYMENT_VERSION);
	UINT32 prepaidInterval;
	pglobalOptions->getPrepaidInterval(&prepaidInterval);
	setDOMElementAttribute(elemPayment,"prepaidInterval", prepaidInterval);
	setDOMElementAttribute(elemPayment,"piid", pglobalOptions->getBI()->getID());
#else
	setDOMElementAttribute(elemPayment,"required",(UINT8*)"false");
#endif
		return E_SUCCESS;
}

SINT32 CAMix::addMixInfo(DOM_Node& a_element, bool a_bForceFirstNode)
{
	// this is a complete mixinfo node to be sent to the InfoService
	DOM_Document docMixInfo;
	if(pglobalOptions->getMixXml(docMixInfo)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	DOM_Node nodeMixInfo = a_element.getOwnerDocument().importNode(
		docMixInfo.getDocumentElement(), true);
	if (a_bForceFirstNode && a_element.hasChildNodes())
	{
		a_element.insertBefore(nodeMixInfo, a_element.getFirstChild());
	}
	else
	{
		a_element.appendChild(nodeMixInfo);
	}
	return E_SUCCESS;
}

SINT32 CAMix::signXML(DOM_Node& a_element)
	{
    CACertStore* tmpCertStore=new CACertStore();
    
    CACertificate* ownCert=pglobalOptions->getOwnCertificate();
    if(ownCert==NULL)
			{
        CAMsg::printMsg(LOG_DEBUG,"Own Test Cert is NULL!\n");
			}
    
    // Operator Certificates
    UINT32 opCertsLength;
    CACertificate** opCert=pglobalOptions->getOpCertificates(opCertsLength);
    if(opCert==NULL)
			{
        CAMsg::printMsg(LOG_DEBUG,"Op Test Cert is NULL!\n");
			}
		else
			{
				// Own  Mix Certificates first, then Operator Certificates
				for(SINT32 i = opCertsLength - 1;  i >=0; i--)
					{
						tmpCertStore->add(opCert[i]); 	
					}
				}
    tmpCertStore->add(ownCert);
    
    if(m_pSignature->signXML(a_element, tmpCertStore)!=E_SUCCESS)
			{
				return E_UNKNOWN;
			}
    delete ownCert;
		for(UINT32 i=0;i<opCertsLength;i++)
			{
				delete opCert[i];
			}
		delete[] opCert;
    delete tmpCertStore;	
    
    return E_SUCCESS;
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
	UINT32 interfaces=pglobalOptions->getListenerInterfaceCount();
	for(UINT32 i=1;i<=interfaces;i++)
	{
		pListener=pglobalOptions->getListenerInterface(i);
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
	
	tmpSock.connect(*pAddr);
	delete pAddr;
	tmpSock.close();
	
	ret = E_SUCCESS;
EXIT:
	CAMsg::printMsg(LOG_DEBUG, "Mix has been reconfigured!\n");
	m_bReconfiguring = false;
	return ret;
}
#endif //DYNAMIC_MIX
#endif //ONLY_LOCAL_PROXY
