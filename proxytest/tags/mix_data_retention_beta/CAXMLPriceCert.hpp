/*
Copyright (c) 2000, The JAP-Team
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.

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
#ifndef CAXMLPRICECERT_H_
#define CAXMLPRICECERT_H_
#ifdef PAYMENT
#include "xml/DOM_Output.hpp"
#include "CAAbstractXMLSignable.hpp"
#include "CAMsg.hpp"
/**
 * Representation of a Price Certificate
 * corresponds to anon.pay.xml.XMLPriceCertificate in the Java implementation
 * Constructor is private, use the getInstance methods
 * 
 * @author Elmar Schraml
 */
class CAXMLPriceCert : public CAAbstractXMLSignable
{
	public:
		/** Tries to create a CAXMLPriceCert object from the given XML string.
		* @retval NULL if the XML data was wrong
		* @retrun a newly allocated CAXMLCostconfirmationObject
		*/
		static CAXMLPriceCert* getInstance(const UINT8 * strXmlData,UINT32 strXMLDataLength);
		
		static CAXMLPriceCert* getInstance(DOMElement* elemRoot);
		~CAXMLPriceCert();
		
		SINT32 toXmlElement(XERCES_CPP_NAMESPACE::DOMDocument* a_doc, DOMElement* & elemRoot);
		
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

		/** @return a newly allocated buffer which must be deleted by the caller 
		* retval NULL if hash of mix cert was not set
		*/
		UINT8 * getSubjectKeyIdentifier()
		{
			UINT8* pTmpStr = NULL;
			if(m_StrSubjectKeyIdentifier!=NULL)
				{
					pTmpStr = new UINT8[strlen((char*)m_StrSubjectKeyIdentifier)+1];
					strcpy((char*)pTmpStr, (char*)m_StrSubjectKeyIdentifier);
				}
			return pTmpStr;	
			
		}	
		
		double getRate()
		{
			return m_lRate;
		}
		
		/** @return a newly allocated buffer which must be deleted by the caller 
		* retval NULL if creation time was not set
		*/
		UINT8* getSignatureTime()
		{
			UINT8* pTmpStr = NULL;
			if(m_StrSignatureTime!=NULL)
				{
					pTmpStr = new UINT8[strlen((char*)m_StrSignatureTime)+1];
					strcpy((char*)pTmpStr, (char*)m_StrSignatureTime);
				}
			return pTmpStr;				
		}
		
		/** @return a newly allocated buffer which must be deleted by the caller 
		* retval NULL if BiID was not set
		*/
		UINT8* getBiID()
		{
			UINT8* pTmpStr = NULL;
			if(m_StrBiID!=NULL)
				{
					pTmpStr = new UINT8[strlen((char*)m_StrBiID)+1];
					strcpy((char*)pTmpStr, (char*)m_StrBiID);
				}
			return pTmpStr;	
		}
		
		static const char* const getXMLElementName()
		{
			return ms_pStrElemName;
		}
	
		 XERCES_CPP_NAMESPACE::DOMDocument* getXMLDocument()
		{
			return m_domDocument;
		}
		
		
		
		
	private:
		CAXMLPriceCert();
		SINT32 setValues();
		
		UINT8* 		m_StrSubjectKeyIdentifier;
		double 		m_lRate;
		UINT8* 		m_StrSignatureTime;
		UINT8*		m_StrBiID;
		DOMElement*  m_signature;
		 XERCES_CPP_NAMESPACE::DOMDocument*	m_domDocument;
		static const char* const ms_pStrElemName;
				
};

#endif //PAYMENT
#endif /*CAXMLPRICECERT_H_*/
