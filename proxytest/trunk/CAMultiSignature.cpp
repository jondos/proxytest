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
 * CAMultiSignature.cpp
 *
 *  Created on: 17.07.2008
 *      Author: zenoxx
 */
#include "StdAfx.h"
#include "CABase64.hpp"
#include "CAUtil.hpp"
#include "xml/DOM_Output.hpp"
#include "CASignature.hpp"
#include "CAMultiSignature.hpp"

CAMultiSignature::CAMultiSignature()
{
	m_signatures = NULL;
	m_sigCount = 0;
	m_xoredID = new UINT8[SHA_DIGEST_LENGTH];
	for(SINT32 i = 0; i<SHA_DIGEST_LENGTH; i++)
	{
		m_xoredID[i] = 0;
	}
}

CAMultiSignature::~CAMultiSignature()
{
	SIGNATURE* tmp;
	while(m_signatures != NULL)
	{
		//delete Signer and CertStore
		delete m_signatures->pSig;
		delete m_signatures->pCerts;
		m_signatures->pCerts = NULL;
		m_signatures->pSig = NULL;
		//store current pointer
		tmp = m_signatures;
		//go to next signature
		m_signatures = m_signatures->next;
		//delete current signature
		delete tmp;
		tmp = NULL;
	}
}

SINT32 CAMultiSignature::addSignature(CASignature* a_signature, CACertStore* a_certs, UINT8* a_ski, UINT32 a_skiLen)
{
	if(a_signature == NULL || a_certs == NULL || a_ski == NULL || a_skiLen != SHA_DIGEST_LENGTH)
		return E_UNKNOWN;
	for(SINT32 i=0; i<SHA_DIGEST_LENGTH; i++)
	{
		m_xoredID[i] = m_xoredID[i] ^ a_ski[i];
	}
	SIGNATURE* newSignature = new SIGNATURE;
	newSignature->pSig = a_signature;
	newSignature->pCerts = a_certs;
	newSignature->next = m_signatures;
	m_signatures = newSignature;
	m_sigCount++;
	return E_SUCCESS;
}

SINT32 CAMultiSignature::signXML(UINT8* in,UINT32 inlen,UINT8* out,UINT32* outlen, bool appendCerts)
{
	if(in == NULL || inlen < 1 || out == NULL || outlen == NULL)
		return E_UNKNOWN;

	XERCES_CPP_NAMESPACE::DOMDocument* doc = parseDOMDocument(in, inlen);
	if(doc == NULL)
		return E_UNKNOWN;
	DOMElement* root = doc->getDocumentElement();
	if(signXML(root, appendCerts) != E_SUCCESS)
		return E_UNKNOWN;
	return DOM_Output::dumpToMem(root,out,outlen);
}

