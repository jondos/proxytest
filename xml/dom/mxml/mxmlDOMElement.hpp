#ifndef __MXML__DOMELEMENT_
#define __MXML__DOMELEMENT_

#include "mxmlDOMNode.hpp"
#include "mxmlDOMAttr.hpp"
#include "mxmlDOMNodeList.hpp"
#include "mxmlDOMNamedNodeMap.hpp"
#include "mxmlDOMDocument.hpp"
#include "mxmlXMLString.hpp"

class DOMElement:public DOMNode
	{
		public:
			const XMLCh* getTagName() const
				{
					return XMLString::replicate(m_xmlchNodeName);
				}
			
			DOMAttr* getAttributeNode(const XMLCh* name) const
				{
					if(m_pAttrs==NULL)
						{
							return NULL;
						}
					for(UINT32 i=0;i<m_pAttrs->getLength();i++)
						{
							DOMNode* node=m_pAttrs->item(i);
							if(XMLString::equals(name,node->getNodeName()))
								{
									return (DOMAttr*)node;
								}
						}
					return NULL;
				}

			
			const XMLCh* getAttribute(const XMLCh* name) const
				{
					DOMAttr* attr=getAttributeNode(name);
					if(attr!=NULL)
						return attr->getNodeValue();					
					return NULL;
				}


			void setAttribute(const XMLCh* name, const XMLCh* value)
				{
					DOMAttr* attr=getAttributeNode(name);
					if(attr!=NULL)
						attr->setNodeValue(value);
					else
						{
							attr=m_docOwner->createAttribute(name);
							attr->setNodeValue(value);
							m_pAttrs->add(attr);
						}
				}

			DOMNamedNodeMap* getAttributes() const
				{
					DOMNamedNodeMap* pMap = new DOMNamedNodeMap();
					for(UINT32 i=0;i<m_pAttrs->getLength();i++)
						{
							DOMNode* node=m_pAttrs->item(i);
							pMap->setNamedItem(node);
						}
					return pMap;
				}


			DOMNodeList* getElementsByTagName(const XMLCh* name) const
				{
					DOMNodeList* list=new DOMNodeList();
					addElementsByTagName(list,name);
					return list;
				}

	private:
			DOMElement(XERCES_CPP_NAMESPACE::DOMDocument* doc,const XMLCh* name):DOMNode(doc)	
				{
					m_xmlchNodeName=XMLString::replicate(name);
					m_nodeType=DOMNode::ELEMENT_NODE;
					m_pAttrs=new DOMNodeList();
				}

			~DOMElement()
				{
					for(UINT32 i=0;i<m_pAttrs->getLength();i++)
						{
							delete m_pAttrs->item(i);
						}
					delete m_pAttrs;
					m_pAttrs = NULL;
				}
			
				DOMElement* clone(XERCES_CPP_NAMESPACE::DOMDocument* doc)
				{
					DOMElement* pNode = new DOMElement(doc,getNodeName());
					pNode->m_xmlchNodeValue = XMLString::replicate(m_xmlchNodeValue);
					if (m_pAttrs != NULL)
						pNode->m_pAttrs = m_pAttrs->clone(doc);
					return pNode;
				}

			void addElementsByTagName(DOMNodeList* list,const XMLCh* name) const
				{
					if(XMLString::equals(m_xmlchNodeName,name))
						{
							list->add((DOMNode*)this);
						}
					DOMNode* child=getFirstChild();
					while(child!=NULL)
						{
							if(child->getNodeType()==DOMNode::ELEMENT_NODE)
								{
									((DOMElement*)child)->addElementsByTagName(list,name);
								}
							child=child->getNextSibling();
						}
				}
			
			friend class XERCES_CPP_NAMESPACE::DOMDocument;
			DOMNodeList* m_pAttrs;


	};

#endif //__MXML__DOMELEMENT_
