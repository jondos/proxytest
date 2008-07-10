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
#include "StdAfx.h"
#ifdef PAYMENT
#include "CAAccountingBIInterface.hpp"
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocket.hpp"
#include "CAHttpClient.hpp"

//extern CACmdLnOptions* pglobalOptions;
//CAMutex* CAAccountingBIInterface::m_pPiInterfaceMutex = new CAMutex();
//CAAccountingBIInterface *CAAccountingBIInterface::m_pPiInterfaceSingleton = NULL;

/**
 * private constructor
 * just initializes fields, server configuration is done separately
 */
CAAccountingBIInterface::CAAccountingBIInterface()
	{
		//m_pSocket =NULL;
		//m_pPiInterfaceMutex = new CAMutex();
		m_pSocket = new CATLSClientSocket();
		m_pPiServerAddress = new CASocketAddrINet();
		m_phttpClient = new CAHttpClient();
	}

/**
 * private destructor
 */
CAAccountingBIInterface::~CAAccountingBIInterface()
	{
		terminateBIConnection();
		if(m_pSocket!=NULL)
		{
			delete m_pSocket;
			m_pSocket = NULL;
		}
		if(m_pPiServerAddress!=NULL)
		{
			delete m_pPiServerAddress;
			m_pPiServerAddress = NULL;
		}
		if(m_phttpClient!=NULL)
		{
			delete m_phttpClient;
			m_phttpClient = NULL;
		}
		/*if(m_pPiInterfaceMutex!=NULL)
		{
			delete m_pPiInterfaceMutex;
			m_pPiInterfaceMutex = NULL;
		}*/
	}


SINT32 CAAccountingBIInterface::setPIServerConfiguration(CAXMLBI* pPiServerConfig)
{
	UINT8 *pPiName = NULL;
	UINT16 piPort = 0;
	CACertificate *pPiCert = NULL;
	
	//m_pPiInterfaceMutex->lock();
		
	if(pPiServerConfig==NULL)
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface: could not configure PI interface: no proper configuration given.\n");
		//m_pPiInterfaceMutex->unlock();
		return E_UNKNOWN;
	}
	
	pPiName = pPiServerConfig->getHostName();
	pPiCert = pPiServerConfig->getCertificate();
	piPort = (UINT16)pPiServerConfig->getPortNumber();
	
	if(pPiName==NULL)
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface: could not configure PI interface: no PI name specified.\n");
		//m_pPiInterfaceMutex->unlock();
		return E_UNKNOWN;
	}
	if(pPiCert==NULL)
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface: could not configure PI interface: no PI certificate specified.\n");
		//m_pPiInterfaceMutex->unlock();
		return E_UNKNOWN;
	}
	
	if(m_pPiServerAddress == NULL)
	{
		//Should never happen
		#ifdef DEBUG
		CAMsg::printMsg(LOG_ERR,"CAAccountingBIInterface: no server address initialized while trying to configure PI interface.\n");
		#endif
		m_pPiServerAddress = new CASocketAddrINet();
	}
	m_pPiServerAddress->setAddr(pPiName, piPort);
	m_pPiServerCertificate = pPiCert; 
	//m_pPiInterfaceMutex->unlock();
	return E_SUCCESS;
}

/**
 * Establishes HTTP(s) connection to the BI (JPI)
 */
SINT32 CAAccountingBIInterface::initBIConnection()
{
	SINT32 rc;
	UINT8 buf[64];
	
	memset(buf,0,64);
	
	//m_pPiInterfaceMutex->lock();
	if(m_pPiServerAddress == NULL)
	{
		//Should never happen
		#ifdef DEBUG
		CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface: no address initialized while trying to connect. Connect aborted.\n");
		#endif
		//m_pPiInterfaceMutex->unlock();
		return E_UNKNOWN;
	}
			
	if(m_pSocket == NULL)
	{
		//Should never happen
		#ifdef DEBUG
		CAMsg::printMsg(LOG_ERR,"CAAccountingBIInterface: no socket initialized while trying to connect. Creating new one.\n");
		#endif
		m_pSocket = new CATLSClientSocket();
	}
	
	if(m_pPiServerCertificate == NULL)
	{
		CAMsg::printMsg(LOG_ERR,"CAAccountingBIInterface: no PI server certificate specified. Connect aborted.\n");
		//m_pPiInterfaceMutex->unlock();
		return E_UNKNOWN;
	}			
	// connect
	m_pSocket->setServerCertificate(m_pPiServerCertificate);
	rc=m_pSocket->connect(*m_pPiServerAddress, PI_CONNECT_TIMEOUT);
	if(rc!=E_SUCCESS)
	{
		m_pPiServerAddress->getHostName(buf, 64);
		CAMsg::printMsg(
				LOG_ERR, 
				"CAAccountingBIInterface: Could not connect to BI at %s:%i. Reason: %i\n", 
				buf, m_pPiServerAddress->getPort(), rc
				//pBI->getHostName(), pBI->getPortNumber(), rc
			);
		m_pSocket->close();
		//m_pPiInterfaceMutex->unlock();
		return E_UNKNOWN;
	}
	m_pPiServerAddress->getHostName(buf, 64);
	CAMsg::printMsg(LOG_DEBUG,"CAAccountingBIInterface: BI connection to %s:%i established!\n", buf,m_pPiServerAddress->getPort());
	m_phttpClient->setSocket(m_pSocket);
	//m_pPiInterfaceMutex->unlock();
	return E_SUCCESS;
}



