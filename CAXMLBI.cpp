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
#if !defined(ONLY_LOCAL_PROXY) && defined (PAYMENT)
#include "CAXMLBI.hpp"
#include "CAMsg.hpp"

const char* const CAXMLBI::ms_pXmlElemName="PaymentInstance";

CAXMLBI::CAXMLBI() : CAAbstractXMLEncodable()
	{
		m_pVeryfire = NULL;
		m_pBiID = NULL;
		m_pHostName = NULL;
		m_pCert = NULL;
	}

CAXMLBI* CAXMLBI::getInstance(const UINT8 * biID, const UINT8 * hostName, const int portNumber, CACertificate * pCert)
	{
		if(biID==NULL||hostName==NULL)
			{
				return NULL;
			}
		CAXMLBI* pBI = new CAXMLBI();		
		if(pCert!=NULL)
			{
				pBI->m_pCert = pCert->clone();
			}
		pBI->m_pBiID = new UINT8[strlen((char*)biID)+1];
		strcpy((char*)pBI->m_pBiID, (char*)biID);
		pBI->m_pHostName = new UINT8[strlen((char*)hostName)+1];
		strcpy((char*)pBI->m_pHostName, (char*)hostName);
		pBI->m_iPortNumber = portNumber;
		pBI->m_pVeryfire = NULL;
		return pBI;
	}

CAXMLBI* CAXMLBI::getInstance(DOMElement* elemRoot)
	{
		if (elemRoot == NULL)
			{
				return NULL;
			}
		CAXMLBI* pPI = new CAXMLBI();
		if(pPI->setValues(elemRoot)!=E_SUCCESS)
			{
				delete pPI;
				return NULL;
			}
		return pPI;
	}

CAXMLBI::~CAXMLBI()
	{
		if(m_pCert!=NULL)
			delete m_pCert;
		if(m_pVeryfire!=NULL)
			delete m_pVeryfire;
		if(m_pBiID!=NULL)
			delete m_pBiID;
		if(m_pHostName!=NULL)
			delete m_pHostName;
	}

SINT32 CAXMLBI::setValues(DOMElement* elemRoot)
	{
		DOMElement* elem=NULL;
		UINT8 strGeneral[256];
		UINT32 strGeneralLen = 255;
		
		if(!equals(elemRoot->getTagName(),CAXMLBI::getXMLElementName()))
		{
			return E_UNKNOWN;
		}
		
		//Parse ID
		if(getDOMElementAttribute(elemRoot, "id", strGeneral, &strGeneralLen)==E_SUCCESS)
			{
				m_pBiID = new UINT8[strGeneralLen+1];
				memcpy(m_pBiID,strGeneral,strGeneralLen);
			}
		else 
			{
				return E_UNKNOWN;
			}
		
		//Parse PI Certificate
		DOMElement* elemCert=NULL;
		getDOMChildByName(elemRoot, "Certificate", elem, false);
		getDOMChildByName(elem, "X509Certificate", elemCert, false);
		CACertificate *pPICert = CACertificate::decode(elemCert, CERT_X509CERTIFICATE, NULL);
		if (pPICert != NULL)
		{
			m_pCert = pPICert;
		}
		else
		{
			CAMsg::printMsg(LOG_CRIT,"No certificate for payment instance available!\n");
			return E_UNKNOWN;
		}
			
		//Parse PI Host
		DOMElement* elemNet=NULL;
		DOMElement* elemListeners=NULL;
		DOMElement* elemListener=NULL;
		DOMElement* elemHost=NULL;
		DOMElement* elemPort=NULL;
		getDOMChildByName(elemRoot, "Network", elemNet, false);
		getDOMChildByName(elemNet, "ListenerInterfaces", elemListeners, false);
		getDOMChildByName(elemListeners, "ListenerInterface", elemListener, false);
		getDOMChildByName(elemListener, "Host", elemHost, false);
		getDOMChildByName(elemListener, "Port", elemPort, false);
		strGeneralLen=255;
		//Parse PI Host and Port
		if(	getDOMElementValue(elemHost, strGeneral, &strGeneralLen)!=E_SUCCESS||
				getDOMElementValue(elemPort, &m_iPortNumber)!=E_SUCCESS)
			{
				delete [] m_pBiID;
				m_pBiID=NULL;
				delete m_pCert;
				m_pCert=NULL;
				return E_UNKNOWN;
			}
		m_pHostName = new UINT8[strGeneralLen+1];
		strcpy((char*)m_pHostName, (char*)strGeneral);
		return E_SUCCESS;
	}
	
SINT32 CAXMLBI::toXmlElement(XERCES_CPP_NAMESPACE::DOMDocument *a_doc, DOMElement* & elemRoot)
	{
		elemRoot = createDOMElement(a_doc,getXMLElementName());
		setDOMElementAttribute(elemRoot, "id", m_pBiID);
		
		//Set network settings
		DOMElement* elemNet = createDOMElement(a_doc,"Network");
		elemRoot->appendChild(elemNet);
		DOMElement* elemListeners = createDOMElement(a_doc,"ListenerInterfaces");
		elemNet->appendChild(elemListeners);	
		DOMElement* elemListener = createDOMElement(a_doc,"ListenerInterface");
		elemListeners->appendChild(elemListener);
		//Set Hostname
		DOMElement* elemHost = createDOMElement(a_doc,"Host");
		elemListener->appendChild(elemHost);
		setDOMElementValue(elemHost, m_pHostName);
		//Set Port
		DOMElement* elemPort = createDOMElement(a_doc,"Port");
		elemListener->appendChild(elemPort);
		setDOMElementValue(elemPort, m_iPortNumber);
		//Set Cert
		if(m_pCert!=NULL)
		{
			DOMElement* elemCert = createDOMElement(a_doc,"Certificate");
			elemRoot->appendChild(elemCert);
			
			DOMDocumentFragment* tmpFrag=NULL;
			m_pCert->encode(tmpFrag, a_doc);
			elemCert->appendChild(tmpFrag);
		}
		
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
