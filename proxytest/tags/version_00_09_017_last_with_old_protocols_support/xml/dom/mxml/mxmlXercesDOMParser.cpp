#ifdef MXML_DOM
#include "../../../StdAfx.h"
#include "../../../CAUtil.hpp"

int xercesdomparser_custom_load_callback( mxml_node_t * node, const char * text)
	{
		node->value.text.string=strdup(text);
		node->type=MXML_TEXT;
		return 0;
	}

mxml_type_t xercesdomparser_load_callback( mxml_node_t *)
	{
		return MXML_CUSTOM;
	}

void xercesdomparser_sax_callback (mxml_node_t * node, mxml_sax_event_t sax_event, void * data)
	{
		XercesDOMParser* pParser=(XercesDOMParser*)data;
		DOMNode* curNode=pParser->m_curNode;
		DOMElement* elem=NULL;
		XERCES_CPP_NAMESPACE::DOMDocument* doc=NULL;
		switch(sax_event)
			{
				case MXML_SAX_ELEMENT_OPEN:
					if(curNode==NULL)
						{
							doc=new XERCES_CPP_NAMESPACE::DOMDocument();
							elem=createDOMElement(doc,node->value.element.name);
							doc->appendChild(elem);
							pParser->m_Doc=doc;
							pParser->m_curNode=elem;
						}
					else
						{
							doc=curNode->getOwnerDocument();
							elem=createDOMElement(doc,node->value.element.name);
							curNode->appendChild(elem);
							pParser->m_curNode=elem;
						}
					for(UINT32 i=0;i<node->value.element.num_attrs;i++)
						{
							setDOMElementAttribute(elem,node->value.element.attrs[i].name,(UINT8*)node->value.element.attrs[i].value);
						}

				break;
				case MXML_SAX_ELEMENT_CLOSE:
					pParser->m_curNode=curNode->getParentNode();
				break;

				case MXML_SAX_DATA:
					if(curNode!=NULL)
						{
							doc=curNode->getOwnerDocument();
							DOMText* t=createDOMText(doc,node->value.text.string);
							curNode->appendChild(t);
						}
				break;
				case MXML_SAX_CDATA: 
				break;
			}
	}
#endif //MXML_DOM
