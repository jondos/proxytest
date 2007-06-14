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

CACertStore::CACertStore()
	{
		m_pCertList=NULL;
		m_cCerts=0;
	}

CACertStore::~CACertStore()
	{
		LP_CERTSTORE_ENTRY tmp;
		while(m_pCertList!=NULL)
			{
				delete m_pCertList->pCert;
				tmp=m_pCertList;
				m_pCertList=m_pCertList->next;
				delete tmp;
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
	*	@param docFrag on ouput holds the created DOM_DocumentFragment
	* @param doc owner document of the new DOM_DocumentFragment
	* @retval E_SUCCESS if successful
	* @retval E_UNKNOWN otherwise
	*/
SINT32 CACertStore::encode(DOM_DocumentFragment& docFrag,DOM_Document& doc)
	{
		docFrag=doc.createDocumentFragment();
		DOM_Element elemX509Data=doc.createElement("X509Data");
		docFrag.appendChild(elemX509Data);
		LP_CERTSTORE_ENTRY tmp;
		tmp=m_pCertList;
		while(tmp!=NULL)
			{
				DOM_DocumentFragment tmpFrag;
				tmp->pCert->encode(tmpFrag,doc);
				elemX509Data.appendChild(tmpFrag);
				tmpFrag=0;
				tmp=tmp->next;
			}
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY
