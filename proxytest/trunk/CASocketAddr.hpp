#ifndef __CASOCKETADDR__
#define __CASOCKETADDR__
class CASocketAddr:public sockaddr_in
	{
		public:
			CASocketAddr();
			CASocketAddr(char* szIP,unsigned short port);
			CASocketAddr(unsigned short port);
			
			int setAddr(char* szIP,unsigned short port);
			operator LPSOCKADDR(){return (::LPSOCKADDR)this;}

	};

typedef CASocketAddr* LPSOCKETADDR;
#endif