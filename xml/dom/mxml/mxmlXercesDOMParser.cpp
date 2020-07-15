#include "../../../StdAfx.h"
#ifdef MXML_DOM
#include "../../../CAUtil.hpp"
/*
int xercesdomparser_custom_load_callback( mxml_node_t * node, const char * text)
	{
		mxmlSetText(node, 0, strdup(text));
		//node->value.text.string=strdup(text);
		//node->type=MXML_TEXT;
		return 0;
	}

mxml_type_t xercesdomparser_load_callback( mxml_node_t *)
	{
		return MXML_CUSTOM;
	}
*/
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
							elem = createDOMElement(doc, mxmlGetElement(node));
							doc->appendChild(elem);
							pParser->m_Doc=doc;
							pParser->m_curNode=elem;
						}
					else
						{
							doc=curNode->getOwnerDocument();
							elem = createDOMElement(doc, mxmlGetElement(node));
							curNode->appendChild(elem);
							pParser->m_curNode=elem;
						}
					for (int i = 0; i < mxmlElementGetAttrCount(node); i++)
						{
							const char *name = NULL;
							const UINT8* val=(const UINT8*)mxmlElementGetAttrByIndex(node, i, &name);
							setDOMElementAttribute(elem,name,val);
						}

				break;
				case MXML_SAX_ELEMENT_CLOSE:
					pParser->m_curNode=curNode->getParentNode();
				break;

				case MXML_SAX_DATA:
					if(curNode!=NULL)
						{
							const char *text = mxmlGetOpaque(node);
							if (text != NULL)
								{
									doc = curNode->getOwnerDocument();
									DOMText *t = createDOMText(doc, text);
									curNode->appendChild(t);
								}
						}
				break;
				
				case MXML_SAX_CDATA: 
				break;

				case MXML_SAX_COMMENT:
				break;

				case MXML_SAX_DIRECTIVE:
				break;

			}
	}
#endif //MXML_DOM
