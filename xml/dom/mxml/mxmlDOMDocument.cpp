#include "../../../StdAfx.h"
#ifdef MXML_DOM
DOMElement* XERCES_CPP_NAMESPACE::DOMDocument::createElement(const XMLCh* tagName)
	{
		return new DOMElement(this,tagName);
	}


DOMNode* XERCES_CPP_NAMESPACE::DOMDocument::importNode(DOMNode* importedNode, bool deep)
	{
		DOMNode* newNode = importedNode->clone(deep,this);
		return newNode;
	}
#endif //MXML_DOM
