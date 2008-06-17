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

// stdafx.h : Include-Datei fuer Standard-System-Include-Dateien,
//  oder projektspezifische Include-Dateien, die haeufig benutzt, aber
//      in unregelmaessigen Abstaenden geaendert werden.

#if !defined(AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_)
#define AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_

#define MIX_VERSION "00.08.13"

#include "doxygen.h"

#if defined(DEBUG)|| defined(_DEBUG)
	#undef DEBUG
	#undef _DEBUG
	#define DEBUG
	#define _DEBUG
#endif

//#define LOG_TRAFFIC_PER_USER //Log detail for traffic per user
//#define LOG_CHANNEL //Log detail for traffic per cahnnel
//#define LOG_PACKET_TIMES //computes statistics about the processing time each packet needs
//#define LOG_DIALOG
//#define COMPRESSED_LOGS
//#define DO_TRACE
//#define PSEUDO_LOG
//#define DELAY_CHANNELS //to enable max channel bandwidth
//#define DELAY_USERS //to enable max per user bandwidth
//#define DELAY_CHANNELS_LATENCY //to enable min latency per channel
//#define HAVE_EPOLL //define if you have epoll support on your (Linux) system
//#define MXML_DOM //define this if you wnat to use the Mix-XML library (www.minixml.org) instead of the default Xerces-C library
//#define COUNTRY_STATS //collect stats about countries users come from
//#define ONLY_LOCAL_PROXY //define to build only the local proxy (aka JAP)
/* LERNGRUPPE: define this to get dynamic mixes */
//#define DYNAMIC_MIX
//#define SDTFA // specific logic needed by SDTFA, http://www.sdtfa.com

//#define LASTMIX_CHECK_MEMORY // only for internal debugging purpose 

//#define PRINT_THREAD_STACK_TRACE //Usefull for debugging output of stack trace if mix dies...
#if !defined(PRINT_THREAD_STACK_TRACE) && defined (DEBUG)&& ! defined(ONLY_LOCAL_PROXY)
	#define PRINT_THREAD_STACK_TRACE
#endif

#if defined(LOG_DIALOG)
	#ifndef LOG_CHANNEL
		#define LOG_CHANNEL
	#endif
	#ifndef COUNTRY_STATS
		#define COUNTRY_STATS
	#endif
#endif
#if (defined(LOG_CHANNEL)) && !defined(LOG_TRAFFIC_PER_USER)
	#define LOG_TRAFFIC_PER_USER
#endif

#ifdef COUNTRY_STATS
	#define LOG_COUNTRIES_INTERVALL 6 //how often to log the country stats (multiplied by 10 seconds)
#endif
#ifdef DELAY_CHANNELS
	#ifndef DELAY_CHANNEL_TRAFFIC
		#define DELAY_CHANNEL_TRAFFIC 10000 //Traffic in bytes after which (download direction) the channel is delayed
	#endif
	//Delay is at the moment constant and calculate as
	// 1000/DELAY_BUCKET_GROW_INTERVALL*DELAY_BUCKET_GROW bytes/s
	#ifndef DELAY_CHANNEL_KBYTE_PER_SECOND
		#define DELAY_BUCKET_GROW_INTERVALL 100 //Time in ms
		#define DELAY_BUCKET_GROW PAYLOAD_SIZE //Grow in bytes
	//so we have around 10 KByte/s at the moment
	#else
		#define DELAY_BUCKET_GROW_INTERVALL (1000/DELAY_CHANNEL_KBYTE_PER_SECOND) //Time in ms
		#define DELAY_BUCKET_GROW PAYLOAD_SIZE //Grow in bytes
	#endif
#endif
#ifdef DELAY_USERS
	#ifndef DELAY_USERS_TRAFFIC
		#define DELAY_USERS_TRAFFIC 100 //Traffic in packets after which (download direction) the user is delayed
	#endif
	//Delay is at the moment constant and calculate as
	// 1000/DELAY_BUCKET_GROW_INTERVALL*DELAY_BUCKET_GROW bytes/s
	#ifndef DELAY_USERS_PACKETS_PER_SECOND
		#define DELAY_USERS_BUCKET_GROW_INTERVALL 100 //Time in ms
		#define DELAY_USERS_BUCKET_GROW 10 //Grow in packets
	//so we have around 10 KByte/s at the moment
	#else
		#define DELAY_USERS_BUCKET_GROW_INTERVALL 1000 //Time in ms
		#define DELAY_USERS_BUCKET_GROW DELAY_USERS_PACKETS_PER_SECOND //Grow in bytes
	#endif
