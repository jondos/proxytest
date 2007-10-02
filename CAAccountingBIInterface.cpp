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

extern CACmdLnOptions* pglobalOptions;

/**
 * Constructor
 * Initiates the DB connection
 */
CAAccountingBIInterface::CAAccountingBIInterface()
	{
		m_pSocket =NULL;
		m_pSocket = new CATLSClientSocket();
	}

/**
 * Destructor
 */
CAAccountingBIInterface::~CAAccountingBIInterface()
	{
		terminateBIConnection();
		if(m_pSocket!=NULL)
			delete m_pSocket;
	}



/**
 * Initiate HTTP(s) connection to the BI (JPI)
 */
SINT32 CAAccountingBIInterface::initBIConnection()
	{
		CASocketAddrINet address;
		SINT32 rc;
		CAXMLBI* pBI= pglobalOptions->getBI();
		
		// fetch BI address
		if(pBI == NULL)
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface: could not get JPI hostname\n");
				return E_UNKNOWN;
			}
		#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG,"CAAccountingBIInterface: try to connect to BI at %s:%i.\n",pBI->getHostName(),pBI->getPortNumber());
		#endif
		address.setAddr(pBI->getHostName(), (UINT16)pBI->getPortNumber());		
		// connect
		m_pSocket->setServerCertificate(pBI->getCertificate());
		rc=m_pSocket->connect(address, 5000);
		if(rc!=E_SUCCESS)
		{
			/*
			UINT8 buf[64];
			memset(buf,0,64);
			address.getHostName(buf, 64);*/
			CAMsg::printMsg(
					LOG_ERR, 
					"CAAccountingBIInterface: Could not connect to BI at %s:%i. Reason: %i\n", 
					//buf, address.getPort(), rc
					pBI->getHostName(), pBI->getPortNumber(), rc
				);
			m_pSocket->close();
			return E_UNKNOWN;
		}
		CAMsg::printMsg(LOG_DEBUG,"CAAccountingBIInterface: BI connection to %s:%i established!\n", pBI->getHostName(),pBI->getPortNumber());
		m_httpClient.setSocket(m_pSocket);
		return E_SUCCESS;
	}



/**
 * Terminate HTTP(s) connection to the BI (JPI)
 */
SINT32 CAAccountingBIInterface::terminateBIConnection()
	{
		m_pSocket->close();
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
	
		pStrCC = cc.dumpToMem(&contentLen);
		if(	pStrCC==NULL || m_httpClient.sendPostRequest((UINT8*)"/settle", pStrCC,contentLen)!= E_SUCCESS)
		{
			delete[] pStrCC;
			return NULL;
		}
		delete[] pStrCC;
		contentLen=0;
		status=0;

		if(m_httpClient.parseHTTPHeader(&contentLen, &status)!=E_SUCCESS ||
			(status!=200) || (contentLen==0))
		{
			CAMsg::printMsg(LOG_ERR, "CAAccountingBIInterface::settle: response fehlerhaft!\n");
			return NULL;
		}
#ifdef DEBUG			
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingBIInterface::settle: got response header [Status,content-Lenght]=[%i,%i]!\n",status,contentLen);
#endif		
		response = new UINT8[contentLen+1];
		if(m_pSocket->receiveFully(response, contentLen)!=E_SUCCESS)
			{
				delete[] response;
				return NULL;
			}
#ifdef DEBUG			
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingBIInterface::settle: response body received!\n");
#endif		
		response[contentLen]='\0';
		pErrMsg = new CAXMLErrorMessage(response);
		delete[] response;
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
