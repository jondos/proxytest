/*
Copyright (c) 2000, The JAP-Team
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

        - Redistributions of source code must retain the above copyright notice,
          this list of conditions and the following disclaimer.

        - Redistributions in binary form must reproduce the above copyright
notice,
          this list of conditions and the following disclaimer in the
documentation and/or
                other materials provided with the distribution.

        - Neither the name of the University of Technology Dresden, Germany nor
the names of its contributors
          may be used to endorse or promote products derived from this software
without specific
                prior written permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE
*/
#include "StdAfx.h"

#include "CAASymCipher.hpp"
#include "CABase64.hpp"
#include "CAUtil.hpp"
#include "xml/DOM_Output.hpp"
CAASymCipher::CAASymCipher()
{
	m_pRSA = NULL;
}

CAASymCipher::~CAASymCipher()
{
	destroy();
}

SINT32 CAASymCipher::destroy()
{
	RSA_free(m_pRSA);
	m_pRSA = NULL;
	return E_SUCCESS;
}

void setRSAFlags(RSA *pRSA)
{
	if (pRSA == NULL)
		return;
	#if  OPENSSL_VERSION_NUMBER >= 0x1000204fL
		RSA_set_flags(pRSA,RSA_FLAG_THREAD_SAFE | RSA_FLAG_NO_BLINDING );
	#else
			pRSA->flags |= RSA_FLAG_THREAD_SAFE;
			pRSA->flags |= RSA_FLAG_SIGN_VER;
		#ifdef RSA_FLAG_NO_BLINDING
			pRSA->flags |= RSA_FLAG_NO_BLINDING;
		#endif
	#endif //OPENSSL_VERSION_NUMBER >= 0x1000204fL
#if OPENSSL_VERSION_NUMBER > 0x0090707fL
//	pRSA->flags|=RSA_FLAG_NO_EXP_CONSTTIME;
#endif
}

/** Decrypts exactly one block which is stored in @c from.
        *The result of the decryption is stored in @c to.
        *@param from one block of cipher text
        *@param to the decrypted plain text
        *@retval E_UNKNOWN in case of an error
        *@retval E_SUCCESS otherwise
        */
SINT32 CAASymCipher::decrypt(const UINT8 *from, UINT8 *to)
{
	if (RSA_private_decrypt(RSA_SIZE, from, to, m_pRSA, RSA_NO_PADDING) == -1)
		return E_UNKNOWN;
	else
		return E_SUCCESS;
}

/** Decrypts one OAEP encoded block which is stored in @c from.
        *@param from one OAEP encoded block of cipher text
        *@param to the plain text
        *@param len on return contains the size of the plaintext
        *@retval E_UNKNOWN in case of an error
        *@retval E_SUCCESS otherwise
        */
SINT32 CAASymCipher::decryptOAEP(const UINT8 *from, UINT8 *to, UINT32 *len)
{
	SINT32 ret =
	  RSA_private_decrypt(RSA_SIZE, from, to, m_pRSA, RSA_PKCS1_OAEP_PADDING);
	if (ret < 0)
		return E_UNKNOWN;
	*len = ret;
	return E_SUCCESS;
}

/** Encrypts one block of plain text using OAEP padding.
        *@param from pointer to one block of plain text
        *@param fromlen size of the plain text
        *@param to the OAEP encoded cipher text
        *@param len on return contains the size of the ciphertext
        *@retval E_UNKNOWN in case of an error
        *@retval E_SUCCESS otherwise
        */
SINT32 CAASymCipher::encryptOAEP(const UINT8 *from, UINT32 fromlen, UINT8 *to,
                                 UINT32 *len)
{
	SINT32 ret =
	  RSA_public_encrypt(fromlen, from, to, m_pRSA, RSA_PKCS1_OAEP_PADDING);
	if (ret < 0)
		return E_UNKNOWN;
	*len = ret;
	return E_SUCCESS;
}

