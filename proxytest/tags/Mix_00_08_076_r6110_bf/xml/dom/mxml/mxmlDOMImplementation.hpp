#include "mxmlDOMTypeDef.hpp"

class DOMImplementation
	{
		public:
			static DOMImplementation* getImplementation();
			XERCES_CPP_NAMESPACE::DOMDocument *createDocument();
	};
