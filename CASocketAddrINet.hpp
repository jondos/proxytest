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
#ifndef __CASOCKETADDRINET__
#define __CASOCKETADDRINET__
#include "CASocketAddr.hpp"
#include "CAMutex.hpp"
/** This class represents a socket address for Internet (IP) connections. */

class CASocketAddrINet:private sockaddr_in,public CASocketAddr
	{
		public:
			static SINT32 init(){m_pcsGet=new CAMutex();return E_SUCCESS;}
			static SINT32 cleanup(){delete m_pcsGet;return E_SUCCESS;}
			//static SINT32 destroy();
			SINT32 getType()const
				{
					return AF_INET;
				}
			CASocketAddrINet();
			CASocketAddrINet(UINT16 port);

			CASocketAddrINet(const CASocketAddrINet& addr);

			CASocketAddr* clone() const
				{
					return new CASocketAddrINet(*this);
				}

			/** Makes a cast to struct SOCKADDR* */
			const SOCKADDR* LPSOCKADDR()const
				{
					#if defined(_WIN32) &&!defined(MSC_VER) //for Borland C++ under Windows
					  return (const ::LPSOCKADDR)(sockaddr_in*)this;
					#else
						return (const ::LPSOCKADDR)(static_cast<const sockaddr_in* const>(this));
					#endif
				}

			/** Returns the Size of the SOCKADDR struct used.
				* @return sizeof(sockaddr_in)
				*/
			SINT32 getSize() const
				{
					return sizeof(sockaddr_in);
				}

			SINT32 setAddr(const UINT8* szIP,UINT16 port);
      SINT32 setIP(UINT8 ip[4]);
			SINT32 setPort(UINT16 port);
      UINT16 getPort() const;
			SINT32 getHostName(UINT8* buff,UINT32 len)const;
			SINT32 getIP(UINT8 buff[4]) const;
			SINT32 getIPAsStr(UINT8* buff,UINT32 len) const;
			bool	 isAnyIP()
				{
					return sin_addr.s_addr==INADDR_ANY;
				}
			static SINT32 getLocalHostName(UINT8* buff,UINT32 len);
			static SINT32 getLocalHostIP(UINT8 ip[4]);
//			operator LPSOCKADDR(){return (::LPSOCKADDR)m_pAddr;}

			/** Returns a human readable representation of this address.
				*
				* @param buff buffer which stores the address string
				* @param bufflen size of the buffer
				* @retval E_SPACE if the buffer is to small
				* @retval E_UNKNOWN if an error occured
				* @retval E_SUCCESS if successfull
				*/
			virtual SINT32 toString(UINT8* buff,UINT32 bufflen) const
				{
					UINT8 tmpbuff[255];
					if(getIPAsStr(tmpbuff,255)!=E_SUCCESS)
						return E_UNKNOWN;
					if(snprintf((char*)buff,bufflen,"INet address: %s:%u",tmpbuff,getPort())<0)
						return E_SPACE;
					return E_SUCCESS;
				}
		private:
			static CAMutex* m_pcsGet;
	};

#endif
