#ifndef __CAFIRSTMIX__
#define __CAFIRSTMIX__

#include "CAMix.hpp"
#include "CASocket.hpp"
#include "CAMuxSocket.hpp"
class CAFirstMix:public CAMix
	{
		public:
			CAFirstMix(){};
			~CAFirstMix(){};
			SINT32 start();
		private:
			SINT32 loop();
		private:
			CASocket socketIn;
			CAMuxSocket muxOut;
			UINT8* recvBuff;
	};

#endif