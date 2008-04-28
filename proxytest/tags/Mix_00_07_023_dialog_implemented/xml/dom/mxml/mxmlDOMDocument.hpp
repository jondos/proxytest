#ifndef __MXML__DOM_DOCUMENT__
#define __MXML__DOM_DOCUMENT__

#include "mxmlDOMTypeDef.hpp"
#include "mxmlDOMNode.hpp"
#include "mxmlDOMElement.hpp"
#include "mxmlDOMDocumentFragment.hpp"
#include "mxmlDOMText.hpp"

namespace XERCES_CPP_NAMESPACE
	{
		class DOMDocument : public DOMNode
			{
				public:
					DOMElement* getDocumentElement()
						{
							return (DOMElement*)m_nodeFirstChild;
						}

					DOMNode*		importNode(DOMNode* importedNode, bool deep);
					DOMDocumentFragment* createDocumentFragment();
					DOMText* createTextNode(const XMLCh* data)
						{
							return new DOMText(this,data);
						}

					DOMElement* createElement(const XMLCh* tagName);

					DOMAttr* createAttribute(const XMLCh* name) 
						{
							return new DOMAttr(this,name);
						}
					void release()
						{
							delete this;
						}
				public:
					DOMDocument():DOMNode(this)
						{
							m_nodeType=DOMNode::DOCUMENT_NODE;
						}

					~DOMDocument()
						{
						}
					
					friend void xercesdomparser_sax_callback (mxml_node_t * node, mxml_sax_event_t sax_event, void * data);
			};
	};
#endif //__MXML__DOM_DOCUMENT__
