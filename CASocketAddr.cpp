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
					sin_addr.s_addr=0;
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

int CASocketAddr::getHostName(char* buff,int len)
	{
		HOSTENT* hosten=gethostbyaddr((const char*)&sin_addr,4,AF_INET);
		strcpy(buff,hosten->h_name);
		return 0;
	}

unsigned short CASocketAddr::getPort()
	{
		return ntohs(sin_port);
	}