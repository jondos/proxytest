#ifndef __MXML_XML_FORMAT_TARGET__
#define __MXML_XML_FORMAT_TARGET__

class XMLFormatter;

class XMLFormatTarget
	{
		public:
			virtual void writeChars(const XMLByte* const toWrite, const unsigned int count, XMLFormatter* const /*formatter*/) = 0;
	};

#endif //__MXML_XML_FORMAT_TARGET__
