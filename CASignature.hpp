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
//			SINT32 sign(UINT8* in,UINT32 inlen,UINT8* sig,UINT32* siglen);
			SINT32 signXML(DOM_Node& node,CACertStore* pIncludeCerts=NULL);
			SINT32 signXML(UINT8* in,UINT32 inlen,UINT8* out,UINT32* outlen,CACertStore* pIncludeCerts=NULL);
			SINT32 setVerifyKey(CACertificate* pCert);
//			SINT32 verify(UINT8* in,UINT32 inlen,UINT8* sig,UINT32 siglen);
			SINT32 verifyXML(const UINT8* const in,UINT32 inlen);
			SINT32 verifyXML(DOM_Node& node,CACertStore* pTrustedCerts=NULL);
			SINT32 getSignatureSize();
			friend CASSLContext;
		private:
			DSA* getDSA(){return m_pDSA;}
			DSA* m_pDSA;
			SINT32 parseSignKeyXML(const UINT8* buff,UINT32 len);
			SINT32 verify(UINT8* in,UINT32 inlen,DSA_SIG* dsaSig);
			SINT32 sign(UINT8* in,UINT32 inlen,DSA_SIG** dsaSig);
	};
#endif
