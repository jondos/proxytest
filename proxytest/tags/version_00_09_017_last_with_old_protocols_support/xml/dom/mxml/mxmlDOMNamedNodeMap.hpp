#ifndef __MXML_DOM_NAMED_NAODE_MAP__
#define __MXML_DOM_NAMED_NAODE_MAP__

class DOMNode;

class DOMNamedNodeMap
	{
		public:
			UINT32 getLength() const;
			DOMNode* item(UINT32 index) const;
	};

#endif //__MXML_DOM_NAMED_NAODE_MAP__
