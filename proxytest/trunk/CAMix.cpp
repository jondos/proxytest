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
#include "CAMix.hpp"
#include "CAUtil.hpp"
#include "CAInfoService.hpp"
#include "CACmdLnOptions.hpp"

CAMix::CAMix()
{
    m_acceptReconfiguration = options.acceptReconfiguration();
		m_pSignature=NULL;
		m_pInfoService=NULL;
		#ifdef REPLAY_DETECTION
			m_pReplayMsgProc=NULL;
		#endif
		m_pMuxOutControlChannelDispatcher=NULL;
		m_pMuxInControlChannelDispatcher=NULL;
	}

SINT32 CAMix::start()
{
		if(initOnce()!=E_SUCCESS)
			return E_UNKNOWN;

    if(m_pSignature != NULL && options.isInfoServiceEnabled())
			{
				CAMsg::printMsg(LOG_DEBUG, "CAMix start: creating InfoService object\n");
        m_pInfoService=new CAInfoService(this);
        
		UINT32 opCertLength;
		CACertificate** opCerts = options.getOpCertificates(opCertLength);
		m_pInfoService->setSignature(m_pSignature, 
			options.getOwnCertificate(), opCerts, opCertLength);


        bool allowReconf = options.acceptReconfiguration();
        bool needReconf = needAutoConfig();

        m_pInfoService->setConfiguring(allowReconf && needReconf);
				CAMsg::printMsg(LOG_DEBUG, "CAMix start: starting InfoService\n");
        m_pInfoService->start();
        while(allowReconf && needReconf)
        {
            CAMsg::printMsg(LOG_INFO, "No next mix or no prev. mix certificate specified. Waiting for auto-config from InfoService.\n");

            sSleep(20);
            needReconf = needAutoConfig();
        }
    }

    while(true)
    {
				CAMsg::printMsg(LOG_DEBUG, "CAMix main: before init()\n");
        if(init() == E_SUCCESS)
        {
					CAMsg::printMsg(LOG_DEBUG, "CAMix main: init() returned success\n");
            if(m_pInfoService != NULL)
            {
                m_pInfoService->setConfiguring(false);
                if( ! m_pInfoService->isRunning())
                    m_pInfoService->start();
            }

            CAMsg::printMsg(LOG_INFO, "The mix is now on-line.\n");
						loop();
						CAMsg::printMsg(LOG_DEBUG, "CAMix main: loop() returned, maybe connection lost.\n");
        }
        else
        {
            CAMsg::printMsg(LOG_DEBUG, "init() failed, maybe no connection.\n");
        }

				CAMsg::printMsg(LOG_DEBUG, "CAMix main: stopping InfoService\n");
        if(m_pInfoService != NULL)
        {
            if(options.acceptReconfiguration())
                m_pInfoService->setConfiguring(true);
            else
                m_pInfoService->stop();
        }
				CAMsg::printMsg(LOG_DEBUG, "CAMix main: before clean()\n");
				clean();
				CAMsg::printMsg(LOG_DEBUG, "CAMix main: after clean()\n");
        sSleep(20);
    }
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

    if(!options.isLastMix())
    {
        ret = true;

        // look for usable target interfaces
        for(UINT32 i=0;i<options.getTargetInterfaceCount();i++)
        {
            TargetInterface oNextMix;
            options.getTargetInterface(oNextMix,i+1);
            if(oNextMix.target_type==TARGET_MIX)
            {
                ret = false;
            }
			}

        if(!options.hasNextMixTestCertificate())
            ret = true;
    }

    if(!options.isFirstMix() && !options.hasPrevMixTestCertificate())
        ret = true;

    return ret;
}

/** This will initialize the XML Cascade Info struct @ref XMLFirstMixToInfoService that is sent to the InfoService
* in \c CAInfoService::sendCascadeHelo()
* @param mixes the <Mixes> element of the XML struct we received from the succeeding mix.
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
    options.getMixId(id,50);

    UINT8 name[255];
    if(options.getCascadeName(name,255)!=E_SUCCESS)
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

    for(UINT32 i=1;i<=options.getListenerInterfaceCount();i++)
    {
        CAListenerInterface* pListener=options.getListenerInterface(i);
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
    DOM_Element elemThisMix=m_docMixCascadeInfo.createElement("Mix");
    setDOMElementAttribute(elemThisMix,"id",id);
    DOM_Node elemMixesDocCascade=m_docMixCascadeInfo.createElement("Mixes");
    count=1;
    if(options.isFirstMix())
    {
        elemMixesDocCascade.appendChild(elemThisMix);
    }
    elemRoot.appendChild(elemMixesDocCascade);

//    UINT8 cascadeId[255];
//		UINT32 cascadeIdLen=255;

    DOM_Node node=mixes.getFirstChild();
    while(node!=NULL)
    {
        if(node.getNodeType()==DOM_Node::ELEMENT_NODE&&node.getNodeName().equals("Mix"))
        {
            elemMixesDocCascade.appendChild(m_docMixCascadeInfo.importNode(node,false));
            count++;
 //           cascadeId = static_cast<const DOM_Element&>(node).getAttribute("id").transcode();
        }
        node=node.getNextSibling();
    }

    if(options.isLastMix())
    {
        elemMixesDocCascade.appendChild(elemThisMix);
        cascadeID = id;
    }
		else if(options.isFirstMix())
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
#else
	setDOMElementAttribute(elemPayment,"required",(UINT8*)"false");
#endif
		return E_SUCCESS;
}

