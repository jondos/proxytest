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
#include "CACertificate.hpp"
#include "CABase64.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

UINT8 * CACertificate::m_spXmlElementName =0;

CACertificate::CACertificate()
	{
		m_pCert = NULL;
		m_pSKI = NULL;
		m_pAKI = NULL;
	}

CACertificate::CACertificate(X509* x)
	{
		m_pCert = x;
		if(m_pCert != NULL)
		{
			m_pSKI = (ASN1_OCTET_STRING*) X509_get_ext_d2i(m_pCert, NID_subject_key_identifier, NULL, NULL);
			m_pAKI = (AUTHORITY_KEYID*) X509_get_ext_d2i (m_pCert, NID_authority_key_identifier, NULL, NULL);
		}
	}

CACertificate* CACertificate::decode(const DOMNode* n,UINT32 type,const char* passwd)
	{
		const DOMNode* node=n;
		switch(type)
			{
				case CERT_PKCS12:
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
											return NULL;
										}
									UINT32 decLen=4096;
									UINT8* decBuff=new UINT8[decLen];
									CABase64::decode((UINT8*)tmpStr,strLen,decBuff,&decLen);
									delete[] tmpStr;
									tmpStr = NULL;
									CACertificate* cert=decode(decBuff,decLen,CERT_PKCS12,passwd);
									delete[] decBuff;
									decBuff = NULL;
									return cert;
								}
							node=node->getNextSibling();
						}
				break;
				case	CERT_X509CERTIFICATE:
					while(node!=NULL)
						{
							if(equals(node->getNodeName(),"X509Certificate"))
								{
									UINT32 strLen=4096;
									UINT8* tmpStr=new UINT8[strLen];
									if(getDOMElementValue(node,tmpStr,&strLen)!=E_SUCCESS)
										{
											delete[] tmpStr;
											tmpStr = NULL;
											return NULL;
										}
									UINT32 decLen=4096;
									UINT8* decBuff=new UINT8[decLen];
									CABase64::decode((UINT8*)tmpStr,strLen,decBuff,&decLen);
									delete[] tmpStr;
									tmpStr = NULL;
									CACertificate* cert=decode(decBuff,decLen,CERT_DER);
									delete[] decBuff;
									decBuff = NULL;
									return cert;
								}
							node=node->getNextSibling();
						}

			}
		return NULL;
	}

CACertificate* CACertificate::decode(const UINT8* buff,UINT32 bufflen,UINT32 type,const char* passwd)
	{
		if(buff==NULL)
			return NULL;
		X509* tmpCert=NULL;
		const UINT8* tmp;
		switch(type)
			{
				case CERT_DER:
					tmp=buff;
					#if OPENSSL_VERSION_NUMBER	> 0x009070CfL
						tmpCert=d2i_X509(NULL,&tmp,bufflen);
					#else
						tmpCert=d2i_X509(NULL,(UINT8**)&tmp,bufflen);
					#endif
				break;
				case CERT_PKCS12:
					PKCS12* tmpPKCS12;
					#if OPENSSL_VERSION_NUMBER	> 0x009070CfL
						tmpPKCS12=d2i_PKCS12(NULL,&buff,bufflen);
					#else
						tmpPKCS12=d2i_PKCS12(NULL,(UINT8**)&buff,bufflen);
					#endif
					if(PKCS12_parse(tmpPKCS12,passwd,NULL,&tmpCert,NULL)!=1)
						{
							PKCS12_free(tmpPKCS12);
							return NULL;
						}
					PKCS12_free(tmpPKCS12);
				break;
				case CERT_XML_X509CERTIFICATE:
					XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(buff,bufflen);
					if(doc == NULL)
					{
						return NULL;
					}
					DOMElement* root=doc->getDocumentElement();
					if(root==NULL||!equals(root->getNodeName(),"X509Certificate"))
					{
						return NULL;
					}
					UINT8* tmpBuff=new UINT8[bufflen];
					UINT32 tmpBuffSize=bufflen;
					getDOMElementValue(root,tmpBuff,&tmpBuffSize);
					CABase64::decode(tmpBuff,tmpBuffSize,tmpBuff,&tmpBuffSize);
					tmp=tmpBuff;
					#if OPENSSL_VERSION_NUMBER	> 0x009070CfL
						tmpCert=d2i_X509(NULL,&tmp,tmpBuffSize);
					#else
						tmpCert=d2i_X509(NULL,(UINT8**)&tmp,tmpBuffSize);
					#endif
					delete[] tmpBuff;
					tmpBuff = NULL;
					break;
			}
		if(tmpCert == NULL)
		{
			return NULL;
		}
		return new CACertificate(tmpCert);
	}

