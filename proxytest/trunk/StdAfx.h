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
    #include <io.h>
    #include <conio.h>
    #include <process.h>
    #define THREAD_RETURN void
		#define THREAD_RETURN_ERROR return
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <pthread.h>
    #include <unistd.h>
    #include <stdlib.h>
    typedef struct sockaddr* LPSOCKADDR;
    #define SOCKET int
    typedef struct hostent HOSTENT;
    #define closesocket(s) close(s)
    #define SOCKET_ERROR -1
    #define SD_RECEIVE 0
    #define SD_SEND 1
    #define SD_BOTH 2
    
    #define CRITICAL_SECTION pthread_mutex_t
    #define DeleteCriticalSection(p) pthread_mutex_destroy(p)
    #define InitializeCriticalSection(p) pthread_mutex_init(p,NULL)
    #define EnterCriticalSection(p) pthread_mutex_lock(p)
    #define LeaveCriticalSection(p) pthread_mutex_unlock(p)
    #define THREAD_RETURN void*
    #define THREAD_RETURN_ERROR return(NULL)
#endif
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

// ZU ERLEDIGEN: Verweisen Sie hier auf zusätzliche Header-Dateien, die Ihr Programm benötigt

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt zusätzliche Deklarationen unmittelbar vor der vorherigen Zeile ein.

#endif // !defined(AFX_STDAFX_H__9A5B051F_FF3A_11D3_9F5E_000001037024__INCLUDED_)
