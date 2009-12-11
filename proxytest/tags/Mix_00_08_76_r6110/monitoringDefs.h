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
#ifndef MONITORING_DEFS_H_
#define MONITORING_DEFS_H_

/**
 * All state and transition definitions can be found here
 * The server monitoring observes different states for multiple status types.
 * The status types are currently:
 *
 * 	1. "Network" which contains informations about the connection of the current mix to the others
 * 	2. "Payment" refers to the current state of the mixes accounting instance module.
 *  3. "System" reflects general information about the actual state of the mix process.
 *
 * A state has a level that shows if the current state of a mix is ok, unknown or critical.
 * To realize a state machine events can be defined here. The events can be linked with states
 * to realize transitions from one state to another if an event is fired.
 *
 *  Several macros can be found here for the definition of status types, states
 *  and events. Transitions can also be realized via macros.
 *
 */

/* How many different status types exist*/

#define STATUS_FLAG(status_type) (1<<(unsigned int)status_type)

#define FIRST_STATUS 0
#define FIRST_EVENT 0
#define ENTRY_STATE 0

#define DOM_ELEMENT_STATUS_MESSAGE_NAME "StatusMessage"
#define DOM_ELEMENT_STATE_NAME "State"
#define DOM_ELEMENT_STATE_LEVEL_NAME "StateLevel"
#define DOM_ELEMENT_STATE_DESCRIPTION_NAME "StateDescription"

#define MAX_DESCRIPTION_LENGTH 50
#define MONITORING_SERVER_PORT 8080
#define XML_STATUS_MESSAGE_MAX_SIZE 3000

#define XML_STATUS_MESSAGE_START "<StatusMessage>"
#define HTTP_ANSWER_PREFIX_FORMAT "HTTP/1.1 200 OK\nContent-Length: %u\nConnection: close\nContent-Type: text/xml; charset=UTF-8\n\n"

#define HTTP_ANSWER_PREFIX_MAX_LENGTH 100

#define EVER 1

//all status types
enum status_type
{
	stat_undef = -1,
	stat_networking = 0,
#ifdef PAYMENT
	stat_payment,
#endif
	stat_system,
	stat_all
};

#define NR_STATUS_TYPES stat_all

//enum type for the id of a state
enum state_type
{
	st_ignore = -1,
	/* networking states */
	st_net_entry = 0,
	st_net_firstMixInit, st_net_firstMixConnectedToNext, st_net_firstMixOnline ,
	st_net_middleMixInit, st_net_middleMixConnectedToPrev,
	st_net_middleMixConnectedToNext, st_net_middleMixOnline,
	st_net_lastMixInit, st_net_lastMixConnectedToPrev, st_net_lastMixOnline,
	st_net_overall, /* only represents the number all network states */
#ifdef PAYMENT
	/* payment states */
	st_pay_entry = 0,
	st_pay_aiInit, st_pay_aiShutdown,
	st_pay_biAvailable, st_pay_biUnreachable, st_pay_biPermanentlyUnreachable,
	st_pay_dbError, st_pay_dbErrorBiUnreachable, st_pay_dbErrorBiPermanentlyUnreachable,
	st_pay_overall, /* only represents the number all payment states */
#endif
	/* system states */
	st_sys_entry = 0,
	st_sys_initializing, st_sys_operating, st_sys_restarting,
	st_sys_shuttingDown, st_sys_ShuttingDownAfterSegFault,
	st_sys_overall /* only represents the number all system states */
};

