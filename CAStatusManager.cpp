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

#include "CAStatusManager.hpp"

#ifdef SERVER_MONITORING

#include "CAMsg.hpp"
#include "CAMutex.hpp"
#include "CAUtil.hpp"
#include "xml/DOM_Output.hpp"
#include "CACmdLnOptions.hpp"
#include "monitoringDefs.h"

/**
 * @author Simon Pecher, JonDos GmbH
 * 
 * Here we keep track of the current mix state to answer
 * server monitoring requests.
 */
CAStatusManager *CAStatusManager::ms_pStatusManager = NULL;
state_t ***CAStatusManager::ms_pAllStates = NULL;
event_t ***CAStatusManager::ms_pAllEvents = NULL;
extern CACmdLnOptions* pglobalOptions;

void CAStatusManager::init()
{
	if(ms_pAllEvents == NULL)
	{
		initEvents();
	}
	if(ms_pAllStates == NULL)
	{
		initStates();
	}
	if(ms_pStatusManager == NULL)
	{
		ms_pStatusManager = new CAStatusManager();
	}
}
	
void CAStatusManager::cleanup()
{
#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG, 
				"CAStatusManager: doing cleanup\n");
#endif
	/*if(ms_pStatusManager->m_pStatusSocket != NULL)
	{
		if(!(ms_pStatusManager->m_pStatusSocket->isClosed()))
		{
			ms_pStatusManager->m_pStatusSocket->close();
		}
	}*/
	
	if(ms_pStatusManager != NULL)
	{
		delete ms_pStatusManager;
		ms_pStatusManager = NULL;
	}
	if(ms_pAllStates != NULL)
	{
		deleteStates();
	}
	if(ms_pAllEvents != NULL)
	{
		deleteEvents();
	}
}

SINT32 CAStatusManager::fireEvent(event_type_t e_type, enum status_type s_type)
{
	if(ms_pStatusManager != NULL)
	{
		return ms_pStatusManager->transition(e_type, s_type);
	}
	else
	{
		CAMsg::printMsg(LOG_CRIT, 
				"StatusManager: cannot handle event %d/%d "
				"because there is no StatusManager deployed\n", 
				s_type, e_type);
		return E_UNKNOWN;
	}
}

CAStatusManager::CAStatusManager()
{
	int i = 0, ret = 0;
	m_pCurrentStates = NULL;
	m_pCurrentStatesInfo = NULL;
	m_pStatusLock = NULL;
	m_pStatusSocket = NULL;
	m_pListenAddr = NULL;
	m_pMonitoringThread = NULL;
	m_pPreparedStatusMessage = NULL;
	m_bTryListen = false;
	
	m_pCurrentStates = new state_t*[NR_STATUS_TYPES];
	
	for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
	{
		m_pCurrentStates[i] = 
				ms_pAllStates[i][ENTRY_STATE];
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "Init state: %s - %s\n", STATUS_NAMES[i], 
						m_pCurrentStates[i]->st_description);
#endif
	}
	
	m_pStatusLock = new CAMutex();
	m_pStatusSocket = new CASocket();
	ret = initSocket();
	if( (ret == E_SUCCESS) || (ret == EADDRINUSE) )
	{
		m_bTryListen = (ret == EADDRINUSE);
		m_pMonitoringThread = new CAThread((UINT8*)"Monitoring Thread");
		m_pMonitoringThread->setMainLoop(serveMonitoringRequests);
		m_pMonitoringThread->start(this);
	}
	else
	{
		CAMsg::printMsg(LOG_ERR, 
					"StatusManager: an error occured while initializing the"
					" server monitoring socket\n");
	}
	initStatusMessage();
	/* pass entry state staus information to the DOM structure */
	for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
	{
		setDOMElementValue(
				(m_pCurrentStatesInfo[i]).dsi_stateType,
				(UINT32)(m_pCurrentStates[i])->st_type);
		setDOMElementValue(
				(m_pCurrentStatesInfo[i]).dsi_stateDesc,
				(UINT8*)(m_pCurrentStates[i])->st_description);
		setDOMElementValue(
				(m_pCurrentStatesInfo[i]).dsi_stateLevel,
				(UINT8*)(STATUS_LEVEL_NAMES[(m_pCurrentStates[i])->st_stateLevel]));
	}
}

