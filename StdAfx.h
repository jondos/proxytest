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

#define NEW_KEY_PROTOCOL


#ifdef _WIN32
    #if _MSC_VER > 1000
    #pragma once
    #endif // _MSC_VER > 1000
		#include <crtdbg.h>
    #include <winsock2.h>
		#define socklen_t int
		#define MSG_NOSIGNAL 0 
    #include <io.h>
    #include <conio.h>
		#include <sys/timeb.h>
		#define GET_NET_ERROR (WSAGetLastError())
		#define ERR_INTERN_TIMEDOUT WSAETIMEDOUT
		#define ERR_INTERN_CONNREFUSED WSAECONNREFUSED
		#define ERR_INTERN_WOULDBLOCK	WSAEWOULDBLOCK
		#define ERR_INTERN_SOCKET_CLOSED WSAENOTSOCK
		#define MSG_DONTWAIT 0
		#define HAVE_VSNPRINTF
		#define vsnprintf _vsnprintf

		typedef signed long SINT32;
		typedef unsigned int UINT;
		typedef signed int SINT;
		typedef unsigned short UINT16;
		typedef signed short SINT16;
		typedef unsigned char UINT8;
		typedef signed char SINT8;
#else
	#if defined(CWDEBUG) &&defined(__cplusplus)
	    #include <libcw/sysd.h>
	    #include <libcw/debug.h>
	#endif

  #ifdef HAVE_CONFIG_H  
		#include "config.h"
		#ifndef HAVE_SOCKLEN_T 
			typedef int socklent_t;
		#endif
	#else
		#define HAVE_UNIX_DOMAIN_PROTOCOL
		#define HAVE_VSNPRINTF
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
			#include <alloca.h>
		#endif
    #ifndef __linux 
    	#include <sys/filio.h>
    	#define MSG_NOSIGNAL 0
    #endif
	#endif 

	#ifdef HAVE_FILIO
		#include <sys/filio.h>
	#endif			
	#ifdef HAVE_POLL
		#include <poll.h>
	#endif			
	#include <sys/ioctl.h>
	#include <sys/types.h>
  #include <sys/socket.h>
  #include <pwd.h>
  #include <sys/un.h>
	#include <sys/poll.h>
  #include <sys/time.h>
  #include <netinet/in.h>
	#ifndef INADDR_NONE
		#define INADDR_NONE -1
	#endif
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
  typedef struct sockaddr* LPSOCKADDR;
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
	#define GETERROR (errno) 
	#define ERR_INTERN_TIMEDOUT ETIMEDOUT
	#define ERR_INTERN_CONNREFUSED ECONNREFUSED
	#define ERR_INTERN_WOULDBLOCK EAGAIN
	#define ERR_INTERN_SOCKET_CLOSED EBADF
  #ifndef min
		#define min(a,b) ((a<b)?(a):(b))
  #endif	
	#if __linux
		#include <linux/types.h>
		typedef __u32 UINT32;
		typedef __s32 SINT32;
		typedef unsigned int UINT;
		typedef signed int SINT;
		typedef __u16 UINT16;
		typedef __s16 SINT16;
		typedef __u8 UINT8;
		typedef __s8 SINT8;
	#elif __sgi
		typedef __uint32_t UINT32;
		typedef __int32_t SINT32;
		typedef unsigned int UINT;
		typedef signed int SINT;
		typedef unsigned short UINT16;
		typedef signed short SINT16;
		typedef unsigned char UINT8;
		typedef signed char SINT8;
	#elif __sun    	
		typedef uint32_t UINT32;
		typedef int32_t SINT32;
		typedef unsigned int UINT;
		typedef signed int SINT;
		typedef uint16_t UINT16;
		typedef int16_t SINT16;
		typedef uint8_t UINT8;
		typedef int8_t SINT8;
	#else     	
		#warning This seams to be a currently not supported plattform - may be things go wrong! 
		#warning Please report the plattform, so that it could be added 
		typedef unsigned int UINT32;
		typedef signed int SINT32;
		typedef unsigned int UINT;
		typedef signed int SINT;
		typedef unsigned short UINT16;
		typedef signed short SINT16;
		typedef unsigned char UINT8;
		typedef signed char SINT8;
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
#define E_UNKNOWN_HOST -400 // A hostname could not be resolved

#include <assert.h>

#include <pthread.h>
#include <semaphore.h>
#define THREAD_RETURN void*
#define THREAD_RETURN_ERROR return(NULL)
#define THREAD_RETURN_SUCCESS return (NULL)


#ifndef DEBUG
#define ASSERT(cond,msg)
#else
#define ASSERT(cond,msg) {if(!(cond)){CAMsg::printMsg(LOG_DEBUG,"ASSERT: %s (File: %s, Line: %s)\n",msg,__FILE__,__LINE__);exit(-1);}}
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
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/asn1.h>

#endif // !defined(AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_)
