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
#ifdef REPLAY_DETECTION
#include "CAReplayCtrlChannelMsgProc.hpp"
#include "CAReplayControlChannel.hpp"
#include "CAMixWithReplayDB.hpp"
#include "CAMix.hpp"
#include "CAReplayControlChannel.hpp"
#include "CACmdLnOptions.hpp"
#include "CAFirstMix.hpp"
#include "CADatabase.hpp"

extern CACmdLnOptions *pglobalOptions;

CAReplayCtrlChannelMsgProc::CAReplayCtrlChannelMsgProc(const CAMixWithReplayDB* pMix)
	{
		CAMsg::printMsg(LOG_DEBUG,"CAReplayCtrlChannelMsgProc - constructor - this=%p\n",this);
		m_pDownstreamReplayControlChannel=NULL;
		m_pUpstreamReplayControlChannel=NULL;
		m_pThreadTimestampPropagation=NULL;
		m_pMix=pMix;
		m_docTemplate=NULL;
		initTimestampsMessageTemplate();
		CAControlChannelDispatcher* pDispatcher=m_pMix->getDownstreamControlChannelDispatcher();

		if(pDispatcher!=NULL)
			{
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG,"CAReplayCtrlChannelMsgProc - constructor - registering downstream replay control channel\n",this);
				#endif
				m_pDownstreamReplayControlChannel=new CAReplayControlChannel(this);
				CAMsg::printMsg(LOG_DEBUG,"m_pDownstreamReplayControlChannel= %p\n",m_pDownstreamReplayControlChannel);
				pDispatcher->registerControlChannel(m_pDownstreamReplayControlChannel);
			}
		pDispatcher=m_pMix->getUpstreamControlChannelDispatcher();
		if(pDispatcher!=NULL)
			{
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG,"CAReplayCtrlChannelMsgProc - constructor - registering upstream replay control channel\n",this);
				#endif
				m_pUpstreamReplayControlChannel=new CAReplayControlChannel(this);
				pDispatcher->registerControlChannel(m_pUpstreamReplayControlChannel);
			}
	}

CAReplayCtrlChannelMsgProc::~CAReplayCtrlChannelMsgProc()
	{
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAReplayCtrlChannelMsgProc::~CAReplayCtrlChannelMsgProc()\n");
		#endif
		stopTimeStampPorpagation();
		CAControlChannelDispatcher* pDispatcher=m_pMix->getDownstreamControlChannelDispatcher();
		if(pDispatcher!=NULL)
			{
				pDispatcher->removeControlChannel(m_pDownstreamReplayControlChannel->getID());
				delete m_pDownstreamReplayControlChannel;
			}
		pDispatcher=m_pMix->getUpstreamControlChannelDispatcher();
		if(pDispatcher!=NULL)
			{
				pDispatcher->removeControlChannel(m_pUpstreamReplayControlChannel->getID());
				delete m_pUpstreamReplayControlChannel;
			}
	}

