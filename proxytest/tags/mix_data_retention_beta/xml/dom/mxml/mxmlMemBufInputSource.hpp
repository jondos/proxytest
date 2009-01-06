#include "mxmlInputSource.hpp"
#include "mxmlDOMTypeDef.hpp"

class  MemBufInputSource : public InputSource
	{
		public:
			MemBufInputSource(const XMLByte* const srcDocBytes,const UINT byteCount, const char* const id)
				{
					m_strXML=new char[byteCount+1];
					memcpy(m_strXML,srcDocBytes,byteCount);
					m_strXML[byteCount]=0;
				}

			~MemBufInputSource()
				{
					delete[] m_strXML;
					m_strXML = NULL;
				}

		protected:
			char* getBuff() const
				{
					return m_strXML;
				}
			friend class XercesDOMParser;

		private:
			char* m_strXML;
	};
