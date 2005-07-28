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
#include "CAMix.hpp"
#include "CAReplayControlChannel.hpp"
#include "CACmdLnOptions.hpp"
#include "CAFirstMix.hpp"

CAReplayCtrlChannelMsgProc::CAReplayCtrlChannelMsgProc(CAMix* pMix)
	{
		m_strGetTimestampsRepsonseMessageTemplate=NULL;
		m_pDownstreamReplayControlChannel=NULL;
		m_pUpstreamReplayControlChannel=NULL;
		m_pMix=pMix;
		CAControlChannelDispatcher* pDispatcher=m_pMix->getDownstreamControlChannelDispatcher();
		if(pDispatcher!=NULL)
			{
				m_pDownstreamReplayControlChannel=new CAReplayControlChannel(this);
				pDispatcher->registerControlChannel(m_pDownstreamReplayControlChannel);
			}
		pDispatcher=m_pMix->getUpstreamControlChannelDispatcher();
		if(pDispatcher!=NULL)
			{
				m_pUpstreamReplayControlChannel=new CAReplayControlChannel(this);
				pDispatcher->registerControlChannel(m_pUpstreamReplayControlChannel);
			}
		if(pMix->getType()==CAMix::FIRST_MIX)
			{
				initTimestampsMessageTemplate();
			}
	}

CAReplayCtrlChannelMsgProc::~CAReplayCtrlChannelMsgProc()
	{
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
		delete m_strGetTimestampsRepsonseMessageTemplate;
	}

SINT32 CAReplayCtrlChannelMsgProc::proccessGetTimestamps(CAReplayControlChannel* pReceiver)
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
		for(SINT32 i=0;i<pMix->getMixCount();i++)
			{
				DOM_Element elemMix=docTemplate.createElement("Mix");
				setDOMElementAttribute(elemMix,"id",mixParameters[i].m_strMixID);
				DOM_Element elemReplay=docTemplate.createElement("Replay");
				elemMix.appendChild(elemReplay);
				DOM_Element elemReplayTimestamp=docTemplate.createElement("ReplayTimestamp");
				tReplayTimestamp rt;
				CADatabase::getReplayTimestampForTime(rt,aktTime,mixParameters[i].m_u32ReplayRefTime);
				setDOMElementAttribute(elemReplayTimestamp,"interval",rt.interval);
				setDOMElementAttribute(elemReplayTimestamp,"offset",rt.offset);
				elemReplay.appendChild(elemReplayTimestamp);
				elemMixes.appendChild(elemMix);
			}
		SINT32 ret=pReceiver->sendXMLMessage(docTemplate);
		return ret;
	}

SINT32 CAReplayCtrlChannelMsgProc::proccessGetTimestamp(CAReplayControlChannel* pReceiver,UINT8* strMixID)
	{
		UINT8 buff[255];
		UINT8 msgBuff[1024];
		options.getMixId(buff,255);
		if(strMixID==NULL||strncmp((char*)strMixID,(char*)buff,255)==0)
			{
				tReplayTimestamp rt;
				m_pMix->getReplayDB()->getCurrentReplayTimestamp(rt);
				const char* strTemplate="<?xml version=\"1.0\" encoding=\"UTF-8\"?><Mix id=\"%s\"><Replay><ReplayTimestamp interval=\"%u\" offset=\"%u\"/><Replay></Mix>";
				sprintf((char*)msgBuff,strTemplate,buff,rt.interval,rt.offset);
				return pReceiver->sendXMLMessage(msgBuff,strlen((char*)msgBuff));
			}
		else if(m_pUpstreamReplayControlChannel!=NULL&&strMixID!=NULL)
			{
				const char* strTemplate="<?xml version=\"1.0\" encoding=\"UTF-8\"?><GetTimestamp id=\"%s\"/>";
				sprintf((char*)msgBuff,strTemplate,buff);
				return m_pUpstreamReplayControlChannel->sendXMLMessage(msgBuff,strlen((char*)msgBuff));
			}
		return E_SUCCESS;
	}

