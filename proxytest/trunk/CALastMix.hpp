#ifndef __CALASTMIX__
#define __CALASTMIX__

#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
#include "CAASymCipher.hpp"

class CALastMix:public CAMix
	{
		public:
			CALastMix(){};
			~CALastMix(){};
		private:
			SINT32 loop();
			SINT32 init();
		private:
			CAMuxSocket		muxIn;
			CASocketAddr	addrSquid;
			CASocketAddr	addrSocks;
			CAASymCipher* pRSA;
	};

#endif