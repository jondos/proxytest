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
#include "CAAbstractXMLEncodable.hpp"
#include "xml/DOM_Output.hpp"

SINT32 CAAbstractXMLEncodable::toXmlDocument(XERCES_CPP_NAMESPACE::DOMDocument* & pDoc)
	{
		DOMElement* pElemRoot=NULL;
		pDoc = createDOMDocument();
		toXmlElement(pDoc, pElemRoot);
		pDoc->appendChild(pElemRoot);
		return E_SUCCESS;
	}

UINT8* CAAbstractXMLEncodable::toXmlString(UINT32* pSize)
	{
		XERCES_CPP_NAMESPACE::DOMDocument* pDoc=NULL;
		toXmlDocument(pDoc);
		UINT8* tmp = DOM_Output::dumpToMem(pDoc, pSize);
		// put null at the end...
		UINT8* tmp2 = new UINT8[*pSize+1];
		memcpy(tmp2, tmp, *pSize);
		tmp2[*pSize]=0;
		delete[] tmp;
		tmp = NULL;
		if (pDoc != NULL)
		{
			pDoc->release();
			pDoc = NULL;
		}
		//delete pDoc;
		return tmp2;
	}
#endif //ONLY_LOCAL_PROXY
