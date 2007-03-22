#ifndef __CAXMLCOSTCONFIRMATION__
#define __CAXMLCOSTCONFIRMATION__
#include "xml/DOM_Output.hpp"
#include "CAPriceInfo.hpp"

/**
this class corresponds to anon.pay.xml.XMLEasyCC in the Java implementation

@author Bastian Voigt
*/
class CAXMLCostConfirmation
	{
		
		private:
			SINT32 setValues();
			CAXMLCostConfirmation();
			
			UINT64				m_lTransferredBytes;
			UINT64				m_lAccountNumber;
			UINT32				m_id; //id of the CC, only set after storing it in the BI's database
//			CAPriceInfo** 		m_priceCerts;		
			UINT8*				m_pStrPIID;
			DOM_Document	m_domDocument;
			static const UINT8* const ms_pStrElemName;
			
		public:
			/** Tries to create an CAXMLCostConfirmation object from the given XML string.
				* @retval NULL if the XML data was wrong
				* @return a newly allocated CAXMLCostconfirmationObject
				*/
			static CAXMLCostConfirmation* getInstance(UINT8 * strXmlData,UINT32 strXMlDataLen);
			/** Tries to create an CAXMLCostConfirmation object from the given XML string.
				* @retval NULL if the XML data was wrong
				* @return a newly allocated CAXMLCostconfirmationObject
				*/
			static CAXMLCostConfirmation* getInstance(DOM_Element &elemRoot);
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

			UINT64 getTransferredBytes()
				{
					return m_lTransferredBytes;
				}
			
			UINT64 getAccountNumber()
				{
					return m_lAccountNumber;
				}
			

			UINT32 getID() 
						{
					return m_id;
				}
			
//dangerous and currently not needed?			
//			CAPriceInfo** getPriceCerts()
//				{
//					return m_priceCerts;	
//				}
			
			/** @return a newly allocated buffer which must be deleted by the caller 
			* 
			*/
			UINT8* getPIID() 
				{
					UINT8* pTmpStr = NULL;
					if(m_pStrPIID!=NULL)
						{
							pTmpStr = new UINT8[strlen((char*)m_pStrPIID)+1];
							strcpy((char*)pTmpStr, (char*)m_pStrPIID);
						}
					return pTmpStr;
				}
				
			static const UINT8* const getXMLElementName()
				{
					return ms_pStrElemName;
				}
			
			DOM_Document getXMLDocument()
			{
				return m_domDocument;
			}
			

			
	};

#endif
