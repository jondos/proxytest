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
#include "CACertStore.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"

CACertStore::CACertStore()
	{
		m_pCertList=NULL;
		m_cCerts=0;
		m_pCurrent=NULL;
	}

CACertStore::~CACertStore()
	{
		LP_CERTSTORE_ENTRY tmp;
		while(m_pCertList!=NULL)
			{
				delete m_pCertList->pCert;
				m_pCertList->pCert = NULL;
				tmp=m_pCertList;
				m_pCertList=m_pCertList->next;
				delete tmp;
				tmp = NULL;
			}
	}

/** Adds a COPY of a given certifcate to this CertStore.
	* @param cert Certifcate of which a copy is added to this cert store
	* @retval E_SUCCESS if successfull
	* @retval E_UNKNOWN if not (for instance if cert==NULL)
	*/
SINT32 CACertStore::add(CACertificate* cert)
	{
		if(cert==NULL)
		{
			return E_UNKNOWN;
		}
		LP_CERTSTORE_ENTRY newEntry=new CERTSTORE_ENTRY;
		newEntry->pCert=cert->clone();
		newEntry->next=m_pCertList;
		m_pCertList=newEntry;
		m_cCerts++;
		return E_SUCCESS;
	}

CACertificate* CACertStore::getFirst()
{
	m_pCurrent = m_pCertList;
	return m_pCurrent->pCert;
}

CACertificate* CACertStore::getNext()
{
	if(m_pCurrent != NULL)
	{
		m_pCurrent = m_pCurrent->next;
		if(m_pCurrent != NULL && m_pCurrent != m_pCertList)
		{
			return m_pCurrent->pCert;
		}
	}
	return NULL;
}

/**
 * This function parses the certificates from a <Mix>-node and
 * tries to build a certPath to the trusted root certificates
 * loaded from the config file. The certificates are parsed
 * from any <Signature>-node that is a direct child of <Mix>
 * (MultiSignature compatible).
 * The function will return a certificate in the following cases:
 * - The certificate is signed by a root CA and there is no other
 *   certificate in the <Signature>-element.
 * - The certificate is signed by another ceritificate of the same
 * 	 <Signature>-element which itself was issued by a root CA.
 *
 * @param mixNode - a <Mix>-Node containing one or more signatures
 * @return the first end certificate that has a certPath to a
 * 		   trusted root certificate or NULL if no cert was found
 * 		   (or something went wrong)
 */
CACertificate* CACertStore::verifyMixCert(DOMNode* mixNode)
{
	UINT32 signatureElementsCount = MAX_SIGNATURE_ELEMENTS;
	DOMNode* signatureElements[MAX_SIGNATURE_ELEMENTS];
	DOMNode* x509Data;
	CACertStore* certPath;
	CACertificate* trustedCert;
	CACertificate* cert;
	CACertificate* mixCert;

	//try to decode the certificates from the Signature elements
	if(mixNode == NULL || m_pCertList == NULL)
	{
		CAMsg::printMsg(LOG_DEBUG , "Error initializing verification.\n");
		return NULL;
	}
	getSignatureElements((DOMElement*)mixNode, signatureElements, &signatureElementsCount);
	if(signatureElementsCount < 1)
	{
		CAMsg::printMsg(LOG_DEBUG , "Error no Signature-Node found!\n");
		return NULL;
	}
	//try to find a valid cert in one of the signature Elements
	for(UINT32 i=0; i<signatureElementsCount; i++)
	{
		getDOMChildByName(signatureElements[i], "X509Data", x509Data, true);
		if(x509Data == NULL)
		{
			CAMsg::printMsg(LOG_DEBUG , "Error X509Data-Node is NULL!\n");
			continue;
		}
		certPath = CACertStore::decode(x509Data, XML_X509DATA);
		if(certPath == NULL)
		{
			continue;
		}

		//now try to find a cert that was signed by a trusted CA
		trustedCert = getFirst();

		while(trustedCert != NULL)
		{
			cert = certPath->getFirst();
			while(cert != NULL)
			{
				if(cert->verify(trustedCert) == E_SUCCESS)
				{
					break;
				}
				cert = certPath->getNext();
			}
			if(cert != NULL)
			{
				break;
			}
			trustedCert = getNext();
		}
		if(trustedCert != NULL && cert != NULL)
		{
			//we found a verified cert
			if(certPath->m_cCerts > 1)
			{
				//try to build a longer certPath
				mixCert = certPath->getFirst();
				while(mixCert != NULL)
				{
					if(mixCert->verify(cert) == E_SUCCESS)
					{
						break;
					}
					mixCert = certPath->getNext();
				}
				if(mixCert != NULL)
				{
					return mixCert;
				}
			}
			else //tricky because there might be a longer certPath in another Signature Element
			{
				return cert;
			}
		}
	}
	return NULL;
}

SINT32 CACertStore::encode(UINT8* buff,UINT32* bufflen,UINT32 type)
	{
		switch (type)
			{
				case XML_X509DATA:
					memcpy(buff,"<X509Data>",10);
					UINT32 len=10;
					LP_CERTSTORE_ENTRY tmp;
					tmp=m_pCertList;
					UINT32 space=*bufflen-10;
					while(tmp!=NULL)
						{
							*bufflen=space;
							tmp->pCert->encode(buff+len,bufflen,CERT_XML_X509CERTIFICATE);
							len+=*bufflen;
							space-=*bufflen;
							tmp=tmp->next;
						}
					memcpy(buff+len,"</X509Data>",11);
					len+=11;
					*bufflen=len;
				break;
			}
		return E_SUCCESS;
	}

/**Creates a XML DocumentFragment which represenst all the Certifcates in this 
	* CertStore
	*
	*	@param docFrag on ouput holds the created DOMElement
	* @param doc owner document of the new DOM_DocumentFragment
	* @retval E_SUCCESS if successful
	* @retval E_UNKNOWN otherwise
	*/
SINT32 CACertStore::encode(DOMElement* & elemRoot,XERCES_CPP_NAMESPACE::DOMDocument* doc)
	{
		elemRoot=createDOMElement(doc,"X509Data");
		LP_CERTSTORE_ENTRY tmp;
		tmp=m_pCertList;
		while(tmp!=NULL)
			{
				DOMElement* tmpElem=NULL;
				tmp->pCert->encode(tmpElem,doc);
				elemRoot->appendChild(tmpElem);
				tmp=tmp->next;
			}
		return E_SUCCESS;
	}

CACertStore* CACertStore::decode(const DOMNode* node, UINT32 type)
{
	switch(type)
	{
		case XML_X509DATA:
			CACertStore* store = new CACertStore();
			DOMNodeList* certs = getElementsByTagName((DOMElement*)node, "X509Certificate");

			for(UINT32 i=0; i<certs->getLength(); i++)
			{
				CACertificate* cert = CACertificate::decode(certs->item(i), CERT_X509CERTIFICATE);
				if(cert != NULL)
				{
					store->add(cert);
				}
			}
			return store;
	}
	return NULL;
}
#endif //ONLY_LOCAL_PROXY
