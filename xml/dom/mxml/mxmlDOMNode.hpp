#ifndef __MXML__DOMNODE_
#define __MXML__DOMNODE_

#include "mxmlDOMTypeDef.hpp"

#include "mxmlXMLString.hpp"

class DOMNodeList;
class DOMNamedNodeMap;

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

			DOMNode* insertBefore(DOMNode* newChild, DOMNode* refChild)
				{
					if (refChild == NULL)
						return appendChild(newChild);
					if(newChild==NULL||newChild->getOwnerDocument()!=m_docOwner)
						return NULL;
					newChild->m_nodeParent=this;
					DOMNode* child = m_nodeFirstChild;
					while (child != NULL)
						{
							if (child == refChild)
								{
									newChild->m_nodeNext = refChild;
									newChild->m_nodePrev = refChild->m_nodePrev;
									if (refChild->m_nodePrev != NULL) //middle or last
										{
											refChild->m_nodePrev->m_nodeNext = newChild;
											refChild->m_nodePrev = newChild;
										}
									else //first element
										{
											m_nodeFirstChild = newChild;
											refChild->m_nodePrev = newChild;
										}
									return newChild;
								}
						}
					return NULL;
				}

			DOMNode* replaceChild(DOMNode* newChild, DOMNode* oldChild)
				{
					if (oldChild == NULL)
						return oldChild;
					DOMNode* child = m_nodeFirstChild;
					while (child != NULL)
						{
							if (child == oldChild)
								{
									if (oldChild->m_nodePrev != NULL) //repalce middle or last element
										{
											oldChild->m_nodePrev->m_nodeNext = newChild;
											newChild->m_nodePrev = oldChild->m_nodePrev;
											newChild->m_nodeNext = oldChild->m_nodeNext;
										}
									else //replace first element
										{
											m_nodeFirstChild = newChild;
											newChild->m_nodePrev = oldChild->m_nodePrev;
											newChild->m_nodeNext = oldChild->m_nodeNext;
										}
									if (oldChild->m_nodeNext != NULL)
										{
											oldChild->m_nodeNext->m_nodePrev = newChild;
										}
									return oldChild;
								}
							child = child->m_nodeNext;
						}
					return NULL;

				}
			DOMNode* removeChild(DOMNode* oldChild)
				{
					if (oldChild == NULL)
						return oldChild;
					DOMNode* child = m_nodeFirstChild;
					while (child != NULL)
						{
							if (child == oldChild)
								{
									if (oldChild->m_nodePrev != NULL) //remove middle or last element
										{
											oldChild->m_nodePrev->m_nodeNext = oldChild->m_nodeNext;
										}
									else //remove first element
										{
											m_nodeFirstChild=oldChild->m_nodeNext;
										}
									if (oldChild->m_nodeNext != NULL)
										{
											oldChild->m_nodeNext->m_nodePrev = oldChild->m_nodePrev;
										}
									return oldChild;
								}
							child = child->m_nodeNext;
						}
					return NULL;
				}

			bool hasChildNodes() const
				{
					return m_nodeFirstChild!=NULL;
				}

			DOMNodeList* getChildNodes() const;
			

			virtual DOMNamedNodeMap* getAttributes() const
				{
					return NULL;
				}

			XERCES_CPP_NAMESPACE::DOMDocument* getOwnerDocument() const
				{
					return m_docOwner;
				}

			UINT getNodeType() const
				{
					return m_nodeType;
				}

			DOMNode* clone(bool bDeep, XERCES_CPP_NAMESPACE::DOMDocument* doc)
				{
					DOMNode* pNode = clone(doc);
					if (bDeep)
						{
							DOMNode* child=m_nodeFirstChild;
							while(child!=NULL)
								{
									pNode->appendChild(child->clone(bDeep,doc));
									child = child->m_nodeNext;
								}
						}
					return pNode;
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

			virtual DOMNode* clone(XERCES_CPP_NAMESPACE::DOMDocument* doc)
				{
					DOMNode* pNode = new DOMNode(doc);
					pNode->m_xmlchNodeName = XMLString::replicate(m_xmlchNodeName);
					pNode->m_xmlchNodeValue = XMLString::replicate(m_xmlchNodeValue);
					return pNode;
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
