#ifndef __MXML_DOMIMPLEMENTATION__
#define __MXML_DOMIMPLEMENTATION__
#include "mxmlDOMTypeDef.hpp"
#include "mxmlDOMDocument.hpp"

class DOMImplementation
	{
		public:
		static DOMImplementation* getImplementation()
				{
					if (m_pImplementation == NULL)
						{
							m_pImplementation = new DOMImplementation();
						}
					return m_pImplementation;

				}
		XERCES_CPP_NAMESPACE::DOMDocument *createDocument()
				{
					return new XERCES_CPP_NAMESPACE::DOMDocument();
				}
		private:
				static DOMImplementation* m_pImplementation;
	};
#endif
