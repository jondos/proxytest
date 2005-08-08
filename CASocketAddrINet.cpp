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

CAMutex* CASocketAddrINet::m_pcsGet=new CAMutex();

/** Must be called once before using one of the CAsocketAddrINet functions */
/*SINT32 CASocketAddrINet::init()
	{
		if(!m_bIsCsInitialized)
			{
				m_bIsCsInitialized=true;
			}
		return E_SUCCESS;
	}
*/
/** Should be called if CASocketAddrINEt functions are not longer needed. If needed again
       init() must be called */
/*SINT32 CASocketAddrINet::destroy()
	{
		if(m_bIsCsInitialized)
			{
				DeleteCriticalSection(&m_csGet);
			}
		m_bIsCsInitialized=false;
		return E_SUCCESS;
	}
*/

/**Constructs a IP-Address for port 0 and ANY local host ip*/
CASocketAddrINet::CASocketAddrINet()
	{
		memset((SOCKADDR*)LPSOCKADDR(),0,getSize());
		sin_family=AF_INET;
		sin_addr.s_addr=INADDR_ANY;
		sin_port=0;
	}

/**Constructs an IP-Address for port and ANY local host ip */
CASocketAddrINet::CASocketAddrINet(UINT16 port)
	{
		memset((SOCKADDR*)LPSOCKADDR(),0,getSize());
		sin_family=AF_INET;
		sin_port=htons(port);
		sin_addr.s_addr=INADDR_ANY;
	}

/**Constructs an IP-Address from an other IP Adress */
CASocketAddrINet::CASocketAddrINet(const CASocketAddrINet& addr)
	{
		memset((SOCKADDR*)LPSOCKADDR(),0,getSize());
		sin_family=AF_INET;
		sin_port=addr.sin_port;
		sin_addr.s_addr=addr.sin_addr.s_addr;
	}

/** Sets the address to szIP and port. szIP could be either a hostname or an
  * IP-Address of the form a.b.c.d . If szIP==NULL, the the IP-Adredress ist set
	* to ANY local IP Address
	* @param szIP new value for IP-Address or hostname (zero terminated string)
	* @param port new value for port
	* @retval E_SUCCESS if no error occurs
	* @retval E_UNKNOWN_HOST if the hostname couldt not be resolved (or the ip is wrong).
	*                        In this case the old values are NOT changed.
	*/
SINT32 CASocketAddrINet::setAddr(const UINT8* szIP,UINT16 port)
	{
		UINT32 newAddr=INADDR_ANY;
		if(szIP!=NULL)
			{
				newAddr=inet_addr((const char*)szIP); //is it a doted string (a.b.c.d) ?
				if(newAddr==INADDR_NONE) //if not try to find the hostname
					{
						m_pcsGet->lock();
						HOSTENT* hostent=gethostbyname((const char*)szIP); //lookup
						if(hostent!=NULL) //get it!
							memcpy(&newAddr,hostent->h_addr_list[0],hostent->h_length);
						else
							{
								m_pcsGet->unlock();
								return E_UNKNOWN_HOST; //not found!
							}
						m_pcsGet->unlock();
					}
			}
		sin_addr.s_addr=newAddr;
		sin_port=htons(port);
		return E_SUCCESS;
	}

/**Changes only(!) the port value of the address
	*
	* @param port new value for port
	*	@return always E_SUCCESS
	*/
SINT32 CASocketAddrINet::setPort(UINT16 port)
	{
		sin_port=htons(port);
		return E_SUCCESS;
	}

/** Returns the port value of the address.
	* @return port
	*/
UINT16 CASocketAddrINet::getPort() const
	{
		return ntohs(sin_port);
	}

/** Returns the hostname for this address.
	* @param buff buffer for the returned zero terminated hostname
	* @param len the size of the buffer
	* @retval E_SUCCESS if no error occured
	* @retval E_UNKNOWN_HOST if the name of the host could not be resolved
	* @retval E_UNSPECIFIED if buff was NULL
	* @retval E_SPACE if size of the buffer is to small
	*/
