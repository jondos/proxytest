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

#include "CAPerformanceServer.hpp"

#ifdef PERFORMANCE_SERVER

#include "CACmdLnOptions.hpp"
#include "CASocketAddrINet.hpp"
#include "CASingleSocketGroup.hpp"
#include "CABase64.hpp"

/**
 * @author Christian Banse
 * 
 * The server-side implementation of our performance monitoring
 * system. It creates a listening server socket which sends out 
 * random data based on a XML request.
 */

CAPerformanceServer* CAPerformanceServer::ms_pPerformanceServer = NULL;
extern CACmdLnOptions* pglobalOptions;

void CAPerformanceServer::init()
{
	if(ms_pPerformanceServer == NULL)
	{
		ms_pPerformanceServer = new CAPerformanceServer();
	}
}

void CAPerformanceServer::cleanup()
{
#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG, 
			"CAPerformanceServer: doing cleanup\n");
#endif
	
	if(ms_pPerformanceServer != NULL)
	{
		delete ms_pPerformanceServer;
		ms_pPerformanceServer = NULL;
	}
}

CAPerformanceServer::CAPerformanceServer()
{
	m_pAcceptThread = NULL;
	m_pSocket = NULL;
	m_pRequestHandler = NULL;
	m_pSocket = new CASocket();
	
	if(initSocket() == E_SUCCESS)
	{
		m_pAcceptThread = new CAThread((UINT8*)"Performance Server Thread");
		m_pAcceptThread->setMainLoop(acceptRequests);
		m_pAcceptThread->start(this);
// is MAX_PENDING_PERF_REQUESTS of 2 enough ?
#define MAX_PENDING_PERF_REQUESTS 2
		m_pRequestHandler = new CAThreadPool(5, MAX_PENDING_PERF_REQUESTS, true);
	}
	else
	{
		CAMsg::printMsg(LOG_ERR, 
				"CAPerformanceServer: error occured while creating server socket\n");
		
		if(m_pSocket != NULL)
		{
			m_pSocket->close();
			m_pSocket = NULL;
		}
	}
}

CAPerformanceServer::~CAPerformanceServer()
{
	if(m_pSocket != NULL)
	{
		CAMsg::printMsg(LOG_INFO, "CAPerformanceServer: trying to close server socket... \n");
		if(m_pSocket->close() == E_SUCCESS)
		{
			CAMsg::printMsg(LOG_INFO, "done\n");
		}
		else
		{
			CAMsg::printMsg(LOG_INFO, "failed\n");
		}
	}
	
	if(m_pAcceptThread != NULL)
	{
		delete m_pAcceptThread;
		m_pAcceptThread = NULL;
	}

	if(m_pRequestHandler != NULL)
	{
		delete m_pRequestHandler;
		m_pRequestHandler = NULL;
	}	
	
	if(m_pSocket != NULL)
	{
		delete m_pSocket;
		m_pSocket = NULL;
	}
}

SINT32 CAPerformanceServer::initSocket()
{
	SINT32 ret = E_UNKNOWN;
	CASocketAddrINet addr;
	
	if(m_pSocket == NULL)
	{
		m_pSocket = new CASocket();
	}
	
	if(!m_pSocket->isClosed())
	{
		CAMsg::printMsg(LOG_ERR, 
				"CAPerformanceServer: server socket already connected\n");
		return E_UNKNOWN;
	}
	
	ret = m_pSocket->create();
	if(ret != E_SUCCESS)
	{
		CAMsg::printMsg(LOG_ERR,
				"CAPerformanceServer: could not create server socket\n");
		return ret;
	}
	m_pSocket->setReuseAddr(true);
	
	UINT8* host = PERFORMANCE_SERVER_HOST;
	UINT16 port = PERFORMANCE_SERVER_PORT;
	bool userdefined = false;
	
	if(pglobalOptions != NULL)
	{
		if(pglobalOptions->getPerformanceServerListenerHost() != NULL)
		{
			host = pglobalOptions->getPerformanceServerListenerHost();
			userdefined = true;
		}
		
		if(pglobalOptions->getPerformanceServerListenerPort() != 0xFFFF)
		{
			port = pglobalOptions->getPerformanceServerListenerPort();
			userdefined = true;
		}
	}
	
	ret = addr.setAddr(host, port);
	if(ret != E_SUCCESS)
	{
		if(ret == E_UNKNOWN_HOST)
		{
			CAMsg::printMsg(LOG_ERR,
					"CAPerformanceServer: invalid host specified: %s\n", host);
			
			if(userdefined)
			{
				host = PERFORMANCE_SERVER_HOST;
				CAMsg::printMsg(LOG_ERR,
						"CAPerformanceServer: trying %s\n", host);
				
				ret = addr.setAddr(host, port);
				
				if(ret != E_SUCCESS)
				{
					CAMsg::printMsg(LOG_ERR,
						"CAPerformanceServer: setting up listener interface on host %s:%d failed\n", host, port);
					return ret;
				}
			}
		}
		else
		{
			CAMsg::printMsg(LOG_ERR,
					"CAPerformanceServer: setting up listener interface on host %s:%d failed\n", host, port);
			return ret;
		}
	}
	
	ret = m_pSocket->listen(addr);
	if(ret != E_SUCCESS)
	{
		CAMsg::printMsg(LOG_ERR,
				"CAPerformanceServer: could not listen on %s:%d (%s) \n", host, port, GET_NET_ERROR_STR(GET_NET_ERROR));
		return ret;
	}

	CAMsg::printMsg(LOG_DEBUG,
			"CAPerformanceServer: listening on %s:%d\n", host, port);
	
	return E_SUCCESS;
}

