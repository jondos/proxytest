#ifndef __CAMIDDLEMIX__
#define __CAMIDDLEMIX__

#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
#include "CAASymCipher.hpp"
class CAMiddleMix:public CAMix
	{
		public:
			CAMiddleMix(){};
			~CAMiddleMix(){};
		private:
			SINT32 loop();
			SINT32 init();
		private:
			CAMuxSocket muxIn;
			CAMuxSocket muxOut;
			CAASymCipher oRSA;
	};

#endif