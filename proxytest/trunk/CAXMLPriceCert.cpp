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
#ifdef PAYMENT
#include "CAXMLPriceCert.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

const char* const CAXMLPriceCert::ms_pStrElemName="PriceCertificate";

CAXMLPriceCert::CAXMLPriceCert()
	{
		m_StrSubjectKeyIdentifier = NULL;
		m_StrSignatureTime = NULL;
		m_StrBiID = NULL;
		m_domDocument = NULL;	
	}

CAXMLPriceCert::~CAXMLPriceCert()
	{
		if (m_StrSubjectKeyIdentifier != NULL) 
			delete[] m_StrSubjectKeyIdentifier;
		if (m_StrSignatureTime != NULL) 
			delete[] m_StrSignatureTime;
		if (m_StrBiID != NULL) 
			delete[] m_StrBiID;
		if(m_domDocument != NULL)
		{
			//CAMsg::printMsg(LOG_DEBUG, "cleaning up internal PriceCert document 0x%x.\n",
			//					m_domDocument);
			m_domDocument->release();
		}
		m_domDocument=NULL;	
	}

CAXMLPriceCert* CAXMLPriceCert::getInstance(const UINT8 * strXmlData,UINT32 strXmlDataLen)
{
	// parse XML
	if(strXmlData==NULL)
		return NULL;
	CAXMLPriceCert* pPC=new CAXMLPriceCert();
	pPC->m_domDocument = parseDOMDocument(strXmlData, strXmlDataLen);
	if(pPC->setValues()!=E_SUCCESS)
		{
			delete pPC;
			return NULL;
		}
	return pPC;	
}

CAXMLPriceCert* CAXMLPriceCert::getInstance(DOMElement* elemRoot)
	{
		if(elemRoot==NULL)
			{
				CAMsg::printMsg(LOG_DEBUG,"CAXMLPriceCert::getInstance: root element is null\n");
				return NULL;
			}	
		CAXMLPriceCert* pPC=new CAXMLPriceCert();
		pPC->m_domDocument=createDOMDocument();
		pPC->m_domDocument->appendChild(pPC->m_domDocument->importNode(elemRoot,true));
		if(pPC->setValues()!=E_SUCCESS)
			{
				delete pPC;
				CAMsg::printMsg(LOG_DEBUG,"CAXMLPriceCert::getInstance.setValues() FAILED \n");
				return NULL;
			}
		return pPC;	
	}

SINT32 CAXMLPriceCert::toXmlElement(XERCES_CPP_NAMESPACE::DOMDocument* a_doc, DOMElement* &elemRoot)
	{
		elemRoot = createDOMElement(a_doc,"PriceCertificate");
	
		setDOMElementAttribute(elemRoot,"version",(UINT8*)"1.1"); 

		DOMElement* elemHashOfMixCert = createDOMElement(a_doc,"SubjectKeyIdentifier");
		setDOMElementValue(elemHashOfMixCert,m_StrSubjectKeyIdentifier);
		elemRoot->appendChild(elemHashOfMixCert);
	
		DOMElement* elemRate = createDOMElement(a_doc,"Rate");
		setDOMElementValue(elemRate,m_lRate);
		elemRoot->appendChild(elemRate);
	
		DOMElement* elemCreationTime = createDOMElement(a_doc,"SignatureTime");
		setDOMElementValue(elemCreationTime,m_StrSignatureTime);
		elemRoot->appendChild(elemCreationTime);
	
		DOMElement* elemBiID = createDOMElement(a_doc,"BiID");
		setDOMElementValue(elemBiID,m_StrBiID);
		elemRoot->appendChild(elemBiID);
	
		//append signature node
		if (m_signature != NULL)
		{			
			elemRoot->appendChild(a_doc->importNode(m_signature,true));
		}
		else
			{
				CAMsg::printMsg(LOG_DEBUG,"Could not import PI signature node!\n");
			}
		return E_SUCCESS;
	}

