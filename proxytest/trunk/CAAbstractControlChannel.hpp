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

#ifndef __CAABSTRACTCONTROLCHANNEL__
#define __CAABSTRACTCONTROLCHANNEL__
#ifndef ONLY_LOCAL_PROXY
#include "CAControlChannelDispatcher.hpp"
#include "xml/DOM_Output.hpp"

/** The base of each control channel. 
	* Controls channels should be derived from CASyncControlChannel or 
	* CAASyncControlChannel
	*/
class CAAbstractControlChannel
{
  public:
    CAAbstractControlChannel(UINT8 id, bool bIsEncrypted)
			{
				m_bIsEncrypted=bIsEncrypted;
				m_ID=id;
				m_pDispatcher=NULL;
			}
			
		virtual ~CAAbstractControlChannel()
			{
			}
    
		/** Call to send a XML message via this control channel. Note that this message can not be bigger than 64 KBytes.
			* @param docMsg XML document to sent over this control channel
			*	@retval E_SPACE, if the serialized XML message is bigger than
			*										0xFFFF bytes
			* @retval E_SUCCESS, if the message that successful send
			* @retval E_UNKNOWN, in case of an error
			*/
		SINT32 sendXMLMessage(const XERCES_CPP_NAMESPACE::DOMDocument* pDocMsg) const
			{
				UINT32 tlen=0xFFFF+2;
				UINT8 tmpB[0xFFFF+2];
				if(DOM_Output::dumpToMem(pDocMsg,tmpB+2,&tlen)!=E_SUCCESS||
					tlen>0xFFFF)
					{
						return E_SPACE;
					}
				tmpB[0]=(UINT8)(tlen>>8);
				tmpB[1]=(UINT8)(tlen&0xFF);
				return m_pDispatcher->sendMessages(m_ID,m_bIsEncrypted,tmpB,tlen+2);
			}

	/** Call to send a XML message via this control channel.
			* @param msgXML buffer which holds the serialized XML message
			* @param msgLen size of msgXML
			*	@retval E_SPACE, if the serialized XML message is bigger than
			*										0xFFFF bytes
			* @retval E_SUCCESS, if the message that successful send
			* @retval E_UNKNOWN, in case of an error
			*/
		SINT32 sendXMLMessage(const UINT8* msgXML,UINT32 msgLen) const
			{
				CAMsg::printMsg(LOG_DEBUG,"Will sent xml msg over control channel\n");
				if(msgLen>0xFFFF)
					{
						return E_SPACE;
					}
				UINT8 tmpB[0xFFFF+2];
				memcpy(tmpB+2,msgXML,msgLen);
				tmpB[0]=(UINT8)(msgLen>>8);
				tmpB[1]=(UINT8)(msgLen&0xFF);
				return m_pDispatcher->sendMessages(m_ID,m_bIsEncrypted,tmpB,msgLen+2);
			}

		/** Returns the id of this control channel.
			* @retval id of control channel
			*/
		UINT32 getID() const
			{
				return m_ID;
			}

    bool isEncrypted();

  protected:
		/** Processes some bytes of a message we got 
				from the communication channel.  We reassemble this fragments
				in a buffer. If all parts are received we call proccessMessagesComplete()*/
		virtual SINT32 proccessMessage(const UINT8* msg, UINT32 msglen)=0;

		/** Called if a whole messages was received, which should be delivered
			* to the final recipient*/
		virtual SINT32 proccessMessageComplete()=0;

		/** Sets the Dispatcher*/
		SINT32 setDispatcher(const CAControlChannelDispatcher* pDispatcher)
			{
				m_pDispatcher=pDispatcher;
				return E_SUCCESS;
			}

		friend class CAControlChannelDispatcher;
		const CAControlChannelDispatcher* m_pDispatcher;
		bool m_bIsEncrypted;
    UINT32 m_ID;
};

#endif

#endif //ONLY_LOCAL_PROXY
