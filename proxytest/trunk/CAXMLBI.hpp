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

#include "CAAbstractXMLEncodable.hpp"
#include "CACertificate.hpp"
#include "CASignature.hpp"
#include "StdAfx.h"
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
		CAXMLBI(const UINT8 * biName, const UINT8 * hostName, const int portNumber, CACertificate * pCert);
		CAXMLBI(DOM_Element & elemRoot);
		~CAXMLBI();
			
		/** returns the BI's unique name (identifier) */
		UINT8 * getName()
			{
				return m_pBiName;
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
			
		SINT32 toXmlElement(DOM_Document &a_doc, DOM_Element &elemRoot);
		
		static UINT8 * getXMLElementName()
			{
				if(!CAXMLBI::ms_pXmlElemName)
				{
					UINT8 name[] = "BI";
					CAXMLBI::ms_pXmlElemName = new UINT8[strlen((char*)name)+1];
					strcpy((char*)CAXMLBI::ms_pXmlElemName, (char*)name);
				}
				return CAXMLBI::ms_pXmlElemName;
			}
			
	
	private:
		SINT32 setValues(DOM_Element &elemRoot);
		UINT8 * m_pBiName;
		UINT8 * m_pHostName;
		CACertificate * m_pCert;
		CASignature * m_pVeryfire;
		UINT32 m_iPortNumber;
		
		static UINT8 * ms_pXmlElemName;
};

#endif
