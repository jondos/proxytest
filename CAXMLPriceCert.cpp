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

const UINT8* const CAXMLPriceCert::ms_pStrElemName=(UINT8*)"PriceCertificate";

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
		m_domDocument=NULL;	
	}

CAXMLPriceCert* CAXMLPriceCert::getInstance(const UINT8 * strXmlData,UINT32 strXmlDataLen)
{
	// parse XML
	if(strXmlData==NULL)
		return NULL;
	MemBufInputSource oInput( strXmlData, strXmlDataLen, "XMLPriceCert" );//third parameter seems to be an arbitrary bufferId internal to Xerces
	DOMParser oParser;
	oParser.parse( oInput );
	CAXMLPriceCert* pPC=new CAXMLPriceCert();
	pPC->m_domDocument = oParser.getDocument();
	if(pPC->setValues()!=E_SUCCESS)
		{
			delete pPC;
			return NULL;
		}
	return pPC;	
}

CAXMLPriceCert* CAXMLPriceCert::getInstance(DOM_Element &elemRoot)
	{
		if(elemRoot==NULL)
			{
				CAMsg::printMsg(LOG_DEBUG,"CAXMLPriceCert::getInstance: root element is null\n");
				return NULL;
			}	
		CAXMLPriceCert* pPC=new CAXMLPriceCert();
		pPC->m_domDocument=DOM_Document::createDocument();
		pPC->m_domDocument.appendChild(pPC->m_domDocument.importNode(elemRoot,true));
		if(pPC->setValues()!=E_SUCCESS)
			{
				delete pPC;
				CAMsg::printMsg(LOG_DEBUG,"CAXMLPriceCert::getInstance.setValues() FAILED \n");
				return NULL;
			}
		return pPC;	
	}

SINT32 CAXMLPriceCert::toXmlElement(DOM_Document &a_doc, DOM_Element &elemRoot)
	{
		elemRoot = a_doc.createElement("PriceCertificate");
	
		setDOMElementAttribute(elemRoot,"version",(UINT8*)"1.1"); 

		DOM_Element elemHashOfMixCert = a_doc.createElement("SubjectKeyIdentifier");
		setDOMElementValue(elemHashOfMixCert,m_StrSubjectKeyIdentifier);
		elemRoot.appendChild(elemHashOfMixCert);
	
		DOM_Element elemRate = a_doc.createElement("Rate");
		setDOMElementValue(elemRate,m_lRate);
		elemRoot.appendChild(elemRate);
	
		DOM_Element elemCreationTime = a_doc.createElement("SignatureTime");
		setDOMElementValue(elemCreationTime,m_StrSignatureTime);
		elemRoot.appendChild(elemCreationTime);
	
		DOM_Element elemBiID = a_doc.createElement("BiID");
		setDOMElementValue(elemBiID,m_StrBiID);
		elemRoot.appendChild(elemBiID);
	
		//append signature node
		if (m_signature != NULL)
		{			
			elemRoot.appendChild(a_doc.importNode(m_signature,true));
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
	DOM_Element elemRoot=m_domDocument.getDocumentElement();
	DOM_Element elem;
	
	char * strTagname = elemRoot.getTagName().transcode();
	if( (strcmp((char *)strTagname, (char *)ms_pStrElemName)!=0) )
		{
			delete[] strTagname;
			CAMsg::printMsg(LOG_DEBUG,"setValues(): failed to get root elem tagname\n");
			return E_UNKNOWN;
		}
	delete[] strTagname;
	//TODO: parsing Strings could be generalized instead of copy-and-paste code for each element
	UINT8 strGeneral[512];
	UINT32 strGeneralLen = 512;
	// parse subjectkeyidentifier(UINT8*)
	if(m_StrSubjectKeyIdentifier!=NULL)
		delete[] m_StrSubjectKeyIdentifier;
	m_StrSubjectKeyIdentifier=NULL;

	getDOMChildByName(elemRoot, (UINT8*)"SubjectKeyIdentifier", elem, false);
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
	getDOMChildByName(elemRoot, (UINT8*)"Rate", elem, false);
	if(getDOMElementValue(elem, &m_lRate)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	CAMsg::printMsg(LOG_DEBUG,"parsing of rate OK \n");



	// parse creation time (UINT8*)
	if(m_StrSignatureTime!=NULL)
		delete[] m_StrSignatureTime;
	m_StrSignatureTime=NULL;
	getDOMChildByName(elemRoot, (UINT8*)"SignatureTime", elem, false);
	
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
	getDOMChildByName(elemRoot, (UINT8*)"BiID", elem, false);
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
	getDOMChildByName(elemRoot, (UINT8*)"Signature", m_signature, true);
	//returns E_UNKNOWN if no signature node present, right now we dont care 
	
	
	
		
	
	CAMsg::printMsg(LOG_DEBUG,"setValues(): finished\n");
	return E_SUCCESS;
}


#endif //PAYMENT
#endif //ONLY_LOCAL_PROXY
