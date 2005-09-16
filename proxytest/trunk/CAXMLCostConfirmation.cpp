#include "StdAfx.h"
#include "CAXMLCostConfirmation.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

UINT8* CAXMLCostConfirmation::ms_pStrElemName=NULL;

CAXMLCostConfirmation::CAXMLCostConfirmation(UINT8 * strXmlData) 
	: CAAbstractXMLSignable()
{
	// parse XML
	MemBufInputSource oInput( strXmlData, strlen((char*)strXmlData), "XMLCostConfirmation" );
	DOMParser oParser;
	oParser.parse( oInput );
	DOM_Document doc = oParser.getDocument();
	DOM_Element elemRoot = doc.getDocumentElement();
	//if(!ms_pStrElemName) initXMLElementName();
	m_pStrAiName = NULL;
	setValues(elemRoot);
}


CAXMLCostConfirmation::CAXMLCostConfirmation(DOM_Document &doc)
	: CAAbstractXMLSignable()
{
	DOM_Element elemRoot = doc.getDocumentElement();
	//if(!ms_pStrElemName) initXMLElementName();
	m_pStrAiName = NULL;
	setValues(elemRoot);
}

CAXMLCostConfirmation::CAXMLCostConfirmation(DOM_Element &elemRoot)
	: CAAbstractXMLSignable()
{
	//if(!ms_pStrElemName) initXMLElementName();
	m_pStrAiName = NULL;
	setValues(elemRoot);
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
	}


SINT32 CAXMLCostConfirmation::setValues(DOM_Element &elemRoot)
	{
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
	
	// parse signature
		getDOMChildByName(elemRoot, (UINT8*)"Signature", elem, false);
		if(elem.isNull())
			{
				CAMsg::printMsg(LOG_DEBUG, "CAXMLCostConfirmation::setValues(): not signed!!!\n");
			}
		else
			{
				setSignature(elem);
			}
		return E_SUCCESS;
	}


SINT32 CAXMLCostConfirmation::toXmlElement(DOM_Document &a_doc, DOM_Element &elemRoot)
{
	elemRoot = a_doc.createElement((char *)getXMLElementName());
	setDOMElementAttribute(elemRoot, "version", (UINT8*)"1.0");
	
	DOM_Element elem = a_doc.createElement("AiID");
	setDOMElementValue(elem, m_pStrAiName);
	elemRoot.appendChild(elem);
		
	elem = a_doc.createElement("TransferredBytes");
	setDOMElementValue(elem, m_lTransferredBytes);
	elemRoot.appendChild(elem);
	
	elem = a_doc.createElement("AccountNumber");
	setDOMElementValue(elem, m_lAccountNumber);
	elemRoot.appendChild(elem);
	
	
	if(!m_signature.isNull())
	{
		DOM_Element elemSig1 = m_signature.getDocumentElement();
		DOM_Node elemSig = a_doc.importNode(elemSig1, true);
		elemRoot.appendChild(elemSig);
	}
	else
	{
		CAMsg::printMsg(LOG_DEBUG, "toXmlElement signature is NULL!!\n");
	}
	
	return E_SUCCESS;
}
