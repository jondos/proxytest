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
/*
#include "StdAfx.h"
#include "CAClientSocket.hpp"
#include "CASocket.hpp"
#include "CAUtil.hpp"
#include "CASocketAddrINet.hpp"
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
	#include "CASocketAddrUnix.hpp"
#endif
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp";


SINT32 CAClientSocket::receiveFullyT(UINT8* buff,UINT32 len,UINT32 msTimeOut)
{
	SINT32 ret;
	UINT32 pos=0;
	UINT64 currentTime,endTime;
	bool bWasNonBlocking; 
	
	getcurrentTimeMillis(currentTime);
	set64(endTime,currentTime);
	add64(endTime,msTimeOut);
	
	getSocket()->getNonBlocking(&bWasNonBlocking);
	if (!bWasNonBlocking)
	{
		getSocket()->setNonBlocking(true);
	}
	
	CASingleSocketGroup oSG(false);
	oSG.add(*getSocket());
	ret = 1;
	for(;;)
	{
		getcurrentTimeMillis(currentTime);
		if(!isLesser64(currentTime,endTime))
		{
			if (!bWasNonBlocking)
			{
				getSocket()->setNonBlocking(false);
			}
			return E_TIMEDOUT;
		}
		msTimeOut=diff64(endTime,currentTime);
		
		if(ret==1)
		{
			ret=receive(buff+pos,len);								
			if(ret<=0)
			{
				if(ret==E_AGAIN)
				{
					//CAMsg::printMsg(LOG_DEBUG, "CAClientSocket:: Select\n");
					ret=oSG.select(msTimeOut);
					//CAMsg::printMsg(LOG_DEBUG, "CAClientSocket:: After select\n");
					continue;
				}
				else
				{
					if (!bWasNonBlocking)
					{
						getSocket()->setNonBlocking(false);
					}
					return E_UNKNOWN;
				}
			}
			pos+=ret;
			len-=ret;
		}
		else if(ret==E_TIMEDOUT)
		{
			if (!bWasNonBlocking)
			{
				getSocket()->setNonBlocking(false);
			}
			return E_TIMEDOUT;
		}

		if(len==0)
		{
			if (!bWasNonBlocking)
			{
				getSocket()->setNonBlocking(false);
			}
			return E_SUCCESS;
		}		
	}
}*/