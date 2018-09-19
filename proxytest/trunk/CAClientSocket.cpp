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

#include "StdAfx.h"
#include "CAClientSocket.hpp"
#include "CASingleSocketGroup.hpp"

SINT32 CAClientSocket::receiveFully(UINT8* buff, UINT32 len)
{
	SINT32 ret;
	UINT32 pos = 0;
#ifdef	__BUILD_AS_SHADOW_PLUGIN__
	CASingleSocketGroup* pSocketGroup = new CASingleSocketGroup(false);
	pSocketGroup->add(getSocket());
#endif
	do
	{
#ifdef	__BUILD_AS_SHADOW_PLUGIN__
		pSocketGroup->select();
#endif
		ret = receive(buff + pos, len);
		if (ret <= 0)
		{
			if (ret == E_AGAIN)
			{
#ifndef	__BUILD_AS_SHADOW_PLUGIN__
				msSleep(100);
#endif
				continue;
			}
			else
			{
				return E_UNKNOWN;
			}
		}
		pos += ret;
		len -= ret;
	} while (len > 0);
	return E_SUCCESS;
}

