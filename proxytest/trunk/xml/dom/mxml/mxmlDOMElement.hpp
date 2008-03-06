#ifndef __MXML__DOMELEMENT_
#define __MXML__DOMELEMENT_

#include "mxmlDOMNode.hpp"
#include "mxmlDOMNodeList.hpp"

class DOMElement:public DOMNode
	{
		public:
			const XMLCh* getTagName() const;
			const XMLCh* getAttribute(const XMLCh* name) const;
			void setAttribute(const XMLCh* name, const XMLCh* value);
			DOMNodeList* getElementsByTagName(const XMLCh* name) const;
	};

#endif //__MXML__DOMELEMENT_
