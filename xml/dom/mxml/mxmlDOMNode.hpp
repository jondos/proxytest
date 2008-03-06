#ifndef __MXML__DOMNODE_
#define __MXML__DOMNODE_

#include "mxmlDOMTypeDef.hpp"

class DOMNode
	{
		public:
			enum NodeType
				{
					ELEMENT_NODE = 1,
				  TEXT_NODE = 2, 
				  DOCUMENT_NODE = 3, 
				  DOCUMENT_FRAGMENT_NODE = 4 
				}; 

			DOMNode* appendChild(DOMNode* newChild);
			void release();
			DOMNode* getFirstChild() const;
			DOMNode* getLastChild() const;  
			DOMNode* getNextSibling() const;  
			DOMNode* getPreviousSibling() const;
			DOMNode* getParentNode() const;
			const XMLCh* getNodeName() const;
			const XMLCh* getNodeValue() const;  
			DOMNode* insertBefore(DOMNode* newChild, DOMNode* refChild);
			DOMNode* replaceChild(DOMNode* newChild, DOMNode* oldChild);
			DOMNode* removeChild(DOMNode* oldChild);
			bool hasChildNodes() const;  
			XERCES_CPP_NAMESPACE::DOMDocument* getOwnerDocument() const;
			UINT getNodeType() const;
	};

#endif //__MXML__DOMNODE_
