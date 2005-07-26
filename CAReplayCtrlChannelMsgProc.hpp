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
#ifndef CA_REPLAY_CONTROL_CHANNEL_MSG_PROC
#define CA_REPLAY_CONTROL_CHANNEL_MSG_PROC
#include "CAThread.hpp"
class CAMix;
class CAReplayControlChannel;

class CAReplayCtrlChannelMsgProc
{
	public:
		/** Initialises the replay control channel messages processor with the necessary information.*/
		CAReplayCtrlChannelMsgProc(CAMix* pMix);
		~CAReplayCtrlChannelMsgProc();

		/** Propagates downstream the current replay timestamp*/ 
		SINT32 propagateCurrentReplayTimestamp();


		/** Sends the current replay timestamp periodically on the downstream replay control channel*/
		SINT32 startTimeStampPorpagation(UINT32 minutesPropagationIntervall);	

		/** proccesses a getTimeStamp request on a reply control channel*/
		SINT32 proccessGetTimestamp(CAReplayControlChannel* pReceiver,UINT8 strMixID);

		/** proccesses a getTimeStamps request on a reply control channel*/
		SINT32 proccessGetTimestamps(CAReplayControlChannel* pReceiver);

	private:
		/** Initalises the tmeplate which is used in response to the getTimeStamps request */
		SINT32 initTimestampsMessageTemplate();
		friend THREAD_RETURN rp_loopPropagateTimestamp(void*);
		CAMix* m_pMix;
		CAReplayControlChannel* m_pDownstreamReplayControlChannel;
		UINT32 m_u32PropagationInterval;
		CAThread* m_pThreadTimestampPropagation;
		UINT8* m_strGetTimestampsRepsonseMessageTemplate;
		UINT32 m_u32GetTimestampsRepsonseMessageTemplateLen;
		volatile bool m_bRun;
};
#endif