SINT32 CACertificate::encode(UINT8* buff,UINT32* bufflen,UINT32 type)
	{
		if(m_pCert==NULL||buff==NULL||bufflen==NULL)
			return E_UNKNOWN;
		int i=0;
		UINT8* tmp=buff;
		switch(type)
			{
				case CERT_DER:
					i=i2d_X509(m_pCert,&tmp);
					if(i==0)
						return E_UNKNOWN;
					*bufflen=i;
				break;
				case CERT_XML_X509CERTIFICATE:
				#define X509_CERTIFICATE_TAGNAME_LEN 17
					memcpy(buff,"<X509Certificate>",X509_CERTIFICATE_TAGNAME_LEN); //we start with '<X509Certificate>'
					tmp+=X509_CERTIFICATE_TAGNAME_LEN;
					i=i2d_X509(m_pCert,&tmp); //now we need DER
					if(i==0)
						return E_UNKNOWN;
					CABase64::encode(	buff+X509_CERTIFICATE_TAGNAME_LEN,i,
														buff+X509_CERTIFICATE_TAGNAME_LEN,bufflen); //now we have it converted to Base64
					memcpy(	buff+X509_CERTIFICATE_TAGNAME_LEN+*bufflen,
									"</X509Certificate>",X509_CERTIFICATE_TAGNAME_LEN+1); //we end it with '</X509Certificate>'
					*bufflen+=2*X509_CERTIFICATE_TAGNAME_LEN+1;
				break;
				default:
					return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

SINT32 CACertificate::encode(DOMElement* & elemRoot,XERCES_CPP_NAMESPACE::DOMDocument* doc)
	{
		elemRoot=createDOMElement(doc,"X509Certificate");
		UINT8 buff[2048]; //TODO: Very bad --> looks like easy buffer overflow... [donn't care at the moment...]
		UINT8* tmp=buff;
		int i=i2d_X509(m_pCert,&tmp); //now we need DER
		UINT32 bufflen=2048;
		CABase64::encode(buff,i,buff,&bufflen); //now we have it converted to Base64
		buff[bufflen]=0;
		setDOMElementValue(elemRoot,buff);
		return E_SUCCESS;
	}

/**
  * LERNGRUPPE
  * Accessor method for the subjectKeyIdentifier (SKI) extension stored in this certificate
  * @return r_ski The SKI as colon-free string
  * @return r_skiLen The length of r_ski
  * @retval E_SUCCESS upon successful retrieval
  * @retval E_UNKNOWN otherwise
  */
SINT32 CACertificate::getSubjectKeyIdentifier(UINT8* r_ski, UINT32 *r_skiLen)
{
    if (m_pSKI == NULL)
    {
#ifdef DEBUG
        CAMsg::printMsg( LOG_ERR, "Unable to get SKI from Certificate, trying to recover\n");
#endif
        setSubjectKeyIdentifier();
        if(m_pSKI == NULL)
        {
            CAMsg::printMsg( LOG_ERR, "Unable to retrieve 1SKI from Certificate\n");
            return E_UNKNOWN;
        }
#ifdef DEBUG
        else
        {
            CAMsg::printMsg( LOG_ERR, "Recovery SUCCESSFUL!\n");
        }
#endif
    }
    // Get the ASCII string format of the subject key identifier
    UINT8* cSki = (UINT8*)i2s_ASN1_OCTET_STRING( NULL, m_pSKI );
    if ( cSki==NULL )
    {
        CAMsg::printMsg( LOG_ERR, "Unable to convert SKI\n");
        return E_UNKNOWN;
    }
#ifdef DEBUG
    CAMsg::printMsg( LOG_ERR, "getSubjectKeyIdentifier: SKI is %s\n", cSki);
#endif
    removeColons(cSki, strlen((const char*)cSki), r_ski, r_skiLen);
	OPENSSL_free(cSki);
    return E_SUCCESS;
}

SINT32 CACertificate::getAuthorityKeyIdentifier(UINT8* r_aki, UINT32* r_akiLen)
{
	if(m_pAKI == NULL)
	{
		return E_UNKNOWN;
	}

	ASN1_OCTET_STRING* pKeyID = NULL;
	pKeyID = m_pAKI->keyid;
	if(pKeyID == NULL)
	{
		return E_UNKNOWN;
	}

	// Get the ASCII string format of the authority key identifier
	UINT8* cKeyID = (UINT8*)i2s_ASN1_OCTET_STRING(NULL, pKeyID);
	if (cKeyID == NULL)
	{
		return E_UNKNOWN;
	}
	removeColons(cKeyID, strlen((const char*)cKeyID), r_aki, r_akiLen);
	OPENSSL_free(cKeyID);
	return E_SUCCESS;
}

/**
  * LERNGRUPPE
  * Removes the colons from the string representation of the given SKI
  * @param a_cSkid The string from which the colons should be removed
  * @param a_cSkidLen The length of a_cSkid
  * @param r_ski
  * @param r_skiLen
  * @return r_ski The SKI as colon-free string
  * @return r_skiLen The length of r_ski
  * @retval E_SUCCESS upon successful removal
  * @retval E_UNKNOWN otherwise
  */
SINT32 CACertificate::removeColons(UINT8* a_cSkid, UINT32 a_cSkidLen, UINT8 *&r_ski, UINT32 *r_skiLen)
{
    UINT32 i = 0, j = 0;
    UINT32 tmp = (2*a_cSkidLen)/3 + 2;
    if(*r_skiLen < tmp)
    {
        CAMsg::printMsg( LOG_ERR, "Unable to copy SKI to target array, size must at least be %i but is only %i!\n", tmp, *r_skiLen);
        return E_UNKNOWN;
    }
    for(i = 0; i < a_cSkidLen; i++)
    {
        if(i % 3 == 2) {
            j++;
            continue;
        }
        r_ski[i-j] = a_cSkid[i];
    }
    r_ski[i-j] = 0;
    *r_skiLen = i-j;
    return E_SUCCESS;
}

/**
  * LERNGRUPPE
  * Sets the subjectKeyIdentifier extension for this certificate to the hash of the public key
  * @retval E_SUCCESS upon successful removal
  * @retval E_UNKNOWN otherwise
  */
SINT32 CACertificate::setSubjectKeyIdentifier()
{
    UINT32 len = 0;
    UINT8 sha_hash[SHA_DIGEST_LENGTH];
    X509_pubkey_digest(m_pCert, EVP_sha1(), sha_hash, &len);
    return setSubjectKeyIdentifier( sha_hash, len );
}

/**
  * LERNGRUPPE
  * Sets the subjectKeyIdentifier extension for this certificate to the given value
  * @param a_value The value which should be set as SKI
  * @param a_valueLen The length of a_value
  * @retval E_SUCCESS upon successful removal
  * @retval E_UNKNOWN otherwise
  */
SINT32 CACertificate::setSubjectKeyIdentifier( UINT8* a_value, UINT32 a_valueLen )
{
    SINT32 ret = E_UNKNOWN;
    ASN1_OCTET_STRING* skid = NULL;

    skid = ASN1_OCTET_STRING_new();
    if(NULL == skid)   goto end;

    ASN1_OCTET_STRING_set(skid, a_value, a_valueLen);
    if( X509_add1_ext_i2d(m_pCert, NID_subject_key_identifier, skid, false, X509V3_ADD_REPLACE) == 1)
    {
    	m_pSKI = skid;
    	ret = E_SUCCESS;
    }

end:
    //ASN1_OCTET_STRING_free(skid);
    return ret;
}

SINT32 CACertificate::getRawSubjectKeyIdentifier(UINT8* r_ski, UINT32* r_skiLen)
{
	if (m_pSKI == NULL)
	{
		setSubjectKeyIdentifier();
		if(m_pSKI == NULL)
		{
			CAMsg::printMsg( LOG_ERR, "Unable to retrieve raw SKI from Certificate\n");
			return E_UNKNOWN;
        }

	}
	if(*r_skiLen < (UINT32) m_pSKI->length)
	{
		CAMsg::printMsg( LOG_ERR, "Unable to copy SKI to target array, size must at least be %i but is only %i!\n", m_pSKI->length, r_skiLen );
		return E_UNKNOWN;
	}
	*r_skiLen = m_pSKI->length;
	for(SINT32 i=0; i<m_pSKI->length; i++)
	{
		r_ski[i] = m_pSKI->data[i];
	}
	return E_SUCCESS;
}

SINT32 CACertificate::verify(const CACertificate* a_cert)
{
	if(a_cert == NULL || a_cert->m_pCert == NULL || m_pCert == NULL)
	{
		return E_UNKNOWN;
	}
	//check validity
	if(!isValid())
	{
		CAMsg::printMsg(LOG_ERR, "Verification Error: Certificate is not valid!\n");
		return E_UNKNOWN;
	}
	//namechaining...
	if(X509_NAME_cmp(X509_get_issuer_name(m_pCert), X509_get_subject_name(a_cert->m_pCert)) != 0)
	{
		CAMsg::printMsg(LOG_ERR, "Verification Error: Names do not match!\n");
		return E_UNKNOWN;
	}
	//keychaining... (only if available)
	if(m_pAKI != NULL && a_cert->m_pSKI != NULL)
	{
		if(ASN1_OCTET_STRING_cmp(m_pAKI->keyid, a_cert->m_pSKI) != 0)
		{
			CAMsg::printMsg(LOG_ERR, "Verification Error: Key Identifiers do not match!\n");
			return E_UNKNOWN;
		}
	}
	//get public key
	EVP_PKEY* pubKey = X509_get_pubkey(a_cert->m_pCert);
	if(pubKey == NULL)
	{
		CAMsg::printMsg(LOG_ERR, "Verification Error: Public Key is NULL!\n");
		return E_UNKNOWN;
	}
	//check if public key and signature algorithm match -> does not work because of openssl bug!
	//SINT32 sigType = X509_get_signature_type(m_pCert);
	/*SINT32 sigType = OBJ_obj2nid((m_pCert)->sig_alg->algorithm);
	SINT32 keyType = EVP_PKEY_type(pubKey->type);
	CAMsg::printMsg(LOG_DEBUG, "sigType is %d and keyType is %d\n", sigType, keyType);
	if((sigType != (NID_dsaWithSHA1 || NID_sha1WithRSAEncryption ||NID_ecdsa_with_SHA1))
			&& keyType != EVP_PKEY_DSA) ||
	   (sigType == NID_sha1WithRSAEncryption && keyType != EVP_PKEY_RSA) ||
	   (sigType == NID_ecdsa_with_SHA1 && keyType != EVP_PKEY_EC))
	{
		CAMsg::printMsg(LOG_ERR, "Verification Error: Signature Algorithm does not match!\n");
		//return E_UNKNOWN;
	}*/
	if(X509_verify(m_pCert, pubKey) == 1)
	{
		CAMsg::printMsg(LOG_DEBUG, "Successfully verified certificate.\n");
		return E_SUCCESS;
	}
	CAMsg::printMsg(LOG_ERR, "Verification Error: Signature is not correct!\n");
	return E_UNKNOWN;
}

bool CACertificate::isValid()
{
	if(X509_cmp_current_time(X509_get_notBefore(m_pCert)) == -1
			&& X509_cmp_current_time(X509_get_notAfter(m_pCert)) == 1)
	{
		return true;
	}
	//check if certificate is valid within grace period of one month
	time_t now = time(NULL); 		//get current time;
	tm* time = new tm;
	time = gmtime_r(&now, time);	//convert time to modifiable format
	if(time->tm_mon < 2)			//go back two month in time
	{
		time->tm_mon = time->tm_mon+10;
		time->tm_year = time->tm_year-1;
	}
	else
	{
		time->tm_mon = time->tm_mon-2;
	}
	time_t ttiq  = mktime(time);  	//convert time back to time_t and check again
	delete time;
	time = NULL;
	if(X509_cmp_time(X509_get_notBefore(m_pCert), &ttiq) == -1
			&& X509_cmp_time(X509_get_notAfter(m_pCert), &ttiq) == 1)
	{
		CAMsg::printMsg(LOG_WARNING, "Certificate is only valid within grace period of two month!\n");
		return true;
	}
	return false;
}

#endif //ONLY_LOCAL_PROXY