SINT32 CAPerformanceServer::sendDummyData(perfrequest_t* request)
{
	SINT32 ret = E_UNKNOWN;
	SINT32 len;
	SINT32 headerLen;
	UINT8* header;
	UINT8* buff;
	UINT8* sendbuff;
	CASingleSocketGroup oSG(true);
	
	if(request->uiDataLength > MAX_DUMMY_DATA_LENGTH || request->uiDataLength == 0)
	{
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG,
				"CAPerformanceServer: 400 Bad Request\n");
#endif		
		sendHTTPResponseHeader(request, 400);
		return E_UNKNOWN;
	}
	
#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG,
			"CAPerformanceServer: 200 OK\n");
#endif	
	header = (UINT8*) createHTTPResponseHeader(200, request->uiDataLength);
	headerLen = strlen((char*)header);
	buff = new UINT8[request->uiDataLength + headerLen + 1];
	memset(buff, 0, sizeof(UINT8)*request->uiDataLength + headerLen + 1);
	strncpy((char*)buff, (char*)header, headerLen);
	
	memset(buff + headerLen, 65, request->uiDataLength);
	buff[request->uiDataLength + headerLen] = '\0';
	oSG.add(*(request->pSocket));
	
	/*
	 * DOES NOT WORK!!
	 * MAY (AND PROBABLY WILL) CRASH THE MIX WITH SPECIFIC packet lengths
	 * 
	 * UINT32 randBytesLen = (request->uiDataLength * 3) / 4;	
	UINT8* randBytes = new UINT8[randBytesLen];
	getRandom(randBytes, randBytesLen);
	
#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG,
			"CAPerformanceServer: generated %d bytes of random data", randBytesLen);
#endif
	
	CABase64::encode(randBytes, randBytesLen, buff + headerLen, &request->uiDataLength);
	*/

	//ret = request->pSocket->sendFullyTimeOut((UINT8*)buff, request->uiDataLength + headerLen, PERFORMANCE_SERVER_TIMEOUT, PERFORMANCE_SERVER_TIMEOUT);
	//ret = request->pSocket->sendFullySelect(m_pServerSocket, (UINT8*)buff, request->uiDataLength + headerLen, PERFORMANCE_SERVER_TIMEOUT);
	
	len = request->uiDataLength + headerLen;
	sendbuff = buff;

	while(m_pSocket != NULL && !m_pSocket->isClosed())
	{
		ret = oSG.select(PERFORMANCE_SERVER_TIMEOUT);
		if(ret == 1)
		{
			ret = request->pSocket->send(sendbuff, len);
			if(ret == len)
			{
				ret = E_SUCCESS;
				break;
			}
			else if(GET_NET_ERROR == ERR_INTERN_WOULDBLOCK)
			{
				continue;
			}
			else if(ret < 0)
			{
				CAMsg::printMsg(LOG_INFO, "CAPerformanceServer: send returned %i - GET_NET_ERROR: %i\n", ret, GET_NET_ERROR);
				break;
			}
			
			len -= ret;
			sendbuff += ret;
		}
	}
	
	if(ret == E_SUCCESS)
	{
		CAMsg::printMsg(LOG_INFO,
				"CAPerformanceServer: sent %d bytes of dummy data to %s\n", request->uiDataLength, request->pstrInfoServiceId);
	}
		
	delete[] header;
	header = NULL;
	delete[] buff;
	buff = NULL;
	sendbuff = NULL;
	
	return ret;
}