SINT32 CAXMLPriceCert::setValues() 
{
	if(m_domDocument==NULL)
	{
		CAMsg::printMsg(LOG_DEBUG,"setValues(): no document\n");
		return E_UNKNOWN;
	}
	DOMElement* elemRoot=m_domDocument->getDocumentElement();
	DOMElement* elem=NULL;
	
	if(!equals(elemRoot->getTagName(),ms_pStrElemName))
		{
			CAMsg::printMsg(LOG_DEBUG,"setValues(): failed to get root elem tagname\n");
			return E_UNKNOWN;
		}
	//TODO: parsing Strings could be generalized instead of copy-and-paste code for each element
	UINT8 strGeneral[512];
	UINT32 strGeneralLen = 512;
	// parse subjectkeyidentifier(UINT8*)
	if(m_StrSubjectKeyIdentifier!=NULL)
		delete[] m_StrSubjectKeyIdentifier;
	m_StrSubjectKeyIdentifier=NULL;

	getDOMChildByName(elemRoot, "SubjectKeyIdentifier", elem, false);
	if(getDOMElementValue(elem, strGeneral, &strGeneralLen)==E_SUCCESS)
		{
			m_StrSubjectKeyIdentifier = new UINT8[strGeneralLen+1];
			memcpy(m_StrSubjectKeyIdentifier, strGeneral,strGeneralLen+1);
			CAMsg::printMsg(LOG_DEBUG,"setValues(): parsing subjectkeyidentifier OK\n");
		}
	else
		{
			delete[] m_StrSubjectKeyIdentifier;
			m_StrSubjectKeyIdentifier=NULL;
			CAMsg::printMsg(LOG_DEBUG,"setValues(): failed to parse string of subjectkeyidentifier\n");
			return E_UNKNOWN;
		}
	
	// parse rate (double)
	getDOMChildByName(elemRoot, "Rate", elem, false);
	if(getDOMElementValue(elem, &m_lRate)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	CAMsg::printMsg(LOG_DEBUG,"parsing of rate OK \n");



	// parse creation time (UINT8*)
	if(m_StrSignatureTime!=NULL)
		delete[] m_StrSignatureTime;
	m_StrSignatureTime=NULL;
	getDOMChildByName(elemRoot, "SignatureTime", elem, false);
	
	if(getDOMElementValue(elem, strGeneral, &strGeneralLen)==E_SUCCESS)
		{
			m_StrSignatureTime = new UINT8[strGeneralLen+1];
			memcpy(m_StrSignatureTime, strGeneral,strGeneralLen+1);
			CAMsg::printMsg(LOG_DEBUG,"setValues(): parsing SignatureTime OK\n");
		}
	else
		{
			delete[] m_StrSignatureTime;
			m_StrSignatureTime=NULL;
			CAMsg::printMsg(LOG_DEBUG,"setValues(): parsing SignatureTime failed\n");
			return E_UNKNOWN;
		}
	
	UINT8 strGeneral2[512];
	UINT32 strGeneralLen2 = 512;	
	
	// parse BiID (UINT8*)
	if(m_StrBiID!=NULL)
		delete[] m_StrBiID;
	m_StrBiID=NULL;
	getDOMChildByName(elemRoot, "BiID", elem, false);
	if(getDOMElementValue(elem, strGeneral2, &strGeneralLen2)==E_SUCCESS)
		{
			m_StrBiID = new UINT8[strGeneralLen2+1];
			memcpy(m_StrBiID, strGeneral2,strGeneralLen2+1);
			CAMsg::printMsg(LOG_DEBUG,"setValues(): parsing BiID OK\n");
		}
	else
		{
			delete[] m_StrBiID;
			m_StrBiID=NULL;
			return E_UNKNOWN;
		}
		
		
	//if present, store signature node as element	
	getDOMChildByName(elemRoot, "Signature", m_signature, true);
	//returns E_UNKNOWN if no signature node present, right now we dont care 
	
	
	
		
	
	CAMsg::printMsg(LOG_DEBUG,"setValues(): finished\n");
	return E_SUCCESS;
}


#endif //PAYMENT
#endif //ONLY_LOCAL_PROXY