CAStatusManager::~CAStatusManager()
{
	int i = 0;
	if(m_pMonitoringThread != NULL)
	{
		if(m_pStatusSocket != NULL)
		{
			if( !(m_pStatusSocket->isClosed()) )
			{
				m_pStatusSocket->close();
			}
		}
		
		delete m_pMonitoringThread;
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, 
				"CAStatusManager: The monitoring thread is no more.\n");
#endif
		m_pMonitoringThread = NULL;
	}
	if(m_pStatusLock != NULL)
	{
		delete m_pStatusLock;
		m_pStatusLock = NULL;
	}
	if(m_pCurrentStates != NULL)
	{
		for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
		{
			m_pCurrentStates[i] = NULL;
		}
		delete[] m_pCurrentStates;
		m_pCurrentStates = NULL;
	}
	if(m_pStatusSocket != NULL)
	{
		delete m_pStatusSocket;
		m_pStatusSocket = NULL;
	}
	if(m_pListenAddr != NULL)
	{
		delete m_pListenAddr;
		m_pListenAddr = NULL;
	}
	if(m_pPreparedStatusMessage != NULL)
	{	
		m_pPreparedStatusMessage->release();
		m_pPreparedStatusMessage = NULL;
	}
}
SINT32 CAStatusManager::initSocket()
{
	SINT32 ret = E_UNKNOWN;
	int errnum = 0;
	
	if(m_pStatusSocket == NULL)
	{
		m_pStatusSocket = new CASocket();
	}
	
	if( !(m_pStatusSocket->isClosed()) )
	{
		CAMsg::printMsg(LOG_ERR, 
				"StatusManager: server monitoring socket already connected.\n");
		return E_UNKNOWN;
	}
	
	ret = m_pStatusSocket->create();
	if(ret != E_SUCCESS)
	{
		CAMsg::printMsg(LOG_ERR, 
				"StatusManager: could not create server monitoring socket.\n");
		return ret;
	}
	/* listen to default server address, if nothing is specified:
	 * localhost:8080
	 */ 
	char *hostname = "localhost";
	UINT16 port = MONITORING_SERVER_PORT;
	bool userdefined = false;
	if(pglobalOptions != NULL)
	{
		if(pglobalOptions->getMonitoringListenerHost()!= NULL)
		{
			hostname = pglobalOptions->getMonitoringListenerHost();
			userdefined = true;
		}
		if(pglobalOptions->getMonitoringListenerPort()!= 0xFFFF)
		{
			port = pglobalOptions->getMonitoringListenerPort();
			userdefined = true;
		}
	}
	m_pListenAddr = new CASocketAddrINet();
	ret = m_pListenAddr->setAddr((UINT8 *) hostname, port);
	if(ret != E_SUCCESS)
	{
		if(ret == E_UNKNOWN_HOST)
		{
			CAMsg::printMsg(LOG_ERR, 
						"StatusManager: could not initialize specified listener interface:"
						" invalid host %s\n", hostname);
			if(userdefined)
			{
				hostname = "localhost";
				CAMsg::printMsg(LOG_ERR, 
						"StatusManager: trying %s.\n", hostname);
				
				ret = m_pListenAddr->setAddr((UINT8 *) hostname, port);
				if(ret != E_SUCCESS)
				{
					CAMsg::printMsg(LOG_ERR, 
								"StatusManager: setting up listener interface %s:%d for "
								"server monitoring failed\n",
								hostname, port);
					return ret;
				}
			}
		}
		else
		{
			CAMsg::printMsg(LOG_ERR, 
						"StatusManager: setting up listener interface %s:%d for "
						"server monitoring failed\n",
						hostname, port);
			return ret;
		}
	}
	ret = m_pStatusSocket->listen(*m_pListenAddr);
		
	if(ret != E_SUCCESS)
	{
		if(ret != E_UNKNOWN)
		{
			errnum = GET_NET_ERROR;
			CAMsg::printMsg(LOG_ERR, 
					"StatusManager: not able to init server socket %s:%d "
					"for server monitoring. %s failed because: %s\n",
					hostname, port,
					((ret == E_SOCKET_BIND) ? "Bind" : "Listen"),
					 GET_NET_ERROR_STR(errnum));
			if( errnum == EADDRINUSE )
			{
				CAMsg::printMsg(LOG_INFO, "retry socket listening later\n");
				return errnum;
				//it's safer to avoid reuseaddr in this case
			}
		}
		return ret;
	}
#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG, 
				"StatusManager: listen to monitoring socket on %s:%d\n",
					hostname, port);