#endif
#ifdef DELAY_CHANNELS_LATENCY
	#define DELAY_CHANNEL_LATENCY 10000 //min latency defaults to 10 second
#endif

#if defined LASTMIX_CHECK_MEMORY && ! defined(QUEUE_SIZE_LOG)
	#define QUEUE_SIZE_LOG
#endif
//#define LOG_CRIME
//#define PAYMENT //to enable payment support, now use configure --enable-payment..
//#define NO_PARKING //to disable control flow
//#define NO_LOOPACCEPTUSER //to disable user accept thread for First Mix

//#define USE_POOL
//#define NEW_MIX_TYPE // to enable the new 1:x mix protocol
//#define WITH_CONTROL_CHANNELS_TEST //enable a Test control Channel
//#define NEW_FLOW_CONTROL //enable for the new flow control mechanism

//#define REPLAY_DETECTION // enable to prevent replay of mix packets
#define REPLAY_TIMESTAMP_PROPAGATION_INTERVALL 1 //How often (in minutes) should the current replay timestamps be propagate

#define KEEP_ALIVE_TRAFFIC_RECV_WAIT_TIME  75000 //How long to wait for a Keep-Alive (or any other packet)
																							       //before we believe that the connection is broken (in ms)
#define KEEP_ALIVE_TRAFFIC_SEND_WAIT_TIME 60000 //How long to wait before we sent a dummy a Keep-Alive-Traffic


//#define SSL_HACK //???


#if defined(PAYMENT) && ! defined(SSL_HACK)
	#define SSL_HACK
#endif

//#define DATABASE_PERFORMANCE_TEST //to performe a performance test of the replay db
//Some constants
#define MAX_POLLFD 8192 //How many sockets to support at max

#define CLIENTS_PER_IP 10 //how many jap connections per IP are allowed?
#define CHANNELS_PER_CLIENT 50 //how many channels per jap client are allowed?

#define FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT 40000 //Timout in waiting for login information to receive from JAP (10 seconds)
#define LAST_MIX_TO_PROXY_CONNECT_TIMEOUT 2000 //Connection timeout for last mix to proxy connections 2 Seconds...
#define AI_LOGIN_SO_TIMEOUT (UINT32) 10000 //5 Seconds...
#define LAST_MIX_TO_PROXY_SEND_TIMEOUT (UINT32)5000 //5 Seconds...
#define MIX_TO_INFOSERVICE_TIMEOUT 30000 //How long to wait when communicating with the InfoService? (ms)
#define NUM_LOGIN_WORKER_TRHEADS 10//How many working threads for login *do not change this until you really know that you are doing!* ??
#define MAX_LOGIN_QUEUE 500 //how many waiting entries in the login queue *do not change this until you really know that you are doing!*??

#define MAX_USER_SEND_QUEUE 100000 //How many bytes could be in each User's send queue, before we suspend the belonging channels
#define MAX_DATA_PER_CHANNEL 100000
#define USER_SEND_BUFFER_RESUME 10000

#define PAYMENT_ACCOUNT_CERT_TIMEOUT 180 //Timeout for receiving the Payment certificate in seconds
#define CLEANUP_THREAD_SLEEP_INTERVAL 60 //sleep interval for payment blocked ip list
#define BALANCE_REQUEST_TIMEOUT 60 //Timeout for Balance requests

#define FLOW_CONTROL_SENDME_HARD_LIMIT 95 //last mix stops sending after this unack packets
#define FLOW_CONTROL_SENDME_SOFT_LIMIT 80 //last mix sends request for 'SENDME' after this unack packets

#define MAX_READ_FROM_PREV_MIX_QUEUE_SIZE 10000000
#define MAX_READ_FROM_NEXT_MIX_QUEUE_SIZE 10000000 //How many bytes could be in the incoming queue ??
#define MAX_MIXIN_SEND_QUEUE_SIZE 10000000
#define MAX_NEXT_MIX_QUEUE_SIZE 10000000

#define DEFAULT_INFOSERVICE "141.76.45.37"

#ifndef MIX_POOL_SIZE
	#define MIX_POOL_SIZE 10  //packets in the Mix pool
#endif
#define MIX_POOL_TIMEOUT 200 //home long to wait (in ms) before a dummy is put in the pool
#define DUMMY_CHANNEL 0

