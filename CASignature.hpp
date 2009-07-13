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
#ifndef __CASIGNATURE__
#define __CASIGNATURE__
#ifndef ONLY_LOCAL_PROXY
#define SIGKEY_XML 1
#define SIGKEY_PKCS12 2
#define SHA1_REFERENCE "http://www.w3.org/2000/09/xmldsig#sha1"
#define DSA_SHA1_REFERENCE "http://www.w3.org/2000/09/xmldsig#dsa-sha1"
#define RSA_SHA1_REFERENCE "http://www.w3.org/2000/09/xmldsig#rsa-sha1"
#define ECDSA_SHA1_REFERENCE "http://www.w3.org/2001/04/xmldsig-more#ecdsa-sha1"
#include "CACertStore.hpp"
class CASSLContext;

class CASignature
	{
		public:
			CASignature();
			~CASignature();
			CASignature* clone();
			SINT32 generateSignKey(UINT32 size);
			SINT32 setSignKey(const UINT8* buff,UINT32 len,UINT32 type,const char* passwd=NULL);
			SINT32 setSignKey(const DOMNode* node,UINT32 type,const char* passwd=NULL);
			/** Gets the secret sign key as XML encode PKCS#12 struct*/
			SINT32 getSignKey(DOMElement* & node,XERCES_CPP_NAMESPACE::DOMDocument* doc);
			SINT32 sign(const UINT8* const in,UINT32 inlen,UINT8* sig,UINT32* siglen) const;
			//SINT32 signXML(DOMNode* node,CACertStore* pIncludeCerts=NULL);
			//SINT32 signXML(UINT8* in,UINT32 inlen,UINT8* out,UINT32* outlen,CACertStore* pIncludeCerts=NULL);
			SINT32 setVerifyKey(CACertificate* pCert);
			/**
			 * Parses the XML representation of a DSA public key
			 */
			SINT32 setVerifyKey(const DOMElement* xmlKey);
			/** Get the public key as XML encoded X509 certificate*/
			SINT32 getVerifyKey(CACertificate**);
			SINT32 getVerifyKeyHash(UINT8* buff,UINT32* len);

			//SINT32 verify(UINT8* in,UINT32 inlen,UINT8* sig,UINT32 siglen);
			//SINT32 verifyXML(const UINT8* const in,UINT32 inlen);
			SINT32 verifyXML(DOMNode* node,CACertStore* pTrustedCerts=NULL);
			SINT32 getSignatureSize() const;
			SINT32 encodeRS(UINT8* out,UINT32* outLen,const DSA_SIG* const pdsaSig) const;

			/**
			* Converts a DSA signature from the XML Signature format to the
			* openSSL R/S BigNumber format.
			*
			* @param in the xml signature value
			*	@param inLen size of the xml signature value
			* @param pDsaSig a pointer to a DSA signature struct whose values will be set according to the xml signature value
			* @retval E_SUCCESS if succesful
			* @retval E_UNKNOWN otherwise
			*/
			SINT32 decodeRS(const UINT8* const in, const UINT32 inLen, DSA_SIG* pDsaSig) const;
			SINT32 verify(const UINT8* const in,UINT32 inlen,DSA_SIG* const dsaSig) const;

			/**
			* Verifies an ASN.1 DER encoded SHA1-DSA signature.
			*
			* @author Bastian Voigt
			* @param in the document that was signed
			* @param inlen the document length
			* @param dsaSig the DER encoded signature
			* @param sigLen the signature length (normally 46 bytes)
			* @retval E_SUCCESS if the signature is valid
			* @retval E_UNKNOWN otherwise
			*/
			SINT32 verifyDER(UINT8* in, UINT32 inlen, const UINT8 * dsaSig, const UINT32 sigLen);

			//MultiCert
			SINT32 verify(UINT8* in, UINT32 inLen, UINT8* sig, const UINT32 sigLen);
			bool isDSA() const;
			bool isRSA() const;
			bool isECDSA() const;
			bool isSet() const;
			UINT8* getSignatureMethod();

			friend class CASSLContext;
		private:
			DSA* m_pDSA;
			DSA* getDSA(){return m_pDSA;}
			RSA* m_pRSA;
			RSA* getRSA(){ return m_pRSA; }
			EC_KEY* m_pEC;
			EC_KEY* getECKey(){ return m_pEC; }

			SINT32 parseSignKeyXML(const UINT8* buff,UINT32 len);
			SINT32 sign(const UINT8* const in,UINT32 inlen,DSA_SIG** dsaSig) const;

			//MultiCert
			//friend class CAMultiSignature;
			SINT32 signRSA(const UINT8* dgst, const UINT32 dgstLen, UINT8* sig, UINT32* sigLen) const;
			SINT32 signECDSA(const UINT8* dgst, const UINT32 dgstLen, UINT8* sig, UINT32* sigLen) const;
			SINT32 verifyRSA(const UINT8* dgst, const UINT32 dgstLen, UINT8* sig, UINT32 sigLen) const;
			SINT32 verifyDSA(const UINT8* dgst, const UINT32 dgstLen, UINT8* sig, UINT32 sigLen) const;
			SINT32 verifyECDSA(const UINT8* dgst, const UINT32 dgstLen, UINT8* sig, UINT32 sigLen) const;


	};
#endif
#endif //ONLY_LOCAL_PROXY
