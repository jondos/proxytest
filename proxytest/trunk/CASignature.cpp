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
#include "StdAfx.h"
#ifndef ONLY_LOCAL_PROXY
#include "CASignature.hpp"
#include "CABase64.hpp"
#include "CAUtil.hpp"
#include "xml/DOM_Output.hpp"

CASignature::CASignature()
	{
		m_pDSA = NULL;
		m_pRSA = NULL;
		m_pEC = NULL;
	}

CASignature::~CASignature()
	{
		if(m_pDSA != NULL)
		{
			DSA_free(m_pDSA);
			m_pDSA = NULL;
		}
		if(m_pRSA != NULL)
		{
			RSA_free(m_pRSA);
			m_pRSA = NULL;
		}
		if(m_pEC != NULL)
		{
			EC_KEY_free(m_pEC);
			m_pEC = NULL;
		}
	}


CASignature* CASignature::clone()
	{
		CASignature* tmpSig=new CASignature();
		if(m_pDSA!=NULL)
		{
			DSA* tmpDSA=DSA_clone(m_pDSA);
			tmpSig->m_pDSA=tmpDSA;
		}
		else if(m_pRSA != NULL)
		{
			RSA* tmpRSA = RSA_clone(m_pRSA);
			tmpSig->m_pRSA = tmpRSA;
		}
		else if(m_pEC != NULL)
		{
			EC_KEY* tmpEC = EC_KEY_dup(m_pEC);
			tmpSig->m_pEC = tmpEC;
		}
		return tmpSig;
	}

