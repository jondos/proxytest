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
class CASocketAddrINet:public CASocketAddr,private sockaddr_in
	{
		public:
			//static SINT32 init();
			//static SINT32 destroy();
			int getType(){return AF_INET;}
			CASocketAddrINet();
			CASocketAddrINet(UINT16 port);

			/** Makes a cast to struct SOCKADDR* */
			::LPSOCKADDR LPSOCKADDR(){return (::LPSOCKADDR)(static_cast<sockaddr_in*>(this));}			

			/** Returns the Size of the SOCKADDR struct used.
				* @return sizeof(sockaddr_in)
				*/
			SINT32 getSize()
				{
					return sizeof(sockaddr_in);
				}
			
			SINT32 setAddr(const UINT8* szIP,UINT16 port);
      SINT32 setPort(UINT16 port);
      UINT16 getPort();
			SINT32 getHostName(UINT8* buff,UINT32 len);
			SINT32 getIP(UINT8 buff[4]);
			SINT32 getIPAsStr(UINT8* buff,UINT32 len);
			static SINT32 getLocalHostName(UINT8* buff,UINT32 len);
			static SINT32 getLocalHostIP(UINT8 ip[4]);
//			operator LPSOCKADDR(){return (::LPSOCKADDR)m_pAddr;}


		private:
			static CAMutex m_csGet;
			static bool m_bIsCsInitialized;
	};

#endif