/** Encrypts one block of plain text using PKCS1 padding.
        *@param from pointer to one block of plain text
        *@param fromlen size of the plain text
        *@param to the OAEP encoded cipher text
        *@param len on return contains the size of the ciphertext
        *@retval E_UNKNOWN in case of an error
        *@retval E_SUCCESS otherwise
        * Temporarly will be removed soon.
        */
SINT32 CAASymCipher::encryptPKCS1(const UINT8 *from, UINT32 fromlen, UINT8 *to,
                                  UINT32 *len)
{
	SINT32 ret = RSA_public_encrypt(fromlen, from, to, m_pRSA, RSA_PKCS1_PADDING);
	if (ret < 0)
		return E_UNKNOWN;
	*len = ret;
	return E_SUCCESS;
}

/** Encrypts exactly one block which is stored in @c from.
 *The result of the encrpytion is stored in @c to.
        *@param from one block of plain text
        *@param to the encrypted cipher text
        *@retval E_UNKNOWN in case of an error
        *@retval E_SUCCESS otherwise
        */
SINT32 CAASymCipher::encrypt(const UINT8 *from, UINT8 *to)
{
	if (RSA_public_encrypt(RSA_SIZE, from, to, m_pRSA, RSA_NO_PADDING) == -1)
		return E_UNKNOWN;
	else
		return E_SUCCESS;
}

/** Generates a new random key-pair of \c size bits.
        *@param size keysize of the new keypair
        *@retval E_UNKNOWN in case of an error
        *@retval E_SUCCESS otherwise
        */
SINT32 CAASymCipher::generateKeyPair(UINT32 size)
{
	RSA_free(m_pRSA);
	m_pRSA = RSA_generate_key(size, 65537, NULL, NULL);
	::setRSAFlags(m_pRSA);
	if (m_pRSA == NULL)
		return E_UNKNOWN;
	else
		return E_SUCCESS;
}

/** Stores the public key in \c buff. The format is as follows:
        *
        * \li \c SIZE-N [2 bytes] - number of bytes which are needed for the
  *modulus n (in network byte order..)
        * \li \c N [SIZE-N bytes] - the modulus \c n as integer (in network byte
  *order)
        * \li \c SIZE-E [2 bytes] - number of bytes which are needed for the
  *exponent e (in network byte order..)
        * \li \c E [SIZE-E bytes] - the exponent \c e as integer (in network
  *byte order)
        *
        *@param buff byte array in which the public key should be stored
        *@param len on input holds the size of \c buff, on return it contains
  *the number
        *           of bytes needed to store the public key
        *@retval E_UNKNOWN in case of an error
        *@retval E_SUCCESS otherwise
        *@see getPublicKeySize()
        *@see setPublicKey()
        */
/*
SINT32 CAASymCipher::getPublicKey(UINT8* buff,UINT32 *len)
{
        if(m_pRSA==NULL||buff==NULL)
                return E_UNKNOWN;
        SINT32 keySize=getPublicKeySize();
        if(keySize<=0||(*len)<(UINT32)keySize)
                return E_UNKNOWN;
        UINT32 aktIndex=0;
        UINT16 size=htons(BN_num_bytes(m_pRSA->n));
        memcpy(buff,&size,sizeof(size));
        aktIndex+=sizeof(size);
        BN_bn2bin(m_pRSA->n,buff+aktIndex);
        aktIndex+=BN_num_bytes(m_pRSA->n);
        size=htons(BN_num_bytes(m_pRSA->e));
        memcpy(buff+aktIndex,&size,sizeof(size));
        aktIndex+=sizeof(size);
        BN_bn2bin(m_pRSA->e,buff+aktIndex);
        aktIndex+=BN_num_bytes(m_pRSA->e);
        *len=aktIndex;
        return E_SUCCESS;
}*/

/** Returns the number of bytes needed to store we public key. This is the
  *number of bytes needed for a
        * call of getPublicKey().
        *@return E_UNKOWN in case of an error
        *        number of bytes otherwise
        *@see getPublicKey
        */
/*SINT32 CAASymCipher::getPublicKeySize()
        {
                if(m_pRSA==NULL||m_pRSA->n==NULL||m_pRSA->e==NULL)
                        return E_UNKNOWN;
                return
   (SINT32)BN_num_bytes(m_pRSA->n)+BN_num_bytes(m_pRSA->e)+4;
        }*/

