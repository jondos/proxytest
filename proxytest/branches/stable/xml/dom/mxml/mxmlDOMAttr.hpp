#ifndef __MXML_DOM_ATTR__
#define __MXML_DOM_ATTR__

#include "mxmlDOMNode.hpp"
#include "mxmlXMLString.hpp"

class DOMAttr : public DOMNode
	{
		protected:
			DOMAttr(XERCES_CPP_NAMESPACE::DOMDocument* doc,const XMLCh* name):DOMNode(doc)	
				{
					m_xmlchNodeName=XMLString::replicate(name);
					m_nodeType=DOMNode::ATTRIBUTE_NODE;
				}

			DOMAttr* clone(XERCES_CPP_NAMESPACE::DOMDocument* doc)
				{
					DOMAttr* pNode = new DOMAttr(doc,m_xmlchNodeName);
					pNode->m_xmlchNodeValue = XMLString::replicate(m_xmlchNodeValue);
					return pNode;
				}
			friend class XERCES_CPP_NAMESPACE::DOMDocument;
	
	};
#endif