/* not used and not necesary
SINT32 CAReplayCtrlChannelMsgProc::proccessGetTimestamps(const CAReplayControlChannel* pReceiver) const
	{
		//Only for the first mix get timestamps is supported for the moment!
		if(m_pMix->getType()!=CAMix::FIRST_MIX)
			{
				return E_UNKNOWN;
			}

		CAFirstMix* pMix=(CAFirstMix*)m_pMix;
		DOM_Document docTemplate=DOM_Document::createDocument();
		DOM_Element elemMixes=docTemplate.createElement("Mixes");
		docTemplate.appendChild(elemMixes);
		tMixParameters* mixParameters=pMix->getMixParameters();
		time_t aktTime=time(NULL);
		for(SINT32 i=0;i<pMix->getMixCount()-1;i++)
			{
				DOM_Element elemMix=docTemplate.createElement("Mix");
				setDOMElementAttribute(elemMix,"id",mixParameters[i].m_strMixID);
				DOM_Element elemReplay=docTemplate.createElement("Replay");
				elemMix.appendChild(elemReplay);
				DOM_Element elemReplayOffset=docTemplate.createElement("ReplayOffset");
				setDOMElementValue(elemReplayOffset,(UINT32) (mixParameters[i].m_u32ReplayOffset+aktTime-pMix->m_u64LastTimestampReceived));
				elemReplay.appendChild(elemReplayOffset);
				elemMixes.appendChild(elemMix);
			}
		return pReceiver->sendXMLMessage(docTemplate);
	}
*/
// possibility for the first mix to request the timestamps
SINT32 CAReplayCtrlChannelMsgProc::proccessGetTimestamp(const CAReplayControlChannel* pReceiver,const UINT8* strMixID) const
	{
		UINT8 buff[255];
		pglobalOptions->getMixId(buff,255);
		if(strMixID==NULL||strncmp((char*)strMixID,(char*)buff,255)==0)//our own replay timestamp is requested
			{
				//First Mixes does not have to send his timestamp!
				if(m_pMix->getType()==CAMix::FIRST_MIX)
					return E_SUCCESS;

				const CAMixWithReplayDB* pMix=(CAMixWithReplayDB*)m_pMix;

				DOMElement* elemReplayTimestamp=NULL;
				if (getDOMChildByName(m_docTemplate->getDocumentElement(),"ReplayOffset",elemReplayTimestamp,true)!=E_SUCCESS){
					return E_UNKNOWN;
					}
				setDOMElementValue(elemReplayTimestamp,(UINT32) (time(NULL)-m_pMix->m_u64ReferenceTime));

				return pReceiver->sendXMLMessage(m_docTemplate);
			}
		else if(m_pUpstreamReplayControlChannel!=NULL&&strMixID!=NULL)//the replay timestamp of some other mix is requested
			{
				XERCES_CPP_NAMESPACE::DOMDocument* doc=createDOMDocument();
				DOMElement *elemGet=createDOMElement(doc,"GetTimestamp");
				setDOMElementAttribute(elemGet,"id",strMixID);
				doc->appendChild(elemGet);

				SINT32 return_value=m_pUpstreamReplayControlChannel->sendXMLMessage(doc);
				doc->release();

				return return_value;
			}
		return E_SUCCESS;	
	}

SINT32 CAReplayCtrlChannelMsgProc::propagateCurrentReplayTimestamp()
	{
		UINT8 buff[255];
		pglobalOptions->getMixId(buff,255);

		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"Start replay timestamp propagation\n");
		#endif

		if(m_pDownstreamReplayControlChannel==NULL)
			return E_UNKNOWN;

		DOMElement* elemReplayTimestamp=NULL;
		if (getDOMChildByName(m_docTemplate->getDocumentElement(),"ReplayOffset",elemReplayTimestamp,true)!=E_SUCCESS){
			return E_UNKNOWN;
			}
		setDOMElementValue(elemReplayTimestamp,(UINT32) (time(NULL)-m_pMix->m_u64ReferenceTime));

		m_pDownstreamReplayControlChannel->sendXMLMessage(m_docTemplate);

		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"Replay timestamp propagation finished\n");
		#endif

		return E_SUCCESS;
	}

THREAD_RETURN rp_loopPropagateTimestamp(void* param)
	{
		CAReplayCtrlChannelMsgProc* pReplayMsgProc=(CAReplayCtrlChannelMsgProc*)param;
		UINT32 propagationInterval=pReplayMsgProc->m_u32PropagationInterval;
		while(pReplayMsgProc->m_bRun)
			{
				if(propagationInterval==0)
					{
						pReplayMsgProc->propagateCurrentReplayTimestamp();
						propagationInterval=pReplayMsgProc->m_u32PropagationInterval;
					}
				sSleep(60);
				propagationInterval--;
			}
		THREAD_RETURN_SUCCESS;
	}

SINT32 CAReplayCtrlChannelMsgProc::startTimeStampPorpagation(UINT32 minutesPropagationInterval)
	{
		m_u32PropagationInterval=minutesPropagationInterval;
		m_pThreadTimestampPropagation=new CAThread();
		m_pThreadTimestampPropagation->setMainLoop(rp_loopPropagateTimestamp);
		m_bRun=true;
		return m_pThreadTimestampPropagation->start(this);
	}

SINT32 CAReplayCtrlChannelMsgProc::stopTimeStampPorpagation()
	{
		m_bRun=false;
		if(m_pThreadTimestampPropagation!=NULL)
			{
				m_pThreadTimestampPropagation->join();
				delete m_pThreadTimestampPropagation;
			}
		m_pThreadTimestampPropagation=NULL;
		return E_SUCCESS;
	}