/** Sets the public key to the vaules stored in \c key. The format must match
  *the format described for getPublicKey().
        *@param key byte array which holds the new public key
        *@param len on input,size of key byte array, on successful return number
  *of bytes 'consumed'
        *@retval E_UNKNOWN in case of an error, the cipher is the uninitialized
  *(no key is set)
        *@retval E_SUCCESS otherwise
        *@see getPublicKey
        */
/*SINT32 CAASymCipher::setPublicKey(UINT8* key,UINT32* len)
        {
                UINT32 aktIndex;
                UINT32 availBytes;
                UINT16 size;

                if(key==NULL||len==NULL||(*len)<6) //the need at least 6 bytes:
4 for the sizes and 1 for n and 1 for e
                        goto _ERROR;
                availBytes=(*len);
                RSA_free(m_pRSA);
                m_pRSA=RSA_new();
                if(m_pRSA==NULL)
                        goto _ERROR;
                memcpy(&size,key,2);
                size=ntohs(size);
                availBytes-=2;
                if(size>availBytes-3) //the need at least 3 bytes for the
exponent...
                        goto _ERROR;
                aktIndex=2;
                availBytes-=size;
                m_pRSA->n=BN_new();
                if(m_pRSA->n==NULL)
                        goto _ERROR;
                BN_bin2bn(key+aktIndex,size,m_pRSA->n);
                aktIndex+=size;
                memcpy(&size,key+aktIndex,2);
                size=ntohs(size);
                availBytes-=2;
                aktIndex+=2;
                if(size>availBytes)
                        goto _ERROR;
                m_pRSA->e=BN_new();
                BN_bin2bn(key+aktIndex,size,m_pRSA->e);
                aktIndex+=size;
                (*len)=aktIndex;
                m_pRSA->flags|=RSA_FLAG_THREAD_SAFE;
                #ifdef RSA_FLAG_NO_BLINDING
                        m_pRSA->flags|=RSA_FLAG_NO_BLINDING;
                #endif
                return E_SUCCESS;
_ERROR:
                RSA_free(m_pRSA);
                m_pRSA=NULL;
                return E_UNKNOWN;
        }
*/

#ifndef ONLY_LOCAL_PROXY

/** Stores the public key in \c buff as XML. The format is as follows:
        *
        * \verbatim
        <RSAKeyValue>
          <Modulus>
            the modulus of the Key as ds::CryptoBinary
          </Modulus>
          <Exponent>
            the exponent of the key as ds::CryptoBinary
          </Exponent>
        <RSAKeyValue>\endverbatim
        *There is NO \\0 at the end.
        *@param buff byte array in which the public key should be stored
        *@param len on input holds the size of \c buff, on return it contains
   the number
        *           of bytes needed to store the public key
        *@retval E_UNKNOWN in case of an error
        *@retval E_SUCCESS otherwise
        *
        *@see setPublicKeyAsXML()
        */
SINT32 CAASymCipher::getPublicKeyAsXML(UINT8 *buff, UINT32 *len)
{
	if (m_pRSA == NULL || buff == NULL)
		return E_UNKNOWN;
	XERCES_CPP_NAMESPACE::DOMDocument *pDoc = createDOMDocument();
	DOMElement *elemRoot = NULL;
	getPublicKeyAsDOMElement(elemRoot, pDoc);
	DOM_Output::dumpToMem(elemRoot, buff, len);
	if (pDoc != NULL)
		{
			pDoc->release();
			pDoc = NULL;
		}
	return E_SUCCESS;
}


