#ifndef __MXML__DOMNODE_
#define __MXML__DOMNODE_

#include "mxmlDOMTypeDef.hpp"
#include "mxmlDOMNamedNodeMap.hpp"
#include "mxmlXMLString.hpp"

class DOMNode
	{
		public:
			enum NodeType
				{
					ELEMENT_NODE = 1,
				  TEXT_NODE = 2, 
				  DOCUMENT_NODE = 3, 
				  DOCUMENT_FRAGMENT_NODE = 4 ,
					ATTRIBUTE_NODE=5
				}; 

			DOMNode* getNextSibling() const
				{
					return m_nodeNext;
				}

			DOMNode* getLastChild() const
				{
					DOMNode* child=m_nodeFirstChild;
					DOMNode* last=child;
					while(child!=NULL)
						{
							last=child;
							child=child->getNextSibling();
						}
					return last;
				}

			DOMNode* appendChild(DOMNode* newChild)
				{
					if(newChild->getOwnerDocument()!=m_docOwner)
						return NULL;
					newChild->m_nodeParent=this;
					if(m_nodeFirstChild==NULL)
						{
							m_nodeFirstChild=newChild;
						}
					else
						{
							DOMNode* child=getLastChild();
							child->m_nodeNext=newChild;
							newChild->m_nodePrev=child;
						}
					return newChild;
				}

			virtual void release()
				{
				}

			DOMNode* getFirstChild() const
				{
					return m_nodeFirstChild;
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
			void setNodeValue(const XMLCh *nodeValue)
				{
					XMLString::release(&m_xmlchNodeValue);
					m_xmlchNodeValue=XMLString::replicate(nodeValue);
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
		
		protected:
			DOMNode(XERCES_CPP_NAMESPACE::DOMDocument* doc)
				{
					m_docOwner=doc;
					m_nodeParent=NULL;
					m_nodePrev=NULL;
					m_nodeNext=NULL;
					m_nodeFirstChild=NULL;
					m_xmlchNodeName=NULL;
					m_xmlchNodeValue=NULL;
				}
			
			virtual ~DOMNode()
				{
					XMLString::release(&m_xmlchNodeName);
					XMLString::release(&m_xmlchNodeValue);
					DOMNode* child=m_nodeFirstChild;
					while(child!=NULL)
						{
							m_nodeFirstChild=child->getNextSibling();
							delete child;
							child=m_nodeFirstChild;
						}
				}

			DOMNode* m_nodeParent;
			DOMNode* m_nodePrev;
			DOMNode* m_nodeNext;
			DOMNode* m_nodeFirstChild;
			XERCES_CPP_NAMESPACE::DOMDocument* m_docOwner;
			XMLCh* m_xmlchNodeName;
			XMLCh* m_xmlchNodeValue;
			UINT m_nodeType;
			friend void xercesdomparser_sax_callback (mxml_node_t * node, mxml_sax_event_t sax_event, void * data);
			friend class DOMElement;
			friend class XERCES_CPP_NAMESPACE::DOMDocument;
	};

#endif //__MXML__DOMNODE_
