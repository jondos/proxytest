#include "mxmlInputSource.hpp"
#include "mxmlDOMTypeDef.hpp"

class XercesDOMParser
	{
		public:
			void parse  ( const InputSource &  source   );
			UINT32 getErrorCount() const;
			XERCES_CPP_NAMESPACE::DOMDocument* getDocument();  
	};
