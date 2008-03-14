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

#ifndef _CASYNCCONTROLCHANNEL_
#define _CASYNCCONTROLCHANNEL_
#ifndef ONLY_LOCAL_PROXY
#include "CAAbstractControlChannel.hpp"

/** A synchronous control channel. This means, that every control message
  * will be proccessed imedially. You have to override proccessXMLMessage().*/
class CASyncControlChannel : public CAAbstractControlChannel
{
	public:
			/** Constructor for a synchronized (e.g. received messages are proccessed imedially) control channel.
				* @param id id of the control channel
				* @param bIsEncrypted if true the control channel is encrypted - NOT IMPLEMENTED at the moment
				*/
		  CASyncControlChannel(UINT8 id, bool bIsEncrypted):  
					CAAbstractControlChannel(id,bIsEncrypted)
				{
					m_MsgBuff=new UINT8[0xFFFF];
					m_aktIndex=0;
					m_MsgBytesLeft=0;
				}

		/**Override this method to receive a XML Message.
			* Note: The DOMDocument reference is valid only within this call, i.e. will be delete afterwards form the caller!
			* If you need to store it for later processing, make a copy of
			* the DOMDocument using docMsg->cloneNode(true)
			*/
		virtual SINT32 processXMLMessage(const XERCES_CPP_NAMESPACE::DOMDocument* docMsg)=0;

	protected:
		SINT32 proccessMessage(const UINT8* msg, UINT32 msglen)
			{
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG,"CASyncControlChannel::proccessMessage - msglen=%u\n",msglen);
				#endif
				if(m_MsgBytesLeft==0)//start of new XML Msg
					{
						if(msglen<2)//this should never happen...
							return E_UNKNOWN;
						m_MsgBytesLeft=(msg[0]<<8)|msg[1];
						#ifdef DEBUG
							CAMsg::printMsg(LOG_DEBUG,"CASyncControlChannel::proccessMessage - start of a new msg of len=%u\n",m_MsgBytesLeft);
						#endif
						msglen-=2;
						m_aktIndex=msglen;
						m_MsgBytesLeft-=msglen;
						memcpy(m_MsgBuff,msg+2,msglen);
					}
				else//received some part...
					{
						msglen=min(m_MsgBytesLeft,msglen);
						memcpy(m_MsgBuff+m_aktIndex,msg,msglen);
						m_aktIndex+=msglen;
						m_MsgBytesLeft-=msglen;
					}
				if(m_MsgBytesLeft==0)
					{//whole msg receveid
						return proccessMessageComplete();
					}
				return E_SUCCESS;
			}

		/** Parses the bytes in m_MsgBuff and calls processXMLMessage()*/		
		SINT32 proccessMessageComplete()
			{
				#ifdef DEBUG
					if(m_aktIndex<0xFFFF)
						{
							m_MsgBuff[m_aktIndex]=0;
							CAMsg::printMsg(LOG_DEBUG,"CASyncControlChannel::proccessMessageComplete() - msg=%s\n",m_MsgBuff);
						}
					else
						CAMsg::printMsg(LOG_DEBUG,"CASyncControlChannel::proccessMessageComplete() \n");
				#endif
				XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(m_MsgBuff,m_aktIndex);
				m_aktIndex=0;
				m_MsgBytesLeft=0;
				if(doc==NULL)
					return E_UNKNOWN;
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG,"CASyncControlChannel::proccessMessageComplete() call processXMLMessage()\n");
				#endif
				SINT32 ret=processXMLMessage(doc);
				doc->release();
				return ret;
			}

		///buffer for assembling the parts of the message
		UINT8* m_MsgBuff;
		///how much bytes have we received already?
		UINT32 m_aktIndex;
		///how much bytes we need until all bytes are received?
		UINT32 m_MsgBytesLeft;
};
#endif 
#endif //ONLY_LOCAL_PROXY
