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
#include "CASocketAddrINet.hpp"

bool CASocketAddrINet::bIsCsInitialized=false;
CRITICAL_SECTION CASocketAddrINet::csGet;

CASocketAddrINet::CASocketAddrINet()
	{
		sin_family=AF_INET;
		sin_addr.s_addr=INADDR_ANY;
		sin_port=0;
	}

//CASocketAddrINet::~CASocketAddrINet()
//	{
//	}


CASocketAddrINet::CASocketAddrINet(char* szIP,UINT16 port)
	{
		setAddr(szIP,port);
	}

SINT32 CASocketAddrINet::init()
	{
		if(!bIsCsInitialized)
			{
				InitializeCriticalSection(&csGet);
				bIsCsInitialized=true;
			}
		return E_SUCCESS;
	}

SINT32 CASocketAddrINet::destroy()
	{
		if(bIsCsInitialized)
			{
				DeleteCriticalSection(&csGet);
			}
		return E_SUCCESS;
	}


SINT32 CASocketAddrINet::getSize()
	{
		return sizeof(sockaddr_in);
	}

SINT32 CASocketAddrINet::setAddr(char* szIP,UINT16 port)
	{
		sin_family=AF_INET;
		sin_port=htons(port);
		sin_addr.s_addr=inet_addr(szIP);
		if(sin_addr.s_addr==INADDR_NONE)
			{
				EnterCriticalSection(&csGet);
				HOSTENT* hostent=gethostbyname(szIP);
				if(hostent==NULL)
					sin_addr.s_addr=INADDR_NONE;
				else
					memcpy(&sin_addr.s_addr,hostent->h_addr_list[0],hostent->h_length);
				LeaveCriticalSection(&csGet);
			}
		return E_SUCCESS;
	}

SINT32 CASocketAddrINet::setPort(UINT16 port)
	{
		sin_port=htons(port);
		return E_SUCCESS;
	}

CASocketAddrINet::CASocketAddrINet(UINT16 port)
	{
		sin_family=AF_INET;
		sin_port=htons(port);
		sin_addr.s_addr=INADDR_ANY;
	}

SINT32 CASocketAddrINet::getHostName(UINT8* buff,UINT32 len)
	{
		SINT32 ret;
		EnterCriticalSection(&csGet);
		HOSTENT* hosten=gethostbyaddr((const char*)&sin_addr,4,AF_INET);
		if(hosten==NULL||hosten->h_name==NULL||strlen(hosten->h_name)>=len)
		 ret=SOCKET_ERROR;
		else
			{
				strcpy((char*)buff,hosten->h_name);
				ret=E_SUCCESS;
			}
		LeaveCriticalSection(&csGet);
		return ret;
	}

UINT16 CASocketAddrINet::getPort()
	{
		return ntohs(sin_port);
	}

SINT32 CASocketAddrINet::getLocalHostName(UINT8* buff,UINT32 len)
	{
		SINT32 ret;
		EnterCriticalSection(&csGet);
		if(gethostname((char*)buff,len)==-1)
			ret=SOCKET_ERROR;
		else
			{
				HOSTENT* hosten=gethostbyname((char*)buff);
				if(hosten==NULL||hosten->h_name==NULL||strlen(hosten->h_name)>=len)
					ret=SOCKET_ERROR;
				else
					{
						strcpy((char*)buff,hosten->h_name);
						ret=E_SUCCESS;
					}
			}
		LeaveCriticalSection(&csGet);
		return ret;
	}

SINT32 CASocketAddrINet::getLocalHostIP(UINT8* ip)
	{
		SINT32 ret;
		char buff[256];
		EnterCriticalSection(&csGet);
		if(gethostname(buff,256)==-1)
			ret=SOCKET_ERROR;
		else
			{
				HOSTENT* hosten=gethostbyname((char*)buff);
				if(hosten==NULL)
					ret=SOCKET_ERROR;
				else
					{
						memcpy((char*)ip,hosten->h_addr_list[0],4);
						ret=E_SUCCESS;
					}
			}
		LeaveCriticalSection(&csGet);
		return ret;
	}

