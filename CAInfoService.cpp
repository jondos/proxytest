#include "StdAfx.h"
#include "CAInfoService.hpp"
#include "CASocket.hpp"
#include "CASocketAddr.hpp"
#include "xml/xmloutput.h"
#include "CACmdLnOptions.hpp"
extern CACmdLnOptions options;
class SocketOutputStream:public XML::OutputStream,public CASocket
	{
		public:
			int write(const char *buf, size_t bufLen)
				{
					return send((char*)buf,bufLen);
				}

	};
THREAD_RETURN InfoLoop(void *p)
	{
		CAInfoService* pInfoService=(CAInfoService*)p;
		SocketOutputStream oSocket;
		CASocketAddr oAddr;
		char* buff=new char[255];
		options.getInfoServerHost(buff,255);
		oAddr.setAddr(buff,options.getInfoServerPort());
		delete buff;
		XML::Output oxmlOut(oSocket);
		int nUser,nRisk,nTraffic;
		while(pInfoService->getRun())
			{
				if(oSocket.connect(&oAddr)==E_SUCCESS)
					{
						oSocket.send("POST /feedback HTTP/1.0\r\n\r\n",27);
						oxmlOut.BeginDocument();
						oxmlOut.BeginElementAttrs("status");
						oxmlOut.WriteAttr("anonServer","anon.inf.tu-dresden.de%3A6543");
						pInfoService->getLevel(&nUser,&nRisk,&nTraffic);
						oxmlOut.WriteAttr("nrOfActiveUsers",nUser);
						oxmlOut.WriteAttr("currentRisk",nRisk);
						oxmlOut.WriteAttr("trafficSituation",nTraffic);
						oxmlOut.EndAttrs();
						oxmlOut.EndElement();
						oxmlOut.EndDocument();
						oSocket.close();
					}
				sleep(60);
			}
	}

CAInfoService::CAInfoService()
	{
		InitializeCriticalSection(&csLevel);
		nUser=nRisk=nTraffic=-1;
		bRun=false;
	}

CAInfoService::~CAInfoService()
	{
		stop();
		DeleteCriticalSection(&csLevel);
	}

int CAInfoService::setLevel(int user,int risk,int traffic)
	{
		EnterCriticalSection(&csLevel);
		nUser=user;
		nRisk=risk;
		nTraffic=traffic;
		LeaveCriticalSection(&csLevel);
		return 0;
	}

int CAInfoService::getLevel(int* puser,int* prisk,int* ptraffic)
	{
		EnterCriticalSection(&csLevel);
		if(puser!=NULL)
			*puser=nUser;
		if(ptraffic!=NULL)
			*ptraffic=nTraffic;
		if(prisk!=NULL)
			*prisk=nRisk;
		LeaveCriticalSection(&csLevel);
		return 0;
	}

int CAInfoService::start()
	{
		bRun=true;
		_beginthread(InfoLoop,0,this);
		return 0;
	}

int CAInfoService::stop()
	{
		bRun=false;
		return 0;
	}



