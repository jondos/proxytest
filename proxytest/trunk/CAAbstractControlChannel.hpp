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
#include "CAControlChannelDispatcher.hpp"

/** The base of each control channel. 
	* Controls channels should be derived from CASyncControlChannel or 
	* CAASyncControlChannel
	*/
class CAAbstractControlChannel
{
  public:
    CAAbstractControlChannel(UINT8 id, bool bIsEncrypted,
														CAControlChannelDispatcher* pDispatcher)
			{
				m_bIsEncrypted=bIsEncrypted;
				m_ID=id;
				m_pDispatcher=pDispatcher;
			}

		virtual SINT32 proccessMessage(UINT8* msg, UINT32 msglen)=0;
    
		/** Call to send a message via this control channel.*/
		SINT32 sendMessage(UINT8* msg, UINT32 msglen)
			{
				return m_pDispatcher->sendMessages(m_ID,m_bIsEncrypted,msg,msglen);
			}

		UINT32 getID()
			{
				return m_ID;
			}

    bool isEncrypted();

  protected:
		CAControlChannelDispatcher* m_pDispatcher;
		bool m_bIsEncrypted;
    UINT32 m_ID;
};
