#include "StdAfx.h"
#include "CAMix.hpp"

SINT32 CAMix::start()
	{
		if(init()==E_SUCCESS)
			return loop();
		else
			return E_UNKNOWN;
	}