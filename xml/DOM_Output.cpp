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
#include "../StdAfx.h"
#include "DOM_Output.hpp"


static const XMLCh  gEndElement[] = { chOpenAngle, chForwardSlash, chNull };


const XMLCh  DOM_Output::m_UTF8[6] =
	{
    chLatin_U, chLatin_T,chLatin_F, chDash,chDigit_8, chNull
	};	

const XMLCh  DOM_Output::m_1_0[4] =
	{
    chDigit_1,chPeriod,chDigit_0, chNull
	};	

const XMLCh  DOM_Output::m_XML[41] =
{
    chOpenAngle,chQuestion,chLatin_x,chLatin_m,chLatin_l,chSpace, //<?xml 
		chLatin_v,chLatin_e,chLatin_r,chLatin_s,chLatin_i,chLatin_o,chLatin_n,chEqual, //version=
		chDoubleQuote,chDigit_1,chPeriod,chDigit_0,chDoubleQuote,chSpace,	 //"1.0" 
		chLatin_e,chLatin_n,chLatin_c,chLatin_o,chLatin_d,chLatin_i,chLatin_n,chLatin_g,chEqual, //encoding=	
		chDoubleQuote,chLatin_U, chLatin_T,chLatin_F, chDash,chDigit_8, chDoubleQuote, //"UTF-8"
		chQuestion,chCloseAngle,chCR,chLF,chNull
};	


XMLFormatter& operator<< (XMLFormatter& strm, const DOMString& s)
	{
    unsigned int lent = s.length();

		if (lent <= 0)
			return strm;

    XMLCh*  buf = new XMLCh[lent + 1];
    XMLString::copyNString(buf, s.rawBuffer(), lent);
    buf[lent] = 0;
    strm << buf;
    delete [] buf;
    return strm;
	}