SINT32 CAPerformanceServer::handleRequest(perfrequest_t* request)
{
	char* line = new char[256];
	memset(line, 0, sizeof(char)*256);
	char* method = NULL;
	char* url = NULL;
	CASocket* pClient;
	
	UINT32 len = 0;
	
	if(request == NULL || request->pSocket == NULL)
	{
		return E_UNKNOWN;
	}
	
	pClient = request->pSocket;
	pClient->setNonBlocking(true);
	pClient->receiveLine((UINT8*)line, 255, PERFORMANCE_SERVER_TIMEOUT);
	
	method = strtok (line," ");
	if(method == NULL || strncmp(method, "POST", 3) != 0)
	{
#ifdef DEBUG
		CAMsg::printMsg(LOG_ERR,
				"CAPerformanceServer: 405 Method Not Allowed\n");

#endif
		sendHTTPResponseHeader(request, 405);
		return E_UNKNOWN;
	}
	
	url = strtok(NULL, " ");
	
	if(url == NULL || strncmp(url, "/senddummydata", 18) != 0)
	{
#ifdef DEBUG		
		CAMsg::printMsg(LOG_ERR,
						"CAPerformanceServer: 404 Not Found\n");
#endif
		sendHTTPResponseHeader(request, 404);
		return E_UNKNOWN;
	}
	
	do
	{
		pClient->receiveLine((UINT8*)line, 255, PERFORMANCE_SERVER_TIMEOUT);
		if(strnicmp(line, "Content-Length: ", 16) == 0)
		{
			len = (UINT32) atol(line + 16);
		}
	} while(strlen(line) > 0);
	delete[] line;
	line = NULL;
	
	if(len == 0)
	{
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG,
				"CAPerformanceServer: 400 Bad Request\n");
#endif
		sendHTTPResponseHeader(request, 400);
		return E_UNKNOWN;
	}
	else
	{
		SINT32 ret = E_UNKNOWN;
		UINT8* buff = new UINT8[len + 1];
		memset(buff, 0, sizeof(UINT8)*len+1);
		pClient->receiveFullyT(buff, len, PERFORMANCE_SERVER_TIMEOUT);
		buff[len] = '\0';
		
		if(parseXMLRequest(request, buff, len) != E_SUCCESS)
		{
#ifdef DEBUG
        	CAMsg::printMsg(LOG_DEBUG,
        			"CAPerformanceServer: 400 Bad Request\n");
#endif
        	sendHTTPResponseHeader(request, 400);
		}
		else
        {
			ret = sendDummyData(request);
        }
		
		delete[] buff;
		buff = NULL;		
		
		return ret;
	}
}

SINT32 CAPerformanceServer::parseXMLRequest(perfrequest_t* request, UINT8* xml, UINT32 len)
{
	SINT32 ret = E_UNKNOWN;
	
	DOMDocument* doc = parseDOMDocument(xml, len);
	DOMElement* root = NULL;
	DOMNode* infoservice = NULL;
	
	if(doc != NULL &&  (root = doc->getDocumentElement()) != NULL && equals(root->getNodeName(), "SendDummyDataRequest"))
	{
		getDOMElementAttribute(root, "dataLength", (SINT32*) &(request->uiDataLength));
		getDOMChildByName(root, "InfoService", infoservice);
		if(infoservice != NULL)
		{
			UINT32 idlen = 255;
			request->pstrInfoServiceId = new UINT8[256];
			memset(request->pstrInfoServiceId, 0, idlen + 1);
			
			getDOMElementAttribute(infoservice, "id", request->pstrInfoServiceId, &idlen);
			ret = E_SUCCESS;
		}
	}
	
	if(doc != NULL)
	{
		doc->release();
		doc = NULL;
	}
	
	return ret;
}