/** We initialise the template used to generate the idividual responses to gettimestamps requests
	* according to the mix ids of the cascade. We later use this template to generate the
	* responses quickly.
	*/
	
SINT32 CAReplayCtrlChannelMsgProc::initTimestampsMessageTemplate()
	{
		//NOT for the first mix! Only propagateCurrentReplayTimestamps is supported for the moment!
		if(m_pMix->getType()==CAMix::FIRST_MIX)
			{
				return E_SUCCESS;
			}

		if(m_docTemplate!=NULL)
			{
				return E_UNKNOWN;
			}

		UINT8 buff[255];
		pglobalOptions->getMixId(buff,255);

		m_docTemplate=createDOMDocument();
		DOMElement *elemMix=createDOMElement(m_docTemplate,"Mix");
		setDOMElementAttribute(elemMix,"id",buff);
		DOMElement *elemReplay=createDOMElement(m_docTemplate,"Replay");
		elemMix->appendChild(elemReplay);
		DOMElement *elemReplayOffset=createDOMElement(m_docTemplate,"ReplayOffset");
//		setDOMElementValue(elemReplayOffset,(UINT32) (time(NULL)-m_pMix->m_u64ReferenceTime));
		elemReplay->appendChild(elemReplayOffset);
		m_docTemplate->appendChild(elemMix);

		return E_SUCCESS;
	}


SINT32 CAReplayCtrlChannelMsgProc::proccessGotTimestamp(const CAReplayControlChannel* pReceiver,const UINT8* strMixID,const UINT32 offset) const
	{
		//if not first mix just forwards them down the drain...
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAReplayCtrlChannelMsgProc::proccessGotTimestamp() \n");
		#endif
		if(m_pMix->getType()!=CAMix::FIRST_MIX)
			{
				XERCES_CPP_NAMESPACE::DOMDocument* doc=createDOMDocument();
				DOMElement *elemMix=createDOMElement(doc,"Mix");
				setDOMElementAttribute(elemMix,"id",strMixID);
				DOMElement *elemReplay=createDOMElement(doc,"Replay");
				elemMix->appendChild(elemReplay);
				DOMElement *elemReplayOffset=createDOMElement(doc,"ReplayOffset");
				setDOMElementValue(elemReplayOffset,(UINT32) offset);
				elemReplay->appendChild(elemReplayOffset);
				doc->appendChild(elemMix);

				SINT32 return_value=m_pDownstreamReplayControlChannel->sendXMLMessage(doc);
				doc->release();

				return return_value;
			}

		//First mix --> update mix parameters
		CAFirstMix* pMix=(CAFirstMix*)m_pMix;
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAReplayCtrlChannelMsgProc::proccessGotTimestamp() - MixID: %s\n",strMixID);
			CAMsg::printMsg(LOG_DEBUG,"CAReplayCtrlChannelMsgProc::proccessGotTimestamp() - LastTimestamp DIFF: %d\n",time(NULL)-pMix->m_u64LastTimestampReceived);
		#endif

		tMixParameters params;
		UINT32 len=strlen((char*)strMixID);
		params.m_strMixID=new UINT8[len+1];
		memcpy(params.m_strMixID,strMixID,len+1);
		params.m_u32ReplayOffset=offset;
		pMix->setMixParameters(params);

		pMix->m_u64LastTimestampReceived=time(NULL);
		delete[] params.m_strMixID;
		return E_SUCCESS;
	}

SINT32 CAReplayCtrlChannelMsgProc::sendGetTimestamp(const UINT8* strMixID)
	{
		if(strMixID==NULL||strlen((const char*)strMixID)>400)
			return E_UNKNOWN;

		XERCES_CPP_NAMESPACE::DOMDocument* doc=createDOMDocument();
		DOMElement *elemGet=createDOMElement(doc,"GetTimestamp");
		setDOMElementAttribute(elemGet,"id",strMixID);
		doc->appendChild(elemGet);

		SINT32 return_value=m_pUpstreamReplayControlChannel->sendXMLMessage(doc);
		doc->release();

		return return_value;
	}

#endif //REPLAY_DETECTION