/** Dumps a Node of an XML Document.
	* @param toWrite Node which will be dumped
	* @param bCanonical if true the dump is done in a 'canonical' way, 
	*											e.g. white spaces are eliminated etc.
	* @retval E_SUCCESS if successful
	* @retval E_UNKNOWN otherwise
*/
SINT32 DOM_Output::dumpNode(DOM_Node& toWrite,bool bCanonical)
	{
    if(toWrite==0)
			return E_UNKNOWN;
		// Get the name and value out for convenience
    DOMString   nodeName = toWrite.getNodeName();

    switch (toWrite.getNodeType())
			{
        case DOM_Node::TEXT_NODE:
					{
						DOMString   nodeValue = toWrite.getNodeValue();
						unsigned long lent = nodeValue.length();
            if(!bCanonical)
							{
								m_pFormatter->formatBuf(nodeValue.rawBuffer(),
                                  lent, XMLFormatter::CharEscapes);
							}
						else //strip whitespaces...
							{
								XMLCh* text=new XMLCh[lent+1];
								memcpy(text,nodeValue.rawBuffer(),lent*sizeof(XMLCh));
								text[lent]=chNull;
								XMLString::trim(text);
								lent=XMLString::stringLen(text);
								if(lent>0)
									{
										m_pFormatter->formatBuf(text,
																			lent, XMLFormatter::CharEscapes);
										
									}
								delete[] text;
							}
            break;
        }


 /*       case DOM_Node::PROCESSING_INSTRUCTION_NODE :
        {
            *gFormatter << XMLFormatter::NoEscapes << gStartPI  << nodeName;
            if (lent > 0)
            {
                *gFormatter << chSpace << nodeValue;
            }
            *gFormatter << XMLFormatter::NoEscapes << gEndPI;
            break;
        }
*/

       case DOM_Node::DOCUMENT_NODE :
					*m_pFormatter<<XMLFormatter::NoEscapes<<m_XML;
       case DOM_Node::DOCUMENT_FRAGMENT_NODE :
        {

            DOM_Node child = toWrite.getFirstChild();
            while( child != 0)
            {
                dumpNode(child,bCanonical);
                // add linefeed in requested output encoding
                if(!bCanonical)
									*m_pFormatter << chLF;
                child = child.getNextSibling();
            }
            break;
        }

  
        case DOM_Node::ELEMENT_NODE :
        {
            // The name has to be representable without any escapes
            *m_pFormatter  << XMLFormatter::NoEscapes
                         << chOpenAngle << nodeName;

            // Output the element start tag.

            // Output any attributes on this element in lexicograhpical order
            DOM_NamedNodeMap attributes = toWrite.getAttributes();
            int attrCount = attributes.getLength();
						char**attr_names=NULL;
						int* sort_indices=NULL;
						if(attrCount>0)
							{
								attr_names=new char*[attrCount];
								sort_indices=new int[attrCount];
								for(int i=0;i<attrCount;i++)
									{
										DOM_Node  attribute = attributes.item(i);
										attr_names[i]=attribute.getNodeName().transcode();
										sort_indices[i]=i;
									}
								//now sort them
								if(attrCount>1)
									{
										for(int i=0;i<attrCount;i++)
											{
												char *akt=attr_names[sort_indices[i]];
												for(int j=i+1;j<attrCount;j++)
													{
														char* tmp=attr_names[sort_indices[j]];
														if(strcmp(akt,tmp)>0)
															{
																int t=sort_indices[i];
																sort_indices[i]=sort_indices[j];
																sort_indices[j]=t;
																akt=tmp;
															}
													}
											}
									}
							}

						for (int i = 0; i < attrCount; i++)
            {
								delete[] attr_names[i];
                DOM_Node  attribute = attributes.item(sort_indices[i]);
	
                //
                //  Again the name has to be completely representable. But the
                //  attribute can have refs and requires the attribute style
                //  escaping.
                //
                *m_pFormatter  << XMLFormatter::NoEscapes
                             << chSpace << attribute.getNodeName()
                             << chEqual << chDoubleQuote
                             << XMLFormatter::AttrEscapes
                             << attribute.getNodeValue()
                             << XMLFormatter::NoEscapes
                             << chDoubleQuote;
            }
            *m_pFormatter << XMLFormatter::NoEscapes << chCloseAngle;

						delete[] attr_names;
						delete[] sort_indices;

            //
            //  Test for the presence of children, which includes both
            //  text content and nested elements.
            //
            DOM_Node child = toWrite.getFirstChild();
            if (child != 0)
            {

                while( child != 0)
                {
                    dumpNode(child,bCanonical);
                    child = child.getNextSibling();
                }

                //
                // Done with children.  Output the end tag.
                //
						}
            *m_pFormatter << XMLFormatter::NoEscapes << gEndElement
                        << nodeName << chCloseAngle;
            break;
        }

/*
        case DOM_Node::ENTITY_REFERENCE_NODE:
            {
                DOM_Node child;
#if 0
                for (child = toWrite.getFirstChild();
                child != 0;
                child = child.getNextSibling())
                {
                    dumpNode(child);
                }
#else
                //
                // Instead of printing the refernece tree
                // we'd output the actual text as it appeared in the xml file.
                // This would be the case when -e option was chosen
                //
                    m_Formatter << XMLFormatter::NoEscapes << chAmpersand
                        << nodeName << chSemiColon;
#endif
                break;
            }


        case DOM_Node::CDATA_SECTION_NODE:
            {
            m_Formatter << XMLFormatter::NoEscapes << gStartCDATA
                        << nodeValue << gEndCDATA;
            break;
        }


        case DOM_Node::COMMENT_NODE:
        {
            m_Formatter << XMLFormatter::NoEscapes << gStartComment
                        << nodeValue << gEndComment;
            break;
        }


        case DOM_Node::DOCUMENT_TYPE_NODE:
        {
            DOM_DocumentType doctype = (DOM_DocumentType &)toWrite;;

            m_Formatter << XMLFormatter::NoEscapes  << gStartDoctype
                        << nodeName;

            DOMString id = doctype.getPublicId();
            if (id != 0)
            {
                m_Formatter << XMLFormatter::NoEscapes << chSpace << gPublic
                    << id << chDoubleQuote;
                id = doctype.getSystemId();
                if (id != 0)
                {
                    m_Formatter << XMLFormatter::NoEscapes << chSpace
                       << chDoubleQuote << id << chDoubleQuote;
                }
            }
            else
            {
                id = doctype.getSystemId();
                if (id != 0)
                {
                    m_Formatter << XMLFormatter::NoEscapes << chSpace << gSystem
                        << id << chDoubleQuote;
                }
            }

            id = doctype.getInternalSubset();
            if (id !=0)
                m_Formatter << XMLFormatter::NoEscapes << chOpenSquare
                            << id << chCloseSquare;

            m_Formatter << XMLFormatter::NoEscapes << chCloseAngle;
            break;
        }


        case DOM_Node::ENTITY_NODE:
        {
            m_Formatter << XMLFormatter::NoEscapes << gStartEntity
                        << nodeName;

            DOMString id = ((DOM_Entity &)toWrite).getPublicId();
            if (id != 0)
                m_Formatter << XMLFormatter::NoEscapes << gPublic
                            << id << chDoubleQuote;

            id = ((DOM_Entity &)toWrite).getSystemId();
            if (id != 0)
                m_Formatter << XMLFormatter::NoEscapes << gSystem
                            << id << chDoubleQuote;

            id = ((DOM_Entity &)toWrite).getNotationName();
            if (id != 0)
                m_Formatter << XMLFormatter::NoEscapes << gNotation
                            << id << chDoubleQuote;

            m_Formatter << XMLFormatter::NoEscapes << chCloseAngle << chLF;

            break;
        }


        case DOM_Node::XML_DECL_NODE:
        {
            DOMString  str;

            m_Formatter << gXMLDecl1 << ((DOM_XMLDecl &)toWrite).getVersion();

            m_Formatter << gXMLDecl2 << gEncodingName;

            str = ((DOM_XMLDecl &)toWrite).getStandalone();
            if (str != 0)
                m_Formatter << gXMLDecl3 << str;

            m_Formatter << gXMLDecl4;

            break;
        }

*/
			default:
				return E_UNKNOWN;
    }
	return E_SUCCESS;
}



// ---------------------------------------------------------------------------
//  ostream << DOMString
//
//  Stream out a DOM string. Doing this requires that we first transcode
//  to char * form in the default code page for the system
// ---------------------------------------------------------------------------
/*ostream& operator<< (ostream& target, const DOMString& s)
{
    char *p = s.transcode();
    target << p;
    delete [] p;
    return target;
}*/
