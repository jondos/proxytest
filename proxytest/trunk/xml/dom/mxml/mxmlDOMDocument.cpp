#ifdef MXML_DOM
#include "../../../StdAfx.h"

DOMElement* XERCES_CPP_NAMESPACE::DOMDocument::createElement(const XMLCh* tagName)
	{
		return new DOMElement(this,tagName);
	}
#endif //MXML_DOM