//enum type for the id of an event
enum event_type
{
	/* networking events */
	ev_net_firstMixInited = 0, ev_net_middleMixInited , ev_net_lastMixInited,
	ev_net_prevConnected, ev_net_nextConnected,
	ev_net_prevConnectionClosed, ev_net_nextConnectionClosed,
	ev_net_keyExchangePrevSuccessful, ev_net_keyExchangeNextSuccessful,
	ev_net_keyExchangePrevFailed, ev_net_keyExchangeNextFailed,
	ev_net_overall, /* only represents the number all network events */
#ifdef PAYMENT
	/* payment events */
	ev_pay_aiInited = 0, ev_pay_aiShutdown,
	ev_pay_biConnectionSuccess, ev_pay_biConnectionFailure, ev_pay_biConnectionCriticalSubseqFailures,
	ev_pay_dbConnectionSuccess, ev_pay_dbConnectionFailure,
	ev_pay_overall,  /* only represents the number all payment events */
#endif
	/* system events */
	ev_sys_start = 0,
	ev_sys_enterMainLoop, ev_sys_leavingMainLoop,
	ev_sys_sigTerm, ev_sys_sigInt, ev_sys_sigSegV,
	ev_sys_overall  /* only represents the number all system events */
};

#ifdef PAYMENT
	#define PAYMENT_STATUS_NAME "PaymentStatus"
	#define NR_PAYMENT_STATES st_pay_overall
	#define NR_PAYMENT_EVENTS ev_pay_overall
#endif

#define NETWORKING_STATUS_NAME "NetworkingStatus"
#define NR_NETWORKING_STATES st_net_overall
#define NR_NETWORKING_EVENTS ev_net_overall

#define SYSTEM_STATUS_NAME "SystemStatus"
#define NR_SYSTEM_STATES st_sys_overall
#define NR_SYSTEM_EVENTS ev_sys_overall

static const char *STATUS_NAMES[NR_STATUS_TYPES] =
{
		NETWORKING_STATUS_NAME,
#ifdef PAYMENT
		PAYMENT_STATUS_NAME,
#endif
		SYSTEM_STATUS_NAME
};

static const int EVENT_COUNT[NR_STATUS_TYPES] =
{
		NR_NETWORKING_EVENTS,
#ifdef PAYMENT
		NR_PAYMENT_EVENTS,
#endif
		NR_SYSTEM_EVENTS
};

static const int STATE_COUNT[NR_STATUS_TYPES] =
{
		NR_NETWORKING_STATES,
#ifdef PAYMENT
		NR_PAYMENT_STATES,
#endif
		NR_SYSTEM_STATES
};


/* indices must correspond to strings in STATUS_LEVEL_NAMES */
enum state_level
{
	stl_ok = 0, stl_warning, stl_critical, stl_unknown, stl_all
};

#define NR_STATE_LEVELS stl_all

static const char *STATUS_LEVEL_NAMES[NR_STATE_LEVELS] =
{
		"OK", "WARNING", "CRITICAL", "UNKNOWN"
};

typedef enum state_type state_type_t;
typedef enum status_type status_type_t;
typedef enum state_type transition_t;
typedef enum event_type event_type_t;
typedef enum state_level state_level_t;

//struct representing an event
struct event
{
	event_type_t ev_type; //id of the event
	status_type_t ev_statusType; //status to which the event belongs
	char *ev_description; //text description of this event.
};

struct state
{
	state_type_t st_type; //id of the state
	status_type_t st_statusType; //status to which the state belongs
	state_level_t st_stateLevel; //level of the state (e.g "ok", "critical")
	char *st_description; //text description of this state.
	struct event *st_cause;
	struct state *st_prev;
	transition_t *st_transitions; //pointer to the transitions which leads from this state to other states
};

typedef struct state state_t;
typedef struct event event_t;

/** helper macros for defining states and events: **/
#define FINISH_STATE_DEFINITIONS(state_array) \
			FINISH_NETWORKING_STATE_DEFINITIONS(state_array) \
			FINISH_PAYMENT_STATE_DEFINITIONS(state_array) \
			FINISH_SYSTEM_STATE_DEFINITIONS(state_array)

#define FINISH_EVENT_DEFINITIONS(event_array) \
			FINISH_NETWORKING_EVENT_DEFINITIONS(event_array) \
			FINISH_PAYMENT_EVENT_DEFINITIONS(event_array) \
			FINISH_SYSTEM_EVENT_DEFINITIONS(event_array)

/* networking states description and transition assignment
 * new networking state definitions can be appended here
 * (after being declared as networking enum state_type)
 */
