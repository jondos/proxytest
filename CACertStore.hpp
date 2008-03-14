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
#ifndef __CA_CERTSTORE__
#define __CA_CERTSTORE__
#ifndef ONLY_LOCAL_PROXY
#include "CACertificate.hpp"
#define XML_X509DATA 2
struct __t_certstore_list
	{
		CACertificate* pCert;
		struct __t_certstore_list* next;
	};
typedef struct __t_certstore_list CERTSTORE_ENTRY;
typedef CERTSTORE_ENTRY* LP_CERTSTORE_ENTRY;

class CACertStore
	{
		public:
			CACertStore();
			~CACertStore();
			SINT32 add(CACertificate* cert);
			CACertificate* getFirst();
			CACertificate* getNext();
			UINT32 getNumber(){return m_cCerts;}
			static CACertStore* decode(UINT8* buff,UINT32 bufflen,UINT32 type);
			SINT32 encode(UINT8* buff,UINT32* bufflen,UINT32 type);
			SINT32 encode(DOMElement* & elemnRoot,XERCES_CPP_NAMESPACE::DOMDocument* doc);
		private:
			LP_CERTSTORE_ENTRY m_pCertList;
			UINT32 m_cCerts;
	};
#endif
#endif //ONLY_LOCAL_PROXY
