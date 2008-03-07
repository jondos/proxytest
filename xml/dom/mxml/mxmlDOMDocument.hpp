#include "mxmlDOMTypeDef.hpp"
#include "mxmlDOMNode.hpp"
#include "mxmlDOMElement.hpp"
#include "mxmlDOMDocumentFragment.hpp"
#include "mxmlDOMText.hpp"

namespace XERCES_CPP_NAMESPACE
	{
		class DOMDocument : public DOMNode
			{
				public:
					DOMElement* getDocumentElement()
						{
							return m_elemRoot;
						}

					DOMNode*		importNode(DOMNode* importedNode, bool deep);
					DOMDocumentFragment* createDocumentFragment();
					DOMText* createTextNode(const XMLCh* data);
					DOMElement* createElement(const XMLCh* tagName);
			private:
					DOMElement* m_elemRoot;
			};
	};