#endif	
	return E_SUCCESS;
}

SINT32 CAStatusManager::transition(event_type_t e_type, status_type_t s_type)
{
	transition_t transitionToNextState = st_ignore;
	state_t *prev = NULL;
	
	if( (m_pStatusLock == NULL) ||
		(m_pCurrentStates == NULL) )
	{
		CAMsg::printMsg(LOG_CRIT, 
				"StatusManager: fatal error\n");
		return E_UNKNOWN;
	}
	if((s_type >= NR_STATUS_TYPES) || (s_type < FIRST_STATUS))
	{
		CAMsg::printMsg(LOG_ERR, 
				"StatusManager: received event for an invalid status type: %d\n", s_type);
		return E_INVALID;
	}
	if((e_type >= EVENT_COUNT[s_type]) || (e_type < FIRST_EVENT))
	{
		CAMsg::printMsg(LOG_ERR, 
				"StatusManager: received an invalid event: %d\n", e_type);
		return E_INVALID;
	}
	
	/* We process incoming events synchronously, so the calling thread
	 * should perform state transition very quickly to avoid long blocking 
	 * of other calling threads
	 */
	m_pStatusLock->lock();
	if(m_pCurrentStates[s_type]->st_transitions == NULL)
	{
		m_pStatusLock->unlock();
		CAMsg::printMsg(LOG_CRIT, 
						"StatusManager: current state is corrupt\n");
		return E_UNKNOWN; 
	}
	transitionToNextState = m_pCurrentStates[s_type]->st_transitions[e_type];
	if(transitionToNextState >= STATE_COUNT[s_type])
	{
		m_pStatusLock->unlock();
		CAMsg::printMsg(LOG_ERR, 
						"StatusManager: transition to invalid state %d\n", transitionToNextState);
		return E_INVALID;
	}
	if(transitionToNextState != st_ignore)
	{
		prev = m_pCurrentStates[s_type];
		m_pCurrentStates[s_type] = ms_pAllStates[s_type][transitionToNextState];
		m_pCurrentStates[s_type]->st_prev = prev;
		m_pCurrentStates[s_type]->st_cause = ms_pAllEvents[s_type][e_type];
		
		/* setting the xml elements of the info message won't be too expensive */ 
		setDOMElementValue(
				(m_pCurrentStatesInfo[s_type]).dsi_stateType,
				(UINT32)(m_pCurrentStates[s_type])->st_type);
		setDOMElementValue(
				(m_pCurrentStatesInfo[s_type]).dsi_stateDesc,
				(UINT8*)(m_pCurrentStates[s_type])->st_description);
		setDOMElementValue(
				(m_pCurrentStatesInfo[s_type]).dsi_stateLevel,
				(UINT8*)(STATUS_LEVEL_NAMES[(m_pCurrentStates[s_type])->st_stateLevel]));
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, 
				"StatusManager: status %s: "
				"transition from state %d (%s) "
				"to state %d (%s) caused by event %d (%s)\n",
				STATUS_NAMES[s_type],
				prev->st_type, prev->st_description,
				m_pCurrentStates[s_type]->st_type, m_pCurrentStates[s_type]->st_description,
				e_type, (ms_pAllEvents[s_type][e_type])->ev_description);
#endif
	}
	m_pStatusLock->unlock();
	return E_SUCCESS;
}