#ifdef EXPORT_ASYM_PRIVATE_KEY
/** Stores the private key in \c buff as XML. The format is as follows (see
   also: XML Key Management Specification [8.2.1]):
        *
        * \verbatim
        <RSAKeyPair>
          <Modulus>
            the modulus of the Key as ds::CryptoBinary
          </Modulus>
          <Exponent>
            the exponent of the key as ds::CryptoBinary
          </Exponent>
         <P>
            first factor of n as ds::CryptoBinary
          </P>
         <Q>
            second factor of n as ds::CryptoBinary
         </Q>
         <DP>
            DP ds::CryptoBinary
         </DP>
         <DQ>
            DQ ds::CryptoBinary
         </DQ>
         <InverseQ>
            ds::CryptoBinary
         </InverseQ>
         <D>
            private exponent d as ds::CryptoBinary
         </D>
        <RSAKeyPair>\endverbatim
        *There is NO \\0 at the end.
        *@param buff byte array in which the public key should be stored
        *@param len on input holds the size of \c buff, on return it contains
   the number
        *           of bytes needed to store the public key
        *@retval E_UNKNOWN in case of an error
        *@retval E_SUCCESS otherwise
        *
        *@see setPrivateKeyAsXML()
        */
SINT32 CAASymCipher::getPrivateKeyAsXML(UINT8 *buff, UINT32 *len)
{
	if (m_pRSA == NULL || buff == NULL)
		return E_UNKNOWN;
	XERCES_CPP_NAMESPACE::DOMDocument *pDoc = createDOMDocument();
	DOMElement *elemRoot = NULL;
	getPrivateKeyAsDOMElement(elemRoot, pDoc);
	DOM_Output::dumpToMem(elemRoot, buff, len);
	if (pDoc != NULL)
		{
			pDoc->release();
			pDoc = NULL;
		}
	return E_SUCCESS;
}

SINT32 CAASymCipher::getPrivateKeyAsDOMElement(
  DOMElement *&elemRoot, XERCES_CPP_NAMESPACE::DOMDocument *docOwner)
{
	if (m_pRSA == NULL)
		return E_UNKNOWN;
	elemRoot = createDOMElement(docOwner, "RSAKeyPair");

	addKeyPart(elemRoot, docOwner, "Modulus", m_pRSA->n);
	addKeyPart(elemRoot, docOwner, "Exponent", m_pRSA->e);
	addKeyPart(elemRoot, docOwner, "P", m_pRSA->p);
	addKeyPart(elemRoot, docOwner, "Q", m_pRSA->q);
	addKeyPart(elemRoot, docOwner, "DP", m_pRSA->dmp1);
	addKeyPart(elemRoot, docOwner, "DQ", m_pRSA->dmq1);
	addKeyPart(elemRoot, docOwner, "InverseQ", m_pRSA->iqmp);
	addKeyPart(elemRoot, docOwner, "D", m_pRSA->d);

	return E_SUCCESS;
}
/** Sets the private key to the values stored in \c key.
        * The format must match the format XML described for
  *getPrivateKeyAsXML().
        *@param key byte array which holds the new public key
        *@param len on input,size of key byte array, on successful return number
  *of bytes 'consumed'
        *@retval E_UNKNOWN in case of an error, the cipher is the uninitialized
  *(no key is set)
        *@retval E_SUCCESS otherwise
        *@see getPrivateKeyAsXML
        */
SINT32 CAASymCipher::setPrivateKeyAsXML(const UINT8 *key, UINT32 len)
{
	if (key == NULL)
		return E_UNKNOWN;

	XERCES_CPP_NAMESPACE::DOMDocument *doc = parseDOMDocument(key, len);
	if (doc == NULL)
		{
			return E_UNKNOWN;
		}
	DOMElement *root = doc->getDocumentElement();
	if (root == NULL)
		{
			return E_UNKNOWN;
		}
	return setPrivateKeyAsDOMNode(root);
}

