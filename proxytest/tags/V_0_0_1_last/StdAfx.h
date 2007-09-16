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

// stdafx.h : Include-Datei f�r Standard-System-Include-Dateien,
//  oder projektspezifische Include-Dateien, die haeufig benutzt, aber
//      in unregelmaessigen Abstaenden geaendert werden.
//

#if !defined(AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_)
#define AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_

#ifdef _WIN32
    #if _MSC_VER > 1000
    #pragma once
    #endif // _MSC_VER > 1000
    #include <winsock2.h>
		#define socklen_t int
		#define MSG_NOSIGNAL 0 
    #include <io.h>
    #include <conio.h>
    #include <process.h>
		#include <sys/timeb.h>
		#ifndef _REENTRANT
			#define CRITICAL_SECTION 
			#define DeleteCriticalSection(p) 
			#define InitializeCriticalSection(p) 
			#define EnterCriticalSection(p) 
			#define LeaveCriticalSection(p) 
		#endif
    #define THREAD_RETURN void
    #define THREAD_RETURN_ERROR return
    #define THREAD_RETURN_SUCCESS return
    #define sleep(i) Sleep(i*1000)
		#define GETERROR (WSAGetLastError())
		#define E_TIMEDOUT WSAETIMEDOUT
		#define E_CONNREFUSED WSAECONNREFUSED
		#ifdef __cplusplus
			#include <string>
			#include <vector>
			#define STRING std::string
			#define VECTOR std::vector
		#endif
	//	#ifndef UINT32
	//		typedef unsigned long UINT32;
	//	#endif
		typedef signed long SINT32;
		typedef unsigned int UINT;
		typedef signed int SINT;
		typedef unsigned short UINT16;
		typedef signed short SINT16;
		typedef unsigned char UINT8;
		typedef signed char SINT8;
#else
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <sys/poll.h>
    #include <sys/time.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <pthread.h>
    #include <unistd.h>
    #include <stdlib.h>
    #include <strings.h>
    #include <syslog.h>
    #include <stdarg.h>
    #include <memory.h>       
    #ifndef O_BINARY
	#define O_BINARY 0
    #endif
    #ifndef MAX_PATH
	#define MAX_PATH 4096
    #endif
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
    #define WSAGetLastError() errno

    #ifndef __linux
	#ifndef INADDR_NONE
    	    #define INADDR_NONE -1
	#endif
    	#define socklen_t int
    	#include <sys/filio.h>
    	#define MSG_NOSIGNAL 0
    #endif
		#ifdef _REENTRANT
			#define CRITICAL_SECTION pthread_mutex_t
			#define DeleteCriticalSection(p) pthread_mutex_destroy(p)
			#define InitializeCriticalSection(p) pthread_mutex_init(p,NULL)
			#define EnterCriticalSection(p) pthread_mutex_lock(p)
			#define LeaveCriticalSection(p) pthread_mutex_unlock(p)
		#else 
			#define CRITICAL_SECTION 
			#define DeleteCriticalSection(p) 
			#define InitializeCriticalSection(p) 
			#define EnterCriticalSection(p) 
			#define LeaveCriticalSection(p) 
		#endif
		#define THREAD_RETURN void*
    #define THREAD_RETURN_ERROR return(NULL)
    #define THREAD_RETURN_SUCCESS return (NULL)
		#define GETERROR (errno) 
		#define E_TIMEDOUT ETIMEDOUT
		#define E_CONNREFUSED ECONNREFUSED
		#ifdef __sgi
			#define HAVE_VSYSLOG 1
			#include <alloca.h>
			#include <ctype.h>
		#endif
		#ifdef __cplusplus
		#ifdef __sgi	
			#include <vector.h>
			#include <mstring.h>
			extern "C++" {typedef basic_string <char> string;}
			#define STRING string
			#define VECTOR vector
		#else
		 #include <string>
		 #include <vector>
			#define STRING std::string
			#define VECTOR std::vector
		#endif
		#endif
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
	#endif
#endif

#define E_SUCCESS 0
#define E_UNKNOWN -1
#define E_UNSPECIFIED -100 // A Parameter was not specified/not set
#include <assert.h>
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
#include <openssl/blowfish.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/asn1.h>

#endif // !defined(AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_)