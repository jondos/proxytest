#include "StdAfx.h"
#include "CAXMLCostConfirmation.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

const UINT8* const CAXMLCostConfirmation::ms_pStrElemName=(UINT8*)"CC";

CAXMLCostConfirmation::CAXMLCostConfirmation(UINT8 * strXmlData) 
	{
		// parse XML
		MemBufInputSource oInput( strXmlData, strlen((char*)strXmlData), "XMLCostConfirmation" );
		DOMParser oParser;
		oParser.parse( oInput );
		m_domDocument = oParser.getDocument();
		m_pStrAiName = NULL;
		m_pStrPIID = NULL;
		setValues();
	}


	CAXMLCostConfirmation::CAXMLCostConfirmation(DOM_Element &elemRoot)
		{
			m_pStrAiName = NULL;
			m_pStrPIID = NULL;
			m_domDocument=DOM_Document::createDocument();
			m_domDocument.appendChild(m_domDocument.importNode(elemRoot,true));
			setValues();
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
			
		// parse accountnumber
		getDOMChildByName(elemRoot, (UINT8*)"AccountNumber", elem, false);
		getDOMElementValue(elem, m_lAccountNumber);
	
		// parse transferredBytes
		getDOMChildByName(elemRoot, (UINT8*)"TransferredBytes", elem, false);
		getDOMElementValue(elem, m_lTransferredBytes);
		
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
		return E_SUCCESS;
	}
