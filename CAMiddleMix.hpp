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
			SINT32 start();
		private:
			SINT32 loop();
		private:
			CAMuxSocket muxIn;
			CAMuxSocket muxOut;
			CAASymCipher oRSA;
	};

#endif