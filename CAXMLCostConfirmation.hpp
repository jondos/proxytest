#ifndef __CAXMLCOSTCONFIRMATION__
#define __CAXMLCOSTCONFIRMATION__

#include "CAAbstractXMLSignable.hpp"

/**
this class corresponds to anon.pay.xml.XMLEasyCC in the Java implementation

@author Bastian Voigt
*/
class CAXMLCostConfirmation : public CAAbstractXMLSignable
{
public:
	CAXMLCostConfirmation(UINT8 * strXmlData);
	CAXMLCostConfirmation(DOM_Document &doc);
	CAXMLCostConfirmation(DOM_Element &elemRoot);
	~CAXMLCostConfirmation();
	
	SINT32 toXmlElement(DOM_Document &a_doc, DOM_Element &elemRoot);
	UINT64 getTransferredBytes() {return m_lTransferredBytes;}
	UINT64 getAccountNumber() {return m_lAccountNumber;}
	
	/** @return a newly allocated buffer which must be deleted by the caller */
	UINT8 * getAiID() 
		{
			UINT8 * pTmpStr = NULL;
			if(m_pStrAiName)
			{
				pTmpStr = new UINT8[strlen((char*)m_pStrAiName)+1];
				strcpy((char*)pTmpStr, (char*)m_pStrAiName);
			}
			return pTmpStr;
		}
	static UINT8 * getXMLElementName();

private:
	static void initXMLElementName();
	SINT32 setValues(DOM_Element &elemRoot);
	
	UINT64 m_lTransferredBytes;
	UINT64 m_lAccountNumber;
	UINT8 * m_pStrAiName;
	static UINT8 * ms_pStrElemName;
};

#endif
