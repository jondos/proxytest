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
			SINT32 setSignKey(const DOM_Node& node,UINT32 type,const char* passwd=NULL);
			/** Gets the secret sign key as XML encode PKCS#12 struct*/
			SINT32 getSignKey(DOM_DocumentFragment& node,DOM_Document& doc);
			SINT32 sign(UINT8* in,UINT32 inlen,UINT8* sig,UINT32* siglen);
			SINT32 signXML(DOM_Node& node,CACertStore* pIncludeCerts=NULL);
			SINT32 signXML(UINT8* in,UINT32 inlen,UINT8* out,UINT32* outlen,CACertStore* pIncludeCerts=NULL);
			SINT32 setVerifyKey(CACertificate* pCert);
			/**
			 * Parses the XML representation of a DSA public key
			 */
			SINT32 setVerifyKey(const DOM_Element& xmlKey);
			/** Get the public key as XML encoded X509 certificate*/ 
			SINT32 getVerifyKey(CACertificate**);
			SINT32 getVerifyKeyHash(UINT8* buff,UINT32* len);

//			SINT32 verify(UINT8* in,UINT32 inlen,UINT8* sig,UINT32 siglen);
			SINT32 verifyXML(const UINT8* const in,UINT32 inlen);
			SINT32 verifyXML(DOM_Node& node,CACertStore* pTrustedCerts=NULL);
			SINT32 getSignatureSize();
			SINT32 encodeRS(UINT8* out,UINT32* outLen,DSA_SIG* pdsaSig);
			
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
			SINT32 decodeRS(const UINT8* in, const UINT32 inLen, DSA_SIG* pDsaSig);
			SINT32 verify(UINT8* in,UINT32 inlen,DSA_SIG* dsaSig);
			
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
			
			friend class CASSLContext;
		private:
			DSA* getDSA(){return m_pDSA;}
			DSA* m_pDSA;
			SINT32 parseSignKeyXML(const UINT8* buff,UINT32 len);
			SINT32 sign(UINT8* in,UINT32 inlen,DSA_SIG** dsaSig);

			
	};
#endif
#endif //ONLY_LOCAL_PROXY
