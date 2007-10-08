#include "../StdAfx.h"
#ifdef POOLSPEEDTEST
#include "../CASocket.hpp"
#include "../CASocketGroup.hpp"
#include "../CAUtil.hpp"
#include "../CACmdLnOptions.hpp"
#include "../CASingleSocketGroup.hpp"
CACmdLnOptions options;
#ifdef _DEBUG
int sockets;
#endif
int main()
{
	printf("Waiting for 1000 (dead) connections...");
	#ifdef _WIN32
		int err=0;
		WSADATA wsadata;
		err=WSAStartup(0x0202,&wsadata);
	#endif
	CASocket oSocketAccept;
	CASocketGroup oGroup(false);
	//CASingleSocketGroup oGroup(false);
	oSocketAccept.create();
	oSocketAccept.setReuseAddr(true);
	oSocketAccept.listen(5000);
	for(int i=0;i<1;i++)
	{
		CASocket* pTmp=new CASocket();
		if(oSocketAccept.accept(*pTmp)!=E_SUCCESS)
		{
			printf("accept failure\n");
			exit(0);
		}
				oGroup.add(*pTmp);
				if(oGroup.remove(*pTmp)!=E_SUCCESS)
				printf("Error in remove!\n");
	}

	printf("Start pool speed test\n");
	for(int i=0;i<10;i++)
	{
		printf("Test: %u ",i);
		UINT64 current,end;
		getcurrentTimeMillis(current);
		for(int j=0;j<1000000;j++)
		{
			oGroup.select(0);
		}
		getcurrentTimeMillis(end);
		printf("takes %u ms\n",diff64(end,current));
	}
	return 0;
}
#endif
