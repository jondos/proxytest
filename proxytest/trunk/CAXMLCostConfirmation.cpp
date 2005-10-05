#include "StdAfx.h"
#include "CAXMLCostConfirmation.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

UINT8* CAXMLCostConfirmation::ms_pStrElemName=NULL;

CAXMLCostConfirmation::CAXMLCostConfirmation(UINT8 * strXmlData) 
{
	// parse XML
	MemBufInputSource oInput( strXmlData, strlen((char*)strXmlData), "XMLCostConfirmation" );
	DOMParser oParser;
	oParser.parse( oInput );
	m_domDocument = oParser.getDocument();
	m_pStrAiName = NULL;
	setValues();
}


CAXMLCostConfirmation::CAXMLCostConfirmation(DOM_Element &elemRoot)
{
	m_pStrAiName = NULL;
	m_domDocument=DOM_Document::createDocument();
	m_domDocument.appendChild(m_domDocument.importNode(elemRoot,true));
	setValues();
}

UINT8 * CAXMLCostConfirmation::getXMLElementName()
{
	if(!CAXMLCostConfirmation::ms_pStrElemName)
	{
		UINT8* elemName = (UINT8*)"CC";
		CAXMLCostConfirmation::ms_pStrElemName = new UINT8[strlen((char *)elemName)+1];
		strcpy((char*)CAXMLCostConfirmation::ms_pStrElemName, (char*)elemName);
	}
	return CAXMLCostConfirmation::ms_pStrElemName;
}

CAXMLCostConfirmation::~CAXMLCostConfirmation()
	{
		if(m_pStrAiName!=NULL)
			delete[] m_pStrAiName;
		m_domDocument=NULL;
	}


SINT32 CAXMLCostConfirmation::setValues()
	{
		DOM_Element elemRoot=m_domDocument.getDocumentElement();
		DOM_Element elem; 
		UINT8 strGeneral[128];
		UINT32 strGeneralLen = 128;
	
		char * strTagname = elemRoot.getTagName().transcode();
		if( (strcmp((char *)strTagname, (char *)getXMLElementName())!=0) )
			{
				delete[] strTagname;
				return E_UNKNOWN;
			}
		delete[] strTagname;
	
		// parse AI Name
		getDOMChildByName(elemRoot, (UINT8*)"AiID", elem, false);
		getDOMElementValue(elem, strGeneral, &strGeneralLen);
		if(m_pStrAiName!=NULL) 
			delete[] m_pStrAiName;
		m_pStrAiName = new UINT8[strGeneralLen+1];
		strcpy((char*)m_pStrAiName, (char*)strGeneral);
	
		// parse accountnumber
		getDOMChildByName(elemRoot, (UINT8*)"AccountNumber", elem, false);
		getDOMElementValue(elem, m_lAccountNumber);
	
		// parse transferredBytes
		getDOMChildByName(elemRoot, (UINT8*)"TransferredBytes", elem, false);
		getDOMElementValue(elem, m_lTransferredBytes);
	
		return E_SUCCESS;
	}