SINT32 CASocketAddrINet::getHostName(UINT8* buff,UINT32 len) const
	{
		if(buff==NULL)
			return E_UNSPECIFIED;
		SINT32 ret;
		m_pcsGet->lock();
		HOSTENT* hosten=gethostbyaddr((const char*)&sin_addr,4,AF_INET);
		if(hosten==NULL||hosten->h_name==NULL)
			ret=E_UNKNOWN_HOST;
		else if(strlen(hosten->h_name)>=len)
			ret=E_SPACE;
		else
			{
				strcpy((char*)buff,hosten->h_name);
				ret=E_SUCCESS;
			}
		m_pcsGet->unlock();
		return ret;
	}

/** Returns the IP-Numbers for this address.
	* @param buff buffer for the returned IP-Address (4 Bytes)
	* @retval E_SUCCESS if no error occured
	*/
SINT32 CASocketAddrINet::getIP(UINT8 buff[4]) const
	{
		memcpy(buff,&sin_addr.s_addr,4);
		return E_SUCCESS;
	}

/** Sets the IP-Numbers for this address.
	* @param ip buffer with the IP-Address (4 Bytes)
	* @retval E_SUCCESS if no error occured
	*/
SINT32 CASocketAddrINet::setIP(UINT8 ip[4])
	{
		memcpy(&sin_addr.s_addr,ip,4);
		return E_SUCCESS;
	}

/** Returns the IP-Number as an address string (doted-format).
	* @param buff buffer for the returned IP-Address
	* @param len buffer-space
	* @retval E_SUCCESS if no error occured
	*/
SINT32 CASocketAddrINet::getIPAsStr(UINT8* buff,UINT32 len) const
	{
		if(buff==NULL)
			return E_UNKNOWN;
		UINT8 ip[4];
		if(getIP(ip)!=E_SUCCESS)
			return E_UNKNOWN;
		char* strAddr=inet_ntoa(sin_addr);
		if(strAddr==NULL)
			return E_UNKNOWN;
		if(strlen(strAddr)>=len)
			return E_UNKNOWN;
		strcpy((char*)buff,strAddr);
		return E_SUCCESS;
	}

/** Returns the name of the local host.
	* @param buff buffer for the returned zero terminated hostname
	* @param len the size of the buffer
	* @retval E_SUCCESS if no error occured
	* @retval E_UNKNOWN_HOST if the name of the host could not be resolved
	* @retval E_UNSPECIFIED if buff was NULL
	* @retval E_SPACE if size of the buffer is to small
	*/
SINT32 CASocketAddrINet::getLocalHostName(UINT8* buff,UINT32 len)
	{
		if(buff==NULL)
			return E_UNSPECIFIED;
		SINT32 ret;
		m_pcsGet->lock();
		if(gethostname((char*)buff,len)==-1)
			ret=E_SPACE;
		else
			{
				HOSTENT* hosten=gethostbyname((char*)buff);
				if(hosten==NULL||hosten->h_name==NULL)
					ret=E_UNKNOWN_HOST;
				else if(strlen(hosten->h_name)>=len)
					ret=E_SPACE;
				else
					{
						strcpy((char*)buff,hosten->h_name);
						ret=E_SUCCESS;
					}
			}
		m_pcsGet->unlock();
		return ret;
	}

/** Returns the local IP-Address as four bytes.
	* @param ip buffer for the returned IP-Address
	* @retval E_SUCCESS if no error occurs
	* @retval E_UNKNOWN in case of an error
	*/
SINT32 CASocketAddrINet::getLocalHostIP(UINT8 ip[4])
	{
		SINT32 ret;
		char buff[256];
		m_pcsGet->lock();
		if(gethostname(buff,256)==-1)
			ret=E_UNKNOWN;
		else
			{
				HOSTENT* hosten=gethostbyname((char*)buff);
				if(hosten==NULL)
					ret=E_UNKNOWN;
				else
					{
						memcpy((char*)ip,hosten->h_addr_list[0],4);
						ret=E_SUCCESS;
					}
			}
		m_pcsGet->unlock();
		return ret;
	}

