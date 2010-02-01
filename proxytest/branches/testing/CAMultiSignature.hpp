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
/*
 * CAMultiSignature.hpp
 *
 *  Created on: 17.07.2008
 *      Author: zenoxx
 */

#ifndef CAMULTISIGNATURE_HPP_
#define CAMULTISIGNATURE_HPP_

#include "CACertStore.hpp"
#include "CACertificate.hpp"

struct __t_signature
{
	CASignature* pSig;
	CACertStore* pCerts;
	UINT8* pSKI;
	struct __t_signature* next;
};
typedef struct __t_signature SIGNATURE;

class CAMultiSignature
{
	public:
		CAMultiSignature();
		virtual ~CAMultiSignature();
		SINT32 addSignature(CASignature* a_signature, CACertStore* a_certs, UINT8* a_ski, UINT32 a_skiLen);
		SINT32 signXML(DOMNode* a_node, bool appendCerts);
		SINT32 signXML(UINT8* in, UINT32 inlen, UINT8* out, UINT32* outlen, bool appendCerts);
		static SINT32 verifyXML(const UINT8* const in, UINT32 inlen, CACertificate* a_cert);
		static SINT32 verifyXML(DOMNode* a_node, CACertificate* a_cert);
		UINT32 getSignatureCount(){ return m_sigCount; }
		SINT32 sign(UINT8* in,UINT32 inlen,UINT8* sig,UINT32* siglen);
		SINT32 getXORofSKIs(UINT8* in, UINT32 inlen);
		SINT32 findSKI(const UINT8* a_strSKI);
	private:
		SIGNATURE* m_signatures;
		UINT32 m_sigCount;
		UINT8* m_xoredID;
		static SINT32 getSignatureElements(DOMNode* parent, DOMNode** signatureNodes, UINT32* length);
		SINT32 getSKI(UINT8* in, UINT32 inlen, UINT8* a_ski);
};

#endif /* CAMULTISIGNATURE_HPP_ */