SINT32 CAPerformanceServer::sendHTTPResponseHeader(perfrequest_t* request, UINT16 code, UINT32 contentLength)
{
	SINT32 ret;
	
	UINT8* buff = createHTTPResponseHeader(code, contentLength);
	ret = request->pSocket->sendFullyTimeOut(buff, strlen((char*)buff), PERFORMANCE_SERVER_TIMEOUT, PERFORMANCE_SERVER_TIMEOUT);
	delete[] buff;
	buff = NULL;
	
	return ret;
}

UINT8* CAPerformanceServer::createHTTPResponseHeader(UINT16 code, UINT32 contentLength)
{
	UINT8* header = new UINT8[256];
	
	snprintf((char*)header, 255, "HTTP/1.1 %s\r\nContent-Length: %u\r\nConnection: close\r\n\r\n", getResponseText(code), contentLength);
	
	return header;
}

char* CAPerformanceServer::getResponseText(UINT16 code)
{
	switch(code)
	{
		case 200:
			return "200 OK";
			
		case 400:
			return "400 Bad Request";
			
		case 404:
			return "404 Not Found";
			
		case 405:
			return "405 Method Not Allowed";
			
		default:
			return "";
	}
}

THREAD_RETURN acceptRequests(void* param)
{
	CAPerformanceServer* pServer = (CAPerformanceServer*) param;
	
	if(pServer == NULL)
	{
		THREAD_RETURN_ERROR;
	}
	
	CASocket* pServerSocket = pServer->m_pSocket;
	
	while(true)
	{
		if(pServerSocket != NULL)
		{
			if(pServerSocket->isClosed())
			{
				CAMsg::printMsg(LOG_INFO,
						"CAPerformanceServer: socket closed. exiting listener thread\n");
				THREAD_RETURN_SUCCESS;
			}
		}
		else
		{
			CAMsg::printMsg(LOG_ERR,
					"CAPerformanceServer: socket disposed. exiting listener thread\n");
			THREAD_RETURN_ERROR;
		}
		
		CASocket* request = new CASocket;
		if(pServerSocket->accept(*request) == E_SUCCESS)
		{
			UINT8 peerIp[4];
			
			if(request->getPeerIP(peerIp) == E_SUCCESS)
			{
				perfrequest_t* t = new perfrequest_t;
				t->ip = new char[16];
				strncpy(t->ip, inet_ntoa(*((in_addr*) &peerIp)), 15);
				
				t->pServer = pServer;
				t->pSocket = request;
				t->uiDataLength = 0;
				t->pstrInfoServiceId = NULL;
				
#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "CAPerformanceServer: accepting connection from %s\n", t->ip);
#endif
				
				if(pServer->m_pRequestHandler->addRequest(handleRequest, t) != E_SUCCESS)
				{
					CAMsg::printMsg(LOG_ERR, "CAPerformanceServer: too many pending requests. discarding request from %s\n", t->ip);
					if(t->pSocket != NULL)
					{
						t->pSocket->close();
						delete t->pSocket;
						t->pSocket = NULL;
					}
					
					if(t->ip != NULL)
					{
						delete[]t->ip;
						t->ip = NULL;
					}
					
					if(t->pstrInfoServiceId != NULL)
					{
						delete[] t->pstrInfoServiceId;
						t->pstrInfoServiceId = NULL;
					}
					delete t;
					t = NULL;
				}
			}
		}
	}
}

THREAD_RETURN handleRequest(void* param)
{
	SINT32 ret;
	perfrequest_t* request = (perfrequest_t*) param;
	
	if(request == NULL || request->pServer == NULL)
	{
		CAMsg::printMsg(LOG_INFO, "CAPerformanceServer: error occured while processing client request\n");
		THREAD_RETURN_ERROR;
	}
	
	ret = request->pServer->handleRequest(request);
	
	if(ret != E_SUCCESS)
	{
		CAMsg::printMsg(LOG_DEBUG, "CAPerformanceServer: error while handling request from client %s (code: %d)\n", request->ip, ret);
	}
	
	if(request->pSocket != NULL)
	{
		request->pSocket->close();
		delete request->pSocket;
		request->pSocket = NULL;
	}
	
	if(request->ip != NULL)
	{
		delete[] request->ip;
		request->ip = NULL;
	}
	
	if(request->pstrInfoServiceId != NULL)
	{
		delete[] request->pstrInfoServiceId;
		request->pstrInfoServiceId = NULL;
	}
	
	delete request;
	request = NULL;
	
	THREAD_RETURN_SUCCESS;
}
#endif /* PERFORMANCE_SERVER */