SINT32 CAMultiSignature::signXML(DOMNode* node, bool appendCerts)
{
	if(m_sigCount == 0)
	{
		CAMsg::printMsg(LOG_ERR, "Trying to sign a document with no signature-keys set!");
		return E_UNKNOWN;
	}

	//getting the Document an the Node to sign
	XERCES_CPP_NAMESPACE::DOMDocument* doc = NULL;
	DOMNode* elemRoot = NULL;
	if(node->getNodeType() == DOMNode::DOCUMENT_NODE)
	{
		doc = (XERCES_CPP_NAMESPACE::DOMDocument*)node;
		elemRoot = doc->getDocumentElement();
	}
	else
	{
		elemRoot = node;
		doc = node->getOwnerDocument();
	}

	//check if there are already Signatures and if so remove them first...
	DOMNode* tmpSignature = NULL;
	while(getDOMChildByName(elemRoot, "Signature", tmpSignature, false) == E_SUCCESS)
	{
		DOMNode* n = elemRoot->removeChild(tmpSignature);
		if (n != NULL)
		{
			n->release();
			n = NULL;
		}
	}
	//get SHA1-Digest
	UINT32 len = 0;
	UINT8* canonicalBuff = DOM_Output::makeCanonical(elemRoot, &len);
	if(canonicalBuff == NULL)
	{
		return E_UNKNOWN;
	}
	UINT8 dgst[SHA_DIGEST_LENGTH];
	SHA1(canonicalBuff, len, dgst);
	delete[] canonicalBuff;
	canonicalBuff = NULL;

	UINT8 digestValue[512];
	len = 512;
	if(CABase64::encode(dgst, SHA_DIGEST_LENGTH, digestValue, &len) != E_SUCCESS)
	{
		return E_UNKNOWN;
	}

	//append a signature for each SIGNATURE element we have
	SIGNATURE* currentSignature = m_signatures;
	UINT32 sigCount = 0;
	for(UINT32 i=0; i<m_sigCount; i++)
	{
		//Creating the Sig-InfoBlock....
		DOMElement* elemSignedInfo = createDOMElement(doc, "SignedInfo");
		DOMElement* elemCanonicalizationMethod = createDOMElement(doc, "CanonicalizationMethod");
		DOMElement* elemSignatureMethod = createDOMElement(doc, "SignatureMethod");
		DOMElement* elemReference = createDOMElement(doc, "Reference");
		elemReference->setAttribute(XMLString::transcode("URI"), XMLString::transcode(""));
		DOMElement* elemDigestMethod = createDOMElement(doc, "DigestMethod");
		if(currentSignature->pSig->isDSA())	//DSA-Signature
		{
			setDOMElementAttribute(elemSignatureMethod, "Algorithm", (UINT8*)DSA_SHA1_REFERENCE);
		}
		else if(currentSignature->pSig->isRSA())
		{
			setDOMElementAttribute(elemSignatureMethod, "Algorithm", (UINT8*)RSA_SHA1_REFERENCE);
		}
#ifdef ECC
		else if(currentSignature->pSig->isECDSA())
		{
			setDOMElementAttribute(elemSignatureMethod, "Algorithm", (UINT8*)ECDSA_SHA1_REFERENCE);
		}
#endif //ECC
		setDOMElementAttribute(elemDigestMethod, "Algorithm", (UINT8*)SHA1_REFERENCE);
		DOMElement* elemDigestValue = createDOMElement(doc, "DigestValue");
		setDOMElementValue(elemDigestValue, digestValue);

		elemSignedInfo->appendChild(elemCanonicalizationMethod);
		elemSignedInfo->appendChild(elemSignatureMethod);
		elemSignedInfo->appendChild(elemReference);
		elemReference->appendChild(elemDigestMethod);
		elemReference->appendChild(elemDigestValue);

		// Signing the SignInfo block....
		canonicalBuff = DOM_Output::makeCanonical(elemSignedInfo,&len);
		if(canonicalBuff==NULL)
		{
			return E_UNKNOWN;
		}

		UINT32 sigLen = currentSignature->pSig->getSignatureSize();
		UINT8* sigBuff=new UINT8[sigLen];
		SINT32 ret = currentSignature->pSig->sign(canonicalBuff, len, sigBuff, &sigLen);
		delete[] canonicalBuff;
		canonicalBuff = NULL;
		if(ret != E_SUCCESS)
		{
			currentSignature = currentSignature->next;
			delete[] sigBuff;
			continue;
		}
		UINT sigSize = 255;
		UINT8 sig[255];
		if(CABase64::encode(sigBuff, sigLen, sig, &sigSize) != E_SUCCESS)
		{
			currentSignature = currentSignature->next;
			delete[] sigBuff;
			continue;
		}

		//Makeing the whole Signature-Block....
		DOMElement* elemSignature = createDOMElement(doc,"Signature");
		DOMElement* elemSignatureValue = createDOMElement(doc,"SignatureValue");
		setDOMElementValue(elemSignatureValue,sig);
		elemSignature->appendChild(elemSignedInfo);
		elemSignature->appendChild(elemSignatureValue);

		//Append KeyInfo if neccassary
		if(appendCerts)
		{
			//Making KeyInfo-Block
			DOMElement* tmpElemCerts = NULL;
			if(currentSignature->pCerts->encode(tmpElemCerts, doc) == E_SUCCESS && tmpElemCerts != NULL)
			{
				DOMElement* elemKeyInfo = createDOMElement(doc, "KeyInfo");
				elemKeyInfo->appendChild(tmpElemCerts);
				elemSignature->appendChild(elemKeyInfo);
			}
		}
		elemRoot->appendChild(elemSignature);
		sigCount++;

		//goto next Signature
		currentSignature = currentSignature->next;
		delete[] sigBuff;
	}
	if(sigCount > 0)
	{
		//CAMsg::printMsg(LOG_DEBUG, "Appended %d Signature(s) to XML-Structure\n", sigCount);
		return E_SUCCESS;
	}
	return E_UNKNOWN;
}