#define FINISH_NETWORKING_STATE_DEFINITIONS(state_array) \
			NET_STATE_DEF(state_array, st_net_entry, \
					"networking entry state", \
					TRANS_NET_ENTRY, stl_unknown) \
			NET_STATE_DEF(state_array, st_net_firstMixInit,\
					"first mix initialized", \
					TRANS_NET_FIRST_MIX_INIT, stl_warning) \
			NET_STATE_DEF(state_array, st_net_firstMixConnectedToNext, \
					"first mix connected to next mix", \
					  TRANS_NET_FIRST_MIX_CONNECTED_TO_NEXT, stl_warning) \
			NET_STATE_DEF(state_array, st_net_firstMixOnline, \
					"first mix online", \
					 TRANS_NET_FIRST_MIX_ONLINE, stl_ok) \
			NET_STATE_DEF(state_array, st_net_middleMixInit, \
					"middle mix initialized", \
					TRANS_NET_MIDDLE_MIX_INIT, stl_warning) \
			NET_STATE_DEF(state_array, st_net_middleMixConnectedToPrev, \
					"middle mix connected to previous mix", \
					TRANS_NET_MIDDLE_MIX_CONNECTED_TO_PREV, stl_warning) \
			NET_STATE_DEF(state_array, st_net_middleMixConnectedToNext, \
					"middle mix connected to next mix", \
					TRANS_NET_MIDDLE_MIX_CONNECTED_TO_NEXT, stl_warning) \
			NET_STATE_DEF(state_array, st_net_middleMixOnline, \
					"middle mix online", \
					TRANS_NET_MIDDLE_MIX_ONLINE, stl_ok) \
			NET_STATE_DEF(state_array, st_net_lastMixInit, \
					"last mix initialized", \
					TRANS_NET_LAST_MIX_INIT, stl_warning) \
			NET_STATE_DEF(state_array, st_net_lastMixConnectedToPrev, \
					"last mix connected to previous mix", \
		  			TRANS_NET_LAST_MIX_CONNECTED_TO_PREV, stl_warning) \
		  	NET_STATE_DEF(state_array, st_net_lastMixOnline, \
		  			"last mix online", \
					TRANS_NET_LAST_MIX_ONLINE, stl_ok)

/* networking events descriptions */
#define FINISH_NETWORKING_EVENT_DEFINITIONS(event_array) \
			NET_EVENT_DEF(event_array, ev_net_firstMixInited, \
					"first mix initialization finished") \
			NET_EVENT_DEF(event_array, ev_net_middleMixInited, \
					"middle mix initialization finished") \
			NET_EVENT_DEF(event_array, ev_net_lastMixInited, \
					"last mix initialization finished") \
			NET_EVENT_DEF(event_array, ev_net_prevConnected, \
					"connection to previous mix established") \
			NET_EVENT_DEF(event_array, ev_net_nextConnected, \
					"connection to next mix established") \
			NET_EVENT_DEF(event_array, ev_net_keyExchangePrevSuccessful, \
					"key exchange with previous mix successful") \
			NET_EVENT_DEF(event_array, ev_net_keyExchangeNextSuccessful, \
		  			"key exchange with next mix successful") \
  			NET_EVENT_DEF(event_array, ev_net_keyExchangePrevFailed, \
  					"key exchange with previous mix failed") \
			NET_EVENT_DEF(event_array, ev_net_keyExchangeNextFailed, \
		  			"key exchange with next mix failed") \
		  	NET_EVENT_DEF(event_array, ev_net_prevConnectionClosed, \
					"connection to previous mix closed") \
			NET_EVENT_DEF(event_array, ev_net_nextConnectionClosed, \
					"connection to next mix closed")

