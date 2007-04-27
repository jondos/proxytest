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
#ifndef ONLY_LOCAL_PROXY
#include "CAXMLCostConfirmation.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"
#include "CAPriceInfo.hpp" 

const UINT8* const CAXMLCostConfirmation::ms_pStrElemName=(UINT8*)"CC";

CAXMLCostConfirmation::CAXMLCostConfirmation()
	{
		m_domDocument = NULL;
		m_pStrPIID = NULL;

	}

CAXMLCostConfirmation* CAXMLCostConfirmation::getInstance(UINT8 * strXmlData,UINT32 strXmlDataLen)
	{
		// parse XML
		if(strXmlData==NULL)
			return NULL;
		MemBufInputSource oInput( strXmlData, strXmlDataLen, "XMLCostConfirmation" );
		DOMParser oParser;
		oParser.parse( oInput );
		CAXMLCostConfirmation* pCC=new CAXMLCostConfirmation();
		pCC->m_domDocument = oParser.getDocument();
		if(pCC->setValues()!=E_SUCCESS)
			{
				delete pCC;
				return NULL;
			}
		return pCC;
	}

CAXMLCostConfirmation* CAXMLCostConfirmation::getInstance(DOM_Element &elemRoot)
	{
		if(elemRoot==NULL)
			return NULL;
		CAXMLCostConfirmation* pCC=new CAXMLCostConfirmation();
		pCC->m_domDocument=DOM_Document::createDocument();
		pCC->m_domDocument.appendChild(pCC->m_domDocument.importNode(elemRoot,true));
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
			delete[] m_pStrPIID;
		
		m_domDocument=NULL;
	}


SINT32 CAXMLCostConfirmation::setValues()
	{
		if(m_domDocument==NULL)
			return E_UNKNOWN;
		DOM_Element elemRoot=m_domDocument.getDocumentElement();
		DOM_Element elem;

		char * strTagname = elemRoot.getTagName().transcode();
		if( (strcmp((char *)strTagname, (char *)ms_pStrElemName)!=0) )
			{
				delete[] strTagname;
				return E_UNKNOWN;
			}
		delete[] strTagname;

		// parse accountnumber
		getDOMChildByName(elemRoot, (UINT8*)"AccountNumber", elem, false);
		if(getDOMElementValue(elem, m_lAccountNumber)!=E_SUCCESS)
			return E_UNKNOWN;

		// parse transferredBytes
		getDOMChildByName(elemRoot, (UINT8*)"TransferredBytes", elem, false);
		if(getDOMElementValue(elem, m_lTransferredBytes)!=E_SUCCESS)
			return E_UNKNOWN;

/** we do not use the price cert hashes for anything in the AI
 * except storing them as part of the xml string in the db
 * 
 * if you need them, make sure the destructor deletes them
 * (and initialize certArray with new(), otherwise it, and m_priceCerts with it, 
 *  will be gone by the end of this method!!!)
 * 
 * 
		//parse PriceCertHash elements 
		//currently does not check syntax, e.g. whether <PriceCertHash> is within <PriceCertificates>
		DOM_NodeList theNodes = elemRoot.getElementsByTagName("PriceCertHash");//should work with root, if not try getting the PriceCertificates from elemRoot first, and then PriceCertHash elements from it
		//determine size and build array
		int nrOfCerts = theNodes.getLength(); //return XMLSim_pStrPriceCertHashze_t, conversion to int okay?
		CAPriceInfo* certArray[nrOfCerts];
		//loop through nodes
		CAPriceInfo* curInfo;
		DOM_Node curNode;
		UINT8* curId;
		UINT8* curPriceCert;
		DOM_Node aiNode;
		DOM_Node idNode;
		bool isAiFound; 
		UINT8* aiAttr;
		for (int i = 0; i< nrOfCerts; i++ )
		{
			isAiFound = false;
			//get single node
			curNode = theNodes.item(i);
			//extract strings for mixid and pricecerthash, and check isAI attribute
			idNode = curNode.getAttributes().getNamedItem("id");
			curId = (UINT8*) idNode.getNodeValue().transcode(); //need transcode() to transform DOMString to char*
			curPriceCert = (UINT8*) curNode.getNodeValue().transcode();
			aiNode = curNode.getAttributes().getNamedItem("isAI"); 
			if ( aiNode != NULL)
			{
				aiAttr = (UINT8*) aiNode.getNodeValue().transcode();
				if (strcmp((const char*)aiAttr,"true"))
					isAiFound = true;
			}
			//build PriceInfo instance and add to array
			if (isAiFound == false && curPriceCert == NULL)
			{
				curInfo = new CAPriceInfo(curId);
			} else
			{
				curInfo = new CAPriceInfo(curId, curPriceCert, isAiFound);	
			}
			certArray[i] = curInfo;
			}
		//set member variable to new array
		m_priceCerts = certArray;
		
	******/	

		// parse PIID
		if(m_pStrPIID!=NULL)
			delete[] m_pStrPIID;
		m_pStrPIID=NULL;
		UINT8 strGeneral[256];
		UINT32 strGeneralLen = 256;
		strGeneralLen=255;
		getDOMChildByName(elemRoot, (UINT8*)"PIID", elem, false);
		if(getDOMElementValue(elem, strGeneral, &strGeneralLen)==E_SUCCESS)
			{
				m_pStrPIID = new UINT8[strGeneralLen+1];
				memcpy(m_pStrPIID, strGeneral,strGeneralLen+1);
			}
		else
			{
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY