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
					UINT32 i=ntohl(m_nIPAddr);
					strcpy((char*)strIP,inet_ntoa(*(struct in_addr *)&i));
					i=ntohl(m_nNetmask);
					strcpy((char*)strNetmask,inet_ntoa(*(struct in_addr *)&i));
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

