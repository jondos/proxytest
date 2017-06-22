#include "../../../StdAfx.h"
#ifdef MXML_DOM
#include "mxmlDOMNode.hpp"
#include "mxmlDOMNodeList.hpp"
DOMNodeList* DOMNode::getChildNodes() const
	{
		DOMNodeList* pList = new DOMNodeList();
		DOMNode* pChild = getFirstChild();
		while(pChild!=NULL)
			{
				pList->add(pChild);
				pChild = pChild->m_nodeNext;
			}
		return pList;
	}
#endif //MXML_DOM
