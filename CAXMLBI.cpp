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
#include "CAXMLBI.hpp"

UINT8 * CAXMLBI::ms_pXmlElemName=0;

CAXMLBI::CAXMLBI(const UINT8 * biName, const UINT8 * hostName, const int portNumber, CACertificate * pCert)
	: CAAbstractXMLEncodable()
	{
		m_pCert = pCert;
		m_pBiName = new UINT8[strlen((char*)biName)+1];
		strcpy((char*)m_pBiName, (char*)biName);
		m_pHostName = new UINT8[strlen((char*)hostName)+1];
		strcpy((char*)m_pHostName, (char*)hostName);
		m_iPortNumber = portNumber;
		m_pVeryfire = 0;
	}

CAXMLBI::CAXMLBI(DOM_Element & elemRoot)
	: CAAbstractXMLEncodable()
	{
		m_pVeryfire = 0;
		m_pBiName = 0;
		m_pHostName = 0;
		m_pCert = 0;
		setValues(elemRoot);
	}

CAXMLBI::~CAXMLBI()
	{
		if(m_pCert)
			delete m_pCert;
		if(m_pVeryfire)
			delete m_pVeryfire;
		if(m_pBiName)
			delete m_pBiName;
		if(m_pHostName)
			delete m_pHostName;
	}

SINT32 CAXMLBI::setValues(DOM_Element &elemRoot)
	{
		UINT8 * rootName;
		DOM_Element elem;
		UINT8 tmp[256];
		UINT32 tmpLen;
		
		rootName = (UINT8*) (elemRoot.getTagName().transcode());
		if(strcmp((char*)rootName, (char*)CAXMLBI::getXMLElementName())!=0)
		{
			delete[] rootName;
			return E_UNKNOWN;
		}
		delete[] rootName;
		
		getDOMChildByName(elemRoot, (UINT8*)"BIName", elem, false); 
		if(m_pBiName) 
			{
				delete m_pBiName;
				m_pBiName = 0;
			}
		if(elem!=NULL)
			{
				tmpLen = 256;
				getDOMElementValue(elem, tmp, &tmpLen);
				if(tmpLen>0)
					{
						m_pBiName = new UINT8[tmpLen+1];
						strcpy((char *)m_pBiName, (char *)tmp);
					}
			}
			
		getDOMChildByName(elemRoot, (UINT8*)"HostName", elem, false);
		if(m_pHostName) 
			{
				delete m_pHostName;
				m_pHostName = 0;
			}
		if(elem!=NULL)
			{
				tmpLen = 256;
				getDOMElementValue(elem, tmp, &tmpLen);
				if(tmpLen>0)
					{
						
						m_pHostName = new UINT8[tmpLen+1];
						strcpy((char *)m_pHostName, (char *)tmp);
					}
			}
			
		getDOMChildByName(elemRoot, (UINT8*)"PortNumber", elem, false); 
		if(elem!=NULL)
			{
				getDOMElementValue(elem, &m_iPortNumber);
			}
		else 
			{
				m_iPortNumber = 0;
			}
		
		getDOMChildByName(elemRoot, (UINT8*)"TestCertificate", elem, false);
		if(m_pCert) 
			{
				delete m_pCert;
				m_pCert = NULL;
			}
		if (elem != NULL)
			{
				DOM_Element elemCert;
				getDOMChildByName(elem, CACertificate::getXmlElementName(), elemCert, false);
				CACertificate * pCert = CACertificate::decode(elemCert, CERT_X509CERTIFICATE);
				if(pCert!=NULL)
					m_pCert = pCert;
			}
		return E_SUCCESS;
	}
	
SINT32 CAXMLBI::toXmlElement(DOM_Document &a_doc, DOM_Element &elemRoot)
	{
		elemRoot = a_doc.createElement((char *)getXMLElementName());
		setDOMElementAttribute(elemRoot, "version", (UINT8*)"1.0");
		
		DOM_Element elemName = a_doc.createElement("BIName");
		elemRoot.appendChild(elemName);
		setDOMElementValue(elemName, m_pBiName);
		
		elemName = a_doc.createElement("HostName");
		elemRoot.appendChild(elemName);
		setDOMElementValue(elemName, m_pHostName);
		
		elemName = a_doc.createElement("PortNumber");
		elemRoot.appendChild(elemName);
		setDOMElementValue(elemName, m_iPortNumber);
		
		if(m_pCert!=NULL)
		{
			DOM_DocumentFragment tmpFrag;
			m_pCert->encode(tmpFrag, a_doc);
			elemRoot.appendChild(tmpFrag);
		}
		
		return E_SUCCESS;
	}
	
