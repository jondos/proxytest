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

#ifndef CAPERFORMANCESERVER_HPP_
#define CAPERFORMANCESERVER_HPP_

/**
 * @author Christian Banse
 * 
 * The server-side implementation of our performance monitoring
 * system. It creates a listening server socket which sends out 
 * random data based on a XML request.
 */

#include "StdAfx.h"

#ifdef PERFORMANCE_SERVER

#include "CASocket.hpp"
#include "CAThread.hpp"
#include "CAThreadPool.hpp"

#define PERFORMANCE_SERVER_HOST		(UINT8*) "localhost"
#define PERFORMANCE_SERVER_PORT		7777	

#define PERFORMANCE_SERVER_TIMEOUT	5000

#define MAX_DUMMY_DATA_LENGTH		1024*1024*20

/** 
 * the server routine which: 
 *  * accepts socket connections,
 *  * outputs a status message
 *  * and closes the socket immediately after that
 */
THREAD_RETURN handleRequest(void* param);
THREAD_RETURN acceptRequests(void* param);

class CAPerformanceServer;

struct perfrequest_t
{
	UINT32 uiDataLength;
	
	char* ip;
	CAPerformanceServer* pServer;
	CASocket* pSocket;

	UINT8* pstrInfoServiceId;
};

class CAPerformanceServer
{
public:
	static void init();
	static void cleanup();
		
private:
	/* ServerSocket */
	CASocket* m_pSocket;
	
	/* The accept thread */
	CAThread* m_pAcceptThread;
	
	/* Thread pool for incoming requests*/
	CAThreadPool* m_pRequestHandler;
	
	/* PerfomanceServer singleton */
	static CAPerformanceServer* ms_pPerformanceServer;
	
	CAPerformanceServer();
	~CAPerformanceServer();
	
	SINT32 initSocket();
	SINT32 handleRequest(perfrequest_t* request);
	SINT32 parseXMLRequest(perfrequest_t* request, UINT8* xml, UINT32 len);
	SINT32 sendDummyData(perfrequest_t* request);
	SINT32 sendHTTPResponseHeader(perfrequest_t* request, UINT16 code, UINT32 contentLength = 0);
	
	UINT8* createHTTPResponseHeader(UINT16 code, UINT32 contentLength);
	char* getResponseText(UINT16 code);
	
	friend THREAD_RETURN handleRequest(void* param);
	friend THREAD_RETURN acceptRequests(void* param);
};

#endif /* PERFORMANCE_SERVER */

#endif /*CAPERFORMANCESERVER_HPP_*/
