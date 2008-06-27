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
#ifndef ONLY_LOCAL_PROXY
#include "CAXMLErrorMessage.hpp"

CAXMLErrorMessage::CAXMLErrorMessage(const UINT32 errorCode, UINT8 * message)
	: CAAbstractXMLEncodable()
	{
		m_strExpires = new char[10];
		m_iErrorCode = errorCode;
		m_strErrMsg = new UINT8[strlen((char *)message)+1];
		strcpy((char *)m_strErrMsg, (char *)message);
		m_messageObject = NULL;
	}



CAXMLErrorMessage::CAXMLErrorMessage(UINT32 errorCode)
	: CAAbstractXMLEncodable()
	{
		UINT8 *errors[] = {
			(UINT8*)"Success",
			(UINT8*)"Internal Server Error",
			(UINT8*)"Wrong format",
			(UINT8*)"Wrong Data", 
			(UINT8*)"Key not found", 
			(UINT8*)"Bad Signature",
			(UINT8*)"Bad request", 
			(UINT8*)"You refused to send an account certificate. I will close the connection.",
			(UINT8*)"You refused to send a current balance. I will close the connection.",
			(UINT8*)"You refused to send a cost confirmation. I will close the connection.",
			(UINT8*)"Your account is empty.",
			(UINT8*)"Cascade is too long",
			(UINT8*)"Database error",
			(UINT8*)"Insufficient balance",
			(UINT8*)"No flatrate offered",
			(UINT8*)"Invalid code",
			(UINT8*)"Costconfirmation is not valid, possible attempt at doublespending!",
			(UINT8*)"One or more price certificates are invalid!",
			(UINT8*)"User is logged in more than once!",
			(UINT8*)"No database record for this cost confirmation was found!",
			(UINT8*)"Operation may have succeded, but this is not clear!",
			(UINT8*)"Account is blocked!"
		};
		m_iErrorCode = errorCode;
		if (m_iErrorCode < 0 || m_iErrorCode >= 19)
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
		m_messageObject = NULL;
		m_strExpires = new char[10];
	}



CAXMLErrorMessage::CAXMLErrorMessage(const UINT32 errorCode, UINT8* message, CAAbstractXMLEncodable* messageObject)
{
	m_iErrorCode = errorCode;
	m_strErrMsg = new UINT8[strlen((char *)message)+1];
	strcpy((char *)m_strErrMsg, (char *)message);
	
	m_messageObject = messageObject;
	m_strExpires = new char[10];
}



CAXMLErrorMessage::CAXMLErrorMessage(UINT8 * strXmlData)
	: CAAbstractXMLEncodable()
{
	m_strErrMsg=NULL;
	m_messageObject = NULL;
	m_strExpires = new char[10];
	XERCES_CPP_NAMESPACE::DOMDocument* doc = parseDOMDocument(strXmlData,strlen((char*)strXmlData));
	
	if(doc != NULL)
	{
		DOMElement* elemRoot = doc->getDocumentElement();
		if (setValues(elemRoot) != E_SUCCESS)
		{
			m_iErrorCode = ERR_NO_ERROR_GIVEN;
		}
		if(doc != NULL)
		{
			doc->release();
			doc = NULL;
		}
	}
	else
	{
		m_iErrorCode = ERR_NO_ERROR_GIVEN;
	}
}


SINT32 CAXMLErrorMessage::setValues(DOMElement* elemRoot)
{	
	UINT8 strGeneral[256];
	UINT32 strGeneralLen = 256;
	
	char strExp[10];
	UINT32 strExpLen = 10;
	
	SINT32 tmp;
	SINT32 rc;
		
	if( ((rc=getDOMElementAttribute(elemRoot, "code", &tmp)) !=E_SUCCESS) ||
			 ((rc=getDOMElementValue(elemRoot, strGeneral, &strGeneralLen)) !=E_SUCCESS)
		)
	{
		UINT8 buff[8192];
		UINT32 len=8192;
		DOM_Output::dumpToMem(elemRoot,buff,&len);
		CAMsg::printMsg(LOG_DEBUG,(char*)buff);
		
		return rc;
	}
	
	m_iErrorCode = (UINT32)tmp;
	if(m_strErrMsg) delete [] m_strErrMsg;
	m_strErrMsg = new UINT8[strGeneralLen+1];
	strcpy((char*)m_strErrMsg, (char*)strGeneral);
	
	/*if((rc=getDOMElementAttribute(elemRoot, "expires", (UINT8*) strExp, &strExpLen)) ==E_SUCCESS)
	{	
		if(m_strExpires != NULL)
		{
			strncpy(m_strExpires, strExp, 10);
		}
	}*/
	DOMElement* objectRootElem=NULL;
	getDOMChildByName(elemRoot, "MessageObject", objectRootElem, false);
	
	//due to lack of RTTI, we need to hardcode how to deal with each specific object type
	if (ERR_OUTDATED_CC == m_iErrorCode)
	{
		DOMElement* ccElem=NULL;
		if (getDOMChildByName(objectRootElem,"CC",ccElem,true) == E_SUCCESS)
		{
			m_messageObject = CAXMLCostConfirmation::getInstance(ccElem);	
		}
	}
	else if (ERR_ACCOUNT_EMPTY == m_iErrorCode)
	{
		DOMElement* confirmedElem=NULL;
		if (getDOMChildByName(objectRootElem,"GenericText",confirmedElem,true) == E_SUCCESS)
		{
			m_messageObject = new UINT64;
			if(getDOMElementValue(confirmedElem, (*(UINT64*)m_messageObject)) != E_SUCCESS)
			{
				delete (UINT64*)m_messageObject;
			}
		}		
	}
	else
	{
		m_messageObject = NULL;
	}
	//add code to parse other types of objects here when adding new error codes with corresponding objects
	
	return E_SUCCESS;
}


CAXMLErrorMessage::~CAXMLErrorMessage()
	{
		if(m_strErrMsg)
		{
			delete [] m_strErrMsg;
			m_strErrMsg = NULL;
		}
		if (m_messageObject != NULL)
		{
			delete m_messageObject;
			m_messageObject = NULL;
		}
		
		if (m_strExpires !=NULL)
		{
			delete[] m_strExpires;
			m_strExpires = NULL;
		}
	}


SINT32 CAXMLErrorMessage::toXmlElement(XERCES_CPP_NAMESPACE::DOMDocument* a_doc, DOMElement* & elemRoot)
	{	
		elemRoot = createDOMElement(a_doc,"ErrorMessage");
		setDOMElementAttribute(elemRoot, "code", m_iErrorCode);
		/*if(m_strExpires != NULL)
		{
			if(m_strExpires[0] != 0)
			{
				setDOMElementAttribute(elemRoot, "expires", (UINT8*)m_strExpires);
			}
		}*/
		setDOMElementValue(elemRoot, m_strErrMsg);

		if (m_messageObject)
		{
			DOMElement* objectRoot = createDOMElement(a_doc,"MessageObject");
			DOMElement* objectElem=NULL;
			//WARNING: this will fail for CAXMLCostConfirmation!!! (since it is not a subclass of CAAbstractXMLEncodable)
			CAAbstractXMLEncodable* encodableObject = (CAAbstractXMLEncodable*) m_messageObject;
			encodableObject->toXmlElement(a_doc,objectElem);
			objectRoot->appendChild(objectElem);
			elemRoot->appendChild(objectRoot);
		}
		
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
#endif //PAYMENT
