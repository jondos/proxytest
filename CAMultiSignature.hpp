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
	private:
		SIGNATURE* m_signatures;
		UINT32 m_sigCount;
		UINT8* m_xoredID;
};

#endif /* CAMULTISIGNATURE_HPP_ */
