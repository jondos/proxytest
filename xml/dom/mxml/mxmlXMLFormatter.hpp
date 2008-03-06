#include "mxmlXMLFormatTarget.hpp"

class XMLFormatter
	{
		public:
			enum EscapeFlags
				{ 
					NoEscapes 
				};
			enum UnRepFlags
				{
					UnRep_Fail
				};
			
			XMLFormatter (const XMLCh *const outEncoding, XMLFormatTarget *const target, const EscapeFlags escapeFlags=NoEscapes, const UnRepFlags unrepFlags=UnRep_Fail);
	};