#ifdef PAYMENT
	/* payment states description and transition assignment
	 * new payment state definitions can be appended here
	 * (after being declared as payment enum state_type)
	 */
	#define FINISH_PAYMENT_STATE_DEFINITIONS(state_array) \
				PAY_STATE_DEF(state_array, st_pay_entry, \
						"payment entry state", \
						TRANS_PAY_ENTRY, stl_unknown) \
				PAY_STATE_DEF(state_array, st_pay_aiInit, \
						"accounting instance initialized", \
						TRANS_PAY_AI_INIT, stl_ok) \
				PAY_STATE_DEF(state_array, st_pay_aiShutdown, \
						"accounting instance shutdown", \
						TRANS_PAY_AI_SHUTDOWN, stl_critical) \
				PAY_STATE_DEF(state_array, st_pay_biAvailable, \
						"payment instance available", \
						TRANS_PAY_BI_AVAILABLE, stl_ok) \
				PAY_STATE_DEF(state_array, st_pay_biUnreachable, \
						"payment instance temporarily unreachable", \
						TRANS_PAY_BI_UNREACHABLE, stl_warning) \
				PAY_STATE_DEF(state_array, st_pay_biPermanentlyUnreachable, \
						"payment instance permanently unreachable", \
						TRANS_PAY_BI_PERMANENTLY_UNREACHABLE, stl_critical) \
				PAY_STATE_DEF(state_array, st_pay_dbError, \
						"pay DB cannot be accessed ", \
						TRANS_PAY_DB_ERROR, stl_critical) \
				PAY_STATE_DEF(state_array, st_pay_dbErrorBiUnreachable, \
						"pay DB cannot be accessed and Payment Instance temporarily unreachable", \
						TRANS_PAY_DB_ERROR_BI_UNREACHABLE, stl_critical) \
				PAY_STATE_DEF(state_array, st_pay_dbErrorBiPermanentlyUnreachable, \
						"pay DB cannot be accessed and Payment Instance permanently unreachable", \
						TRANS_PAY_DB_ERROR_BI_PERMANENTLY_UNREACHABLE, stl_critical)

	/* payment events descriptions */
	#define FINISH_PAYMENT_EVENT_DEFINITIONS(event_array) \
				PAY_EVENT_DEF(event_array, ev_pay_aiInited, \
						"accounting instance initialization finished") \
				PAY_EVENT_DEF(event_array, ev_pay_aiShutdown, \
			  			"accounting instance shutdown finished") \
	  			PAY_EVENT_DEF(event_array, ev_pay_biConnectionSuccess, \
  						"successfully connected to payment instance") \
				PAY_EVENT_DEF(event_array, ev_pay_biConnectionFailure, \
  						"connection to payment instance failed") \
				PAY_EVENT_DEF(event_array, ev_pay_biConnectionCriticalSubseqFailures, \
  						"connection to payment instance failed permanently") \
				PAY_EVENT_DEF(event_array, ev_pay_dbConnectionSuccess, \
  						"pay DB access successful") \
				PAY_EVENT_DEF(event_array, ev_pay_dbConnectionFailure, \
  						"pay DB access failed")
#else
	#define FINISH_PAYMENT_STATE_DEFINITIONS(state_array)
	#define FINISH_PAYMENT_EVENT_DEFINITIONS(event_array)
#endif /* PAYMENT */

/* system states description and transition assignment
 * new system state definitions can be appended here
 * (after being declared as system enum state_type)
 */
#define FINISH_SYSTEM_STATE_DEFINITIONS(state_array) \
			SYS_STATE_DEF(state_array, st_sys_entry, \
					"system entry state", \
					TRANS_SYS_ENTRY, stl_unknown) \
			SYS_STATE_DEF(state_array, st_sys_initializing, \
					"mix is initializing", \
					TRANS_SYS_INITIALIZING, stl_warning) \
			SYS_STATE_DEF(state_array, st_sys_operating, \
					"mix is operating", \
					TRANS_SYS_OPERATING, stl_ok) \
			SYS_STATE_DEF(state_array, st_sys_restarting, \
					"mix is restarting", \
					TRANS_SYS_RESTARTING, stl_warning) \
			SYS_STATE_DEF(state_array, st_sys_shuttingDown, \
					"mix is shutting down", \
					TRANS_SYS_SHUTTING_DOWN, stl_critical) \
			SYS_STATE_DEF(state_array, st_sys_ShuttingDownAfterSegFault, \
					"mix is shutting down due to a segmentation fault!", \
					TRANS_SYS_SHUTTING_DOWN_AFTER_SEG_FAULT, stl_critical)

