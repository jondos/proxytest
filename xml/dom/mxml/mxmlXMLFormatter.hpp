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
			
			XMLFormatter(const XMLCh *const outEncoding, XMLFormatTarget *const target, const EscapeFlags escapeFlags = NoEscapes, const UnRepFlags unrepFlags = UnRep_Fail)
				{
					m_pFormatTarget = target;
					m_EscapeFlag = escapeFlags;
				}

			void formatBuf(const XMLCh *const toFormat, const unsigned int count, const EscapeFlags)
				{
				m_pFormatTarget->writeChars(toFormat, count, this);
				}
			
			XMLFormatter& operator<< (const XMLCh toFormat)
				{
					XMLCh c = toFormat;
					formatBuf(&toFormat, 1, m_EscapeFlag);
					return *this;
				}

			XMLFormatter& operator<< (const XMLCh* toFormat)
				{
					formatBuf(toFormat, XMLString::stringLen(toFormat), m_EscapeFlag);
					return *this;
				}

		private:
			EscapeFlags m_EscapeFlag;
			XMLFormatTarget* m_pFormatTarget;
	};
