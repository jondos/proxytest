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
			SINT32 start();
		private:
			SINT32 loop();
		private:
			CASocket socketIn;
			CASocket socketSOCKSIn;
			CAMuxSocket muxOut;
			UINT8 chainlen;
			CAASymCipher* arRSA;
	};

#endif