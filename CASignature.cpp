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
		m_pDSA=NULL;
	}

CASignature::~CASignature()
	{
		if(m_pDSA!=NULL)
			DSA_free(m_pDSA);
	}


CASignature* CASignature::clone()
	{
		CASignature* tmpSig=new CASignature();
		if(m_pDSA!=NULL)
			{
				DSA* tmpDSA=DSA_clone(m_pDSA);
				tmpSig->m_pDSA=tmpDSA;
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

SINT32 CASignature::getSignKey(DOM_DocumentFragment& node,DOM_Document& doc)
	{
		CACertificate* pCert;
		getVerifyKey(&pCert);
		EVP_PKEY* pPKey=EVP_PKEY_new();
		EVP_PKEY_set1_DSA(pPKey,m_pDSA);		
		PKCS12* pPKCS12=PKCS12_create(NULL,NULL, pPKey,pCert->m_pCert,NULL,0,0,0,0,0);
		delete pCert;
		EVP_PKEY_free(pPKey);
		UINT8* buff=NULL;
		SINT32 len=i2d_PKCS12(pPKCS12,&buff);
		UINT32 outlen=2*len;
		UINT8* outbuff=new UINT8[outlen];
		CABase64::encode(buff,len,outbuff,&outlen);
		outbuff[outlen]=0;
		OPENSSL_free(buff);
		node=doc.createDocumentFragment();
		DOM_Element elem=doc.createElement("X509PKCS12");
		node.appendChild(elem);
		setDOMElementValue(elem,outbuff);
		return E_SUCCESS;
	}

SINT32 CASignature::setSignKey(const DOM_Node& n,UINT32 type,const char* passwd)
	{
		DOM_Node node=n; 
		switch(type)
			{
				case SIGKEY_PKCS12:
					while(node!=NULL)
						{
							if(node.getNodeName().equals("X509PKCS12"))
								{
									UINT32 strLen=4096;
									UINT8* tmpStr=new UINT8[strLen];
									if(getDOMElementValue(node,tmpStr,&strLen)!=E_SUCCESS)
										{
											delete[] tmpStr;
											return E_UNKNOWN;
										}
									UINT32 decLen=4096;
									UINT8* decBuff=new UINT8[decLen];
									CABase64::decode((UINT8*)tmpStr,strLen,decBuff,&decLen);
									delete [] tmpStr;
									SINT32 ret=setSignKey(decBuff,decLen,SIGKEY_PKCS12,passwd);
									delete[] decBuff;
									return ret;
								}
							node=node.getNextSibling();
						}
			}
		return E_UNKNOWN;
	}

SINT32 CASignature::setSignKey(const UINT8* buff,UINT32 len,UINT32 type,const char* passwd)
	{
		if(buff==NULL||len<1)
			return E_UNKNOWN;
		switch (type)
			{
				case SIGKEY_XML:
					return parseSignKeyXML(buff,len);

				case SIGKEY_PKCS12:
					#if OPENSSL_VERSION_NUMBER	> 0x009070AfL
						PKCS12* tmpPKCS12=d2i_PKCS12(NULL,(const UINT8**)&buff,len);	
					#else
						PKCS12* tmpPKCS12=d2i_PKCS12(NULL,(UINT8**)&buff,len);	
					#endif
					EVP_PKEY* key=NULL;
//					X509* cert=NULL;
					if(PKCS12_parse(tmpPKCS12,passwd,&key,NULL,NULL)!=1)
						return E_UNKNOWN;
	//				X509_free(cert);
					if(EVP_PKEY_type(key->type)!=EVP_PKEY_DSA)
						{
							EVP_PKEY_free(key);
							return E_UNKNOWN;
						}
					DSA* tmpDSA=DSA_clone(key->pkey.dsa);
					EVP_PKEY_free(key);
					if(DSA_sign_setup(tmpDSA,NULL,&tmpDSA->kinv,&tmpDSA->r)!=1)
						{
							DSA_free(tmpDSA);
							return E_UNKNOWN;
						}
					DSA_free(m_pDSA);
					m_pDSA=tmpDSA;
					return E_SUCCESS;
			}
		return E_UNKNOWN;
	}


//XML Decode...
SINT32 CASignature::parseSignKeyXML(const UINT8* buff,UINT32 len)
	{

		MemBufInputSource oInput(buff,len,"sigkey");
		DOMParser oParser;
		oParser.parse(oInput);
		DOM_Document doc=oParser.getDocument();
		DOM_Element rootKeyInfo=doc.getDocumentElement();
		if(!rootKeyInfo.getNodeName().equals("KeyInfo"))
			return E_UNKNOWN;
		DOM_Node elemKeyValue;
		if(getDOMChildByName(rootKeyInfo,(UINT8*)"KeyValue",elemKeyValue)!=E_SUCCESS)
			return E_UNKNOWN;
		if(getDOMChildByName(elemKeyValue,(UINT8*)"DSAKeyValue",elemKeyValue)!=E_SUCCESS)
			return E_UNKNOWN;
		UINT8 tbuff[4096];
		UINT32 tlen=4096;
		DSA* tmpDSA=DSA_new();
		DOM_Node child=elemKeyValue.getFirstChild();
		while(child!=NULL)
			{
				DOMString name=child.getNodeName();
				DOM_Node text=child.getFirstChild();
				if(!text.isNull())
					{
						char* tmpStr=text.getNodeValue().transcode();
						tlen=4096;
						CABase64::decode((UINT8*)tmpStr,strlen(tmpStr),tbuff,&tlen);
						delete tmpStr;
						if(name.equals("P"))
							{
								if(tmpDSA->p!=NULL)
									BN_free(tmpDSA->p);
								tmpDSA->p=BN_bin2bn(tbuff,tlen,NULL);
							}
						else if(name.equals("Q"))
							{
								if(tmpDSA->q!=NULL)
									BN_free(tmpDSA->q);
								tmpDSA->q=BN_bin2bn(tbuff,tlen,NULL);
							}
						else if(name.equals("G"))
							{
								if(tmpDSA->g!=NULL)
										BN_free(tmpDSA->g);
									tmpDSA->g=BN_bin2bn(tbuff,tlen,NULL);
							}
						else if(name.equals("X"))
							{
								if(tmpDSA->priv_key!=NULL)
									BN_free(tmpDSA->priv_key);
								tmpDSA->priv_key=BN_bin2bn(tbuff,tlen,NULL);

							}
						else if(name.equals("Y"))
							{
								if(tmpDSA->pub_key!=NULL)
									BN_free(tmpDSA->pub_key);
								tmpDSA->pub_key=BN_bin2bn(tbuff,tlen,NULL);
							}
					}
				child=child.getNextSibling();
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


SINT32 CASignature::sign(UINT8* in,UINT32 inlen,UINT8* sig,UINT32* siglen)
	{
		DSA_SIG* signature;
		if(	sign(in,inlen,&signature)!=E_SUCCESS||
				encodeRS(sig,siglen,signature)!=E_SUCCESS)
			return E_UNKNOWN;
		return E_SUCCESS;
	}

SINT32 CASignature::sign(UINT8* in,UINT32 inlen,DSA_SIG** pdsaSig)
	{
		UINT8 dgst[SHA_DIGEST_LENGTH];
		SHA1(in,inlen,dgst);
		*pdsaSig=DSA_do_sign(dgst,SHA_DIGEST_LENGTH,m_pDSA);
		if(*pdsaSig!=NULL)
		 return E_SUCCESS;
		return E_UNKNOWN;
	}

SINT32 CASignature::getSignatureSize()
	{
		return DSA_size(m_pDSA);
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
SINT32 CASignature::signXML(UINT8* in,UINT32 inlen,UINT8* out,UINT32* outlen,CACertStore* pIncludeCerts)
	{
		if(in==NULL||inlen<1||out==NULL||outlen==NULL)
			return E_UNKNOWN;
		
		MemBufInputSource oInput(in,inlen,"signxml");
		DOMParser oParser;
		oParser.parse(oInput);
		DOM_Document doc=oParser.getDocument();
		if(doc==NULL)
			return E_UNKNOWN;
		DOM_Element root=doc.getDocumentElement();
		if(signXML(root,pIncludeCerts)!=E_SUCCESS)
			return E_UNKNOWN;
		return DOM_Output::dumpToMem(root,out,outlen);
	}

/** Signs a DOM Node. The XML Signature is include in the XML Tree as a Child of the Node.
	* If ther is already a Signature is is removed first.
	* @param node Node which should be signed 
	* @param pIncludeCerts points to a CACertStore, which holds CACertificates, 
	*					which should be included in the XML Signature for easy verification;
	*					if null no certificates will be included
	*	@retval E_SUCCESS, if the Signature could be successful created
	* @retval E_UNKNOWN, otherwise
*/
SINT32 CASignature::signXML(DOM_Node& node,CACertStore* pIncludeCerts)
	{	
		//getting the Document an the Node to sign
		DOM_Document doc;
		DOM_Node elemRoot;
		if(node.getNodeType()==DOM_Node::DOCUMENT_NODE)
			{ //Hm, I am to stupid to do it better...
				DOM_Document* tmpDoc=static_cast<DOM_Document*>(&node);
				doc=*tmpDoc;
				elemRoot=doc.getDocumentElement();
			}
		else
			{
				doc=node.getOwnerDocument();
				elemRoot=node;
			}

		//check if there is already a Signature and if so remove it first...
		DOM_Node tmpSignature;
		if(getDOMChildByName(elemRoot,(UINT8*)"Signature",tmpSignature,false)==E_SUCCESS)
			elemRoot.removeChild(tmpSignature);

		//Calculating the Digest...
		UINT32 len=0;
		UINT8* canonicalBuff=DOM_Output::makeCanonical(elemRoot,&len);
		if(canonicalBuff==NULL)
			return E_UNKNOWN;

		UINT8 dgst[SHA_DIGEST_LENGTH];
		SHA1(canonicalBuff,len,dgst);
		delete[]canonicalBuff;

		UINT8 tmpBuff[1024];
		len=1024;
		if(CABase64::encode(dgst,SHA_DIGEST_LENGTH,tmpBuff,&len)!=E_SUCCESS)
			return E_UNKNOWN;
		tmpBuff[len]=0;


		//Creating the Sig-InfoBlock....
		DOM_Element elemSignedInfo=doc.createElement("SignedInfo");
		DOM_Element elemReference=doc.createElement("Reference");
		elemReference.setAttribute("URI","");
		DOM_Element elemDigestValue=doc.createElement("DigestValue");
		setDOMElementValue(elemDigestValue,tmpBuff);
		elemSignedInfo.appendChild(elemReference);
		elemReference.appendChild(elemDigestValue);

		// Signing the SignInfo block....

		canonicalBuff=DOM_Output::makeCanonical(elemSignedInfo,&len);
		if(canonicalBuff==NULL)
			return E_UNKNOWN;
		
		DSA_SIG* pdsaSig=NULL;
		
		SINT32 ret=sign(canonicalBuff,len,&pdsaSig);
		delete[] canonicalBuff;

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

		UINT sigSize=255;
		UINT8 sig[255];
		if(CABase64::encode(tmpBuff,len,sig,&sigSize)!=E_SUCCESS)
			return E_UNKNOWN;
		sig[sigSize]=0;

		//Makeing the whole Signature-Block....
		DOM_Element elemSignature=doc.createElement("Signature");
		DOM_Element elemSignatureValue=doc.createElement("SignatureValue");
		setDOMElementValue(elemSignatureValue,sig);
		elemSignature.appendChild(elemSignedInfo);
		elemSignature.appendChild(elemSignatureValue);
	
		if(pIncludeCerts!=NULL)
			{
				//Making KeyInfo-Block
				DOM_DocumentFragment tmpDocFrag;
				if(pIncludeCerts->encode(tmpDocFrag,doc)==E_SUCCESS&&!tmpDocFrag.isNull())
					{
						DOM_Element elemKeyInfo=doc.createElement("KeyInfo");
						elemKeyInfo.appendChild(doc.importNode(tmpDocFrag,true));
						tmpDocFrag=0;
						elemSignature.appendChild(elemKeyInfo);
					}
			}
		elemRoot.appendChild(elemSignature);
		return E_SUCCESS;
	}


SINT32 CASignature::getVerifyKey(CACertificate** ppCert)
	{
		//We need this DAS key as EVP key...
		EVP_PKEY* pPKey=EVP_PKEY_new();
		EVP_PKEY_set1_DSA(pPKey,m_pDSA);
		*ppCert=new CACertificate();
		(*ppCert)->m_pCert=X509_new();
		X509_set_version((*ppCert)->m_pCert,2);
		ASN1_TIME* pTime=ASN1_TIME_new();
		ASN1_TIME_set(pTime,time(NULL));
		X509_set_notBefore((*ppCert)->m_pCert,pTime);
		X509_set_notAfter((*ppCert)->m_pCert,pTime);
		X509_set_pubkey((*ppCert)->m_pCert,pPKey);
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
				DSA_free(m_pDSA);
				m_pDSA=NULL;
				return E_SUCCESS;
			}
		EVP_PKEY *key=X509_get_pubkey(pCert->m_pCert);
		if(EVP_PKEY_type(key->type)!=EVP_PKEY_DSA)
			{
				EVP_PKEY_free(key);
				return E_UNKNOWN;
			}
		DSA* tmpDSA=DSA_clone(key->pkey.dsa);
		EVP_PKEY_free(key);
		DSA_free(m_pDSA);
		m_pDSA=tmpDSA;
		return E_SUCCESS;
	}
	
/**
 * Parses the XML representation of a DSA public key
 */
SINT32 CASignature::setVerifyKey(const DOM_Element& xmlKey)
{
	UINT8 decodeBuffer[4096];
	UINT32 len = 4096, encodedLen = 0;
	DSA * tmpDSA = NULL;
	
	if(xmlKey==NULL)
	{
		DSA_free(m_pDSA);
		m_pDSA = NULL;
		return E_SUCCESS;
	}
	char * tmpStr = xmlKey.getTagName().transcode();
	if(strcmp(tmpStr, "JapPublicKey")!=0)
	{
		CAMsg::printMsg(LOG_DEBUG, 
				"CASignature::setVerifyKey(): no JapPublicKey!"
				"Tagname is %s", tmpStr);
		delete[] tmpStr;
		return E_UNKNOWN;
	}
	delete[] tmpStr;


	if( getDOMElementAttribute(xmlKey,"version",decodeBuffer,&len)!=E_SUCCESS || 
		strcmp((char*)decodeBuffer, "1.0")!=0 )
	{
		CAMsg::printMsg(LOG_DEBUG, 
				"CASignature::setVerifyKey(): JapPublicKey has unknown version %s. "
				"Version 1.0 expected!", tmpStr);
		return E_UNKNOWN;
	}
	
	DOM_Element elemDsaKey;
	if(getDOMChildByName(xmlKey, (UINT8*)"DSAKeyValue", elemDsaKey, false)
			!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG, 
					"CASignature::setVerifyKey(): DSAKeyValue not found!");
			return E_UNKNOWN;
		}
	
	tmpDSA=DSA_new();
	
	// parse "Y"
	DOM_Element elem;
	if(getDOMChildByName(elemDsaKey, (UINT8*)"Y", elem, false)
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
	if(getDOMChildByName(elemDsaKey, (UINT8*)"G", elem, false)
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
	if(getDOMChildByName(elemDsaKey, (UINT8*)"P", elem, false)
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
	if(getDOMChildByName(elemDsaKey, (UINT8*)"Q", elem, false)
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


SINT32 CASignature::verify(UINT8* in,UINT32 inlen,DSA_SIG* dsaSig)
	{
		if(m_pDSA==NULL||dsaSig==NULL||dsaSig->r==NULL||dsaSig->s==NULL)
			return E_UNKNOWN;
		UINT8 dgst[SHA_DIGEST_LENGTH];
		SHA1(in,inlen,dgst);
		if(DSA_do_verify(dgst,SHA_DIGEST_LENGTH,dsaSig,m_pDSA)==1)
		 return E_SUCCESS;
		return E_UNKNOWN;
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


SINT32 CASignature::verifyXML(const UINT8* const in,UINT32 inlen)
	{
		MemBufInputSource oInput(in,inlen,"sigverify");
		DOMParser oParser;
		oParser.parse(oInput);
		DOM_Document doc=oParser.getDocument();
		DOM_Element root=doc.getDocumentElement();
		return verifyXML(root,NULL);
	}


/** Verifies a XML Signature under node root.*/
SINT32 CASignature::verifyXML(DOM_Node& root,CACertStore* trustedCerts)
	{
		DOM_Element elemSignature;
		getDOMChildByName(root,(UINT8*)"Signature",elemSignature);
		if(elemSignature.isNull())
			return E_UNKNOWN;
		DOM_Element elemSigValue;
		getDOMChildByName(elemSignature,(UINT8*)"SignatureValue",elemSigValue);
		if(elemSigValue.isNull())
			return E_UNKNOWN;
		DOM_Element elemSigInfo;
		getDOMChildByName(elemSignature,(UINT8*)"SignedInfo",elemSigInfo);
		if(elemSigInfo.isNull())
			return E_UNKNOWN;
		DOM_Element elemReference;
		getDOMChildByName(elemSigInfo,(UINT8*)"Reference",elemReference);
		if(elemReference.isNull())
			return E_UNKNOWN;
		DOM_Element elemDigestValue;
		getDOMChildByName(elemReference,(UINT8*)"DigestValue",elemDigestValue);
		if(elemDigestValue.isNull())
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
		if(tmpSiglen!=40)
			return E_UNKNOWN;
		//extract r and s and make the ASN.1 sequenz
			//Making DER-Encoding of r and s.....
            // ASN.1 Notation:
            //  sequence
            //    {
            //          integer r
            //          integer s
            //    }
            //--> Der-Encoding
            // 0x30 //Sequence
            // 46 // len in bytes
            // 0x02 // integer
            // 21? // len in bytes of r
					  // 0x00  // fir a '0' to mark this value as positiv integer
            // ....   //value of r
            // 0x02 //integer
            // 21 //len of s
					  // 0x00  // first a '0' to mark this value as positiv integer
						// ... value of s
		/*UINT8 sig[48];
		sig[0]=0x30;
		sig[1]=46;
		sig[2]=0x02;
		sig[3]=21;
		sig[4]=0;
		memcpy(sig+5,tmpSig,20);
		sig[25]=0x02;
		sig[26]=21;
		sig[27]=0;
		memcpy(sig+28,tmpSig+20,20);
*/
		DSA_SIG* dsaSig=DSA_SIG_new();
		dsaSig->r=BN_bin2bn(tmpSig,20,dsaSig->r);
		dsaSig->s=BN_bin2bn(tmpSig+20,20,dsaSig->s);
		UINT8* out=new UINT8[5000];
		UINT32 outlen=5000;
		if(DOM_Output::makeCanonical(elemSigInfo,out,&outlen)!=E_SUCCESS||
				verify(out,outlen,dsaSig)!=E_SUCCESS)
			{
				DSA_SIG_free(dsaSig);
				delete[] out;
				return E_UNKNOWN;
			}
		DSA_SIG_free(dsaSig);
		DOM_Node tmpNode=root.removeChild(elemSignature);
		outlen=5000;
		DOM_Output::makeCanonical(root,out,&outlen);
		root.appendChild(tmpNode);
		UINT8 dgst1[SHA_DIGEST_LENGTH];
		SHA1(out,outlen,dgst1);
		delete[] out;
		for(int i=0;i<SHA_DIGEST_LENGTH;i++)
			{
				if(dgst1[i]!=dgst[i])
					return E_UNKNOWN;	
			}
		return E_SUCCESS;
	}

SINT32 CASignature::encodeRS(UINT8* out,UINT32* outLen,DSA_SIG* pdsaSig)
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

	
SINT32 CASignature::decodeRS(const UINT8* in, const UINT32 inLen, DSA_SIG* pDsaSig)
{
	ASSERT(pDsaSig!=NULL, "DSA_SIG is null");
	ASSERT(inLen>20, "Inbuffer is <=20 bytes");
	pDsaSig->r = BN_bin2bn(in, 20, NULL);
	pDsaSig->s = BN_bin2bn(in+20, inLen-20, NULL);
	return E_SUCCESS;
}
#endif //ONLY_LOCAL_PROXY
