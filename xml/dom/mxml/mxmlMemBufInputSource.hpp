#include "mxmlInputSource.hpp"
#include "mxmlDOMTypeDef.hpp"

class  MemBufInputSource : public InputSource
	{
	public:
		 MemBufInputSource(const XMLByte* const srcDocBytes,const UINT byteCount, const char* const id);
	};
