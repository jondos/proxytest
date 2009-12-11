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
#ifndef CAABSTRACTXMLENCODABLE_HPP
#define CAABSTRACTXMLENCODABLE_HPP

#ifndef ONLY_LOCAL_PROXY

#include "StdAfx.h"


/**
Abstract base class for classes which can be converted to an XML structure.
This corresponds to anon.util.IXMLEncodable in the Java implementation
@author Bastian Voigt
*/
class CAAbstractXMLEncodable{
public:
	CAAbstractXMLEncodable()
		{
		}

	/** pure virtual destructor. Define real destructor in your derived class */
	virtual ~CAAbstractXMLEncodable() 
		{
		}
	
	/**
	 * Creates the XML structure inside an existing DOM_Document, but does not
	 * append it to any node.
	 *
	 * @param a_doc an existing DOM_Document
	 * @param elemRoot on return contains the root element of the created XML structure.
	 * Note that the element is not appended to any node in the document
	 */
	virtual SINT32 toXmlElement(XERCES_CPP_NAMESPACE::DOMDocument* a_pDoc, DOMElement* & pElemRoot)=0;
	
	/** 
	 * returns a pointer to the tagname of this XML structure's top level element.
	 * The buffer is allocated dynamically, the caller must delete[] it !!
	 * This is commented out because virtual static functions are not allowed,
	 * but should be implemented by subclasses nevertheless..
	 */
	// virtual static UINT8 * getXMLElementName()=0;
	
	/**
	 * Creates a new XML document, then calls toXmlElement and appends the element
	 * as DocumentElement.
	 */
	SINT32 toXmlDocument(XERCES_CPP_NAMESPACE::DOMDocument* & pDoc);
	
	/**
	 * Converts the XML structure to a null-terminated C-String representation.
	 * @param size on return contains the size of the allocated buffer
	 * @return a newly allocated buffer which must be delete[] by the caller
	 */
	UINT8 * toXmlString(UINT32* pSize);
};

#endif //ONLY_LOCAL_PROXY
#endif