/**
 * Terminate HTTP(s) connection to the BI (JPI)
 */
SINT32 CAAccountingBIInterface::terminateBIConnection()
{
	//m_pPiInterfaceMutex->lock();
	if(m_pSocket == NULL)
	{
		//Should never happen
		#ifdef DEBUG
		CAMsg::printMsg(LOG_ERR,"CAAccountingBIInterface: no socket initialized while trying to close connection.\n");
		#endif
		//m_pPiInterfaceMutex->unlock();
		return E_UNKNOWN;
	}
	m_pSocket->close();
	//m_pPiInterfaceMutex->unlock();
	return E_SUCCESS;
}



/**
 * Send a cost confirmation to the JPI
 * TODO: Error handling
 */
CAXMLErrorMessage * CAAccountingBIInterface::settle(CAXMLCostConfirmation &cc)
{
	UINT8 * pStrCC=NULL;
	UINT8* response=NULL;
	UINT32 contentLen=0, status=0;
	CAXMLErrorMessage *pErrMsg;
	
	//m_pPiInterfaceMutex->lock();
	pStrCC = cc.dumpToMem(&contentLen);
	if(	pStrCC==NULL || m_phttpClient->sendPostRequest((UINT8*)"/settle", pStrCC,contentLen)!= E_SUCCESS)
	{
		delete[] pStrCC;
		pStrCC = NULL;
		//m_pPiInterfaceMutex->unlock();
		return NULL;
	}
	delete[] pStrCC;
	pStrCC = NULL;
	contentLen=0;
	status=0;

	if(m_phttpClient->parseHTTPHeader(&contentLen, &status)!=E_SUCCESS ||
		(status!=200) || (contentLen==0))
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface::settle: response fehlerhaft!\n");
		//m_pPiInterfaceMutex->unlock();
		return NULL;
	}
#ifdef DEBUG			
	CAMsg::printMsg(LOG_DEBUG, "CAAccountingBIInterface::settle: got response header [Status,content-Lenght]=[%i,%i]!\n",status,contentLen);
#endif		
	response = new UINT8[contentLen+1];
	if(m_pSocket->receiveFully(response, contentLen)!=E_SUCCESS)
		{
			delete[] response;
			response = NULL;
			//m_pPiInterfaceMutex->unlock();
			return NULL;
		}
#ifdef DEBUG			
	CAMsg::printMsg(LOG_DEBUG, "CAAccountingBIInterface::settle: response body received!\n");
#endif		
	response[contentLen]='\0';
	pErrMsg = new CAXMLErrorMessage(response);
	delete[] response;
	response = NULL;
	//m_pPiInterfaceMutex->unlock();
	return pErrMsg;
}

/*CAAccountingBIInterface *CAAccountingBIInterface::getInstance(CAXMLBI* pPiServerConfig)
{
	SINT32 ret;
	m_pPiInterfaceMutex->lock();
	if(m_pPiInterfaceSingleton == NULL)
	{
		if(pPiServerConfig == NULL)
		{
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingBIInterface::could not create PI interface: no proper configuration specified!\n");
			m_pPiInterfaceMutex->unlock();
			return NULL;
		}
		m_pPiInterfaceSingleton = new CAAccountingBIInterface();
		ret = m_pPiInterfaceSingleton->setPIServerConfiguration(pPiServerConfig);
		if(ret != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingBIInterface::could not create PI interface: no proper configuration specified!\n");
			m_pPiInterfaceMutex->unlock();
			return NULL;
		}
	}
	else 
	{
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingBIInterface:: PI Interface already running!\n");
	}
	m_pPiInterfaceMutex->unlock();
	return m_pPiInterfaceSingleton;
	
}*/

/*

**
 * Request a new Balance certificate from the JPI
 * TODO: Error handling
 *
 * @param balanceCert an old balance certificate that the AI wishes to update
 * @param response a buffer to receive the new balanceCert
 * @param responseLen the maximum size of the buffer
 *
SINT32 CAAccountingBIInterface::update(UINT8 *balanceCert, UINT8 * response, UINT32 *responseLen)
{
	UINT8 requestF[] = "<?xml version=\"1.0\">\n<Balances>\n%s</Balances>\n";
	UINT32 sendbuflen = strlen((char *)balanceCert) + strlen((char *)requestF) + 10;
	UINT8* sendbuf=new UINT8[sendbuflen];
	UINT32 status;
	sprintf((char *)sendbuf, (char *)requestF, (char *)balanceCert);
	m_httpClient.sendPostRequest((UINT8 *)"/update", sendbuf, strlen((char *)sendbuf));
	delete sendbuf;
	receiveResponse(&status, response, responseLen);
	return E_SUCCESS;
}
*/

#endif
