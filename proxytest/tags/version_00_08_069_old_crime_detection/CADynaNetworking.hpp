/*
Copyright (c) 2000, The JAP-Team 
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:
 
	- Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.
 
	- Redistributions in binary form must reproduce the above copyright notice, 
	  this list of conditions and the following disclaimer in the documentation and/or 
		other materials provided with the distribution.
 
	- Neither the name of the University of Technology Dresden, Germany nor the names of its contributors 
	  may be used to endorse or promote products derived from this software without specific 
		prior written permission. 
 
	
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS 
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/
#ifndef __CADYNANETWORKING__
#define __CADYNANETWORKING__

#include "StdAfx.h"

#ifdef DYNAMIC_MIX

#include "CACmdLnOptions.hpp"
#include "CAListenerInterface.hpp"
#include "CASocketAddrINet.hpp"
#include "CAThread.hpp"
#include "CAInfoServiceAware.hpp"

#define MAX_CONTENT_LENGTH 0x00FFFF

class CADynaNetworking : CAInfoServiceAware
	{
		public:
			CADynaNetworking();
			~CADynaNetworking();
			SINT32 verifyConnectivity();
			SINT32 updateNetworkConfiguration(UINT16 a_port);
			CAListenerInterface *getWorkingListenerInterface();
		private:
			SINT32 resolveInternalIp(UINT8* r_strIp);
			SINT32 resolveExternalIp(UINT8* r_strIp, UINT32 len);
			SINT32 createListenerInterface(DOM_Element r_elemListeners, DOM_Document a_ownerDoc,const UINT8 *a_ip, UINT32 a_port, bool a_bHidden, bool a_bVirtual);
			SINT32 createListenerInterface(DOM_Element r_elemListeners, DOM_Document a_ownerDoc);
			SINT32 getInterfaceIp(UINT32* r_ip);
			bool isInternalIp(UINT32 p_ip);
			SINT32 sendConnectivityRequest(DOM_Element *r_elemRoot, UINT32 a_port);
	};
#endif //DYNAMIC_MIX
#endif 
