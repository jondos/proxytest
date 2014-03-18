#include "../../../StdAfx.h"
#ifdef MXML_DOM
DOMElement* XERCES_CPP_NAMESPACE::DOMDocument::createElement(const XMLCh* tagName)
	{
		return new DOMElement(this,tagName);
	}
#endif //MXML_DOM