#define FM_PACKET_STATS_LOG_INTERVALL 1 //Intervall in Minutes for loggin packet stats for the first Mix
#define LM_PACKET_STATS_LOG_INTERVALL 1 //Intervall in Minutes for loggin packet stats for the last Mix


//#define MIX_CASCADE_PROTOCOL_VERSION_0_9 9  //with new payment protocol
#define MIX_CASCADE_PROTOCOL_VERSION_0_8 8  //with replay detection + control channels + first mix symmetric
#define MIX_CASCADE_PROTOCOL_VERSION_0_7 7  //with replay detection + control channels (obsolete)
#define MIX_CASCADE_PROTOCOL_VERSION_0_6 6  //with new flow control
#define MIX_CASCADE_PROTOCOL_VERSION_0_5 5  //with control channels (obsolte - allway with control channels)
#define MIX_CASCADE_PROTOCOL_VERSION_0_4 4  //symmetric communication to first mix -> new default protocol
#define MIX_CASCADE_PROTOCOL_VERSION_0_3 3 //with reply detection [deprecated - not in use anymore!]
#define MIX_CASCADE_PROTOCOL_VERSION_0_2 2 //old normal protocol

#ifdef REPLAY_DETECTION
	#define MIX_CASCADE_PROTOCOL_VERSION "0.81"
//#elif defined(PAYMENT)
	//#define MIX_CASCADE_PROTOCOL_VERSION "0.9"
#else
	#define MIX_CASCADE_PROTOCOL_VERSION "0.4"
#endif

#define PAYMENT_VERSION "2.0"

#if defined (_WIN32) &&!defined(__CYGWIN__)
	//For Visual C++    #if defined(_MSC_VER)
	//For Borland C++    #if defined(__BCPLUSPLUS__)
	#define _CRT_SECURE_NO_DEPRECATE
	#define _CRT_SECURE_NO_WARNINGS    
	#if _MSC_VER > 1000
		#pragma once
	#endif // _MSC_VER > 1000
	#define _WIN32_WINDOWS 0x0410
	#include <winsock2.h>
	#if defined(_MSC_VER) &&defined (_DEBUG)
		#include <crtdbg.h>
		#define HAVE_CRTDBG
	#endif
	#define socklen_t int
	#define MSG_NOSIGNAL 0
    #include <io.h>
	#include <conio.h>
	#include <sys/timeb.h>
	#include <process.h>
	#ifdef _MSC_VER
		#define ftime _ftime
		#define timeb _timeb
	#endif
	#include <malloc.h>
	#define GET_NET_ERROR (WSAGetLastError())
	#define GET_NET_ERROR_STR(x) ("Unknown error")
	#define RESETERROR errno=0;
	#define GETERROR (errno)
	#define ERR_INTERN_TIMEDOUT WSAETIMEDOUT
	#define ERR_INTERN_CONNREFUSED WSAECONNREFUSED
	#define ERR_INTERN_WOULDBLOCK	WSAEWOULDBLOCK
	#define ERR_INTERN_SOCKET_CLOSED WSAENOTSOCK
	#define MSG_DONTWAIT 0
	#define O_NONBLOCK 0
	#define HAVE_VSNPRINTF
	#define HAVE_SNPRINTF
	#if _MSC_VER <1500
		#define vsnprintf _vsnprintf
	#endif
	#define snprintf _snprintf
	#define getpid _getpid
	#define HAVE_PTHREAD_MUTEX_INIT
	#define HAVE_PTHREAD_COND_INIT
	#define HAVE_SEM_INIT
