#ifndef __CALASTMIX__
#define __CALASTMIX__

#include "CAMix.hpp"
#include "CAMuxSocket.hpp"
#include "CASocketAddr.hpp"
#include "CAASymCipher.hpp"

class CALastMix:public CAMix
	{
		public:
			CALastMix(){};
			~CALastMix(){};
			SINT32 start();
		private:
			SINT32 loop();
		private:
			CAMuxSocket		muxIn;
			CASocketAddr	addrSquid;
			CASocketAddr	addrSocks;
			CAASymCipher* pRSA;
	};

#endif