/* payment events descriptions */
#define FINISH_SYSTEM_EVENT_DEFINITIONS(event_array) \
			SYS_EVENT_DEF(event_array, ev_sys_start, \
					"mix startup") \
			SYS_EVENT_DEF(event_array, ev_sys_enterMainLoop, \
					"mix entering main loop") \
			SYS_EVENT_DEF(event_array, ev_sys_leavingMainLoop, \
					"mix leaving main loop") \
			SYS_EVENT_DEF(event_array, ev_sys_sigTerm, \
					"mix caught SIG_TERM") \
			SYS_EVENT_DEF(event_array, ev_sys_sigInt, \
					"mix caught SIG_INT") \
			SYS_EVENT_DEF(event_array, ev_sys_sigSegV, \
					"mix caught SIG_SEGV")

/* convenience macros for special status state and event definitions */
#define NET_STATE_DEF(state_array, state_type, description, transitions, stateLevel) \
			STATE_DEF(state_array, stat_networking, state_type, description, transitions, stateLevel)

#define NET_EVENT_DEF(event_array, event_type, description) \
			EVENT_DEF(event_array, stat_networking, event_type, description)

#ifdef PAYMENT
	#define PAY_STATE_DEF(state_array, state_type, description, transitions, stateLevel) \
				STATE_DEF(state_array, stat_payment, state_type, description, transitions, stateLevel)

	#define PAY_EVENT_DEF(event_array, event_type, description) \
				EVENT_DEF(event_array, stat_payment, event_type, description)
#else
	#define PAY_STATE_DEF(state_array, state_type, description, transitions, stateLevel)
	#define PAY_EVENT_DEF(event_array, event_type, description)
#endif

#define SYS_STATE_DEF(state_array, state_type, description, transitions, stateLevel) \
			STATE_DEF(state_array, stat_system, state_type, description, transitions, stateLevel)

#define SYS_EVENT_DEF(event_array, event_type, description) \
			EVENT_DEF(event_array, stat_system, event_type, description)

/* This macro is used for assigning state description and state transitions
 * to the initialized states in the function "initStates()"
 */
#define STATE_DEF(state_array, status_type, state_type, description, transitions, stateLevel) \
			state_array[status_type][state_type]->st_description = (char *) description; \
			state_array[status_type][state_type]->st_transitions = transitions; \
			state_array[status_type][state_type]->st_stateLevel = stateLevel;
/* Same for events description assignment */
#define EVENT_DEF(event_array, status_type, event_type, description) \
			event_array[status_type][event_type]->ev_description  = (char *) description;

/**
 * a convenience function for easily defining state transitions
 * @param s_type the status type of the state for which the transitions
 * 		  are to be defined
 * @param transitionCount the number of transitions to define
 * @param ... an event_type (of type event_type_t) followed by a
 * 		  state transition (of type transition_t)
 * 		  IMPORTANT: an event type MUST be followed by a state transition!
 * @return pointer to the array where the transitions are stored, (which of course
 * 			has to be disposed by delete[] when reference is not needed anymore)!
 */
transition_t *defineTransitions(status_type_t s_type, int transitionCount, ...);

/** NETWORKING STATE TRANSITIONS **/

/* transitions for the networking entry state: */
#define TRANS_NET_ENTRY \
	(defineTransitions(stat_networking, 3, \
			ev_net_firstMixInited, st_net_firstMixInit, \
			ev_net_middleMixInited, st_net_middleMixInit, \
			ev_net_lastMixInited, st_net_lastMixInit))

/* transitions for st_net_firstMixInit: */
#define TRANS_NET_FIRST_MIX_INIT \
	(defineTransitions(stat_networking, 1, \
			ev_net_nextConnected, st_net_firstMixConnectedToNext))

