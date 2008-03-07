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
#ifndef __CAASYMCIPHER__
#define __CAASYMCIPHER__
#include "CACertificate.hpp"

#define RSA_SIZE 128

class CAASymCipher
	{
		public:
			CAASymCipher();
			~CAASymCipher();
			SINT32 destroy();
			SINT32 decrypt(const UINT8* from,UINT8* to);
			SINT32 decryptOAEP(const UINT8* from,UINT8* to,UINT32* len);
			SINT32 encrypt(const UINT8* from,UINT8* to);
			SINT32 encryptOAEP(const UINT8* from,UINT32 fromlen,UINT8* to,UINT32* len);
			SINT32 generateKeyPair(UINT32 size);
			//SINT32 getPublicKey(UINT8* buff,UINT32 *len);
#ifndef ONLY_LOCAL_PROXY
			SINT32 getPublicKeyAsXML(UINT8* buff,UINT32* len);
			SINT32 getPublicKeyAsDocumentFragment(DOMDocumentFragment* & pDFrag);
			//SINT32 getPublicKeySize();
			//SINT32 setPublicKey(UINT8* buff,UINT32* len);
			SINT32 setPublicKey(const CACertificate* pCert);
			SINT32 setPublicKeyAsXML(const UINT8* buff,UINT32 len);
#endif
			SINT32 setPublicKeyAsDOMNode(DOMNode* node);

			//Set the public key from a Base64 encodes exponent and modulus
			SINT32 setPublicKey(const UINT8* modulus,UINT32 moduluslen,const UINT8* exponent,UINT32 exponentlen);
		private:
			RSA* m_pRSA;
	};

#endif
