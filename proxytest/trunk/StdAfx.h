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
//

#if !defined(AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_)
#define AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_

#define MIX_VERSION "00.03.66"

#if defined(DEBUG)|| defined(_DEBUG)
	#undef DEBUG
	#undef _DEBUG
	#define DEBUG
	#define _DEBUG
#endif
//#define LOG_CHANNEL
//#define LOG_PACKET_TIMES //computes statistics about the processing time each packet needs
//#define COMPRESSED_LOGS
//#define DO_TRACE
//#define PSEUDO_LOG
//#define DELAY_CHANNELS //to enable max channel bandwidth
//#define HAVE_EPOLL //define if you have epoll support on your (Linux) system
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
		#define DELAY_BUCKET_GROW_INTERVALL (1/DELAY_CHANNEL_KBYTE_PER_SECOND) //Time in ms
		#define DELAY_BUCKET_GROW PAYLOAD_SIZE //Grow in bytes
	#endif	
#endif
//#define LOG_CRIME
//#define PAYMENT //to enable payment support, now use configure --enable-payment..
//#define NO_PARKING //to disable control flow
//#define NO_LOOPACCEPTUSER //to disable user accept thread for First Mix

//#define USE_POOL
//#define FIRST_MIX_SYMMETRIC //to enable use of only symmetric encryption for first mix
//#define NEW_MIX_TYPE // to enable the new 1:x mix protocol
//#define WITH_CONTROL_CHANNELS //enable Control channels
//#define WITH_CONTROL_CHANNELS_TEST //enable a Test Channel

//#define WITH_TIMESTAMP // Add timestamps to the channel-open packets.
#define TIMESTAMP_SIZE 0

//Some constants
#define MAX_POLLFD 8192 //How many sockets to support at max

#define FIRST_MIX_RECEIVE_SYM_KEY_FROM_JAP_TIME_OUT 30000 //Timout in waiting for login information to receive from JAP (10 seconds)
#define LAST_MIX_TO_PROXY_CONNECT_TIMEOUT 2000 //Connection timeout for last mix to proxy connections 2 Seconds...
#define LAST_MIX_TO_PROXY_SEND_TIMEOUT (UINT32)5000 //5 Seconds...
#define MIX_TO_INFOSERVICE_TIMEOUT 10000 //How long to wait than communicating with the InfoService? (ms)
#define NUM_LOGIN_WORKER_TRHEADS 50 //How many working threads for login ??
#define MAX_LOGIN_QUEUE 500 //how many waiting entries in the login queue ??

#define MAX_USER_SEND_QUEUE 100000 //How many bytes could be in each User's send queue, before we suspend the belonging channels

#ifndef MIX_POOL_SIZE
	#define MIX_POOL_SIZE 10  //packets in the Mix pool
#endif
#define MIX_POOL_TIMEOUT 200 //home long to wait (in ms) before a dummy is put in the pool
#define DUMMY_CHANNEL 0

#define FM_PACKET_STATS_LOG_INTERVALL 1 //Intervall in Minutes for loggin packet stats for the first Mix
#define LM_PACKET_STATS_LOG_INTERVALL 1 //Intervall in Minutes for loggin packet stats for the last Mix
#if defined(LOG_CHANNEL) &&!defined(LOG_PACKET_TIMES)
	#define LOG_PACKET_TIMES
#endif

#define MIX_CASCADE_PROTOCOL_VERSION_0_5 5  //with control channels
#define MIX_CASCADE_PROTOCOL_VERSION_0_4 4  //symmetric communication to first mix
#define MIX_CASCADE_PROTOCOL_VERSION_0_3 3 //with reply detection
#define MIX_CASCADE_PROTOCOL_VERSION_0_2 2 //noraml protocol

#ifdef PAYMENT
	#define WITH_CONTROL_CHANNELS
#endif
#ifdef WITH_CONTROL_CHANNELS
	#define MIX_CASCADE_PROTOCOL_VERSION "0.5"
#elif defined(FIRST_MIX_SYMMETRIC)
	#define MIX_CASCADE_PROTOCOL_VERSION "0.4"
#else
	#ifdef WITH_TIMESTAMP
		// Enable replay detection. Requires WITH_TIMESTAMP
		#define REPLAY_DETECTION
		#undef TIMESTAMP_SIZE
		#define TIMESTAMP_SIZE 	2
		#define MIX_CASCADE_PROTOCOL_VERSION "0.3"
	#else
		#define MIX_CASCADE_PROTOCOL_VERSION "0.2"
	#endif
