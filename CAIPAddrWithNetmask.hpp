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
#pragma once
class CAIPAddrWithNetmask
	{
		public:
			CAIPAddrWithNetmask()
				{
					m_nIPAddr=0;
					m_nNetmask=0xFFFFFFFF;
					m_nIPAddrAndNetmask=0;
				}

			CAIPAddrWithNetmask(const UINT8* strIPAddr,const UINT8* strNetmask)
				{
					m_nIPAddr=htonl(inet_addr((const char*)strIPAddr));
					m_nNetmask=htonl(inet_addr((const char*)strNetmask));
					m_nIPAddrAndNetmask=m_nIPAddr&m_nNetmask;
				}

			~CAIPAddrWithNetmask(void);

			SINT32 setAddr(const UINT8* strIPAddr)
				{
					m_nIPAddr=htonl(inet_addr((const char*)strIPAddr));
					m_nIPAddrAndNetmask=(m_nIPAddr&m_nNetmask);
					return E_SUCCESS;
				}

			SINT32 setNetmask(const UINT8* strNetmask)
				{
					m_nNetmask=htonl(inet_addr((const char*)strNetmask));
					m_nIPAddrAndNetmask=(m_nIPAddr&m_nNetmask);
					return E_SUCCESS;
				}
		
			//Note IP-Addr must be in network byte order!
			bool equals(UINT32 nIPAddr) const
				{
					return m_nIPAddrAndNetmask==(nIPAddr&m_nNetmask); 
				}
			
			bool equals(const UINT8* pIPAddr) const
				{
					UINT32 nIPAddr=pIPAddr[0];
					nIPAddr<<=8;
					nIPAddr|=pIPAddr[1];
					nIPAddr<<=8;
					nIPAddr|=pIPAddr[2];
					nIPAddr<<=8;
					nIPAddr|=pIPAddr[3];
					return m_nIPAddrAndNetmask==(nIPAddr&m_nNetmask); 
				}

			SINT32 toString(UINT8* buff,UINT32* buffLen)
				{
					UINT8 strIP[255];
					UINT8 strNetmask[255];
					struct in_addr addr;
					addr.s_addr=ntohl(m_nIPAddr);
					strcpy((char*)strIP,inet_ntoa(addr));
					addr.s_addr=ntohl(m_nNetmask);
					strcpy((char*)strNetmask,inet_ntoa(addr));
					SINT32 ret=snprintf((char*)buff,*buffLen,"%s/%s",strIP,strNetmask);
					if(ret<=0||((UINT32)ret)>=*buffLen)
						{
							buff[0]=0;
							*buffLen=0;
							return E_UNKNOWN;
						}
					*buffLen=ret;
					return E_SUCCESS;
				}
		
		private:
			UINT32 m_nIPAddr;
			UINT32 m_nNetmask;
			UINT32 m_nIPAddrAndNetmask;
	};