/* prepares (once) a DOM template for all status messages */
SINT32 CAStatusManager::initStatusMessage()
{
	int i = 0;
	m_pPreparedStatusMessage = createDOMDocument();
	m_pCurrentStatesInfo = new dom_state_info[NR_STATUS_TYPES];
	DOMElement *elemRoot = createDOMElement(m_pPreparedStatusMessage, DOM_ELEMENT_STATUS_MESSAGE_NAME);
	DOMElement *status_dom_element = NULL;
	
	for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
	{
		status_dom_element = 
			createDOMElement(m_pPreparedStatusMessage, STATUS_NAMES[i]); 
		(m_pCurrentStatesInfo[i]).dsi_stateType = 
			createDOMElement(m_pPreparedStatusMessage, DOM_ELEMENT_STATE_NAME);
#ifdef DEBUG		
		setDOMElementValue((m_pCurrentStatesInfo[i]).dsi_stateType, (UINT8*)"Statenumber");
#endif		
		(m_pCurrentStatesInfo[i]).dsi_stateLevel =
			createDOMElement(m_pPreparedStatusMessage, DOM_ELEMENT_STATE_LEVEL_NAME);
#ifdef DEBUG
		setDOMElementValue((m_pCurrentStatesInfo[i]).dsi_stateLevel, (UINT8*)"OK or Critcal or something");
#endif
		(m_pCurrentStatesInfo[i]).dsi_stateDesc =
			createDOMElement(m_pPreparedStatusMessage, DOM_ELEMENT_STATE_DESCRIPTION_NAME);
#ifdef DEBUG
		setDOMElementValue((m_pCurrentStatesInfo[i]).dsi_stateDesc, (UINT8*)"Description of the state");
#endif
		status_dom_element->appendChild((m_pCurrentStatesInfo[i]).dsi_stateType);
		status_dom_element->appendChild((m_pCurrentStatesInfo[i]).dsi_stateLevel);
		status_dom_element->appendChild((m_pCurrentStatesInfo[i]).dsi_stateDesc);
		elemRoot->appendChild(status_dom_element);
	}
	m_pPreparedStatusMessage->appendChild(elemRoot);
#ifdef DEBUG
	UINT32 debuglen = XML_STATUS_MESSAGE_MAX_SIZE - 1;
	UINT8 debugout[XML_STATUS_MESSAGE_MAX_SIZE];
	memset(debugout, 0, (sizeof(UINT8)*XML_STATUS_MESSAGE_MAX_SIZE));
	DOM_Output::dumpToMem(m_pPreparedStatusMessage,debugout,&debuglen);		
	CAMsg::printMsg(LOG_DEBUG, "the status message template looks like this: %s \n",debugout);
#endif
	return E_SUCCESS;
}

