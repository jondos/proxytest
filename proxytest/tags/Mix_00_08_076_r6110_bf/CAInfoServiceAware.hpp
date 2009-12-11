#ifndef __CADINFOSERVICEAWARE__
#define __CADINFOSERVICEAWARE__

#include "StdAfx.h"
#ifdef DYNAMIC_MIX

#include "CASocketAddrINet.hpp"

#define MAX_CONTENT_LENGTH 0x00FFFF

class CAInfoServiceAware
{

protected:
	SINT32 sendRandomInfoserviceRequest(UINT8 *a_strRequest, DOM_Element *r_elemRoot, UINT8* postData, UINT32 postLen);
	SINT32 sendRandomInfoserviceGetRequest(UINT8 *a_strRequest, DOM_Element *r_elemRoot);
	SINT32 sendInfoserviceRequest(CASocketAddrINet* a_addr, UINT8 *a_strRequest, DOM_Element *r_elemRoot, UINT8* postData, UINT32 postLen);
	SINT32 sendInfoserviceGetRequest(CASocketAddrINet* a_addr, UINT8 *a_strRequest, DOM_Element *r_elemRoot);
};
#endif //DYNAMIC_MIX
#endif