#endif

#if defined (_WIN32) &&!defined(__CYGWIN__)
		//For Visual C++    #if defined(_MSC_VER)
		//For Borland C++    #if defined(__BCPLUSPLUS__)
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
		#ifdef MSC_VER
			#define ftime _ftime
			#define timeb _timeb
		#endif
		#include <malloc.h>
		#define GET_NET_ERROR (WSAGetLastError())
		#define GET_NET_ERROR_STR(x) ("Unknown error")
		#define ERR_INTERN_TIMEDOUT WSAETIMEDOUT
		#define ERR_INTERN_CONNREFUSED WSAECONNREFUSED
		#define ERR_INTERN_WOULDBLOCK	WSAEWOULDBLOCK
		#define ERR_INTERN_SOCKET_CLOSED WSAENOTSOCK
		#define MSG_DONTWAIT 0
		#define O_NONBLOCK 0
		#define HAVE_VSNPRINTF
		#define HAVE_SNPRINTF
		#define vsnprintf _vsnprintf
		#define snprintf _snprintf
		#define atoll _atoi64
		#define HAVE_ATOLL
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
		#define HAVE_ATOLL
		#define HAVE_POLL
		#ifndef __linux
			#define HAVE_TCP_KEEPALIVE
		#endif
		#ifdef __sgi
			#undef HAVE_TCP_KEEPALIVE
		#endif
		#ifdef __FreeBSD__
			#undef HAVE_TCP_KEEPALIVE
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
#endif


//Error constants...

#define E_SUCCESS 0
#define E_UNKNOWN -1
#define E_UNSPECIFIED -100 // A Parameter was not specified/not set
#define E_SPACE -101//there was not enough memory (or space in a buffer)
#define E_QUEUEFULL -200 // If a Send Queue contains more data then a defined number
#define E_AGAIN -300 //If something was'nt completed und should request again later..
#define E_TIMEDOUT -301 //An opertion has timed out
#define E_SOCKETCLOSED -302 //An operation which required an open socket uses a closed socket
#define E_SOCKET_LISTEN -303 //An error occured during listen
#define E_SOCKET_ACCEPT -304 //An error occured during accept
#define E_SOCKET_BIND -305 //An error occured during bind
#define E_UNKNOWN_HOST -400 // A hostname could not be resolved
#define E_FILE_OPEN -500 //Error in opening a file
#define E_FILE_READ -501 //Error in opening a file
#define E_XML_PARSE -600 //Error in parsing XML
#define E_NOT_CONNECTED -700 //Something is not connected that should be 
														// (like a TCP/IP connection or a database connection)
#define E_NOT_FOUND -701 //Something was not found
#define E_INVALID -800 // sth is invalid (e.g. signature verifying)

#include <assert.h>

#include <pthread.h>
//#include <semaphore.h>
#define THREAD_RETURN void*
#define THREAD_RETURN_ERROR return(NULL)
#define THREAD_RETURN_SUCCESS return (NULL)


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
#include "popt/system.h"
#include "popt/popt.h"
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/asn1.h>
#include <openssl/pkcs12.h>
#include <openssl/x509v3.h>

//For DOM
#include <util/XercesDefs.hpp>
#include <util/PlatformUtils.hpp>
#include <util/XMLString.hpp>
#include <util/XMLUniDefs.hpp>
#include <framework/XMLFormatter.hpp>
#include <util/TranscodingException.hpp>
#include <framework/MemBufInputSource.hpp>

#if (XERCES_VERSION_MAJOR >1)
	#include <dom/deprecated/DOM.hpp>
	#include <dom/deprecated/DOMParser.hpp>
#else
	#include <dom/DOM.hpp>
	#include <parsers/DOMParser.hpp>
#endif

#if (_XERCES_VERSION >= 20200)
    XERCES_CPP_NAMESPACE_USE
#endif

//For large file support
#ifndef O_LARGEFILE
	#define O_LARGEFILE 0
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
#ifdef PAYMENT_SUPPORT
#include <mysql.h>
#endif

//For Payment
#ifdef PAYMENT
	#include <postgresql/libpq-fe.h>
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
#define MIX_VERSION_INFO "Mix-Version: " MIX_VERSION "\nUsing: " OPENSSL_VERSION_TEXT "\nUsing Xerces-C: " MY_XERCES_VERSION "\n"

#include "basetypedefs.h"
#include "typedefs.hpp"
#endif // !defined(AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_)
