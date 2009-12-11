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
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
#include "CASocketAddrUnix.hpp"


/** Constructs an address with an empty path*/
CASocketAddrUnix::CASocketAddrUnix()
	{
		sun_family=AF_LOCAL;
		memset(sun_path,0,sizeof(sun_path));
	}

/**Constructs an Unix Adress from an other Unix-Address */
CASocketAddrUnix::CASocketAddrUnix(const CASocketAddrUnix& addr)
	{
		sun_family=AF_LOCAL;
		memcpy(sun_path,addr.sun_path,sizeof(sun_path));
	}

/** Sets the path for the unix domain protocol address.
	* @param path the new path value (zero terminated)
	* @retval E_SUCCESS if no error occured
	* @retval E_UNSPECIFIED if path was NULL
	* @retval E_SPACE if path was to long
	*/
SINT32 CASocketAddrUnix::setPath(const char* path)
	{
		if(path==NULL)
			return E_UNSPECIFIED;
//		((sockaddr_un*)m_pAddr)->sun_len=strlen(path);
		if(strlen(path)>=sizeof(sun_path))
			return E_SPACE;
		strcpy(sun_path,path);
		return E_SUCCESS;
	}

/** Gets the path for the unix domain protocol address.
  * The returned char array has to be freed by the caller using delete[].
	* @retval NULL if path was no specified yet
	* @retval copy of the path value
	*/
UINT8* CASocketAddrUnix::getPath() const
	{
		UINT32 len=strlen(sun_path);
		if(len==0)
			return NULL;
		UINT8* p=new UINT8[len+1];
		strcpy((char*)p,sun_path);
		return p;
	}

#endif
