#include "StdAfx.h"
#include "CAInfoService.hpp"
#include "CASocket.hpp"
#include "CASocketAddr.hpp"
#include "xml/xmloutput.h"
#include "CACmdLnOptions.hpp"
extern CACmdLnOptions options;
/*class SocketOutputStream:public XML::OutputStream,public CASocket
	{
		public:
			int write(const char *buf, size_t bufLen)
				{
					return send((char*)buf,bufLen);
				}
	};
*/
class BufferOutputStream:public XML::OutputStream
	{
		public:
			BufferOutputStream(unsigned int initsize,unsigned int g)
				{
					buffer=(char*)malloc(initsize);
					size=initsize;
					used=0;
					grow=g;
				}
			
			~BufferOutputStream()
				{
					free(buffer);
				}
			
			int write(const char *buf, size_t bufLen)
				{
					if(size-used<bufLen)
						{
							size+=grow;
							buffer=(char*)realloc(buffer,size);
						}
					memcpy(buffer+used,buf,bufLen);
					used+=bufLen;
					return bufLen;
				}

			char* getBuff()
				{
					return buffer;
				}
			
			unsigned int getBufferSize()
				{
					return used;	
				}

			int reset()
				{
					used=0;
					return 0;
				}

		private:
			char* buffer;
			unsigned int size;
			unsigned int used;
			unsigned int grow;
	};

THREAD_RETURN InfoLoop(void *p)
	{
		CAInfoService* pInfoService=(CAInfoService*)p;
		CASocket oSocket;
		CASocketAddr oAddr;
		CASignature* pSignature=pInfoService->getSignature();
		char* buff=new char[1024];
		options.getInfoServerHost(buff,1024);
		oAddr.setAddr(buff,options.getInfoServerPort());
		BufferOutputStream oBufferStream(1024,1024);
		XML::Output oxmlOut(oBufferStream);
		int nUser,nRisk,nTraffic;
		unsigned int buffLen;
		char strAnonServer[255];
		CASocketAddr::getLocalHostName(buff,255);
		
//*>> Beginn very ugly hack for anon.inf.tu-dresden.de --> new Concepts needed!!!!!1		
		if(strncmp(strAnonServer,"ithif77",7)==0)
			strcpy(strAnonServer,"anon.inf.tu-dresden.de");
//end hack....
		sprintf(strAnonServer,"%s%%3A%u",buff,options.getServerPort());
		int helocount=10;
		while(pInfoService->getRun())
			{
				if(oSocket.connect(&oAddr)==E_SUCCESS)
					{
						oBufferStream.reset();
						oxmlOut.BeginDocument("1.0","UTF-8",true);
						oxmlOut.BeginElementAttrs("MixCascadeStatus");
						oxmlOut.WriteAttr("id",strAnonServer);
						pInfoService->getLevel(&nUser,&nRisk,&nTraffic);
						oxmlOut.WriteAttr("nrOfActiveUsers",nUser);
						oxmlOut.WriteAttr("currentRisk",nRisk);
						oxmlOut.WriteAttr("trafficSituation",nTraffic);
						oxmlOut.EndAttrs();
						oxmlOut.EndElement();
						oxmlOut.EndDocument();
						buffLen=1024;
						pSignature->signXML(oBufferStream.getBuff(),oBufferStream.getBufferSize(),buff,&buffLen);
						oSocket.send("POST /feedback HTTP/1.0\r\n\r\n",27);
						oSocket.send(buff,buffLen);
					}
				oSocket.close();	
				if(helocount==0)
					{
						pInfoService->sendHelo();
						helocount=10;
					}
				sleep(60);
			}
		delete buff;
		THREAD_RETURN_SUCCESS;
	}

CAInfoService::CAInfoService()
	{
		InitializeCriticalSection(&csLevel);
		nUser=nRisk=nTraffic=-1;
		bRun=false;
		pSignature=NULL;
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

int CAInfoService::setSignature(CASignature* pSig)
	{
		pSignature=pSig;
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
		if(pSignature==NULL)
			return -1;
		bRun=true;
		#ifdef _WIN32
		 _beginthread(InfoLoop,0,this);
		#else
		 pthread_t othread;
		 pthread_create(&othread,NULL,InfoLoop,this);
		#endif
		return 0;
	}

int CAInfoService::stop()
	{
		bRun=false;
		return 0;
	}

int CAInfoService::sendHelo()
	{
		CASocket oSocket;
		CASocketAddr oAddr;
		char hostname[255];
		options.getInfoServerHost(hostname,255);
		oAddr.setAddr(hostname,options.getInfoServerPort());
		if(oSocket.connect(&oAddr)==E_SUCCESS)
			{
				BufferOutputStream oBufferStream(1024,1024);
				char* buff=new char[1024];
				XML::Output oxmlOut(oBufferStream);
				oBufferStream.reset();
				unsigned int buffLen;
				oxmlOut.BeginDocument("1.0","UTF-8",true);
				oxmlOut.BeginElementAttrs("MixCascade");
				CASocketAddr::getLocalHostName(hostname,255);
//*>> Beginn very ugly hack for anon.inf.tu-dresden.de --> new Concepts needed!!!!!1		
		if(strncmp(hostname,"ithif77",7)==0)
			strcpy(hostname,"anon.inf.tu-dresden.de");
//end hack....
				sprintf(buff,"%s%%3A%u",hostname,options.getServerPort());
				oxmlOut.WriteAttr("id",buff);
				oxmlOut.EndAttrs();
				options.getCascadeName(buff,1024);
				oxmlOut.WriteElement("Name",buff);
				oxmlOut.WriteElement("IP",hostname);
				oxmlOut.WriteElement("Port",options.getServerPort());
				oxmlOut.EndElement();
				oxmlOut.EndDocument();
				buffLen=1024;
				pSignature->signXML(oBufferStream.getBuff(),oBufferStream.getBufferSize(),buff,&buffLen);
				oSocket.send("POST /helo HTTP/1.0\r\n\r\n",23);
				oSocket.send(buff,buffLen);
				oSocket.close();
				delete buff;		
				return 0;				
			}
		return -1;
	}


