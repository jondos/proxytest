#include "CAXMLCostConfirmation.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

CAXMLCostConfirmation::CAXMLCostConfirmation(UINT8 * strXmlData) 
	: CAAbstractXMLSignable()
{
	// parse XML
	MemBufInputSource oInput( strXmlData, strlen((char*)strXmlData), "XMLCostConfirmation" );
	DOMParser oParser;
	oParser.parse( oInput );
	DOM_Document doc = oParser.getDocument();
	DOM_Element elemRoot = doc.getDocumentElement();
	if(!ms_pStrElemName) initXMLElementName();
	m_pStrAiName = NULL;
	setValues(elemRoot);
}


CAXMLCostConfirmation::CAXMLCostConfirmation(DOM_Document &doc)
	: CAAbstractXMLSignable()
{
	DOM_Element elemRoot = doc.getDocumentElement();
	if(!ms_pStrElemName) initXMLElementName();
	m_pStrAiName = NULL;
	setValues(elemRoot);
}

CAXMLCostConfirmation::CAXMLCostConfirmation(DOM_Element &elemRoot)
	: CAAbstractXMLSignable()
{
	if(!ms_pStrElemName) initXMLElementName();
	m_pStrAiName = NULL;
	setValues(elemRoot);
}

void CAXMLCostConfirmation::initXMLElementName()
{
	UINT8* elemName = (UINT8*)"CC";
	ms_pStrElemName = new UINT8[strlen((char *)elemName)+1];
	strcpy((char*)ms_pStrElemName, (char*)elemName);
}

UINT8 * CAXMLCostConfirmation::getXMLElementName()
{
	if(!ms_pStrElemName)
		initXMLElementName();
	UINT8 * pTmpStr = new UINT8[strlen((char*)ms_pStrElemName)+1];
	strcpy((char*)pTmpStr, (char*)ms_pStrElemName);
	return pTmpStr;
}


CAXMLCostConfirmation::~CAXMLCostConfirmation()
{
	CAMsg::printMsg(LOG_DEBUG, "XMLCC destructor");
	if(m_pStrAiName)
		delete[] m_pStrAiName;
}


SINT32 CAXMLCostConfirmation::setValues(DOM_Element &elemRoot)
{
	DOM_Element elem; 
	UINT8 strGeneral[128];
	UINT32 strGeneralLen = 128;
	char * strTagname = elemRoot.getTagName().transcode();
	if( (strcmp((char *)strTagname, (char *)ms_pStrElemName)!=0) )
	{
		delete[] strTagname;
		return E_UNKNOWN;
	}
	
	// parse AI Name
	getDOMChildByName(elemRoot, (UINT8*)"AiID", elem, false);
	getDOMElementValue(elem, strGeneral, &strGeneralLen);
	if(m_pStrAiName) delete[] m_pStrAiName;
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
	if(!elem.isNull())
	{
		setSignature(elem);
	}
	
	delete[] strTagname;
	return E_SUCCESS;
}


SINT32 CAXMLCostConfirmation::toXmlElement(DOM_Document &a_doc, DOM_Element &elemRoot)
{
	elemRoot = a_doc.createElement((char *)ms_pStrElemName);
	setDOMElementAttribute(elemRoot, "version", (UINT8*)"1.0");
	
	DOM_Element elem = a_doc.createElement("TransferredBytes");
	setDOMElementValue(elem, m_lTransferredBytes);
	elemRoot.appendChild(elem);
	
	elem = a_doc.createElement("AccountNumber");
	setDOMElementValue(elem, m_lAccountNumber);
	elemRoot.appendChild(elem);
	
	elem = a_doc.createElement("AiID");
	setDOMElementValue(elem, m_pStrAiName);
	elemRoot.appendChild(elem);
	
	if(!m_signature.isNull())
	{
		elemRoot.appendChild(a_doc.importNode(m_signature, true));
	}
	
	return E_SUCCESS;
}