#else
	//__linux is not defined on power pc so we define our own __linux if __linux__ is defined
	#if defined(__linux__) && !defined(__linux)
		#define __linux
	#endif
	#if defined(CWDEBUG)
	  #include <libcw/sysd.h>
	  #include <libcw/debug.h>
	#endif

  #ifdef HAVE_CONFIG_H
		#include "config.h"
		#ifndef HAVE_SOCKLEN_T
			typedef int socklen_t;
		#endif
	#else
		#define HAVE_UNIX_DOMAIN_PROTOCOL
		#define HAVE_VSNPRINTF
		#define HAVE_SNPRINTF
		#define HAVE_POLL
		#define HAVE_O_SYNC
		#define HAVE_PTHREAD_MUTEX_INIT
		#define HAVE_PTHREAD_COND_INIT
		#define HAVE_SEM_INIT
		#ifndef __linux
			#define HAVE_TCP_KEEPALIVE
		#endif
		#ifdef __sgi
			#undef HAVE_TCP_KEEPALIVE
		#endif
		#ifdef __FreeBSD__
			#undef HAVE_TCP_KEEPALIVE
			#undef HAVE_O_SYNC
			#define HAVE_O_FSYNC
		#endif
		#ifdef __CYGWIN__
			#undef HAVE_TCP_KEEPALIVE
			#define MSG_DONTWAIT 0
		#endif
		#if !defined(__FreeBSD__)&&!defined(__linux)
    	typedef int socklen_t;
		#endif
    #ifndef O_BINARY
			#define O_BINARY 0
    #endif
    #ifndef MAX_PATH
			#define MAX_PATH 4096
    #endif
		#ifdef __sgi
			#undef HAVE_VSNPRINTF
			#undef HAVE_SNPRINTF
			#include <alloca.h>
		#endif
    #if !defined( __linux) &&!defined(__CYGWIN__)
    	#include <sys/filio.h>
    	#define MSG_NOSIGNAL 0
    #endif
	#endif //Have config.h

	#ifdef HAVE_FILIO
		#include <sys/filio.h>
	#endif
	#ifdef HAVE_POLL
		#include <poll.h>
	#endif
	#ifdef HAVE_EPOLL
		#include <sys/epoll.h>
	#endif
	#ifdef HAVE_MALLOC_H
	    #include <malloc.h>
	#endif
	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <pwd.h>
	#include <sys/un.h>
	#include <sys/time.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <strings.h>
	#include <syslog.h>
	#include <stdarg.h>
	#include <memory.h>
	#include <sys/resource.h>
	#include <sys/wait.h>
	#include <termios.h> 

	#include <ctype.h>
    typedef struct sockaddr SOCKADDR;
  typedef SOCKADDR* LPSOCKADDR;
  #define SOCKET int
  typedef struct hostent HOSTENT;
	#define ioctlsocket(a,b,c) ioctl(a,b,c)
  #define closesocket(s) close(s)
  #define SOCKET_ERROR -1
  #define INVALID_SOCKET -1
  #define SD_RECEIVE 0
  #define SD_SEND 1
  #define SD_BOTH 2
  #define GET_NET_ERROR (errno)
	#define GET_NET_ERROR_STR(x) (strerror(x))
	#define RESETERROR errno=0;
	#define GETERROR (errno)
	#define ERR_INTERN_TIMEDOUT ETIMEDOUT
	#define ERR_INTERN_CONNREFUSED ECONNREFUSED
	#define ERR_INTERN_WOULDBLOCK EAGAIN
	#define ERR_INTERN_SOCKET_CLOSED EBADF
	#ifndef INADDR_NONE
		#define INADDR_NONE -1
	#endif
  #ifndef AF_LOCAL
		#define AF_LOCAL AF_UNIX
  #endif
	#if !defined(HAVE_MSG_DONTWAIT)&&!defined(MSG_DONTWAIT)
		#define MSG_DONTWAIT 0
	#endif
#endif //WIn32 ?

#include "basetypedefs.h"

#include <assert.h>

#include <pthread.h>
//the following definition are just for threading support beside pthread
#undef USE_SEMAPHORE //normally we do not need semaphores
#ifdef HAVE_PTHREAD_COND_INIT
	#define HAVE_PTHREAD_CV //normally we use the pthread conditional variables
#endif
#ifdef HAVE_PTHREAD_MUTEX_INIT
	#define HAVE_PTHREAD_MUTEXES //normally we use the pthread mutexs
#endif

#if !defined(HAVE_PTHREAD_CV) || !defined (HAVE_PTHREAD_MUTEXES) //if we do not have pthread mutexes or cvs we emulate them with semphores
 #define USE_SEMAPHORE
#endif

#ifdef HAVE_SEM_INIT
	#define HAVE_PTHREAD_SEMAPHORE //normally we use pthread semaphores
#endif

#ifdef USE_SEMAPHORE
	#ifdef HAVE_PTHREAD_SEMAPHORE
		#include <semaphore.h>
	#endif
#endif

#ifdef OS_TUDOS
	#define Assert
	#define THREAD_RETURN void
	#define THREAD_RETURN_ERROR return
	#define THREAD_RETURN_SUCCESS return
	#include <l4/thread/thread.h>
	#include <l4/util/macros.h>
	#include <l4/env/errno.h>
	#undef Assert
	#undef ASSERT
#else
	#define THREAD_RETURN void*
	#define THREAD_RETURN_ERROR return(NULL)
	#define THREAD_RETURN_SUCCESS return (NULL)
#endif

