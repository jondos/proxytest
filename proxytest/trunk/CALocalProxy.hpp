#ifndef __CALOCALPROXY__
#define __CALOCALPROXY__

#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
#include "CAASymCipher.hpp"
#include "CASocket.hpp"
class CALocalProxy:public CAMix
	{
		public:
			CALocalProxy(){};
			~CALocalProxy(){};
		private:
			SINT32 loop();
			SINT32 init();
		private:
			CASocket socketIn;
			CASocket socketSOCKSIn;
			CAMuxSocket muxOut;
			UINT8 chainlen;
			CAASymCipher* arRSA;
	};

#endif