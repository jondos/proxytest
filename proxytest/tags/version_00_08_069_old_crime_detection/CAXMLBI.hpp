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

#ifndef __CAXMLBI__
#define __CAXMLBI__

#if !defined (ONLY_LOCAL_PROXY) && defined(PAYMENT)
#include "CAAbstractXMLEncodable.hpp"
#include "CACertificate.hpp"
#include "CASignature.hpp"
#include "CAUtil.hpp"

/**
 * This class contains the parameters needed for 
 * talking to a BI (payment instance - bezahlinstanz).
 * It corresponds to anon.pay.BI in the Java implementation
 * @author Bastian Voigt (bavoigt@inf.fu-berlin.de)
 */
class CAXMLBI : public CAAbstractXMLEncodable
{
	public:
		static CAXMLBI* getInstance(const UINT8 * biID, const UINT8 * hostName, const int portNumber, CACertificate * pCert);
		static CAXMLBI* getInstance(DOMElement* elemRoot);
		~CAXMLBI();
			
		/** returns the BI's unique name (identifier) */
		UINT8 * getID()
			{
				return m_pBiID;
			}
		
		/** returns a CASignature object for veriying this BI's signatures 
		 * Don't delete it because it is 0wned by this!
		 */
		CASignature * getVerifier()
			{
				if (!m_pVeryfire)
				{
					m_pVeryfire = new CASignature;
					m_pVeryfire->setVerifyKey(m_pCert);
				}
				return m_pVeryfire;
			}
		
		/** returns the hostname of the host on which this BI is running */
		UINT8* getHostName()
			{
				return m_pHostName;
			}
		
		/** gets the port number */
		UINT32 getPortNumber()
			{
				return m_iPortNumber;
			}
			
		SINT32 toXmlElement( XERCES_CPP_NAMESPACE::DOMDocument* a_doc, DOMElement* & elemRoot);
		
		CACertificate* getCertificate()
		{
			return m_pCert;
		}
		
		static const char* const getXMLElementName()
			{
				return CAXMLBI::ms_pXmlElemName;
			}
			
	
	private:
		SINT32 setValues(DOMElement* elemRoot);
		UINT8 * m_pBiID;
		UINT8 * m_pHostName;
		CACertificate * m_pCert;
		CASignature * m_pVeryfire;
		UINT32 m_iPortNumber;
		CAXMLBI();
		static const char* const ms_pXmlElemName;
};

#endif //ONLY_LOCAL_PROXY
#endif
