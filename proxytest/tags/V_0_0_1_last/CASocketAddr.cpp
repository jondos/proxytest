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
#include "CASocketAddr.hpp"
CASocketAddr::CASocketAddr()
	{
		sin_family=AF_INET;
		sin_port=0;
		sin_addr.s_addr=0;
	}

CASocketAddr::CASocketAddr(char* szIP,UINT16 port)
	{
		setAddr(szIP,port);
	}

int CASocketAddr::setAddr(char* szIP,UINT16 port)
	{
		sin_family=AF_INET;
		sin_port=htons(port);
		sin_addr.s_addr=inet_addr(szIP);
		if(sin_addr.s_addr==INADDR_NONE)
			{
				HOSTENT* hostent=gethostbyname(szIP);
				if(hostent==NULL)
					sin_addr.s_addr=INADDR_NONE;
				else
					memcpy(&sin_addr.s_addr,hostent->h_addr_list[0],hostent->h_length);
			}
		return 0;
	}
			
CASocketAddr::CASocketAddr(UINT16 port)
	{
		sin_family=AF_INET;
		sin_port=htons(port);
		/*HOSTENT* hostent=gethostbyname("localhost");
		if(hostent==NULL)
			sin_addr.s_addr=0;
		else
			memcpy(&sin_addr.s_addr,hostent->h_addr_list[0],hostent->h_length);
	*/
		sin_addr.s_addr=INADDR_ANY;
	}

SINT32 CASocketAddr::getHostName(UINT8* buff,UINT32 len)
	{
		HOSTENT* hosten=gethostbyaddr((const char*)&sin_addr,4,AF_INET);
		if(hosten==NULL||hosten->h_name==NULL||strlen(hosten->h_name)>=len)
		 return SOCKET_ERROR;
		strcpy((char*)buff,hosten->h_name);
		return E_SUCCESS;
	}

UINT16 CASocketAddr::getPort()
	{
		return ntohs(sin_port);
	}

SINT32 CASocketAddr::getLocalHostName(UINT8* buff,UINT32 len)
	{
		if(gethostname((char*)buff,len)==-1)
			return SOCKET_ERROR;
		HOSTENT* hosten=gethostbyname((char*)buff);
		if(hosten==NULL||hosten->h_name==NULL||strlen(hosten->h_name)>=len)
			return SOCKET_ERROR;
		strcpy((char*)buff,hosten->h_name);
		return E_SUCCESS;
	}

SINT32 CASocketAddr::getLocalHostIP(UINT8* ip)
	{
		char buff[256];
		if(gethostname(buff,256)==-1)
			return SOCKET_ERROR;
		HOSTENT* hosten=gethostbyname((char*)buff);
		if(hosten==NULL)
			return SOCKET_ERROR;
		memcpy((char*)ip,hosten->h_addr_list[0],4);
		return E_SUCCESS;
	}