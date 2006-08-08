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

UINT8 * CACertificate::m_spXmlElementName =0;

CACertificate::CACertificate()
	{
		m_pCert=NULL;
	}

CACertificate* CACertificate::decode(const DOM_Node &n,UINT32 type,const char* passwd)
	{
		DOM_Node node=n;
		switch(type)
			{
				case CERT_PKCS12:					
					while(node!=NULL)
						{
							if(node.getNodeName().equals("X509PKCS12"))
								{
									UINT32 strLen=4096;
									UINT8* tmpStr=new UINT8[strLen];
									if(getDOMElementValue(node,tmpStr,&strLen)!=E_SUCCESS)
										{
											delete tmpStr;
											return NULL;
										}
									UINT32 decLen=4096;
									UINT8* decBuff=new UINT8[decLen];
									CABase64::decode((UINT8*)tmpStr,strLen,decBuff,&decLen);
									delete tmpStr;
									CACertificate* cert=decode(decBuff,decLen,CERT_PKCS12,passwd);
									delete[] decBuff;
									return cert;
								}
							node=node.getNextSibling();
						}
				break;
				case	CERT_X509CERTIFICATE:
					while(node!=NULL)
						{
							if(node.getNodeName().equals("X509Certificate"))
								{
									UINT32 strLen=4096;
									UINT8* tmpStr=new UINT8[strLen];
									if(getDOMElementValue(node,tmpStr,&strLen)!=E_SUCCESS)
										{
											delete tmpStr;
											return NULL;
										}
									UINT32 decLen=4096;
									UINT8* decBuff=new UINT8[decLen];
									CABase64::decode((UINT8*)tmpStr,strLen,decBuff,&decLen);
									delete tmpStr;
									CACertificate* cert=decode(decBuff,decLen,CERT_DER);
									delete[] decBuff;
									return cert;
								}
							node=node.getNextSibling();
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
		PKCS12* tmpPKCS12;
		switch(type)
			{
				case CERT_DER:
					tmp=buff;
					#if OPENSSL_VERSION_NUMBER	> 0x009070AfL
						tmpCert=d2i_X509(NULL,&tmp,bufflen);
					#else
						tmpCert=d2i_X509(NULL,(UINT8**)&tmp,bufflen);
					#endif
				break;
				case CERT_PKCS12:
					#if OPENSSL_VERSION_NUMBER	> 0x009070AfL
						tmpPKCS12=d2i_PKCS12(NULL,&buff,bufflen);	
					#else
						tmpPKCS12=d2i_PKCS12(NULL,(UINT8**)&buff,bufflen);	
					#endif
					if(PKCS12_parse(tmpPKCS12,passwd,NULL,&tmpCert,NULL)!=1)
						return NULL;
				break;
				case CERT_XML_X509CERTIFICATE:
					MemBufInputSource oInput(buff,bufflen,"certxml");
					DOMParser oParser;
					oParser.parse(oInput);
					DOM_Document doc=oParser.getDocument();
					DOM_Element root=doc.getDocumentElement();
					if(root.isNull()||!root.getNodeName().equals("X509Certificate"))
						return NULL;
					UINT8* tmpBuff=new UINT8[bufflen];
					UINT32 tmpBuffSize=bufflen;
					getDOMElementValue(root,tmpBuff,&tmpBuffSize);
					CABase64::decode(tmpBuff,tmpBuffSize,tmpBuff,&tmpBuffSize);
					tmp=tmpBuff;
					#if OPENSSL_VERSION_NUMBER	> 0x009070AfL
						tmpCert=d2i_X509(NULL,&tmp,tmpBuffSize);
					#else
						tmpCert=d2i_X509(NULL,(UINT8**)&tmp,tmpBuffSize);
					#endif
					delete[] tmpBuff;
				break;
			}
		if(tmpCert==NULL)
			return NULL;
		CACertificate* cert=new CACertificate;
		cert->m_pCert=tmpCert;
		return cert;
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

SINT32 CACertificate::encode(DOM_DocumentFragment& docFrag,DOM_Document& doc)
	{
		docFrag=doc.createDocumentFragment();
		DOM_Element elemCert=doc.createElement("X509Certificate");
		docFrag.appendChild(elemCert);
		UINT8 buff[2048]; //TODO: Very bad --> looks like easy buffer overflow... [donn't care at the moment...]
		UINT8* tmp=buff;
		int i=i2d_X509(m_pCert,&tmp); //now we need DER
		UINT32 bufflen=2048;
		CABase64::encode(buff,i,buff,&bufflen); //now we have it converted to Base64
		buff[bufflen]=0;
		setDOMElementValue(elemCert,buff);
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
