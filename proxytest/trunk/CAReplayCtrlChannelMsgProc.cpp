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
#include "CAReplayCtrlChannelMsgProc.hpp"
#include "CAReplayControlChannel.hpp"
#include "CAMix.hpp"
#include "CAReplayControlChannel.hpp"
#include "CACmdLnOptions.hpp"

CAReplayCtrlChannelMsgProc::CAReplayCtrlChannelMsgProc(CAMix* pMix)
	{
		m_strGetTimestampsRepsonseMessageTemplate=NULL;
		m_pDownstreamReplayControlChannel=new CAReplayControlChannel(this);
		m_pMix=pMix;
		CAControlChannelDispatcher* pDispatcher=m_pMix->getDownstreamControlChannelDispatcher();
		if(pDispatcher!=NULL)
			{
				pDispatcher->registerControlChannel(m_pDownstreamReplayControlChannel);
			}
		if(pMix->getType()==CAMix::FIRST_MIX)
			{
				initTimestampsMessageTemplate();
			}
	}

SINT32 CAReplayCtrlChannelMsgProc::proccessGetTimestamps(CAReplayControlChannel* pReceiver)
	{
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

/** We initialise the tmplate used to generate the idividual responses to gettimesampts requests
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
		DOM_Document docMixCascadeInfo;
		m_pMix->getMixCascadeInfo(docMixCascadeInfo);
		DOM_Document docTemplate=DOM_Document::createDocument();
		DOM_Element elemMixes;
		if(getDOMChildByName(docMixCascadeInfo,(UINT8*)"Mixes",elemMixes,true)!=E_SUCCESS)
			return E_UNKNOWN;
		docTemplate.appendChild(docTemplate.importNode(elemMixes,true));
		m_strGetTimestampsRepsonseMessageTemplate=DOM_Output::dumpToMem(docTemplate,&m_u32GetTimestampsRepsonseMessageTemplateLen);
		return E_SUCCESS;
	}