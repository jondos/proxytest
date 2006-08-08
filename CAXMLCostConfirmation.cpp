#include "StdAfx.h"
#ifndef ONLY_LOCAL_PROXY
#include "CAXMLCostConfirmation.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

const UINT8* const CAXMLCostConfirmation::ms_pStrElemName=(UINT8*)"CC";

CAXMLCostConfirmation::CAXMLCostConfirmation()
	{
		m_domDocument = NULL;
		m_pStrAiName = NULL;
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
		if(m_pStrAiName!=NULL)
			delete[] m_pStrAiName;
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

		// parse AI Name
		getDOMChildByName(elemRoot, (UINT8*)"AiID", elem, false);
		if(m_pStrAiName!=NULL)
			delete[] m_pStrAiName;
		m_pStrAiName=NULL;
		UINT8 strGeneral[256];
		UINT32 strGeneralLen = 256;
		if(getDOMElementValue(elem, strGeneral, &strGeneralLen)==E_SUCCESS)
			{
				m_pStrAiName = new UINT8[strGeneralLen+1];
				memcpy(m_pStrAiName, strGeneral,strGeneralLen+1);
			}
		else
			{
				return E_UNKNOWN;
			}

		// parse PIID
		if(m_pStrPIID!=NULL)
			delete[] m_pStrPIID;
		m_pStrPIID=NULL;
		strGeneralLen=255;
		getDOMChildByName(elemRoot, (UINT8*)"PIID", elem, false);
		if(getDOMElementValue(elem, strGeneral, &strGeneralLen)==E_SUCCESS)
			{
				m_pStrPIID = new UINT8[strGeneralLen+1];
				memcpy(m_pStrPIID, strGeneral,strGeneralLen+1);
			}
		else
			{
				delete[] m_pStrAiName;
				m_pStrAiName=NULL;
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
