#ifndef __MXML_DOM_TEXT__
#define __MXML_DOM_TEXT__

#include "mxmlDOMNode.hpp"

class DOMText:public DOMNode
	{
		protected:
			DOMText(XERCES_CPP_NAMESPACE::DOMDocument* doc,const XMLCh* value):DOMNode(doc)	
				{
					setNodeValue(value);
					m_xmlchNodeName=XMLString::replicate((XMLCh*)"#text");
					m_nodeType=DOMNode::TEXT_NODE;
				}
			friend class XERCES_CPP_NAMESPACE::DOMDocument;
	};

#endif //__MXML_DOM_TEXT__
