#ifndef __CASOCKETADDR__
#define __CASOCKETADDR__
class CASocketAddr:public sockaddr_in
	{
		public:
			CASocketAddr();
			CASocketAddr(char* szIP,UINT16 port);
			CASocketAddr(UINT16 port);

			int setAddr(char* szIP,UINT16 port);
			UINT16 getPort();
			SINT32 getHostName(UINT8* buff,UINT32 len);
			static SINT32 getLocalHostName(UINT8* buff,UINT32 len);
			operator LPSOCKADDR(){return (::LPSOCKADDR)this;}

	};

typedef CASocketAddr* LPSOCKETADDR;
#endif
