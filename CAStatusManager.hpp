/*
Copyright (c) The JAP-Team, JonDos GmbH

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation and/or
       other materials provided with the distribution.
    * Neither the name of the University of Technology Dresden, Germany, nor the name of
       the JonDos GmbH, nor the names of their contributors may be used to endorse or
       promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef CASTATUSMANAGER_HPP_
#define CASTATUSMANAGER_HPP_

/**
 * @author Simon Pecher, JonDos GmbH
 * 
 * Logic to describe and process the state machine of a mix
 * for reporting networking, payment and system status to server
 * monitoring systems
 */
#include "StdAfx.h"

#ifdef SERVER_MONITORING

#include "CASocket.hpp"
#include "CAThread.hpp"
#include "CASocketAddrINet.hpp"
/* This contains the whole state machine 
 * definitions including typedefs 
 */
#include "monitoringDefs.h"

struct dom_state_info
{
	DOMElement *dsi_stateType;
	DOMElement *dsi_stateLevel;
	DOMElement *dsi_stateDesc;
};

typedef struct dom_state_info dom_state_info_t;

/** 
 * the server routine which: 
 *  * accepts socket connections,
 *  * outputs a status message
 *  * and closes the socket immediately after that
 */
THREAD_RETURN serveMonitoringRequests(void* param);

class CAStatusManager
{

public:
	
	static void init();
	static void cleanup();
	/* Function to be called when an event occured, 
	 * may cause changing of current state 
	 */
	static SINT32 fireEvent(event_type_t e_type, status_type_t s_type);

private:
	/* current states for each status type */
	state_t **m_pCurrentStates;
	/* Keeps references to the nodes of the DOM tree
	 *  where the current Status informations are set 
	 */
	dom_state_info_t *m_pCurrentStatesInfo;
	/* synchronized access to all fields */  
	CAMutex *m_pStatusLock;
	/* ServerSocket to listen for monitoring requests */ 
	CASocket *m_pStatusSocket;
	CASocketAddrINet *m_pListenAddr;
	volatile bool m_bTryListen; 
	/* Thread to answer monitoring requests */
	CAThread *m_pMonitoringThread;
	/* the DOM structure to create the XML status message */
	XERCES_CPP_NAMESPACE::DOMDocument* m_pPreparedStatusMessage; 
	
	/* StatusManger singleton */
	static CAStatusManager *ms_pStatusManager; 
	/* holds references all defined states 
	 * accessed by [status_type][state_type] 
	 */
	static state_t ***ms_pAllStates; 
	/* holds references all defined events 
	 * accessed by [status_type][event_type] 
	 */
	static event_t ***ms_pAllEvents;  
	
	CAStatusManager();
	virtual ~CAStatusManager();
	
	SINT32 initSocket();
	
	static void initStates();
	static void initEvents();
	
	static void deleteStates();
	static void deleteEvents();
	
	/* changes state  (only called by fireEvent) */
	SINT32 transition(event_type_t e_type, status_type_t s_type);
	SINT32 initStatusMessage();
	
	/* monitoring server routine */
	friend THREAD_RETURN serveMonitoringRequests(void* param);		
};

#define MONITORING_FIRE_NET_EVENT(e_type) \
	MONITORING_FIRE_EVENT(e_type, stat_networking) 
#define MONITORING_FIRE_PAY_EVENT(e_type) \
	MONITORING_FIRE_EVENT(e_type, stat_payment)
#define MONITORING_FIRE_SYS_EVENT(e_type) \
	MONITORING_FIRE_EVENT(e_type, stat_system) 

#define MONITORING_FIRE_EVENT(e_type, s_type) \
			CAStatusManager::fireEvent(e_type, s_type)

#else /* SERVER_MONITORING */

#define MONITORING_FIRE_NET_EVENT(e_type) 
#define MONITORING_FIRE_PAY_EVENT(e_type)
#define MONITORING_FIRE_SYS_EVENT(e_type)

#define MONITORING_FIRE_EVENT(e_type, s_type) 

#endif /* SERVER_MONITORING */ 

#endif /*CASTATUSMANAGER_HPP_*/
