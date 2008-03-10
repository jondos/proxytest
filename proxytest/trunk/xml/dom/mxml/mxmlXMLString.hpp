#ifndef __MXML_XML_STRING__
#define __MXML_XML_STRING__
#include "mxmlDOMTypeDef.hpp"

class XMLString
	{
		public:
			static char* transcode  ( const XMLCh *const   toTranscode)
				{
					UINT32 len=strlen((char*)toTranscode)+1;
					char* newBuf=new char[len];
					memcpy(newBuf,toTranscode,len);
					return newBuf;
				}
			
			static XMLCh* transcode  ( const char *const   toTranscode)
				{
					UINT32 len=strlen(toTranscode)+1;
					XMLCh* newBuf=new XMLCh[len];
					memcpy(newBuf,toTranscode,len);
					return newBuf;
				}

			static void release  ( XMLCh **  buf)
				{
					delete[] *buf;
					buf=NULL;
				}

			static void release  ( char **  buf)
				{
					delete[] *buf;
					buf=NULL;
				}

			static bool equals ( const XMLCh *const   str1, const XMLCh *const   str2)
				{
					return strcmp((char*)str1,(char*)str2)==0;
				}

			static void trim(XMLCh* const toTrim);
			
			static XMLCh* replicate(const XMLCh* const toRep)
				{
					UINT32 len=XMLString::stringLen(toRep)+1;
					XMLCh* c=new XMLCh[len];
					memcpy(c,toRep,len);
					return c;
				}
			
			static UINT32 stringLen(const XMLCh* const src)
				{
					return strlen((char*)src);
				}

			static SINT32 compareString(const XMLCh *const str1, const XMLCh *const str2);
	};
#endif //__MXML_XML_STRING__