/* transitions for st_net_firstMixConnectedToNext: */
#define TRANS_NET_FIRST_MIX_CONNECTED_TO_NEXT \
	(defineTransitions(stat_networking, 2, \
			ev_net_keyExchangeNextSuccessful, st_net_firstMixOnline, \
			ev_net_nextConnectionClosed, st_net_firstMixInit))

/* transitions for st_net_firstMixOnline: */
#define TRANS_NET_FIRST_MIX_ONLINE \
	(defineTransitions(stat_networking, 1, \
			ev_net_nextConnectionClosed, st_net_firstMixInit))

/* transitions for st_net_middleMixInit: */
#define TRANS_NET_MIDDLE_MIX_INIT \
	(defineTransitions(stat_networking, 1, \
			ev_net_prevConnected, st_net_middleMixConnectedToPrev))

/* transitions for st_net_middleMixConnectedToPrev: */
#define TRANS_NET_MIDDLE_MIX_CONNECTED_TO_PREV \
	(defineTransitions(stat_networking, 2, \
			ev_net_nextConnected, st_net_middleMixConnectedToNext, \
			ev_net_prevConnectionClosed, st_net_middleMixInit))

/* transitions for st_net_middleMixConnectedToNext: */
#define TRANS_NET_MIDDLE_MIX_CONNECTED_TO_NEXT \
	(defineTransitions(stat_networking, 4, \
			ev_net_keyExchangeNextFailed, st_net_middleMixInit, \
			ev_net_keyExchangePrevFailed, st_net_middleMixInit, \
			ev_net_keyExchangeNextSuccessful, st_net_middleMixConnectedToNext, \
			ev_net_keyExchangePrevSuccessful, st_net_middleMixOnline))

/* transitions for st_net_middleMixOnline: */
#define TRANS_NET_MIDDLE_MIX_ONLINE \
	(defineTransitions(stat_networking, 2, \
			ev_net_prevConnectionClosed, st_net_middleMixInit, \
			ev_net_nextConnectionClosed, st_net_middleMixInit))

/* transitions for st_net_lastMixInit: */
#define TRANS_NET_LAST_MIX_INIT \
	(defineTransitions(stat_networking, 1, \
			ev_net_prevConnected, st_net_lastMixConnectedToPrev))

/* transitions for st_net_lastMixConnectedToPrev: */
#define TRANS_NET_LAST_MIX_CONNECTED_TO_PREV \
	(defineTransitions(stat_networking, 2, \
			ev_net_keyExchangePrevFailed, st_net_lastMixInit, \
			ev_net_keyExchangePrevSuccessful, st_net_lastMixOnline))


/* transitions for st_net_lastMixOnline: */
#define TRANS_NET_LAST_MIX_ONLINE \
	(defineTransitions(stat_networking, 1, \
			ev_net_prevConnectionClosed, st_net_lastMixInit))

#ifdef PAYMENT

/** PAYMENT STATE TRANSITIONS **/

/* transitions for st_pay_entry */
#define TRANS_PAY_ENTRY \
	(defineTransitions(stat_payment, 1, \
			ev_pay_aiInited, st_pay_aiInit))

/* transitions for st_pay_aiInit */
#define TRANS_PAY_AI_INIT \
	(defineTransitions(stat_payment, 4, \
			ev_pay_aiShutdown, st_pay_aiShutdown, \
			ev_pay_biConnectionSuccess, st_pay_biAvailable, \
			ev_pay_biConnectionFailure, st_pay_biUnreachable, \
			ev_pay_dbConnectionFailure, st_pay_dbError))

/* transitions for st_pay_aiShutdown */
#define TRANS_PAY_AI_SHUTDOWN \
	(defineTransitions(stat_payment, 1, \
			ev_pay_aiInited, st_pay_aiInit))

/* transitions for st_pay_biAvailable */
#define TRANS_PAY_BI_AVAILABLE \
	(defineTransitions(stat_payment, 3, \
			ev_pay_aiShutdown, st_pay_aiShutdown, \
			ev_pay_biConnectionFailure, st_pay_biUnreachable, \
			ev_pay_dbConnectionFailure, st_pay_dbError))

