#include "StdAfx.h"
#include "CAInfoService.hpp"
#include "CASocket.hpp"
#include "CASocketAddr.hpp"
#include "xml/xmloutput.h"
#include "CACmdLnOptions.hpp"
extern CACmdLnOptions options;

class BufferOutputStream:public XML::OutputStream
	{
		public:
			BufferOutputStream(UINT32 initsize,UINT32 g)
				{
					buffer=(UINT8*)malloc(initsize);
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
							buffer=(UINT8*)realloc(buffer,size);
						}
					memcpy(buffer+used,buf,bufLen);
					used+=bufLen;
					return bufLen;
				}

			UINT8* getBuff()
				{
					return buffer;
				}
			
			UINT32 getBufferSize()
				{
					return used;	
				}

			SINT32 reset()
				{
					used=0;
					return E_SUCCESS;
				}

		private:
			UINT8* buffer;
			UINT32 size;
			UINT32 used;
			UINT32 grow;
	};

THREAD_RETURN InfoLoop(void *p)
	{
		CAInfoService* pInfoService=(CAInfoService*)p;
		CASocket oSocket;
		CASocketAddr oAddr;
		CASignature* pSignature=pInfoService->getSignature();
		UINT8* buff=new UINT8[1024];
		options.getInfoServerHost(buff,1024);
		oAddr.setAddr((char*)buff,options.getInfoServerPort());
		BufferOutputStream oBufferStream(1024,1024);
		XML::Output oxmlOut(oBufferStream);
		UINT32 nUser,nRisk,nTraffic;
		UINT32 buffLen;
		char strAnonServer[255];
		CASocketAddr::getLocalHostName((UINT8*)buff,255);
		
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
						oxmlOut.WriteAttr("id",(char*)strAnonServer);
						pInfoService->getLevel(&nUser,&nRisk,&nTraffic);
						oxmlOut.WriteAttr("nrOfActiveUsers",(int)nUser);
						oxmlOut.WriteAttr("currentRisk",(int)nRisk);
						oxmlOut.WriteAttr("trafficSituation",(int)nTraffic);
						oxmlOut.EndAttrs();
						oxmlOut.EndElement();
						oxmlOut.EndDocument();
						buffLen=1024;
						pSignature->signXML(oBufferStream.getBuff(),oBufferStream.getBufferSize(),(UINT8*)buff,&buffLen);
						oSocket.send((UINT8*)"POST /feedback HTTP/1.0\r\n\r\n",27);
						oSocket.send((UINT8*)buff,buffLen);
					}
				oSocket.close();	
				if(helocount==0)
					{
						pInfoService->sendHelo();
						helocount=10;
					}
				else 
				 helocount--;
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

SINT32 CAInfoService::setLevel(UINT32 user,UINT32 risk,UINT32 traffic)
	{
		EnterCriticalSection(&csLevel);
		nUser=user;
		nRisk=risk;
		nTraffic=traffic;
		LeaveCriticalSection(&csLevel);
		return 0;
	}

SINT32 CAInfoService::setSignature(CASignature* pSig)
	{
		pSignature=pSig;
		return E_SUCCESS;
	}

SINT32 CAInfoService::getLevel(UINT32* puser,UINT32* prisk,UINT32* ptraffic)
	{
		EnterCriticalSection(&csLevel);
		if(puser!=NULL)
			*puser=nUser;
		if(ptraffic!=NULL)
			*ptraffic=nTraffic;
		if(prisk!=NULL)
			*prisk=nRisk;
		LeaveCriticalSection(&csLevel);
		return E_SUCCESS;
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

SINT32 CAInfoService::sendHelo()
	{
		CASocket oSocket;
		CASocketAddr oAddr;
		UINT8 hostname[255];
		if(options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			return E_UNKNOWN;
		oAddr.setAddr((char*)hostname,options.getInfoServerPort());
		if(oSocket.connect(&oAddr)==E_SUCCESS)
			{
				BufferOutputStream oBufferStream(1024,1024);
				char* buff=new char[1024];
				XML::Output oxmlOut(oBufferStream);
				oBufferStream.reset();
				UINT buffLen;
				oxmlOut.BeginDocument("1.0","UTF-8",true);
				oxmlOut.BeginElementAttrs("MixCascade");
				CASocketAddr::getLocalHostName((UINT8*)hostname,255);
//*>> Beginn very ugly hack for anon.inf.tu-dresden.de --> new Concepts needed!!!!!1		
		if(strncmp((char*)hostname,"ithif77",7)==0)
			strcpy((char*)hostname,"anon.inf.tu-dresden.de");
//end hack....
				sprintf(buff,"%s%%3A%u",hostname,options.getServerPort());
				oxmlOut.WriteAttr("id",(char*)buff);
				oxmlOut.EndAttrs();
				if(options.getCascadeName((UINT8*)buff,1024)!=E_SUCCESS)
					goto _LABEL_ERROR;
				oxmlOut.WriteElement("Name",(char*)buff);
				oxmlOut.WriteElement("IP",(char*)hostname);
				oxmlOut.WriteElement("Port",(int)options.getServerPort());
				oxmlOut.EndElement();
				oxmlOut.EndDocument();
				buffLen=1024;
				if(pSignature->signXML(oBufferStream.getBuff(),oBufferStream.getBufferSize(),(UINT8*)buff,&buffLen)!=E_SUCCESS)
					goto _LABEL_ERROR;
				oSocket.send((UINT8*)"POST /helo HTTP/1.0\r\n\r\n",23);
				oSocket.send((UINT8*)buff,buffLen);
				oSocket.close();
				delete buff;		
				return E_SUCCESS;	
_LABEL_ERROR:
				if(buff!=NULL)
					delete buff;
				return E_UNKNOWN;
			}
		return E_UNKNOWN;
	}


