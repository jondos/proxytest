#include "mxmlXMLFormatTarget.hpp"

class XMLFormatter
	{
		public:
			enum EscapeFlags
				{ 
					NoEscapes,
					AttrEscapes,
					CharEscapes
				};
			enum UnRepFlags
				{
					UnRep_Fail
				};
			
			XMLFormatter (const XMLCh *const outEncoding, XMLFormatTarget *const target, const EscapeFlags escapeFlags=NoEscapes, const UnRepFlags unrepFlags=UnRep_Fail);
			void formatBuf(const XMLCh *const toFormat, const unsigned int count, const EscapeFlags);
			
			XMLFormatter& operator<< (const XMLCh toFormat);
			XMLFormatter& operator<< (const XMLCh* toFormat);
	};
