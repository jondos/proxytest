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
#ifndef ONLY_LOCAL_PROXY
#include "CAXMLErrorMessage.hpp"

CAXMLErrorMessage::CAXMLErrorMessage(const UINT32 errorCode, UINT8 * message)
	: CAAbstractXMLEncodable()
	{
		m_iErrorCode = errorCode;
		m_strErrMsg = new UINT8[strlen((char *)message)+1];
		strcpy((char *)m_strErrMsg, (char *)message);
	}



CAXMLErrorMessage::CAXMLErrorMessage(UINT32 errorCode)
	: CAAbstractXMLEncodable()
	{
		UINT8 *errors[] = {
			(UINT8*)"Success", (UINT8*)"Internal Server Error",
			(UINT8*)"Wrong format", (UINT8*)"Wrong Data", (UINT8*)"Key not found", 
			(UINT8*)"Bad Signature", (UINT8*)"Bad request", 
			(UINT8*)"You refused to send an account certificate. I will close the connection.",
			(UINT8*)"You refused to send a current balance. I will close the connection.",
			(UINT8*)"You refused to send a cost confirmation. I will close the connection.",
			(UINT8*)"Your account is empty."
		};
		m_iErrorCode = errorCode;
		if (m_iErrorCode < 0 || m_iErrorCode >= 11)
		{
			UINT8 defaultMsg[] = "Unknown Error";
			m_strErrMsg = new UINT8[strlen((char *)defaultMsg)+1];
			strcpy((char *)m_strErrMsg, (char *)defaultMsg);
		}
		else
		{
			m_strErrMsg = new UINT8[strlen((char *)errors[errorCode])+1];
			strcpy((char *)m_strErrMsg, (char *)errors[errorCode]);
		}
	}


CAXMLErrorMessage::CAXMLErrorMessage(UINT8 * strXmlData)
	: CAAbstractXMLEncodable()
{
	MemBufInputSource oInput( strXmlData, strlen((char*)strXmlData), "XMLErrorMessage" );
	DOMParser oParser;
	oParser.parse( oInput );
	DOM_Document doc = oParser.getDocument();
	DOM_Element elemRoot = doc.getDocumentElement();
	m_strErrMsg=NULL;
	setValues(elemRoot);
}


SINT32 CAXMLErrorMessage::setValues(DOM_Element &elemRoot)
{
	UINT8 strGeneral[256];
	UINT32 strGeneralLen = 256;
	SINT32 tmp;
	SINT32 rc;
	if( ((rc=getDOMElementAttribute(elemRoot, "code", &tmp)) !=E_SUCCESS) ||
			((rc=getDOMElementValue(elemRoot, strGeneral, &strGeneralLen)) !=E_SUCCESS)
		)
	{
		return rc;
	}
	m_iErrorCode = (UINT32)tmp;
	if(m_strErrMsg) delete [] m_strErrMsg;
	m_strErrMsg = new UINT8[strGeneralLen+1];
	strcpy((char*)m_strErrMsg, (char*)strGeneral);
	return ERR_OK;
}


CAXMLErrorMessage::~CAXMLErrorMessage()
	{
		if(m_strErrMsg)
			delete [] m_strErrMsg;
	}


SINT32 CAXMLErrorMessage::toXmlElement(DOM_Document &a_doc, DOM_Element &elemRoot)
	{
		elemRoot = a_doc.createElement("ErrorMessage");
		setDOMElementAttribute(elemRoot, "code", m_iErrorCode);
		setDOMElementValue(elemRoot, m_strErrMsg);
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
