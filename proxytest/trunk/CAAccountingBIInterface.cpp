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

extern CACmdLnOptions options;

/**
 * Constructor
 * Initiates the DB connection
 * @param useTLS if nonzero, a TLS socket is used.
 */
CAAccountingBIInterface::CAAccountingBIInterface(UINT32 useTLS)
{
	// init some vars
	m_bUseTLS = useTLS;
	if(useTLS)
	{
		m_pSocket = new CATLSClientSocket();
	}
	else
	{
		m_pSocket = new CASocket();
	}
}



/**
 * Destructor
 */
CAAccountingBIInterface::~CAAccountingBIInterface()
{
	terminateBIConnection();
	if(m_pSocket)
		delete m_pSocket;
}



/**
 * Initiate HTTP(s) connection to the BI (JPI)
 */
SINT32 CAAccountingBIInterface::initBIConnection()
{
	CASocketAddrINet address;
	SINT32 rc;
	CAXMLBI * pBI;
	
	// fetch BI address
	if(!(pBI = options.getBI()))
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface: could not get JPI hostname");
	}
	address.setAddr(pBI->getHostName(), pBI->getPortNumber());
	
	// connect
	if((rc=m_pSocket->connect(address))!=E_SUCCESS)
		{
			UINT8 buf[64];
			address.getHostName(buf, 64);
			CAMsg::printMsg(
					LOG_ERR, 
					"CAAccountingBIInterface: could not connect to BI at %s:%d. Reasion: %d\n", 
					buf, address.getPort(), rc
				);
			m_connected = false;
			return E_UNKNOWN;
		}
	m_httpClient.setSocket(m_pSocket);
	m_connected = true;
	return E_SUCCESS;
}



/**
 * Terminate HTTP(s) connection to the BI (JPI)
 */
SINT32 CAAccountingBIInterface::terminateBIConnection()
{
	if(m_connected)
		{
			m_pSocket->close();
		}
	return E_SUCCESS;
}



/**
 * Send a cost confirmation to the JPI
 * TODO: Error handling
 */
CAXMLErrorMessage * CAAccountingBIInterface::settle(CAXMLCostConfirmation &cc)
{
	UINT8 * pStrCC;
	UINT8* response;
	UINT32 contentLen, status;
	CAXMLErrorMessage *pErrMsg;
	
	pStrCC = cc.toXmlString(contentLen);
	if(m_httpClient.sendPostRequest((UINT8*)"/settle", pStrCC, strlen((char*)pStrCC))
		!= E_SUCCESS)
	{
		delete[] pStrCC;
		return NULL;
	}
	delete[] pStrCC;
	
	if(m_httpClient.parseHTTPHeader(&contentLen, &status)!=E_SUCCESS ||
		 (status!=200) || (contentLen==0))
	{
		return NULL;
	}
	response = new UINT8[contentLen+1];
	m_pSocket->receiveFully(response, contentLen);
	response[contentLen]='\0';
	pErrMsg = new CAXMLErrorMessage(response);
	return pErrMsg;
}


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
