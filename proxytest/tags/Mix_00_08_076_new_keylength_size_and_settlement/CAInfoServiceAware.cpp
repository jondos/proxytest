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
#ifdef DYNAMIC_MIX
#include "CAInfoServiceAware.hpp"
#include "CASocket.hpp"
#include "CASocketAddrINet.hpp"
#include "CAHttpClient.hpp"
#include "CACmdLnOptions.hpp"

/**
  * LERNGRUPPE
  * Sends a request to the InfoService at the given CASocketAddrINet. The result should be a XML structure which is returned by r_elemRoot
  * @param a_addr The CASocketAddrINet of the InfoService
  * @param a_strRequest The request to send
  * @param a_postData POST data to include in the request (if this is NULL, a GET request will be issued, otherwise a POST request)
  * @param a_postLen The length of a_postData
  * @return r_elemRoot The root element of the resulting XML structure
  * @retval E_SUCCESS if everything went well
  * @retval E_UNKNOWN otherwise
  */
SINT32 CAInfoServiceAware::sendInfoserviceRequest(CASocketAddrINet* a_addr, UINT8 *a_strRequest, DOM_Element *r_elemRoot, UINT8* a_postData, UINT32 a_postLen)
{
	CASocket socket;
	CAHttpClient httpClient;
	UINT32 status, contentLength;
	UINT8 *content = NULL;
	SINT32 ret = E_UNKNOWN;
	DOM_Document doc;
	DOMParser oParser;
	MemBufInputSource *oInput = NULL;

	if(socket.connect(*a_addr)!=E_SUCCESS)
		return E_UNKNOWN;

#ifdef DEBUG
// 	CAMsg::printMsg(LOG_DEBUG, "CAInfoServiceAware::sendInfoserviceRequest - connected to InfoService\n");
#endif
	//Send request
	httpClient.setSocket(&socket);
	if( a_postData != NULL )
		httpClient.sendPostRequest(a_strRequest, a_postData, a_postLen);
	else
		httpClient.sendGetRequest(a_strRequest);
	httpClient.parseHTTPHeader(&contentLength, &status);
#ifdef DEBUG
// 	CAMsg::printMsg(LOG_DEBUG, "CAInfoServiceAware::sendInfoserviceRequest - Request sent, HTTP status: %i, content length: %i\n", status,
// 	                contentLength);
#endif
	if(status!=200|| contentLength>MAX_CONTENT_LENGTH)
	{
		return E_UNKNOWN;
	}
	content = new UINT8[contentLength];

	if(httpClient.getContent(content, &contentLength)!=E_SUCCESS)
	{
		delete []content;
		content = NULL;
		return E_UNKNOWN;
	}
#ifdef DEBUG
// 	CAMsg::printMsg(LOG_DEBUG, "CAInfoServiceAware::sendInfoserviceRequest - Answer was %s\n", (char*)content);
#endif
	socket.close();
	//Parse XML
	oInput = new MemBufInputSource( content, contentLength, "tmp" );
	oParser.parse( *oInput );
	delete[] content;
	content = NULL;
	delete oInput;
	oInput = NULL;
	doc = oParser.getDocument();
	if(doc==NULL)
		return E_UNKNOWN;
	*r_elemRoot=doc.getDocumentElement();
	return E_SUCCESS;
}

/**
  * LERNGRUPPE
  * Sends a request to the InfoService at the given CASocketAddrINet. The result should be a XML structure which is returned by r_elemRoot
  * The request issued will be a GET request.
  * @param a_addr The CASocketAddrINet of the InfoService
  * @param a_strRequest The request to send
  * @return r_elemRoot The root element of the resulting XML structure
  * @retval E_SUCCESS if everything went well
  * @retval E_UNKNOWN otherwise
  */
SINT32 CAInfoServiceAware::sendInfoserviceGetRequest(CASocketAddrINet* a_addr, UINT8 *a_strRequest, DOM_Element *r_elemRoot)
{
	return sendInfoserviceRequest(a_addr, a_strRequest, r_elemRoot, NULL, 0);
}

/**
  * LERNGRUPPE
  * Sends a request to a random InfoService from the list of known InfoServices in the options. The result should be a XML structure which is returned by r_elemRoot
	* The request issued is a GET request.
  * @param a_strRequest The request to send
  * @return r_elemRoot The root element of the resulting XML structure
  * @retval E_SUCCESS if everything went well
  * @retval E_UNKNOWN otherwise
  */
SINT32 CAInfoServiceAware::sendRandomInfoserviceGetRequest(UINT8 *a_strRequest, DOM_Element *r_elemRoot)
{
	return sendRandomInfoserviceRequest(a_strRequest, r_elemRoot, NULL, 0);
}

/**
  * LERNGRUPPE
  * Sends a request to a random InfoService from the list of known InfoServices in the options. The result should be a XML structure which is returned by r_elemRoot
  * @param a_addr The CASocketAddrINet of the InfoService
  * @param a_strRequest The request to send
  * @param a_postData POST data to include in the request (if this is NULL, a GET request will be issued, otherwise a POST request)
  * @param a_postLen The length of a_postData
  * @return r_elemRoot The root element of the resulting XML structure
  * @retval E_SUCCESS if everything went well
  * @retval E_UNKNOWN otherwise
  */
SINT32 CAInfoServiceAware::sendRandomInfoserviceRequest(UINT8 *a_strRequest, DOM_Element *r_elemRoot, UINT8* a_postData, UINT32 a_postLen)
{
	CASocketAddrINet *address;
	if( pglobalOptions->getRandomInfoService(address) != E_SUCCESS)
	{
// 		CAMsg::printMsg( LOG_ERR, "CAInfoServiceAware::sendInfoserviceRequest - Unable to get a random InfoService - This will cause problems! Check your configuration!\n");
		return E_UNKNOWN;
	}

	return sendInfoserviceRequest(address, a_strRequest, r_elemRoot, a_postData, a_postLen);
}

#endif // DYNAMIC_MIX
