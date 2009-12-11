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
#include "CAReplayControlChannel.hpp"
#include "CAReplayCtrlChannelMsgProc.hpp"

CAReplayControlChannel::CAReplayControlChannel(const CAReplayCtrlChannelMsgProc* pProcessor)
	:CASyncControlChannel(REPLAY_CONTROL_CHANNEL_ID,false)
	{
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAReplayControlChannel - constructor - pProcessor=%p\n",pProcessor);
		#endif	
		m_pProcessor=pProcessor;
	}

CAReplayControlChannel::~CAReplayControlChannel(void)
	{
		//@todo: delete m_MsgBuff of superclass CASyncControlChannel
	}

SINT32 CAReplayControlChannel::processXMLMessage(const XERCES_CPP_NAMESPACE::DOMDocument* doc)
	{
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG,"CAReplayControlChannel::processXMLMessage()\n");
		#endif
		DOMElement* elemRoot=doc->getDocumentElement();
		if(elemRoot==NULL)
			return E_UNKNOWN;

		if(equals(elemRoot->getNodeName(),"GetTimestamp"))
			{
				UINT8 buff[255];
				UINT32 bufflen=255;
				if(getDOMElementAttribute(elemRoot,"id",buff,&bufflen)!=E_SUCCESS)
					return E_UNKNOWN;
				buff[bufflen]=0;
				m_pProcessor->proccessGetTimestamp(this,buff);
			}
		else if(equals(elemRoot->getNodeName(),"Mix"))
			{
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG,"CAReplayControlChannel::processXMLMessage() - got a timestamp\n");
				#endif
				UINT8 buff[255];
				UINT32 bufflen=255;
				if(getDOMElementAttribute(elemRoot,"id",buff,&bufflen)!=E_SUCCESS)
					return E_UNKNOWN;
				buff[bufflen]=0;
				DOMElement *child;
				getDOMChildByName(elemRoot,"Replay",child,false);
				DOMElement *elemReplayTimestamp;
				getDOMChildByName(child,"ReplayOffset",elemReplayTimestamp,false);
				UINT32 offset=0;
				if( getDOMElementValue(elemReplayTimestamp,offset,0)!=E_SUCCESS)
					return E_UNKNOWN;
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG,"CAReplayControlChannel::processXMLMessage() - call m_pProcessor->proccessGotTimestamp() - m_pProcessor=%p\n",m_pProcessor);
				#endif
				m_pProcessor->proccessGotTimestamp(this,buff,offset);
			}
		return E_SUCCESS;
	}

#endif