SINT32 CAASymCipher::setPrivateKeyAsDOMNode(DOMNode *node)
{
	DOMNode *root = node;
	while (root != NULL)
		{
			if (equals(root->getNodeName(), "RSAKeyPair"))
				{
					RSA *tmpRSA = RSA_new();
					DOMNode *child = root->getFirstChild();
					while (child != NULL)
						{
							if (equals(child->getNodeName(), "Modulus"))
								{
									getKeyPart(&tmpRSA->n, child);
								}
							else if (equals(child->getNodeName(), "Exponent"))
								{
									getKeyPart(&tmpRSA->e, child);
								}
							else if (equals(child->getNodeName(), "P"))
								{
									getKeyPart(&tmpRSA->p, child);
								}
							else if (equals(child->getNodeName(), "Q"))
								{
									getKeyPart(&tmpRSA->q, child);
								}
							else if (equals(child->getNodeName(), "DP"))
								{
									getKeyPart(&tmpRSA->dmp1, child);
								}
							else if (equals(child->getNodeName(), "DQ"))
								{
									getKeyPart(&tmpRSA->dmq1, child);
								}
							else if (equals(child->getNodeName(), "InverseQ"))
								{
									getKeyPart(&tmpRSA->iqmp, child);
								}
							else if (equals(child->getNodeName(), "D"))
								{
									getKeyPart(&tmpRSA->d, child);
								}
							child = child->getNextSibling();
						}
					if (tmpRSA->n != NULL && tmpRSA->e != NULL && tmpRSA->e != NULL &&
					    tmpRSA->p != NULL && tmpRSA->q != NULL && tmpRSA->d != NULL &&
					    tmpRSA->iqmp != NULL && tmpRSA->dmp1 != NULL &&
					    tmpRSA->dmq1 != NULL)
						{
							if (m_pRSA != NULL)
								RSA_free(m_pRSA);
							m_pRSA = tmpRSA;
							::setRSAFlags(m_pRSA);
							return E_SUCCESS;
						}
					RSA_free(tmpRSA);
					return E_UNKNOWN;
				}
			root = root->getNextSibling();
		}
	return E_UNKNOWN;
}
#endif // EXPORT_ASYM_PRIVATE_KEY

/** Sets the public key to the values stored in \c key.
        * The format must match the format XML described for
  *getPublicKeyAsXML().
        *@param key byte array which holds the new public key
        *@param len on input,size of key byte array, on successful return number
  *of bytes 'consumed'
        *@retval E_UNKNOWN in case of an error, the cipher is the uninitialized
  *(no key is set)
        *@retval E_SUCCESS otherwise
        *@see getPublicKeyAsXML
        */
SINT32 CAASymCipher::setPublicKeyAsXML(const UINT8 *key, UINT32 len)
{
	if (key == NULL)
		return E_UNKNOWN;

	XERCES_CPP_NAMESPACE::DOMDocument *doc = parseDOMDocument(key, len);
	if (doc == NULL)
		{
			return E_UNKNOWN;
		}
	DOMElement *root = doc->getDocumentElement();
	if (root == NULL)
		{
			return E_UNKNOWN;
		}
	return setPublicKeyAsDOMNode(root);
}

#endif // ONLY_LOCAL_PROXY

SINT32 CAASymCipher::getKeyPart(BIGNUM **part, DOMNode *node)
{
	if (*part != NULL)
		BN_free(*part);
	UINT8 *tmpStr = new UINT8[4096];
	UINT32 tmpStrLen = 4096;
	getDOMElementValue(node, tmpStr, &tmpStrLen);
	UINT8 decBuff[4096];
	UINT32 decLen = 4096;
	CABase64::decode(tmpStr, tmpStrLen, decBuff, &decLen);
	delete[] tmpStr;
	tmpStr = NULL;
	*part = BN_bin2bn(decBuff, decLen, NULL);
	return E_SUCCESS;
}

SINT32 CAASymCipher::setPublicKeyAsDOMNode(DOMNode *node)
{
	DOMNode *root = node;
	while (root != NULL)
		{
			if (equals(root->getNodeName(), "RSAKeyValue"))
				{
					RSA *tmpRSA = RSA_new();
					BIGNUM* n = NULL;
					BIGNUM* e = NULL;
					DOMNode *child = root->getFirstChild();
					while (child != NULL)
						{
							if (equals(child->getNodeName(), "Modulus"))
								{
									getKeyPart(&n, child);
								}
							else if (equals(child->getNodeName(), "Exponent"))
								{
									getKeyPart(&e, child);
								}
							child = child->getNextSibling();
						}
					if (n != NULL && e != NULL)
						{
							if (m_pRSA != NULL)
								RSA_free(m_pRSA);
							m_pRSA = tmpRSA;
							#if  OPENSSL_VERSION_NUMBER >= 0x1000204fL
								RSA_set0_key(m_pRSA,n,e, NULL );
							#else
								m_pRSA->n=n;
								m_pRSA->e=e;
							#endif
							::setRSAFlags(m_pRSA);
							return E_SUCCESS;
						}
					RSA_free(tmpRSA);
					return E_UNKNOWN;
				}
			root = root->getNextSibling();
		}
	return E_UNKNOWN;
}

