#include "../StdAfx.h"
#include "../CASocket.hpp"
#include "../CASocketGroup.hpp"
#include "../CAUtil.hpp"
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
	oSocketAccept.create();
	oSocketAccept.listen(5000);
	for(int i=0;i<1000;i++)
	{
		CASocket* pTmp=new CASocket();
		if(oSocketAccept.accept(*pTmp)!=E_SUCCESS)
		{
			printf("accept failure\n");
			exit(0);
		}
		oGroup.add(*pTmp);
	}

	printf("Start pool speed test\n");
	for(int i=0;i<10;i++)
	{
		printf("Test: %u ",i);
		UINT64 current,end;
		getcurrentTimeMillis(current);
		for(int j=0;j<100000;j++)
		{
			oGroup.select(0);
		}
		getcurrentTimeMillis(end);
		printf("takes %u ms\n",diff64(end,current));
	}
	return 0;
}