SINT32 CAReplayCtrlChannelMsgProc::propagateCurrentReplayTimestamp()
	{
		if(m_pDownstreamReplayControlChannel==NULL)
			return E_UNKNOWN;
		const char* strMsgTemplate="<?xml version=\"1.0\" encoding=\"UTF-8\"?><Mix id=\"%s\"><Replay><ReplayTimestamp interval=\"%u\" offset=\"%u\"/></Replay></Mix>"; 
		tReplayTimestamp replayTimestamp;
		m_pMix->getReplayDB()->getCurrentReplayTimestamp(replayTimestamp);
		UINT8 buff[255];
		UINT8* msgBuff=new UINT8[1024];
		options.getMixId(buff,255);
		sprintf((char*)msgBuff,strMsgTemplate,buff,replayTimestamp.interval,replayTimestamp.offset);
		m_pDownstreamReplayControlChannel->sendXMLMessage(msgBuff,strlen((char*)msgBuff));
		delete msgBuff;
		return E_SUCCESS;
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

/** We initialise the template used to generate the idividual responses to gettimestamps requests
	* according to the mix ids of the cascade. We later use this template to generate the
	* responses quickly.
	*/
SINT32 CAReplayCtrlChannelMsgProc::initTimestampsMessageTemplate()
	{
		//Only for the first mix get timestamps is supported for the moment!
		if(!m_pMix->getType()==CAMix::FIRST_MIX)
			{
				return E_UNKNOWN;
			}
		CAFirstMix* pMix=(CAFirstMix*)m_pMix;
		DOM_Document docTemplate=DOM_Document::createDocument();
		DOM_Element elemMixes=docTemplate.createElement("Mixes");
		docTemplate.appendChild(elemMixes);
		tMixParameters* mixParameters=pMix->getMixParameters();
		for(SINT32 i=0;i<pMix->getMixCount();i++)
			{
				DOM_Element elemMix=docTemplate.createElement("Mix");
				setDOMElementAttribute(elemMix,"id",mixParameters[i].m_strMixID);
				DOM_Element elemReplay=docTemplate.createElement("Replay");
				elemMix.appendChild(elemReplay);
				DOM_Element elemReplayTimestamp=docTemplate.createElement("ReplayTimestamp");
				setDOMElementAttribute(elemReplayTimestamp,"interval",(UINT8*)"%u");
				setDOMElementAttribute(elemReplayTimestamp,"offset",(UINT8*)"%u");
				elemReplay.appendChild(elemReplayTimestamp);
				elemMixes.appendChild(elemMix);
			}
		UINT8* buff=DOM_Output::dumpToMem(docTemplate,&m_u32GetTimestampsRepsonseMessageTemplateLen);
		m_strGetTimestampsRepsonseMessageTemplate=new UINT8[m_u32GetTimestampsRepsonseMessageTemplateLen+1];
		memcpy(m_strGetTimestampsRepsonseMessageTemplate,buff,m_u32GetTimestampsRepsonseMessageTemplateLen);
		m_strGetTimestampsRepsonseMessageTemplate[m_u32GetTimestampsRepsonseMessageTemplateLen]=0;
		m_u32GetTimestampsRepsonseMessageTemplateLen++;
		delete[] buff;
		return E_SUCCESS;
	}

SINT32 CAReplayCtrlChannelMsgProc::proccessGotTimestamp(CAReplayControlChannel* pReceiver,UINT8* strMixID,tReplayTimestamp& rt)
	{
		//if not first mix just forwards them down the drain...
		if(m_pMix->getType()!=CAMix::FIRST_MIX)
			{
				UINT8 msgBuff[1024];
				const char* strTemplate="<?xml version=\"1.0\" encoding=\"UTF-8\"?><Mix id=\"%s\"><Replay><ReplayTimestamp interval=\"%u\" offset=\"%u\"/></Replay></Mix>";
				sprintf((char*)msgBuff,strTemplate,strMixID,rt.interval,rt.offset);
				return m_pDownstreamReplayControlChannel->sendXMLMessage(msgBuff,strlen((char*)msgBuff));
			}
		//First mix --> update mix parameters
		UINT32 refTime;
		CADatabase::getTimeForReplayTimestamp(refTime,rt);
		CAFirstMix* pMix=(CAFirstMix*)m_pMix;
		tMixParameters params;
		UINT32 len=strlen((char*)strMixID);
		params.m_strMixID=new UINT8[len+1];
		memcpy(params.m_strMixID,strMixID,len+1);
		params.m_u32ReplayRefTime=refTime;
		pMix->setMixParameters(params);
		delete[] params.m_strMixID;
		return E_SUCCESS;
	}

SINT32 CAReplayCtrlChannelMsgProc::sendGetTimestamp(const UINT8* strMixID)
	{
		UINT8 msgBuff[512];
		const char* strTemplate="<?xml version=\"1.0\" encoding=\"UTF-8\"?><GetTimestamp id=\"%s\"/>";
		sprintf((char*)msgBuff,strTemplate,strMixID);
		return m_pUpstreamReplayControlChannel->sendXMLMessage(msgBuff,strlen((char*)msgBuff));
	}

#endif //REPLAY_DETECTION