SINT32 CAMultiSignature::verifyXML(const UINT8* const in,UINT32 inlen, CACertificate* a_cert)
{
	XERCES_CPP_NAMESPACE::DOMDocument* doc = parseDOMDocument(in,inlen);
	if(doc == NULL)
	{
		return E_UNKNOWN;
	}
	DOMElement* root = doc->getDocumentElement();
	if(root == NULL)
	{
		return E_UNKNOWN;
	}
	return CAMultiSignature::verifyXML(root, a_cert);
}

SINT32 CAMultiSignature::verifyXML(DOMNode* root, CACertificate* a_cert)
{
	CASignature* sigVerifier = new CASignature();
	if(sigVerifier->setVerifyKey(a_cert) != E_SUCCESS)
	{
		CAMsg::printMsg(LOG_ERR, "Failed to set verify Key!");
		return E_UNKNOWN;
	}
	UINT8* signatureMethod = sigVerifier->getSignatureMethod();

	UINT32 signatureElementsCount = MAX_SIGNATURE_ELEMENTS;
	DOMNode* signatureElements[MAX_SIGNATURE_ELEMENTS];

	getSignatureElements((DOMElement*)root, signatureElements, &signatureElementsCount);
	CAMsg::printMsg(LOG_DEBUG, "Found %d Signature(s) in XML-Structure\n", signatureElementsCount);

	UINT8 dgst[255];
	UINT32 dgstlen=255;
	UINT8* out = NULL;
	UINT32 outlen;
	bool verified = false;
	//go through all appended Signatures an try to verify them with the given cert
	for(UINT32 i=0; i<signatureElementsCount; i++)
	{
		dgstlen=255;
		CAMsg::printMsg(LOG_DEBUG, "Trying to verify signature %d of %d!\n", i+1, signatureElementsCount);
		DOMNode* elemSignature = signatureElements[i];

		if(elemSignature == NULL)
		{
			CAMsg::printMsg(LOG_DEBUG, "Error: signature element is NULL\n");
			continue;
		}
		DOMNode* elemSigInfo;
		getDOMChildByName(elemSignature, "SignedInfo", elemSigInfo);
		if(elemSigInfo == NULL)
		{
			CAMsg::printMsg(LOG_DEBUG, "Error: signed info is NULL\n");
			continue;
		}
		//check if SignatureMethod fits...
		DOMNode* elemSigMethod;
		getDOMChildByName(elemSigInfo, "SignatureMethod", elemSigMethod);
		UINT32 algLen = 255;
		UINT8 algorithm[255];
		getDOMElementAttribute(elemSigMethod, (const char*)"Algorithm", algorithm, &algLen);
		//if signatureMethod is set check if its equal
		if(signatureMethod != NULL &&
				strncmp((const char*)algorithm, (const char*)signatureMethod, algLen) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG, "Did NOT find matching SignatureMethods: %s and %s!\n", signatureMethod, algorithm);
			continue;
		}
		DOMNode* elemSigValue;
		getDOMChildByName(elemSignature, "SignatureValue", elemSigValue);
		if(elemSigValue == NULL)
		{
			CAMsg::printMsg(LOG_DEBUG, "Error: signature value is NULL\n");
			continue;
		}
		DOMNode* elemReference;
		getDOMChildByName(elemSigInfo, "Reference", elemReference);
		if(elemReference == NULL)
		{
			CAMsg::printMsg(LOG_DEBUG, "Error: signature reference is NULL\n");
			continue;
		}
		DOMNode* elemDigestValue;
		getDOMChildByName(elemReference, "DigestValue", elemDigestValue);
		if(elemDigestValue == NULL)
		{
			CAMsg::printMsg(LOG_DEBUG, "Error: digest value is NULL\n");
			continue;
		}
		if(getDOMElementValue(elemDigestValue,dgst,&dgstlen)!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG, "Error: could not get digest value from XML\n");
			continue;
		}
		if(CABase64::decode(dgst,dgstlen,dgst,&dgstlen)!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG, "Error: could not decode digest value\n");
			continue;
		}
		if(dgstlen!=SHA_DIGEST_LENGTH)
		{
			CAMsg::printMsg(LOG_DEBUG, "Error: digest is %d long, should be %d\n", dgstlen, SHA_DIGEST_LENGTH);
			continue;
		}
		UINT32 tmpSiglen = 255;
		UINT8 tmpSig[255];
		if(getDOMElementValue(elemSigValue,tmpSig,&tmpSiglen)!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG, "Error: could not get signature value from XML\n");
			continue;
		}
		if(CABase64::decode(tmpSig,tmpSiglen,tmpSig,&tmpSiglen)!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG, "Error: could not decode signature value\n");
			continue;
		}
		outlen = 5000;
		out = new UINT8[outlen];
		if(DOM_Output::makeCanonical(elemSigInfo, out, &outlen) == E_SUCCESS)
		{
			if(sigVerifier->verify(out, outlen, tmpSig, tmpSiglen) == E_SUCCESS)
			{
				CAMsg::printMsg(LOG_DEBUG, "Signature verification successful!\n");
				verified = true;
				break;
			}
		}
		CAMsg::printMsg(LOG_WARNING, "Signature verification not successful!\n");
		delete[] out;
		out = NULL;
		continue;
	}
	if(verified)
	{
		//the signature could be verified, now check digestValue
		//first remove Signature-nodes from root and store them
		DOMNode* removedSignatures[MAX_SIGNATURE_ELEMENTS];

		for(UINT32 i=0; i<signatureElementsCount; i++)
		{
			removedSignatures[i] = root->removeChild(signatureElements[i]);
			if(removedSignatures[i] == NULL)
			{
				//TODO do what? Verification will most likely fail, so just log the error for the moment
				CAMsg::printMsg(LOG_ERR, "Error removing signature-element %d of %d from Root-Node\n", i+1, signatureElementsCount);
			}
		}

		outlen = 5000;
		DOM_Output::makeCanonical(root, out, &outlen);

		//append Signature-nodes again
		for(UINT32 i=0; i<signatureElementsCount; i++)
		{
			if(removedSignatures[i] != NULL)
			{
				root->appendChild(removedSignatures[i]);
			}
		}

		UINT8 newDgst[SHA_DIGEST_LENGTH];
		SHA1(out, outlen, newDgst);
		delete[] out;
		out = NULL;
		for(int i=0; i<SHA_DIGEST_LENGTH; i++)
		{
			if(newDgst[i] != dgst[i])
			{
				CAMsg::printMsg(LOG_ERR, "Error checking XML-Signature DigestValue!\n");
				return E_UNKNOWN;
			}
		}
		return E_SUCCESS;
	}
	CAMsg::printMsg(LOG_ERR, "XML-Signature could not be verified!\n");
	return E_UNKNOWN;
}
/** Method for producing a single Signature for Key Exchange */
SINT32 CAMultiSignature::sign(UINT8* in,UINT32 inlen,UINT8* sig,UINT32* siglen)
{
	if(m_sigCount < 1)
	{
		return E_UNKNOWN;
	}
	return m_signatures->pSig->sign(in, inlen, sig, siglen);
}

SINT32 CAMultiSignature::getXORofSKIs(UINT8* in, UINT32 inlen)
{
	UINT8* tmp = (UINT8*) hex_to_string(m_xoredID, SHA_DIGEST_LENGTH);
	CACertificate::removeColons(tmp, strlen((const char*)tmp), in, &inlen);
	return E_SUCCESS;
}
