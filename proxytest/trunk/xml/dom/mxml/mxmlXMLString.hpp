#include "mxmlDOMTypeDef.hpp"

class XMLString
	{
		public:
			static char* transcode  ( const XMLCh *const   toTranscode);
			static XMLCh* transcode  ( const char *const   toTranscode);
			static void release  ( XMLCh **  buf);  
			static void release  ( char **  buf);
			static bool equals ( const XMLCh *const   str1, const XMLCh *const   str2);
			static void trim(XMLCh* const toTrim);
			static XMLCh* replicate(const XMLCh* const toRep);
			static UINT32 stringLen(const XMLCh* const src);
	};