THREAD_RETURN serveMonitoringRequests(void* param)
{
	CASocket monitoringRequestSocket;
	int ret = 0;
	CAStatusManager *statusManager = (CAStatusManager*) param;

	if(statusManager == NULL)
	{
		CAMsg::printMsg(LOG_CRIT, 
				"Monitoring Thread: fatal error, exiting.\n");
		THREAD_RETURN_ERROR;
	}
	
	for(;EVER;)
	{
		
		if(statusManager->m_pStatusSocket != NULL)
		{
			if(statusManager->m_bTryListen)
			{
				sleep(10);
				
				if(statusManager->m_pListenAddr == NULL)
				{
					CAMsg::printMsg(LOG_ERR, 
								"Monitoring Thread: bind error, leaving loop.\n");
					THREAD_RETURN_ERROR;
				}
				ret = statusManager-> m_pStatusSocket->listen(*(statusManager->m_pListenAddr));
				
				if(ret == E_UNKNOWN)
				{
					CAMsg::printMsg(LOG_ERR, 
							"Monitoring Thread: bind error, leaving loop.\n");
							THREAD_RETURN_ERROR;
				}
				statusManager->m_bTryListen = (ret != E_SUCCESS);
#ifdef DEBUG
				if(statusManager->m_bTryListen)
				{
					
						CAMsg::printMsg(LOG_DEBUG, 
							"Monitoring Thread: wait again for listen: %s\n",
							GET_NET_ERROR_STR(GET_NET_ERROR));
				}				
				else
				{
					CAMsg::printMsg(LOG_DEBUG, 
								"Monitoring Thread: socket listening again\n");
				}
#endif				
				continue;
			}
			if(statusManager->m_pStatusSocket->isClosed())
			{
				CAMsg::printMsg(LOG_INFO, 
							"Monitoring Thread: server socket closed, leaving loop.\n");
				THREAD_RETURN_SUCCESS;
			}
		}
		else
		{
			CAMsg::printMsg(LOG_ERR, 
						"Monitoring Thread: server socket disposed, leaving loop.\n");
			THREAD_RETURN_ERROR;
		}
		
		if(statusManager->m_pStatusSocket->accept(monitoringRequestSocket) == E_SUCCESS)
		{
			UINT32 xmlStatusMessageLength = XML_STATUS_MESSAGE_MAX_SIZE - 1;
			char xmlStatusMessage[XML_STATUS_MESSAGE_MAX_SIZE];
			memset(xmlStatusMessage, 0, (sizeof(char)*XML_STATUS_MESSAGE_MAX_SIZE));
			statusManager->m_pStatusLock->lock();
			DOM_Output::dumpToMem(
					statusManager->m_pPreparedStatusMessage,
					(UINT8*)xmlStatusMessage,
					&xmlStatusMessageLength);
			statusManager->m_pStatusLock->unlock();
			
			char http_prefix[HTTP_ANSWER_PREFIX_MAX_LENGTH];
			memset(http_prefix, 0, (sizeof(char)*HTTP_ANSWER_PREFIX_MAX_LENGTH));
			snprintf(http_prefix, HTTP_ANSWER_PREFIX_MAX_LENGTH,
					HTTP_ANSWER_PREFIX_FORMAT, xmlStatusMessageLength);
			size_t http_prefix_length = strlen(http_prefix);
			
			char statusMessageResponse[http_prefix_length+xmlStatusMessageLength+1];
			strncpy(statusMessageResponse, http_prefix, http_prefix_length);
			strncpy((statusMessageResponse+http_prefix_length), xmlStatusMessage, xmlStatusMessageLength);
			statusMessageResponse[xmlStatusMessageLength+http_prefix_length]=0;
			
#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "the status message looks like this: %s \n",statusMessageResponse);
#endif	
			
			if(monitoringRequestSocket.send((UINT8*)statusMessageResponse, 
											(http_prefix_length+xmlStatusMessageLength)) < 0)
			{
				CAMsg::printMsg(LOG_ERR, 
						"StatusManager: error: could not send status message.\n");
			}
			monitoringRequestSocket.close();
		}
		else
		{
			CAMsg::printMsg(LOG_ERR, 
					"StatusManager: error: could not process monitoring request.\n");
		}
	}
}

void CAStatusManager::initStates()
{
	int i = 0, j = 0; 
	ms_pAllStates = new state_t**[NR_STATUS_TYPES];
	
	for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
	{
		ms_pAllStates[i] = 
			new state_t*[STATE_COUNT[i]];
		
		for(j = ENTRY_STATE; j < STATE_COUNT[i]; j++)
		{
			ms_pAllStates[i][j] = new state_t; 
			/* only state identifier are set, transitions and state description
			 * must be set via macro
			 **/
			ms_pAllStates[i][j]->st_type = (state_type_t) j;
			ms_pAllStates[i][j]->st_statusType = (status_type_t) i;
		}
	}
	FINISH_STATE_DEFINITIONS(ms_pAllStates);
	
}

