// stdafx.h : Include-Datei für Standard-System-Include-Dateien,
//  oder projektspezifische Include-Dateien, die häufig benutzt, aber
//      in unregelmäßigen Abständen geändert werden.
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
#else
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <sys/poll.h>
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
    
//    #define min(a,b) ((a<b)?(a):(b))
		#define GETERROR (errno) 
		#define E_TIMEDOUT ETIMEDOUT
		#define E_CONNREFUSED ECONNREFUSED
//		#ifdef __sgi
			#include <alloca.h>
			#include <ctype.h>
			#include <mstring.h>
			extern "C++" {typedef basic_string <char> string;}
//    #endif
#endif

#define E_SUCCESS 0		

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
		// ZU ERLEDIGEN: Verweisen Sie hier auf zusätzliche Header-Dateien, die Ihr Programm benötigt

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt zusätzliche Deklarationen unmittelbar vor der vorherigen Zeile ein.

#endif // !defined(AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_)
