#ifndef __MXML__DOMNODE_
#define __MXML__DOMNODE_

#include "mxmlDOMTypeDef.hpp"
#include "mxmlDOMNamedNodeMap.hpp"

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

			DOMNode* appendChild(DOMNode* newChild)
				{
					if(newChild->getOwnerDocument()!=m_docOwner)
						return NULL;
					
					return newChild;
				}

			void release();
			DOMNode* getFirstChild() const
				{
					return m_nodeFirstChild;
				}

			DOMNode* getLastChild() const;  
			DOMNode* getNextSibling() const
				{
					return m_nodeNext;
				}
			DOMNode* getPreviousSibling() const
				{
					return m_nodePrev;
				}
			DOMNode* getParentNode() const
				{
					return m_nodeParent;
				}
			const XMLCh* getNodeName() const
				{
					return m_xmlchNodeName;
				}

			const XMLCh* getNodeValue() const
				{
					return m_xmlchNodeValue;
				}

			DOMNode* insertBefore(DOMNode* newChild, DOMNode* refChild);
			DOMNode* replaceChild(DOMNode* newChild, DOMNode* oldChild);
			DOMNode* removeChild(DOMNode* oldChild);
			bool hasChildNodes() const
				{
					return m_nodeFirstChild!=NULL;
				}

			DOMNamedNodeMap* getAttributes() const;
			XERCES_CPP_NAMESPACE::DOMDocument* getOwnerDocument() const
				{
					return m_docOwner;
				}

			UINT getNodeType() const
				{
					return m_nodeType;
				}
		
		private:
			DOMNode* m_nodeParent;
			DOMNode* m_nodePrev;
			DOMNode* m_nodeNext;
			DOMNode* m_nodeFirstChild;
			XERCES_CPP_NAMESPACE::DOMDocument* m_docOwner;
			XMLCh* m_xmlchNodeName;
			XMLCh* m_xmlchNodeValue;
			UINT m_nodeType;
	};

#endif //__MXML__DOMNODE_