SINT32 CASignature::generateSignKey(UINT32 size)
	{
		if(m_pDSA!=NULL)
			DSA_free(m_pDSA);
		m_pDSA=NULL;
		m_pDSA=DSA_generate_parameters(size,NULL,0,NULL,NULL,NULL,NULL);
		if(m_pDSA==NULL)
			return E_UNKNOWN;
		if(DSA_generate_key(m_pDSA)!=1)
			{
				DSA_free(m_pDSA);
				m_pDSA=NULL;
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

SINT32 CASignature::getSignKey(DOMElement* & elem,XERCES_CPP_NAMESPACE::DOMDocument* doc)
	{
		CACertificate* pCert=NULL;
		getVerifyKey(&pCert);
		EVP_PKEY* pPKey=EVP_PKEY_new();
		EVP_PKEY_set1_DSA(pPKey,m_pDSA);
		PKCS12* pPKCS12=PKCS12_create(NULL,NULL, pPKey,pCert->m_pCert,NULL,0,0,0,0,0);
		delete pCert;
		pCert = NULL;
		EVP_PKEY_free(pPKey);
		UINT8* buff=NULL;
		SINT32 len=i2d_PKCS12(pPKCS12,&buff);
		UINT32 outlen=2*len;
		UINT8* outbuff=new UINT8[outlen];
		CABase64::encode(buff,len,outbuff,&outlen);
		outbuff[outlen]=0;
		OPENSSL_free(buff);
		elem=createDOMElement(doc,"X509PKCS12");
		setDOMElementValue(elem,outbuff);
		return E_SUCCESS;
	}

SINT32 CASignature::setSignKey(const DOMNode* n,UINT32 type,const char* passwd)
	{
		const DOMNode* node=n;
		switch(type)
		{
			case SIGKEY_PKCS12:
				while(node!=NULL)
				{
					if(equals(node->getNodeName(),"X509PKCS12"))
					{
						UINT32 strLen=4096;
						UINT8* tmpStr=new UINT8[strLen];
						if(getDOMElementValue(node,tmpStr,&strLen)!=E_SUCCESS)
						{
							delete[] tmpStr;
							tmpStr = NULL;
							return E_UNKNOWN;
						}
						UINT32 decLen=4096;
						UINT8* decBuff=new UINT8[decLen];
						CABase64::decode((UINT8*)tmpStr,strLen,decBuff,&decLen);
						delete [] tmpStr;
						tmpStr = NULL;
						SINT32 ret=setSignKey(decBuff,decLen,SIGKEY_PKCS12,passwd);
						delete[] decBuff;
						decBuff = NULL;
						return ret;
					}
					node=node->getNextSibling();
				}
		}
		return E_UNKNOWN;
	}

SINT32 CASignature::setSignKey(const UINT8* buff,UINT32 len,UINT32 type,const char* passwd)
	{
		if(buff==NULL||len<1)
		{
			return E_UNKNOWN;
		}
		switch (type)
		{
			case SIGKEY_XML:
				return parseSignKeyXML(buff,len);

			case SIGKEY_PKCS12:
				#if OPENSSL_VERSION_NUMBER	> 0x009070CfL
					PKCS12* tmpPKCS12=d2i_PKCS12(NULL,(const UINT8**)&buff,len);
				#else
					PKCS12* tmpPKCS12=d2i_PKCS12(NULL,(UINT8**)&buff,len);
				#endif

				EVP_PKEY* key=NULL;
				if(PKCS12_parse(tmpPKCS12,passwd,&key,NULL,NULL)!=1)
				{
					PKCS12_free(tmpPKCS12);
					return E_UNKNOWN;
				}
				PKCS12_free(tmpPKCS12);
				if(EVP_PKEY_type(key->type) == EVP_PKEY_DSA)
				{
					// found DSA key
					DSA* tmpDSA = DSA_clone(key->pkey.dsa);
					EVP_PKEY_free(key);
					if(DSA_sign_setup(tmpDSA,NULL,&tmpDSA->kinv,&tmpDSA->r)!=1)
					{
						DSA_free(tmpDSA);
						return E_UNKNOWN;
					}
					DSA_free(m_pDSA);
					m_pDSA = tmpDSA;
					return E_SUCCESS;
				}
				else if(EVP_PKEY_type(key->type) == EVP_PKEY_RSA)
				{
					// found RSA key
					RSA* tmpRSA = RSA_clone(key->pkey.rsa);
					EVP_PKEY_free(key);
					key = NULL;
					tmpRSA->flags |= RSA_FLAG_THREAD_SAFE;
					tmpRSA->flags |= RSA_FLAG_SIGN_VER;
					#ifdef RSA_FLAG_NO_BLINDING
						tmpRSA->flags |= RSA_FLAG_NO_BLINDING;
					#endif
					#if OPENSSL_VERSION_NUMBER	> 0x0090707fL
						tmpRSA->flags |= RSA_FLAG_NO_EXP_CONSTTIME;
					#endif
					RSA_free(m_pRSA);
					m_pRSA=tmpRSA;
					return E_SUCCESS;
				}
				else if(EVP_PKEY_type(key->type) == EVP_PKEY_EC)
				{
					// found EC key
					EC_KEY* tmpECKey = EC_KEY_dup(key->pkey.ec);
					EVP_PKEY_free(key);
					key = NULL;
					EC_KEY_free(m_pEC);
					m_pEC = tmpECKey;
					return E_SUCCESS;
				}
				else
				{
					EVP_PKEY_free(key);
				}
			}
			return E_UNKNOWN;
	}


//XML Decode...
SINT32 CASignature::parseSignKeyXML(const UINT8* buff,UINT32 len)
	{

		XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(buff,len);
		if(doc == NULL)
		{
			return E_UNKNOWN;
		}
		DOMElement* rootKeyInfo=doc->getDocumentElement();
		if(rootKeyInfo == NULL)
		{
			return E_UNKNOWN;
		}
		if(!equals(rootKeyInfo->getNodeName(),"KeyInfo"))
		{
			return E_UNKNOWN;
		}
		DOMNode* elemKeyValue;
		if(getDOMChildByName(rootKeyInfo,"KeyValue",elemKeyValue)!=E_SUCCESS)
			return E_UNKNOWN;
		if(getDOMChildByName(elemKeyValue,"DSAKeyValue",elemKeyValue)!=E_SUCCESS)
			return E_UNKNOWN;
		UINT8 tbuff[4096];
		UINT32 tlen=4096;
		DSA* tmpDSA=DSA_new();
		DOMNode* child=elemKeyValue->getFirstChild();
		while(child!=NULL)
			{
				char* name=XMLString::transcode(child->getNodeName());
				DOMNode* text=child->getFirstChild();
				if(text!=NULL)
					{
						char* tmpStr=XMLString::transcode(text->getNodeValue());
						tlen=4096;
						CABase64::decode((UINT8*)tmpStr,strlen(tmpStr),tbuff,&tlen);
						XMLString::release(&tmpStr);
						if(strcmp(name,"P")==0)
							{
								if(tmpDSA->p!=NULL)
									BN_free(tmpDSA->p);
								tmpDSA->p=BN_bin2bn(tbuff,tlen,NULL);
							}
						else if(strcmp(name,"Q")==0)
							{
								if(tmpDSA->q!=NULL)
									BN_free(tmpDSA->q);
								tmpDSA->q=BN_bin2bn(tbuff,tlen,NULL);
							}
						else if(strcmp(name,"G")==0)
							{
								if(tmpDSA->g!=NULL)
										BN_free(tmpDSA->g);
									tmpDSA->g=BN_bin2bn(tbuff,tlen,NULL);
							}
						else if(strcmp(name,"X")==0)
							{
								if(tmpDSA->priv_key!=NULL)
									BN_free(tmpDSA->priv_key);
								tmpDSA->priv_key=BN_bin2bn(tbuff,tlen,NULL);

							}
						else if(strcmp(name,"Y")==0)
							{
								if(tmpDSA->pub_key!=NULL)
									BN_free(tmpDSA->pub_key);
								tmpDSA->pub_key=BN_bin2bn(tbuff,tlen,NULL);
							}
					}
				XMLString::release(&name);
				child=child->getNextSibling();
			}
		if(DSA_sign_setup(tmpDSA,NULL,&tmpDSA->kinv,&tmpDSA->r)!=1)
			{
				DSA_free(tmpDSA);
				return E_UNKNOWN;
			}
		if(m_pDSA!=NULL)
			DSA_free(m_pDSA);
		m_pDSA=tmpDSA;
		return E_SUCCESS;
	}

/**
 * Perform Signature with either DSA, RSA or ECDSA
 */
SINT32 CASignature::sign(const UINT8* in,UINT32 inlen,UINT8* sig,UINT32* siglen) const
	{
		if(m_pDSA != NULL)
		{
			DSA_SIG* signature = NULL;
			if(	sign(in,inlen,&signature) != E_SUCCESS)
			{
				return E_UNKNOWN;
			}
			if(encodeRS(sig,siglen,signature) != E_SUCCESS)
			{
				DSA_SIG_free(signature);
				return E_UNKNOWN;
			}
			DSA_SIG_free(signature);
			return E_SUCCESS;
		}
		else if(m_pRSA != NULL || m_pEC != NULL)
		{
			UINT8 dgst[SHA_DIGEST_LENGTH];
			SHA1(in,inlen,dgst);
			if(m_pRSA != NULL) //either RSA or EC is set
			{
				return signRSA(dgst, SHA_DIGEST_LENGTH, sig, siglen);
			}
			else //(isECDSA())
			{
				return signECDSA(dgst, SHA_DIGEST_LENGTH, sig, siglen);
			}
		}
		return E_UNKNOWN;
	}

// TODO integrate this function to the one above!
SINT32 CASignature::sign(const UINT8* in,UINT32 inlen,DSA_SIG** pdsaSig) const
	{
		UINT8* dgst=new UINT8[SHA_DIGEST_LENGTH];
		SHA1(in,inlen,dgst);
		*pdsaSig=DSA_do_sign(dgst,SHA_DIGEST_LENGTH,m_pDSA);
		delete []dgst;
		if(*pdsaSig!=NULL)
		 return E_SUCCESS;
		return E_UNKNOWN;
	}

SINT32 CASignature::getSignatureSize() const
	{
		if(isDSA())
		{
			return DSA_size(m_pDSA);
		}
		if(isRSA())
		{
			return RSA_size(m_pRSA);
		}
		if(isECDSA())
		{
			const EC_GROUP* tmpGroup = EC_KEY_get0_group(m_pEC);

			BIGNUM* order = BN_new();
			EC_GROUP_get_order(tmpGroup, order, NULL);
			SINT32 size = BN_num_bytes(order) * 2;
			return size;
		}
		return E_UNKNOWN;
	}

/** Signs an XML Document.
	* @param in source byte array of the XML Document, which should be signed
	* @param inlen size of the source byte array
	* @param out destination byte array which on return contains the XML Document including the XML Signature
	* @param outlen size of destination byte array, on return contains the len of the signed XML document
	* @param pIncludeCerts points to a CACertStore, which holds CACertificates,
	*					which should be included in the XML Signature for easy verification; if NULL
	*					no Certs will be included
	*	@retval E_SUCCESS, if the Signature could be successful created
	* @retval E_SPACE, if the destination byte array is to small for the signed XML Document
	* @retval E_UNKNOWN, otherwise
*/
/*SINT32 CASignature::signXML(UINT8* in,UINT32 inlen,UINT8* out,UINT32* outlen,CACertStore* pIncludeCerts)
	{
		if(in==NULL||inlen<1||out==NULL||outlen==NULL)
			return E_UNKNOWN;

		XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(in,inlen);
		if(doc==NULL)
			return E_UNKNOWN;
		DOMElement* root=doc->getDocumentElement();
		SINT32 ret=signXML(root,pIncludeCerts);
		if(ret!=E_SUCCESS)
			{
				doc->release();
				return E_UNKNOWN;
			}
		ret=DOM_Output::dumpToMem(root,out,outlen);
		doc->release();
		return ret;
	}
*/
/** Signs a DOM Node. The XML Signature is include in the XML Tree as a Child of the Node.
	* If ther is already a Signature is is removed first.
	* @param node Node which should be signed
	* @param pIncludeCerts points to a CACertStore, which holds CACertificates,
	*					which should be included in the XML Signature for easy verification;
	*					if null no certificates will be included
	*	@retval E_SUCCESS, if the Signature could be successful created
	* @retval E_UNKNOWN, otherwise
*/
/*SINT32 CASignature::signXML(DOMNode* node,CACertStore* pIncludeCerts)
	{
		//getting the Document an the Node to sign
		XERCES_CPP_NAMESPACE::DOMDocument* doc=NULL;
		DOMNode* elemRoot=NULL;
		if(node->getNodeType()==DOMNode::DOCUMENT_NODE)
			{
				doc=(XERCES_CPP_NAMESPACE::DOMDocument*)node;
				elemRoot=doc->getDocumentElement();
			}
		else
			{
				elemRoot=node;
				doc=node->getOwnerDocument();
			}

		//check if there is already a Signature and if so remove it first...
		DOMNode* tmpSignature=NULL;
		if(getDOMChildByName(elemRoot,"Signature",tmpSignature,false)==E_SUCCESS)
			{
				DOMNode* n=elemRoot->removeChild(tmpSignature);
				if (n != NULL)
				{
					n->release();
					n = NULL;
				}
			}

		//Calculating the Digest...
		UINT32 len=0;
		UINT8* canonicalBuff=DOM_Output::makeCanonical(elemRoot,&len);
		if(canonicalBuff==NULL)
			return E_UNKNOWN;

		UINT8 dgst[SHA_DIGEST_LENGTH];
		SHA1(canonicalBuff,len,dgst);
		delete[]canonicalBuff;
		canonicalBuff = NULL;

		UINT8 tmpBuff[1024];
		len=1024;
		if(CABase64::encode(dgst,SHA_DIGEST_LENGTH,tmpBuff,&len)!=E_SUCCESS)
			return E_UNKNOWN;
		tmpBuff[len]=0;


		//Creating the Sig-InfoBlock....
		DOMElement* elemSignedInfo=createDOMElement(doc,"SignedInfo");
		DOMElement* elemCanonicalizationMethod=createDOMElement(doc,"CanonicalizationMethod");
		DOMElement* elemSignatureMethod=createDOMElement(doc,"SignatureMethod");
		if(isDSA())
		{
			setDOMElementAttribute(elemSignatureMethod, "Algorithm", (UINT8*)DSA_SHA1_REFERENCE);
		}
		else if(isRSA())
		{
			setDOMElementAttribute(elemSignatureMethod, "Algorithm", (UINT8*)RSA_SHA1_REFERENCE);
		}
		else if(isECDSA())
		{
			setDOMElementAttribute(elemSignatureMethod, "Algorithm", (UINT8*)ECDSA_SHA1_REFERENCE);
		}
		DOMElement* elemReference=createDOMElement(doc,"Reference");
		//setDOMElementAttribute(elemReference,"URI",(UINT8*)"");
		DOMElement* elemDigestMethod=createDOMElement(doc, "DigestMethod");
		setDOMElementAttribute(elemDigestMethod, "Algorithm", (UINT8*)SHA1_REFERENCE);
		DOMElement* elemDigestValue=createDOMElement(doc,"DigestValue");
		setDOMElementValue(elemDigestValue,tmpBuff);

		elemSignedInfo->appendChild(elemCanonicalizationMethod);
		elemSignedInfo->appendChild(elemSignatureMethod);
		elemSignedInfo->appendChild(elemReference);
		elemReference->appendChild(elemDigestMethod);
		elemReference->appendChild(elemDigestValue);

		// Signing the SignInfo block....
		canonicalBuff=DOM_Output::makeCanonical(elemSignedInfo,&len);
		if(canonicalBuff==NULL)
		{
			return E_UNKNOWN;
		}
		if(isDSA())
		{
			DSA_SIG* pdsaSig=NULL;
			SINT32 ret=sign(canonicalBuff,len,&pdsaSig);
			delete[] canonicalBuff;
			canonicalBuff = NULL;
			if(ret!=E_SUCCESS)
			{
				DSA_SIG_free(pdsaSig);
				return E_UNKNOWN;
			}
			len=1024;
			encodeRS(tmpBuff,&len,pdsaSig);
			//memset(tmpBuff,0,40); //make first 40 bytes '0' --> if r or s is less then 20 bytes long!
													//(Due to be compatible to the standarad r and s must be 20 bytes each)
			//BN_bn2bin(pdsaSig->r,tmpBuff+20-BN_num_bytes(pdsaSig->r)); //so r is 20 bytes with leading '0'...
			//BN_bn2bin(pdsaSig->s,tmpBuff+40-BN_num_bytes(pdsaSig->s));
			DSA_SIG_free(pdsaSig);
		}
		else if(isRSA() || isECDSA())
		{
			UINT32 sigLen = getSignatureSize();
			SINT32 ret = sign(canonicalBuff, len, tmpBuff, &sigLen);
			delete[] canonicalBuff;
			canonicalBuff = NULL;

			if(ret != E_SUCCESS)
			{
				return E_UNKNOWN;
			}
			len = sigLen;
		}
		else
		{
			return E_UNKNOWN;
		}
		UINT sigSize=255;
		UINT8 sig[255];
		if(CABase64::encode(tmpBuff,len,sig,&sigSize)!=E_SUCCESS)
			return E_UNKNOWN;
		sig[sigSize]=0;

		//Makeing the whole Signature-Block....
		DOMElement* elemSignature=createDOMElement(doc,"Signature");
		DOMElement* elemSignatureValue=createDOMElement(doc,"SignatureValue");
		setDOMElementValue(elemSignatureValue,sig);
		elemSignature->appendChild(elemSignedInfo);
		elemSignature->appendChild(elemSignatureValue);

		if(pIncludeCerts!=NULL)
			{
				//Making KeyInfo-Block
				DOMElement* tmpElemCerts=NULL;
				if(pIncludeCerts->encode(tmpElemCerts,doc)==E_SUCCESS && tmpElemCerts!=NULL)
					{
						DOMElement* elemKeyInfo=createDOMElement(doc,"KeyInfo");
						elemKeyInfo->appendChild(tmpElemCerts);
						elemSignature->appendChild(elemKeyInfo);
					}
			}
		elemRoot->appendChild(elemSignature);
		return E_SUCCESS;
	}*/


SINT32 CASignature::getVerifyKey(CACertificate** ppCert)
	{
		//We need this DAS key as EVP key...
		EVP_PKEY* pPKey=EVP_PKEY_new();
		EVP_PKEY_set1_DSA(pPKey,m_pDSA);
		*ppCert=new CACertificate();
		(*ppCert)->m_pCert=X509_new();
    // LERNGRUPPE
    // We nned to use Version 3 to use extensions
// 		X509_set_version((*ppCert)->m_pCert,2);
 		X509_set_version((*ppCert)->m_pCert,3);
		ASN1_TIME* pTime=ASN1_TIME_new();
		ASN1_TIME_set(pTime,time(NULL));
		X509_set_notBefore((*ppCert)->m_pCert,pTime);
		X509_set_notAfter((*ppCert)->m_pCert,pTime);
		X509_set_pubkey((*ppCert)->m_pCert,pPKey);
// LERNGRUPPE
// Add the subjectKeyIdentifier-Extension to the certificate
                if( (*ppCert)->setSubjectKeyIdentifier() != E_SUCCESS )
                {
                    CAMsg::printMsg( LOG_ERR, "Couldn't add the SKI to the certificate!\n");
                    return E_UNKNOWN;
                }

		X509_sign((*ppCert)->m_pCert,pPKey,EVP_sha1());
		EVP_PKEY_free(pPKey);
		return E_SUCCESS;
	}

/** Calculates a SHA hash of the public key, which is represented as SubjectPublicKeyInfo
	*/
SINT32 CASignature::getVerifyKeyHash(UINT8* buff,UINT32* len)
	{
		 UINT8* tmpBuff=NULL;
		 int l=i2d_DSA_PUBKEY(m_pDSA,&tmpBuff);
		 SHA1(tmpBuff,l,buff);
		 *len=SHA_DIGEST_LENGTH;
		 OPENSSL_free(tmpBuff);
		 return E_SUCCESS;
	}

/** Set the key for signature testing to the one include in pCert. If pCert ==NULL clears the
	* signature test key
	* @param pCert Certificate including the test key
	* @retval E_SUCCESS, if succesful
	* @retval E_UNKNOWN otherwise
*/
SINT32 CASignature::setVerifyKey(CACertificate* pCert)
	{
		if(pCert==NULL)
		{
			if(isDSA())
			{
				DSA_free(m_pDSA);
				m_pDSA = NULL;
			}
			else if (isRSA())
			{
				RSA_free(m_pRSA);
				m_pRSA = NULL;
			}
			else if (isECDSA());
			{
				EC_KEY_free(m_pEC);
				m_pEC = NULL;
			}
			return E_SUCCESS;
		}

		EVP_PKEY *key=X509_get_pubkey(pCert->m_pCert);
		if(EVP_PKEY_type(key->type) == EVP_PKEY_DSA)
		{
			DSA* tmpDSA=DSA_clone(key->pkey.dsa);
			EVP_PKEY_free(key);
			DSA_free(m_pDSA);
			m_pDSA=tmpDSA;
			return E_SUCCESS;
		}
		if(EVP_PKEY_type(key->type) == EVP_PKEY_RSA)
		{
			RSA* tmpRSA = RSA_clone(key->pkey.rsa);
			EVP_PKEY_free(key);
			RSA_free(m_pRSA);
			m_pRSA = tmpRSA;
			return E_SUCCESS;
		}
		if(EVP_PKEY_type(key->type) == EVP_PKEY_EC)
		{
			EC_KEY* tmpEC = EC_KEY_dup(key->pkey.ec);
			EVP_PKEY_free(key);
			EC_KEY_free(m_pEC);
			m_pEC = tmpEC;
			return E_SUCCESS;
		}
		//key-type is unknown
		EVP_PKEY_free(key);
		return E_UNKNOWN;
	}

/**
 * Parses the XML representation of a DSA public key
 */
SINT32 CASignature::setVerifyKey(const DOMElement* xmlKey)
{
	UINT8 decodeBuffer[4096];
	UINT32 len = 4096;
	UINT32 encodedLen = 0;
	DSA * tmpDSA = NULL;

	if(xmlKey==NULL)
	{
		DSA_free(m_pDSA);
		m_pDSA = NULL;
		return E_SUCCESS;
	}
	if(!equals(xmlKey->getTagName(), "JapPublicKey")!=0)
	{
		char* tmpStr=XMLString::transcode(xmlKey->getTagName());
		CAMsg::printMsg(LOG_DEBUG, "CASignature::setVerifyKey(): no JapPublicKey! -- Tagname is %s\n", tmpStr);
		XMLString::release(&tmpStr);
		return E_UNKNOWN;
	}

	decodeBuffer[0]=0;
	if( getDOMElementAttribute(xmlKey,"version",decodeBuffer,&len)!=E_SUCCESS ||
		strcmp((char*)decodeBuffer, "1.0")!=0 )
	{
		CAMsg::printMsg(LOG_DEBUG,
				"CASignature::setVerifyKey(): JapPublicKey has unknown version %s. "
				"Version 1.0 expected!",decodeBuffer);
		return E_UNKNOWN;
	}

	DOMNode* elemDsaKey;
	if(getDOMChildByName(xmlKey, "DSAKeyValue", elemDsaKey, false)
			!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG,
					"CASignature::setVerifyKey(): DSAKeyValue not found!");
			return E_UNKNOWN;
		}

	tmpDSA=DSA_new();

	// parse "Y"
	DOMNode* elem;
	if(getDOMChildByName(elemDsaKey, "Y", elem, false)
			!=E_SUCCESS)
		{
			return E_UNKNOWN;
		}
	len=4096;
	if(getDOMElementValue(elem, decodeBuffer, &len)!=E_SUCCESS)
	{
		DSA_free(tmpDSA);
		return E_UNKNOWN;
	}
	encodedLen = len; len = 4096;
	if(CABase64::decode(decodeBuffer, encodedLen, decodeBuffer, &len)!=E_SUCCESS)
	{
		DSA_free(tmpDSA);
		return E_UNKNOWN;
	}
	tmpDSA->pub_key = BN_bin2bn(decodeBuffer,len,NULL);


	// parse "G"
	len = 4096;
	if(getDOMChildByName(elemDsaKey, "G", elem, false)
			!=E_SUCCESS)
		{
			DSA_free(tmpDSA);
			return E_UNKNOWN;
		}
	if(getDOMElementValue(elem, decodeBuffer, &len)!=E_SUCCESS)
	{
		DSA_free(tmpDSA);
		return E_UNKNOWN;
	}
	encodedLen = len; len = 4096;
	if(CABase64::decode(decodeBuffer, encodedLen, decodeBuffer, &len)!=E_SUCCESS)
	{
		DSA_free(tmpDSA);
		return E_UNKNOWN;
	}
	tmpDSA->g=BN_bin2bn(decodeBuffer,len,NULL);


	// parse "P"
	len = 4096;
	if(getDOMChildByName(elemDsaKey,"P", elem, false)
			!=E_SUCCESS)
		{
			DSA_free(tmpDSA);
			return E_UNKNOWN;
		}
	if(getDOMElementValue(elem, decodeBuffer, &len)!=E_SUCCESS)
	{
		DSA_free(tmpDSA);
		return E_UNKNOWN;
	}
	encodedLen = len; len = 4096;
	if(CABase64::decode(decodeBuffer, encodedLen, decodeBuffer, &len)!=E_SUCCESS)
	{
		DSA_free(tmpDSA);
		return E_UNKNOWN;
	}
	tmpDSA->p=BN_bin2bn(decodeBuffer,len,NULL);


	// parse "Q"
	len = 4096;
	if(getDOMChildByName(elemDsaKey, "Q", elem, false)
			!=E_SUCCESS)
		{
			DSA_free(tmpDSA);
			return E_UNKNOWN;
		}
	if(getDOMElementValue(elem, decodeBuffer, &len)!=E_SUCCESS)
	{
		DSA_free(tmpDSA);
		return E_UNKNOWN;
	}
	encodedLen = len;
	len = 1024;
	if(CABase64::decode(decodeBuffer, encodedLen, decodeBuffer, &len)!=E_SUCCESS)
	{
		DSA_free(tmpDSA);
		return E_UNKNOWN;
	}
	tmpDSA->q=BN_bin2bn(decodeBuffer,len,NULL);

	if( tmpDSA->pub_key!=NULL && tmpDSA->g!=NULL && tmpDSA->p!=NULL && tmpDSA->q!=NULL)
	{
		if(m_pDSA!=NULL)
		{
			DSA_free(m_pDSA);
		}
		m_pDSA = tmpDSA;
		return E_SUCCESS;
	}
	DSA_free(tmpDSA);
	return E_UNKNOWN;
}


SINT32 CASignature::verify(const UINT8* const in,UINT32 inlen,DSA_SIG* const dsaSig) const
	{
		if(m_pDSA==NULL||dsaSig==NULL||dsaSig->r==NULL||dsaSig->s==NULL)
			return E_UNKNOWN;
		SINT32 ret=E_UNKNOWN;
		UINT8* dgst=new UINT8[SHA_DIGEST_LENGTH];
		SHA1(in,inlen,dgst);
		if(DSA_do_verify(dgst,SHA_DIGEST_LENGTH,dsaSig,m_pDSA)==1)
			{
				ret=E_SUCCESS;
			}
		delete [] dgst;
		return ret;
	}


/**
 * Verifies an ASN.1 DER encoded SHA1-DSA signature
 *
 * @author Bastian Voigt
 * @param in the document that was signed
 * @param inlen, the document length
 * @param dsaSig the DER encoded signature
 * @param sigLen the signature length (normally 46 bytes)
 * @return E_SUCCESS if the signature is valid,
 * 		E_UNKNOWN if an error occurs,
 * 		E_INVALID if the signature is invalid
 */
SINT32 CASignature::verifyDER(UINT8* in, UINT32 inlen, const UINT8 * dsaSig, const UINT32 sigLen)
	{
		UINT8 dgst[SHA_DIGEST_LENGTH];
		UINT32 rc;

		if(m_pDSA==NULL||dsaSig==NULL)
			return E_UNKNOWN;
		SHA1(in,inlen,dgst);
		if((rc=DSA_verify(0, dgst, SHA_DIGEST_LENGTH, dsaSig, sigLen, m_pDSA))==1)
			return E_SUCCESS;
		else if(rc==0)
			return E_INVALID; // wrong signature
		return E_UNKNOWN;
	}


/*SINT32 CASignature::verifyXML(const UINT8* const in,UINT32 inlen)
	{
		XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(in,inlen);
		if(doc == NULL)
		{
			return E_UNKNOWN;
		}
		DOMElement* root=doc->getDocumentElement();
		if(root == NULL)
		{
			return E_UNKNOWN;
		}
		//CAMsg::printMsg(LOG_DEBUG,"verified document 0x%x doesn't clean up itself!\n",
							//doc);
		return verifyXML(root,NULL);
	}*/


/** Verifies a XML Signature under node root.*/
SINT32 CASignature::verifyXML(DOMNode* root,CACertStore* trustedCerts)
	{
		DOMNode* elemSignature;
		getDOMChildByName(root,"Signature",elemSignature);
		if(elemSignature==NULL)
			return E_UNKNOWN;
		DOMNode* elemSigValue;
		getDOMChildByName(elemSignature,"SignatureValue",elemSigValue);
		if(elemSigValue==NULL)
			return E_UNKNOWN;
		DOMNode* elemSigInfo;
		getDOMChildByName(elemSignature,"SignedInfo",elemSigInfo);
		if(elemSigInfo==NULL)
			return E_UNKNOWN;
		DOMNode* elemReference;
		getDOMChildByName(elemSigInfo,"Reference",elemReference);
		if(elemReference==NULL)
			return E_UNKNOWN;
		DOMNode* elemDigestValue;
		getDOMChildByName(elemReference,"DigestValue",elemDigestValue);
		if(elemDigestValue==NULL)
			return E_UNKNOWN;

		UINT8 dgst[255];
		UINT32 dgstlen=255;
		if(getDOMElementValue(elemDigestValue,dgst,&dgstlen)!=E_SUCCESS)
			return E_UNKNOWN;
		if(CABase64::decode(dgst,dgstlen,dgst,&dgstlen)!=E_SUCCESS)
			return E_UNKNOWN;
		if(dgstlen!=SHA_DIGEST_LENGTH)
			return E_UNKNOWN;
		UINT8 tmpSig[255];
		UINT32 tmpSiglen=255;
		if(getDOMElementValue(elemSigValue,tmpSig,&tmpSiglen)!=E_SUCCESS)
			return E_UNKNOWN;
		if(CABase64::decode(tmpSig,tmpSiglen,tmpSig,&tmpSiglen)!=E_SUCCESS)
			return E_UNKNOWN;

		UINT8* out=new UINT8[5000];
		UINT32 outlen=5000;
		if(DOM_Output::makeCanonical(elemSigInfo, out, &outlen) != E_SUCCESS)
		{
			delete[] out;
			out = NULL;
			return E_UNKNOWN;
		}
		if(isDSA())
		{
			if(tmpSiglen!=40)
			{
				delete[] out;
				out = NULL;
				return E_UNKNOWN;
			}
			DSA_SIG* dsaSig=DSA_SIG_new();
	   		dsaSig->r=BN_bin2bn(tmpSig,20,dsaSig->r);
			dsaSig->s=BN_bin2bn(tmpSig+20,20,dsaSig->s);
			if(verify(out,outlen,dsaSig)!=E_SUCCESS)
			{
				DSA_SIG_free(dsaSig);
				delete[] out;
				out = NULL;
				return E_UNKNOWN;
			}
			DSA_SIG_free(dsaSig);
		}
		else if(isRSA() || isECDSA())
		{
			UINT8 sha1[SHA_DIGEST_LENGTH];
			SHA1(out, outlen, sha1);
			SINT32 ret;
			if(m_pRSA != NULL)
			{
				ret = RSA_verify(NID_sha1, sha1, SHA_DIGEST_LENGTH, tmpSig, tmpSiglen, m_pRSA);
			}
			else
			{
				ret = ECDSA_verify(NID_sha1, sha1, SHA_DIGEST_LENGTH, tmpSig, tmpSiglen, m_pEC);
			}
			if(ret != 1)
			{
				delete[] out;
				out = NULL;
				return E_UNKNOWN;
			}
		}

		DOMNode* tmpNode=root->removeChild(elemSignature);
		outlen=5000;
		DOM_Output::makeCanonical(root,out,&outlen);
		root->appendChild(tmpNode);
		UINT8 dgst1[SHA_DIGEST_LENGTH];
		SHA1(out,outlen,dgst1);
		delete[] out;
		out = NULL;
		for(int i=0;i<SHA_DIGEST_LENGTH;i++)
		{
			if(dgst1[i]!=dgst[i])
			{
				return E_UNKNOWN;
			}
		}
		return E_SUCCESS;
	}

SINT32 CASignature::encodeRS(UINT8* out,UINT32* outLen,const DSA_SIG* const pdsaSig) const
	{
		UINT32 rSize, sSize;
		memset(out,0,40); //make first 40 bytes '0' --> if r or s is less then 20 bytes long!
											//(Due to be compatible to the standarad r and s must be 20 bytes each)
		rSize = BN_num_bytes(pdsaSig->r);
		sSize = BN_num_bytes(pdsaSig->s);
		BN_bn2bin(pdsaSig->r,out+20-rSize); //so r is 20 bytes with leading '0'...
		BN_bn2bin(pdsaSig->s,out+40-sSize);
		*outLen=40;
		return E_SUCCESS;
	}


SINT32 CASignature::decodeRS(const UINT8* const in, const UINT32 inLen, DSA_SIG* pDsaSig) const
{
	ASSERT(pDsaSig!=NULL, "DSA_SIG is null");
	ASSERT(inLen>20, "Inbuffer is <=20 bytes");
	pDsaSig->r = BN_bin2bn(in, 20, NULL);
	pDsaSig->s = BN_bin2bn(in+20, inLen-20, NULL);
	return E_SUCCESS;
}

SINT32 CASignature::signRSA(const UINT8* dgst, UINT32 dgstLen, UINT8* sig, UINT32* sigLen) const
{
	if(RSA_sign(NID_sha1, dgst, dgstLen, sig, sigLen, m_pRSA) != 1)
	{
		return E_UNKNOWN;
	}

	return E_SUCCESS;
}

SINT32 CASignature::signECDSA(const UINT8* dgst, UINT32 dgstLen, UINT8* sig, UINT32* sigLen) const
{
	UINT32 len = getSignatureSize();
	if(len > *sigLen)
	{
		return E_UNKNOWN;
	}
	ECDSA_SIG* ecdsaSig = ECDSA_do_sign(dgst, dgstLen, m_pEC);
	if(ecdsaSig == NULL)
	{
		return E_UNKNOWN;
	}
	memset(sig, 0, *sigLen);
	UINT32 rSize, sSize;
	rSize = BN_num_bytes(ecdsaSig->r);
	sSize = BN_num_bytes(ecdsaSig->s);

	UINT32 rPos = (len/2)-rSize;
	UINT32 sPos = len-sSize;

	//CAMsg::printMsg(LOG_DEBUG, "Sig-Positions r: %d(size=%d), s: %d(size=%d)\n", rPos, rSize, sPos, sSize);
	BN_bn2bin(ecdsaSig->r, sig + rPos);
	BN_bn2bin(ecdsaSig->s, sig + sPos);
	*sigLen = len;

	UINT32 tmplen = 255;
	UINT8 tmpbuff[tmplen];
	CABase64::encode(sig, *sigLen, tmpbuff, &tmplen);
	ECDSA_SIG_free(ecdsaSig);

	return E_SUCCESS;
}

SINT32 CASignature::verify(UINT8* in, UINT32 inLen, UINT8* sig, const UINT32 sigLen)
{
	UINT8 sha1[SHA_DIGEST_LENGTH];
	SHA1(in, inLen, sha1);
	SINT32 ret = -1;
	if(isDSA())
	{
		ret = verifyDSA(sha1, SHA_DIGEST_LENGTH, sig, sigLen);
	}
	else if(isRSA())
	{
		ret = verifyRSA(sha1, SHA_DIGEST_LENGTH, sig, sigLen);
	}
	else if(isECDSA())
	{
		ret = verifyECDSA(sha1, SHA_DIGEST_LENGTH, sig, sigLen);
	}
	if(ret == 1)
	{
		return E_SUCCESS;
	}
	return E_UNKNOWN;
}

SINT32 CASignature::verifyRSA(const UINT8* dgst, const UINT32 dgstLen, UINT8* sig, UINT32 sigLen) const
{
	if(sigLen != (UINT32)getSignatureSize())
	{
		return E_UNKNOWN;
	}
	return RSA_verify(NID_sha1, dgst, dgstLen, sig, sigLen, m_pRSA);
}

SINT32 CASignature::verifyDSA(const UINT8* dgst, const UINT32 dgstLen, UINT8* sig, UINT32 sigLen) const
{
	if(sigLen != 40)
	{
		return E_UNKNOWN;
	}
	DSA_SIG* dsaSig = DSA_SIG_new();
	dsaSig->r = BN_bin2bn(sig, 20, dsaSig->r);
	dsaSig->s = BN_bin2bn(sig+20, 20, dsaSig->s);

	SINT32 ret = DSA_do_verify(dgst, dgstLen, dsaSig, m_pDSA);
	DSA_SIG_free(dsaSig);

	return ret;
}

SINT32 CASignature::verifyECDSA(const UINT8* dgst, const UINT32 dgstLen, UINT8* sig, UINT32 sigLen) const
{
	SINT32 len = sigLen / 2;
	ECDSA_SIG* ecdsaSig = ECDSA_SIG_new();
	ecdsaSig->r = BN_bin2bn(sig, len, ecdsaSig->r);
	ecdsaSig->s = BN_bin2bn(sig+len, len, ecdsaSig->s);

	SINT32 ret = ECDSA_do_verify(dgst, dgstLen, ecdsaSig, m_pEC);
	ECDSA_SIG_free(ecdsaSig);

	return ret;
}

bool CASignature::isDSA() const
{
	if(m_pDSA != NULL)
	{
		return true;
	}
	return false;
}

bool CASignature::isRSA() const
{
	if(m_pRSA != NULL)
	{
		return true;
	}
	return false;
}

bool CASignature::isECDSA() const
{
	if(m_pEC != NULL)
	{
		return true;
	}
	return false;
}

bool CASignature::isSet() const
{
	return (isDSA() || isRSA() || isECDSA());
}

UINT8* CASignature::getSignatureMethod()
{
	if(m_pDSA != NULL)
	{
		return (UINT8*)DSA_SHA1_REFERENCE;
	}
	if(m_pRSA != NULL)
	{
		return (UINT8*)RSA_SHA1_REFERENCE;
	}
	if(m_pEC != NULL)
	{
		return (UINT8*)ECDSA_SHA1_REFERENCE;
	}
	return NULL;
}

#endif //ONLY_LOCAL_PROXY