#ifndef ONLY_LOCAL_PROXY
/** Sets the public key which is used for encryption to the contained in the
        *	provided certificate. The key has to be a RSA public key.
        *	@retval E_SUCCESS if successful
        *	@retval E_UNKNOWN otherwise (in this case the key leaves
  *untouched)
        */
SINT32 CAASymCipher::setPublicKey(const CACertificate *pCert)
{
	if (pCert == NULL)
		return E_UNKNOWN;
	EVP_PKEY *pubkey = X509_get_pubkey(pCert->m_pCert);
	int keyType = 0;
#if  OPENSSL_VERSION_NUMBER >= 0x1000204fL
	keyType = EVP_PKEY_id(pubkey);
#else
	keyType = pubkey->type;
#endif

	if (pubkey == NULL || (keyType != EVP_PKEY_RSA && keyType != EVP_PKEY_RSA2))
		return E_UNKNOWN;
	RSA *r = NULL;
#if  OPENSSL_VERSION_NUMBER >= 0x1000204fL
	r = EVP_PKEY_get1_RSA(pubkey);
#else
	r = pubkey->pkey.rsa;
#endif

	if (RSA_size(r) != 128)
		return E_UNKNOWN;
	if (m_pRSA != NULL)
		RSA_free(m_pRSA);
	m_pRSA = r;
	::setRSAFlags(m_pRSA);
	return E_SUCCESS;
}
#endif // ONLY_LOCAL_PROXY

SINT32 CAASymCipher::setPublicKey(const UINT8 *m, UINT32 mlen, const UINT8 *e, UINT32 elen)
{
	RSA *tmpRSA = RSA_new();
	UINT32 decLen = 4096;
	UINT8 decBuff[4096];
	CABase64::decode(m, mlen, decBuff, &decLen);
	BIGNUM* bnE = BN_bin2bn(decBuff, decLen, NULL);
	decLen = 4096;
	CABase64::decode(e, elen, decBuff, &decLen);
	BIGNUM* bnN = BN_bin2bn(decBuff, decLen, NULL);
	if (bnN != NULL && bnE != NULL)
		{
			if (m_pRSA != NULL)
				RSA_free(m_pRSA);
			m_pRSA = tmpRSA;
			#if  OPENSSL_VERSION_NUMBER >= 0x1000204fL
				RSA_set0_key(m_pRSA,bnN,bnE, NULL );
			#else
				m_pRSA->n=bnN;
				m_pRSA->e=bnE;
			#endif
			::setRSAFlags(m_pRSA);
			return E_SUCCESS;
		}
	RSA_free(tmpRSA);
	return E_UNKNOWN;
}

#ifdef INTEL_IPP_CRYPTO
IppStatus __STDCALL myIppBitSupplier(Ipp32u *pData, int nBits,
                                     void *pEbsParams)
{
	getRandom((UINT8 *)pData, (nBits + 7) / 8);
	return ippStsNoErr;
}
#endif

