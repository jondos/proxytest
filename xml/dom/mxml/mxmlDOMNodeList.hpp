#ifndef __MXML_DOM_NODE_LIST__
#define __MXML_DOM_NODE_LIST__

#include "mxmlDOMNode.hpp"

class DOMNodeList
	{
		public:   
			DOMNode* item(UINT32 index) const; 
			UINT32 getLength() const; 
	};

#endif //__MXML_DOM_NODE_LIST__
