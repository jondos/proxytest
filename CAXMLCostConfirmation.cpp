/*
Copyright (c) 2000, The JAP-Team
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the  following disclaimer.

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
#include "CAXMLCostConfirmation.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"
#include "CAPriceInfo.hpp" 

const char* const CAXMLCostConfirmation::ms_pStrElemName="CC";

CAXMLCostConfirmation::CAXMLCostConfirmation()
	{
		m_domDocument = NULL;
		m_pStrPIID = NULL;
		m_priceCerts = NULL;

	}

CAXMLCostConfirmation* CAXMLCostConfirmation::getInstance(UINT8 * strXmlData,UINT32 strXmlDataLen)
	{
		// parse XML
		if(strXmlData==NULL)
			return NULL;
		CAXMLCostConfirmation* pCC=new CAXMLCostConfirmation();
		pCC->m_domDocument = parseDOMDocument(strXmlData, strXmlDataLen);
		if(pCC->setValues()!=E_SUCCESS)
			{
				delete pCC;
				return NULL;
			}
		return pCC;
	}

CAXMLCostConfirmation* CAXMLCostConfirmation::getInstance(DOMElement* elemRoot)
	{
		if(elemRoot==NULL)
			return NULL;
		CAXMLCostConfirmation* pCC=new CAXMLCostConfirmation();
		pCC->m_domDocument=createDOMDocument();
		pCC->m_domDocument->appendChild(pCC->m_domDocument->importNode(elemRoot,true));
		if(pCC->setValues()!=E_SUCCESS)
			{
				delete pCC;
				return NULL;
			}
		return pCC;
	}



CAXMLCostConfirmation::~CAXMLCostConfirmation()
	{

		if(m_pStrPIID!=NULL)
		{
			delete[] m_pStrPIID;
		}
		
		if (m_priceCerts != NULL)
		{
			for (UINT32 i = 0; i < m_priceCertsLen; i++)
			{
				if (m_priceCerts[i])
				{
					delete m_priceCerts[i];				
				}
			}
			delete[] m_priceCerts;
			m_priceCerts = NULL;
		}
		
		m_domDocument=NULL;
	}


SINT32 CAXMLCostConfirmation::setValues()
	{
		if(m_domDocument==NULL)
			return E_UNKNOWN;
		DOMElement* elemRoot=m_domDocument->getDocumentElement();
		if (elemRoot == NULL)
		{
			return E_UNKNOWN;
		}
		
		DOMElement* elem=NULL;


		if( !equals(elemRoot->getTagName(),ms_pStrElemName) )
			{
				return E_UNKNOWN;
			}

		// parse accountnumber
		getDOMChildByName(elemRoot, "AccountNumber", elem, false);
		if(getDOMElementValue(elem, m_lAccountNumber)!=E_SUCCESS)
			return E_UNKNOWN;

		// parse transferredBytes
		getDOMChildByName(elemRoot, "TransferredBytes", elem, false);
		if(getDOMElementValue(elem, m_lTransferredBytes)!=E_SUCCESS)
			return E_UNKNOWN;

		// parse PIID
		if(m_pStrPIID!=NULL)
			delete[] m_pStrPIID;
		m_pStrPIID=NULL;
		UINT8 strGeneral[256];
		UINT32 strGeneralLen = 256;
		strGeneralLen=255;
		getDOMChildByName(elemRoot, "PIID", elem, false);
		if(getDOMElementValue(elem, strGeneral, &strGeneralLen)==E_SUCCESS)
		{
			m_pStrPIID = new UINT8[strGeneralLen+1];
			memcpy(m_pStrPIID, strGeneral,strGeneralLen+1);
		}
		else
		{
			return E_UNKNOWN;
		}			
			

/** we do not use the price cert hashes for anything in the AI
 * except storing them as part of the xml string in the db
 * 
 * if you need them, make sure the destructor deletes them
 * (and initialize certArray with new(), otherwise it, and m_priceCerts with it, 
 *  will be gone by the end of this method!!!)
 * 
 */
 
 	//CAMsg::printMsg(LOG_DEBUG, "Parsing PriceCertificates\n");
 
		//parse PriceCertHash elements 
		//currently does not check syntax, e.g. whether <PriceCertHash> is within <PriceCertificates>
		if (getDOMChildByName(elemRoot, "PriceCertificates", elem, false) != E_SUCCESS)
		{
			return E_UNKNOWN;
		}		
		
		//CAMsg::printMsg(LOG_DEBUG, "Looking for PriceCertHash\n");
		
		// one last test if the tag is really in the right XML layer; throw away elemRoot here...		
		if (getDOMChildByName(elem, "PriceCertHash", elemRoot, false) != E_SUCCESS)
		{
			return E_UNKNOWN;
		}
		
		//CAMsg::printMsg(LOG_DEBUG, "Parsing PriceCertHash\n");
		
		DOMNodeList* theNodes = getElementsByTagName(elem,"PriceCertHash");
		if (theNodes->getLength() <= 0)
		{
			return E_UNKNOWN;
		}
		
		//determine size and build array
		m_priceCertsLen = theNodes->getLength();
		m_priceCerts = new CAPriceInfo*[m_priceCertsLen];
		
		//loop through nodes
		const DOMNode* curNode=NULL;
		UINT8* curId;
		UINT8* curHash;
		UINT32 len;
		SINT32 curPosition;
		
		for (UINT32 i = 0; i < m_priceCertsLen; i++ )
		{
			m_priceCerts[i] = NULL;
		}
		
		for (UINT32 i = 0; i < m_priceCertsLen; i++ )
		{
			//get single node
			curNode = theNodes->item(i);
			
			
			//CAMsg::printMsg(LOG_DEBUG, "Parsing id\n");
			
			//extract strings for mixid and pricecerthash, and check isAI attribute
			curId = new UINT8[100];
			len = 100;
			if (getDOMElementAttribute(curNode, "id", curId, &len) != E_SUCCESS)
			{
				delete curId;
				return E_UNKNOWN;
			}
		

			//CAMsg::printMsg(LOG_DEBUG, "Parsing hash\n");
			curHash = new UINT8[100];
			len = 100;
			if (getDOMElementValue(curNode, curHash, &len) != E_SUCCESS)
			{
				delete curId;
				delete curHash;
				return E_UNKNOWN;
			}
			
			//CAMsg::printMsg(LOG_DEBUG, "Parsing position\n");
			if (getDOMElementAttribute(curNode, "position", &curPosition) != E_SUCCESS)
			{
				curPosition = -1;
			}
			
			//CAMsg::printMsg(LOG_DEBUG, "Adding cert info\n");
			m_priceCerts[i] = new CAPriceInfo(curId, curHash, curPosition);	
		}

		
			
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
#endif //PAYMENT