SINT32 CAASymCipher::testSpeed()
{
	const UINT32 runs = 10000;
	UINT8 *inBuff = new UINT8[128];
	UINT8 *outBuff = new UINT8[128];
	getRandom(inBuff, 128);
	inBuff[0] &= 0x7F;
#ifdef INTEL_IPP_CRYPTO
	int size = 0;
	ippsRSAGetSize(1024, 512, IppRSAprivate, &size);
	IppsRSAState *pCtx = (IppsRSAState *)new UINT8[size];
	IppStatus ret = ippsRSAInit(1024, 512, IppRSAprivate, pCtx);
	if (ret != ippStsNoErr)
		{
			printf("Error in RSA init!\n");
			return E_UNKNOWN;
		}
	ippsBigNumGetSize(1, &size);
	IppsBigNumState *pE = (IppsBigNumState *)(new UINT8[size]);
	ippsBigNumInit(1, pE);
	UINT32 pEValue[] = {0x010001};
	ret = ippsSet_BN(IppsBigNumPOS, 1, pEValue, pE);
	if (ret != ippStsNoErr)
		{
			printf("Error in setBN(e)!\n");
			return E_UNKNOWN;
		}
	ippsRSAGenerate(pE, 1024, 512, 1024, pCtx, myIppBitSupplier, NULL);
	if (ret != ippStsNoErr)
		{
			printf("Error in RSA generate key!\n");
			return E_UNKNOWN;
		}

	ippsBigNumGetSize(32, &size);
	IppsBigNumState *pY = (IppsBigNumState *)(new UINT8[size]);
	ippsBigNumInit(32, pY);

	IppsBigNumState *pX = (IppsBigNumState *)(new UINT8[size]);
	ippsBigNumInit(32, pX);
	UINT32 *pXValue = new UINT32[32];
	memcpy(pXValue, inBuff, 128);
	pXValue[0] &= 0x7FFFFFFF;
	ippsSet_BN(IppsBigNumPOS, 32, pXValue, pX);

#else
	CAASymCipher *pCipher = new CAASymCipher();
	pCipher->generateKeyPair(1024);
	UINT32 iLen = 128;
	pCipher->encryptOAEP(inBuff, 86, inBuff, &iLen);
#endif
	UINT64 start, end;
	getcurrentTimeMillis(start);
	for (UINT32 i = 0; i < runs; i++)
		{
#ifdef INTEL_IPP_CRYPTO
			IppStatus ret = ippsRSADecrypt(pX, pY, pCtx);
			if (ret != ippStsNoErr)
				{
					printf("Error in RSADEcrypt %i!\n", ret);
					return E_UNKNOWN;
				}
#else
			pCipher->decryptOAEP(inBuff, outBuff, &iLen);
//				pCipher->decrypt(inBuff,outBuff);
#endif
		}
	getcurrentTimeMillis(end);
	UINT32 d = diff64(end, start);
	printf("CAASymCiper::testSpeed() takes %u ms for %u decrypts!\n", d, runs);
	return E_SUCCESS;
}

#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX

SINT32 CAASymCipher::addKeyPart(DOMElement *elemRoot,
                                XERCES_CPP_NAMESPACE::DOMDocument *docOwner,
                                const char *partName, BIGNUM *part)
{
	DOMElement *node = createDOMElement(docOwner, partName);
	elemRoot->appendChild(node);
	UINT8 tmpBuff[256];
	UINT32 size = 256;
	BN_bn2bin(part, tmpBuff);
	CABase64::encode(tmpBuff, BN_num_bytes(part), tmpBuff, &size);
	tmpBuff[size] = 0;
	DOMText *tmpTextNode = createDOMText(docOwner, (const char *const)tmpBuff);
	node->appendChild(tmpTextNode);
	return E_SUCCESS;
}


SINT32 CAASymCipher::getPublicKeyAsDOMElement(DOMElement *&elemRoot, XERCES_CPP_NAMESPACE::DOMDocument *docOwner)
{
	if (m_pRSA == NULL)
		return E_UNKNOWN;
	elemRoot = createDOMElement(docOwner, "RSAKeyValue");
	BIGNUM* n = NULL;
	BIGNUM* e = NULL;
#if  OPENSSL_VERSION_NUMBER >= 0x1000204fL
	RSA_get0_key(m_pRSA,(const BIGNUM**) &n,(const BIGNUM**) &e, NULL );
#else
	n = m_pRSA->n;
	e = m_pRSA->e;
#endif
	addKeyPart(elemRoot, docOwner, "Modulus", n);
	addKeyPart(elemRoot, docOwner, "Exponent", e);

	return E_SUCCESS;
}


#endif