#ifndef DEBUG
	#define ASSERT(cond,msg)
#else
	#define ASSERT(cond,msg) {if(!(cond)){CAMsg::printMsg(LOG_DEBUG,"ASSERT: %s (File: %s, Line: %u)\n",msg,__FILE__,__LINE__);}}
#endif


#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include "trio/trio.hpp"
#include "popt/system.h"
#include "popt/popt.h"
#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#ifndef ONLY_LOCAL_PROXY
	#include <openssl/asn1.h>
	#include <openssl/pkcs12.h>
	#include <openssl/x509v3.h>
	#include <openssl/ssl.h>
	#include <openssl/dsa.h>
	#include <openssl/sha.h>
	#include <openssl/md5.h>
#endif

//For DOM
#ifdef MXML_DOM
	#include <mxml.h>
	#include "xml/dom/mxml/mxmlDOM.hpp"
#else
#include <util/XercesDefs.hpp>
#include <util/PlatformUtils.hpp>
#include <util/XMLString.hpp>
#include <util/XMLUniDefs.hpp>
#include <framework/XMLFormatter.hpp>
#include <util/TranscodingException.hpp>
#include <framework/MemBufInputSource.hpp>

/*#if (XERCES_VERSION_MAJOR >1)
	#include <dom/deprecated/DOM.hpp>
	#include <dom/deprecated/DOMParser.hpp>
#else
	#include <dom/DOM.hpp>
	#include <parsers/DOMParser.hpp>
#endif
*/
#include <dom/DOM.hpp>
#include <parsers/XercesDOMParser.hpp>

#if (_XERCES_VERSION >= 20200)
    XERCES_CPP_NAMESPACE_USE
#endif
#endif //wich DOM-Implementation to use?

//For large file support
#ifndef O_LARGEFILE
	#define O_LARGEFILE 0
#endif

//O_SYNC defined ???
#ifndef HAVE_O_SYNC
	#ifdef HAVE_O_FSYNC
		#define O_SYNC O_FSYNC
	#else
		#define	O_SYNC 0
	#endif
#endif

//The min() macro
#ifndef min
	#define min(a,b) ((a<b)?(a):(b))
#endif

//The max() macro
#ifndef max
	#define max(a,b) ((a>b)?(a):(b))
#endif

//For MySQL
#if defined(COUNTRY_STATS)
    #ifdef HAVE_CONFIG_H
	#ifdef HAVE_MYSQL_MYSQL_H
	    #include <mysql/mysql.h>
	#else
	    #include <mysql.h>
	#endif
    #else //HAVE_CONFIG_H
	#include <mysql/mysql.h>
    #endif
#endif

//For Payment
#ifdef PAYMENT
	#ifdef HAVE_CONFIG_H
		#ifdef HAVE_POSTGRESQL_LIBPQ_FE_H
	    #include <postgresql/libpq-fe.h>
		#elif defined(HAVE_PGSQL_LIBPQ_FE_H)
	    #include <pgsql/libpq-fe.h>
		#else
	    #include <libpq-fe.h>
		#endif
  #elif defined(__FreeBSD__) ||defined (_WIN32)
		#include <libpq-fe.h>
			#else
		#include <postgresql/libpq-fe.h>
  #endif
#endif
//Compressed Logs
#ifdef COMPRESSED_LOGS
#include <zlib.h>
#endif

//For CPPUnit Test
#ifdef __UNIT_TEST__
	#include <cppunit/ui/text/TestRunner.h>
	#include <cppunit/extensions/HelperMacros.h>
	#include <cppunit/TestFixture.h>
#endif

//Mix Version Info as multiline String
#ifdef XERCES_FULLVERSIONDOT
	#define MY_XERCES_VERSION XERCES_FULLVERSIONDOT
#elif defined(Xerces_DLLVersionStr)
	#define MY_XERCES_VERSION Xerces_DLLVersionStr
#else
	#define MY_XERCES_VERSION "unknown"
#endif

#ifdef PAYMENT
	#define PAYMENT_VERSION_INFO " (payment)"
#else
	#define PAYMENT_VERSION_INFO
#endif

#define MIX_VERSION_INFO "Mix-Version: " MIX_VERSION PAYMENT_VERSION_INFO "\nUsing: " OPENSSL_VERSION_TEXT "\nUsing Xerces-C: " MY_XERCES_VERSION "\n"

#include "errorcodes.hpp"
#include "typedefs.hpp"
#include "controlchannelids.h"

#endif // !defined(AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_)