void CAStatusManager::deleteStates()
{
	int i = 0, j = 0;
	
	for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
	{
		//m_pCurrentStates[i] = NULL;
		for(j = ENTRY_STATE; j < STATE_COUNT[i]; j++)
		{
			if(ms_pAllStates[i][j] != NULL)
			{
				if(ms_pAllStates[i][j]->st_transitions != NULL)
				{
					delete[] ms_pAllStates[i][j]->st_transitions;
					ms_pAllStates[i][j]->st_transitions = NULL;
				}
				//todo: delete state descriptions ?
				delete ms_pAllStates[i][j];
				ms_pAllStates[i][j] = NULL;
			}
		}
		delete[] ms_pAllStates[i];
		ms_pAllStates[i] = NULL;
	}
	delete[] ms_pAllStates;
	ms_pAllStates = NULL;
}

void CAStatusManager::initEvents()
{
	int i = 0, j = 0;
	ms_pAllEvents = new event_t**[NR_STATUS_TYPES];
	
	for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
	{
		ms_pAllEvents[i] = new event_t*[EVENT_COUNT[i]];
		for(j = FIRST_EVENT; j < EVENT_COUNT[i]; j++)
		{
			ms_pAllEvents[i][j] = new event_t;
			ms_pAllEvents[i][j]->ev_type = (event_type_t) j;
			ms_pAllEvents[i][j]->ev_statusType = (status_type_t) i;
		}
	}
	FINISH_EVENT_DEFINITIONS(ms_pAllEvents);
}

void CAStatusManager::deleteEvents()
{
	int i = 0, j = 0;
	
	for(i = FIRST_STATUS; i < NR_STATUS_TYPES; i++)
	{
		for(j = FIRST_EVENT; j < EVENT_COUNT[i]; j++)
		{
			//todo: delete event descriptions ?
			delete ms_pAllEvents[i][j];
			ms_pAllEvents[i][j] = NULL;
		}
		delete[] ms_pAllEvents[i];
		ms_pAllEvents[i] = NULL;
	}
	delete[] ms_pAllEvents;
	ms_pAllEvents = NULL;
}

transition_t *defineTransitions(status_type_t s_type, SINT32 transitionCount, ...)
{
	int i = 0;
	va_list ap;
	transition_t *transitions = NULL;
	event_type_t specifiedEventTypes[transitionCount];
	transition_t specifiedTransitions[transitionCount];
	
	/* read in the specified events with the correspondig transitions */
	va_start(ap, transitionCount);
	for(i = 0; i < transitionCount; i++)
	{
		specifiedEventTypes[i] = (event_type_t) va_arg(ap, int);
		specifiedTransitions[i] = (transition_t) va_arg(ap, int);
	}
	va_end(ap);
	
	if((s_type >= NR_STATUS_TYPES) || (s_type < FIRST_STATUS))
	{
		/* invalid status type specified */
		return NULL;
	}
	if(transitionCount > (EVENT_COUNT[s_type]))
	{
		/* more transitions specified than events defined*/ 
		return NULL;
	}
	
	transitions = new transition_t[(EVENT_COUNT[s_type])];
	memset(transitions, st_ignore, (sizeof(transition_t)*(EVENT_COUNT[s_type])));
	for(i = 0; i < transitionCount; i++)
	{
		if((specifiedEventTypes[i] >= EVENT_COUNT[s_type]) || 
			(specifiedEventTypes[i] < 0)) 
		{
			/* specified event is invalid */
			CAMsg::printMsg(LOG_WARNING, 
						"StatusManager: definiton of an invalid state transition (invalid event %d).\n",
						specifiedEventTypes[i]);
			continue;
		}
		if((specifiedTransitions[i] >= STATE_COUNT[s_type]) ||
			(specifiedTransitions[i] < st_ignore))
		{
			/* corresponding transition to event is not valid */
			CAMsg::printMsg(LOG_WARNING, 
						"StatusManager: definiton of an invalid state transition (invalid state %d).\n",
						specifiedTransitions[i]);
			continue;
		}
		transitions[specifiedEventTypes[i]] = specifiedTransitions[i];
	}
	/*for(i=0; i < (EVENT_COUNT[s_type]); i++)
	{
		CAMsg::printMsg(LOG_DEBUG, "transitions %d: %d\n", i, transitions[i]);
	}*/
	return transitions;
	
}
#endif
