#include "StdAfx.h"
#include "CASocketAddr.hpp"
CASocketAddr::CASocketAddr()
	{
		sin_family=AF_INET;
		sin_port=0;
		sin_addr.s_addr=0;
	}

CASocketAddr::CASocketAddr(char* szIP,unsigned short port)
	{
		setAddr(szIP,port);
	}

int CASocketAddr::setAddr(char* szIP,unsigned short port)
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
			
CASocketAddr::CASocketAddr(unsigned short port)
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