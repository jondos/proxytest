#ifndef __MXML_DOM_NAMED_NAODE_MAP__
#define __MXML_DOM_NAMED_NAODE_MAP__
#include "mxmlDOMNodeList.hpp"
class DOMNode;

class DOMNamedNodeMap
	{
		public:
			DOMNamedNodeMap()
				{
					pMap = new DOMNodeList();
				}
			DOMNode* setNamedItem(DOMNode *arg)
				{
					pMap->add(arg);
					return NULL;
				}
			UINT32 getLength() const
				{
					return pMap->getLength();
				}

			DOMNode* item(UINT32 index) const
				{
					return pMap->item(index);
				}

		private:
			DOMNodeList* pMap;
	};

#endif //__MXML_DOM_NAMED_NAODE_MAP__