/* transitions for st_pay_biUnreachable */
#define TRANS_PAY_BI_UNREACHABLE \
	(defineTransitions(stat_payment, 4, \
			ev_pay_aiShutdown, st_pay_aiShutdown, \
			ev_pay_biConnectionSuccess, st_pay_biAvailable, \
			ev_pay_biConnectionCriticalSubseqFailures, st_pay_biPermanentlyUnreachable, \
			ev_pay_dbConnectionFailure, st_pay_dbErrorBiUnreachable))

/* transitions for st_pay_biPermanentlyUnreachable */
#define TRANS_PAY_BI_PERMANENTLY_UNREACHABLE \
	(defineTransitions(stat_payment, 3, \
			ev_pay_aiShutdown, st_pay_aiShutdown, \
			ev_pay_biConnectionSuccess, st_pay_biAvailable, \
			ev_pay_dbConnectionFailure, st_pay_dbErrorBiPermanentlyUnreachable))

/* transitions for st_pay_dbError */
#define TRANS_PAY_DB_ERROR \
	(defineTransitions(stat_payment, 2, \
			ev_pay_aiShutdown, st_pay_aiShutdown, \
			ev_pay_dbConnectionSuccess, st_pay_aiInit))

/* transitions for st_pay_dbErrorBiUnreachable */
#define TRANS_PAY_DB_ERROR_BI_UNREACHABLE \
	(defineTransitions(stat_payment, 2, \
			ev_pay_aiShutdown, st_pay_aiShutdown, \
			ev_pay_dbConnectionSuccess, st_pay_biUnreachable))

/* transitions for st_pay_dbErrorBiPermanentlyUnreachable */
#define TRANS_PAY_DB_ERROR_BI_PERMANENTLY_UNREACHABLE \
	(defineTransitions(stat_payment, 2, \
			ev_pay_aiShutdown, st_pay_aiShutdown, \
			ev_pay_dbConnectionSuccess, st_pay_biPermanentlyUnreachable))

#endif /* PAYMENT */

/** SYSTEM STATE TRANSITIONS **/

/* transitions for st_sys_entry */
#define TRANS_SYS_ENTRY \
	(defineTransitions(stat_system, 1, \
			ev_sys_start, st_sys_initializing))

/* transitions for st_sys_initializing */
#define TRANS_SYS_INITIALIZING \
	(defineTransitions(stat_system, 4, \
			ev_sys_enterMainLoop, st_sys_operating, \
			ev_sys_sigTerm, st_sys_shuttingDown, \
			ev_sys_sigInt, st_sys_shuttingDown, \
			ev_sys_sigSegV, st_sys_ShuttingDownAfterSegFault))

/* transitions for st_sys_operating */
#define TRANS_SYS_OPERATING \
	(defineTransitions(stat_system, 4, \
			ev_sys_leavingMainLoop, st_sys_restarting, \
			ev_sys_sigTerm, st_sys_shuttingDown, \
			ev_sys_sigInt, st_sys_shuttingDown, \
			ev_sys_sigSegV, st_sys_ShuttingDownAfterSegFault))

/* transitions for st_sys_restarting */
#define TRANS_SYS_RESTARTING \
	(defineTransitions(stat_system, 4, \
			ev_sys_enterMainLoop, st_sys_operating, \
			ev_sys_sigTerm, st_sys_shuttingDown, \
			ev_sys_sigInt, st_sys_shuttingDown, \
			ev_sys_sigSegV, st_sys_ShuttingDownAfterSegFault))

/* transitions for st_sys_shuttingDown */
#define TRANS_SYS_SHUTTING_DOWN \
	(defineTransitions(stat_system, 1, \
			ev_sys_sigSegV, st_sys_ShuttingDownAfterSegFault))

/* transitions for st_sys_ShuttingDownAfterSegFault */
#define TRANS_SYS_SHUTTING_DOWN_AFTER_SEG_FAULT \
	(defineTransitions(stat_system, 0))

#endif /* MONITORING_DEFS_H_ */
