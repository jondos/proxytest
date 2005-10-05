#ifndef __CAXMLCOSTCONFIRMATION__
#define __CAXMLCOSTCONFIRMATION__
#include "xml/DOM_Output.hpp"
/**
this class corresponds to anon.pay.xml.XMLEasyCC in the Java implementation

@author Bastian Voigt
*/
class CAXMLCostConfirmation
{
public:
	CAXMLCostConfirmation(UINT8 * strXmlData);
	CAXMLCostConfirmation(DOM_Element &elemRoot);
	~CAXMLCostConfirmation();
	
	/** dumps the XML CC to memory without trailing '0'.*/
	UINT8* dumpToMem(UINT32* pLen)
		{
			if(m_domDocument==NULL)
				return NULL;
			return DOM_Output::dumpToMem(m_domDocument,pLen);
		}

	/** dumps the XML CC to a string (with trailing '0').*/
	SINT32 toXMLString(UINT8* buff,UINT32* bufflen)
		{
			if(DOM_Output::dumpToMem(m_domDocument,buff,bufflen)!=E_SUCCESS)
				return E_UNKNOWN;
			buff[*bufflen]=0;
			return E_SUCCESS;
		}

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
	//static void initXMLElementName();
	SINT32 setValues();
	
	UINT64				m_lTransferredBytes;
	UINT64				m_lAccountNumber;
	UINT8*				m_pStrAiName;
	DOM_Document	m_domDocument;
	static UINT8* ms_pStrElemName;
};

#endif
