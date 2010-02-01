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
#ifdef PAYMENT
#include "CAAccountingControlChannel.hpp"

/**
 Creates a new accounting controlchannel and stores a pointer to this
 in the pHashEntry.
*/
CAAccountingControlChannel::CAAccountingControlChannel(fmHashTableEntry * pHashEntry)
 : CASyncControlChannel(ACCOUNT_CONTROL_CHANNEL_ID, true)
	{
		m_pHashEntry = pHashEntry;
		if (m_pHashEntry == NULL)
		{
			CAMsg::printMsg(LOG_CRIT,"CAAccountingControlChannel: User hash entry is NULL - creating control channel will fail!");
		}
		if (CAAccountingInstance::initTableEntry(m_pHashEntry) == E_SUCCESS)
		{
			m_pHashEntry->pAccountingInfo->pControlChannel = this;
		}
	}


CAAccountingControlChannel::~CAAccountingControlChannel()
	{
		// todo cleanup hashtable entry
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "~CAAccountingControlChannel destructor\n");
		#endif
		CAAccountingInstance::cleanupTableEntry(m_pHashEntry);
	}


/**
* processMessage - receives an XML msg and appends it to the AI message queue where it
* will be processed asynchronously
*/
SINT32 CAAccountingControlChannel::processXMLMessage(const  XERCES_CPP_NAMESPACE::DOMDocument* a_doc)
	{
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "Invoking processJapMessage\n");
		#endif
		return CAAccountingInstance::processJapMessage( m_pHashEntry,a_doc );
	}
#endif

