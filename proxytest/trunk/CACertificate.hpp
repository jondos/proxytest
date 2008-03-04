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
#ifndef __CACERTIFICATE__
#define __CACERTIFICATE__
#ifndef ONLY_LOCAL_PROXY
#define CERT_DER 1
#define CERT_XML_X509CERTIFICATE 2
#define CERT_PKCS12 3
#define CERT_X509CERTIFICATE 4
class CASignature;
class CAASymCipher;
class CASSLContext;
class CATLSClientSocket;
class CACertificate
	{
		friend class CASignature;
		friend class CAASymCipher;
		friend class CASSLContext;
		friend class CATLSClientSocket;
	
		public:
			~CACertificate(){X509_free(m_pCert);}
			CACertificate* clone() const
				{
					CACertificate* tmp=new CACertificate;
					tmp->m_pCert=X509_dup(m_pCert);
					return tmp;
				}
			
			/** Extracts a certificate from an encoded (DER,XML) form.*/
			static CACertificate* decode(const UINT8* buff,UINT32 bufflen,UINT32 type,const char* passwd=NULL);
			static CACertificate* decode(const DOMNode* node,UINT32 type,const char* passwd=NULL);
			SINT32 encode(UINT8* buff,UINT32* bufflen,UINT32 type);
			SINT32 encode(DOMDocumentFragment* & docFrag,XERCES_CPP_NAMESPACE::DOMDocument* doc);
					
			static UINT8 * getXmlElementName() 
				{
					if(!CACertificate::m_spXmlElementName)
					{
						UINT8 name[] = "X509Certificate";
						CACertificate::m_spXmlElementName = new UINT8[strlen((char*)name)+1];
						strcpy((char*)CACertificate::m_spXmlElementName, (char*)name);
					}
					return CACertificate::m_spXmlElementName;
				}
			
                /* LERNGRUPPE */
                /* SubjectKeyIdentifier Extension handling */
		SINT32 getSubjectKeyIdentifier(UINT8* r_ski, UINT32 *r_skiLen);
                SINT32 setSubjectKeyIdentifier( UINT8* a_value, UINT32 a_valueLen );
                SINT32 setSubjectKeyIdentifier();	
		private:
			CACertificate();
		private:
			X509* getX509(){return m_pCert;}
			X509* m_pCert;
		static UINT8 * m_spXmlElementName;
                /* LERNGRUPPE */
                /* SubjectKeyIdentifier Extension handling */
                SINT32 removeColons(UINT8* a_cSki, UINT32 a_cSkiLen, UINT8 *&r_ski, UINT32 *r_skiLen);
	};
#endif
#endif //ONLY_LOCAL_PROXY
