#ifndef __CASOCKETADDR__
#define __CASOCKETADDR__
class CASocketAddr:public sockaddr_in
	{
		public:
			CASocketAddr();
			CASocketAddr(char* szIP,unsigned short port);
			CASocketAddr(unsigned short port);

			int setAddr(char* szIP,unsigned short port);
			unsigned short getPort();
			int getHostName(char* buff,int len);
			static int getLocalHostName(char* buff,int len);
			operator LPSOCKADDR(){return (::LPSOCKADDR)this;}

	};

typedef CASocketAddr* LPSOCKETADDR;
#endif
