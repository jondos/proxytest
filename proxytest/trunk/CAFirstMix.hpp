#ifndef __CAFIRSTMIX__
#define __CAFIRSTMIX__

#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
class CAFirstMix:public CAMix
	{
		public:
			CAFirstMix(){};
			~CAFirstMix(){};
		private:
			SINT32 loop();
			SINT32 init();
		private:
			CASocket socketIn;
			CAMuxSocket muxOut;
			UINT8* recvBuff;
	};

#endif