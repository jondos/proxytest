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
#include "StdAfx.h"
#include "CAInfoService.hpp"
#include "CASocket.hpp"
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
			
			virtual ~BufferOutputStream()
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
		SINT32 tmpUser,tmpRisk,tmpTraffic;
		UINT32 buffLen;
		char strAnonServer[255];
	//	CASocketAddr::getLocalHostName((UINT8*)buff,255);
		CASocketAddr::getLocalHostIP(buff);
//*>> Beginn very ugly hack for anon.inf.tu-dresden.de --> new Concepts needed!!!!!1		
		if(strncmp((char*)buff,"ithif46",7)==0)
			strcpy((char*)buff,"mix.inf.tu-dresden.de");
//end hack....
		sprintf(strAnonServer,"%u.%u.%u.%u%3A%u",buff[0],buff[1],buff[2],buff[3],options.getServerPort());
		int helocount=10;
		while(pInfoService->getRun())
			{
				if(oSocket.connect(&oAddr)==E_SUCCESS)
					{
						oBufferStream.reset();
						oxmlOut.BeginDocument("1.0","UTF-8",true);
						oxmlOut.BeginElementAttrs("MixCascadeStatus");
						oxmlOut.WriteAttr("id",(char*)strAnonServer);
						pInfoService->getLevel(&tmpUser,&tmpRisk,&tmpTraffic);
						oxmlOut.WriteAttr("nrOfActiveUsers",(int)tmpUser);
						oxmlOut.WriteAttr("currentRisk",(int)tmpRisk);
						oxmlOut.WriteAttr("trafficSituation",(int)tmpTraffic);
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
		iUser=iRisk=iTraffic=-1;
		bRun=false;
		pSignature=NULL;
	}

CAInfoService::~CAInfoService()
	{
		stop();
		DeleteCriticalSection(&csLevel);
	}

SINT32 CAInfoService::setLevel(SINT32 user,SINT32 risk,SINT32 traffic)
	{
		EnterCriticalSection(&csLevel);
		iUser=user;
		iRisk=risk;
		iTraffic=traffic;
		LeaveCriticalSection(&csLevel);
		return 0;
	}

SINT32 CAInfoService::setSignature(CASignature* pSig)
	{
		pSignature=pSig;
		return E_SUCCESS;
	}

SINT32 CAInfoService::getLevel(SINT32* puser,SINT32* prisk,SINT32* ptraffic)
	{
		EnterCriticalSection(&csLevel);
		if(puser!=NULL)
			*puser=iUser;
		if(ptraffic!=NULL)
			*ptraffic=iTraffic;
		if(prisk!=NULL)
			*prisk=iRisk;
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
		if(strncmp((char*)hostname,"ithif46",7)==0)
			strcpy((char*)hostname,"mix.inf.tu-dresden.de");
//end hack....
				sprintf(buff,"%s%%3A%u",hostname,options.getServerPort());
				oxmlOut.WriteAttr("id",(char*)buff);
				oxmlOut.EndAttrs();
				if(options.getCascadeName((UINT8*)buff,1024)!=E_SUCCESS)
					{if(buff!=NULL)delete buff;return E_UNKNOWN;}
				oxmlOut.WriteElement("Name",(char*)buff);
				oxmlOut.WriteElement("IP",(char*)hostname);
				oxmlOut.WriteElement("Port",(int)options.getServerPort());
				oxmlOut.EndElement();
				oxmlOut.EndDocument();
				buffLen=1024;
				if(pSignature->signXML(oBufferStream.getBuff(),oBufferStream.getBufferSize(),(UINT8*)buff,&buffLen)!=E_SUCCESS)
					{if(buff!=NULL)delete buff;return E_UNKNOWN;}
				oSocket.send((UINT8*)"POST /helo HTTP/1.0\r\n\r\n",23);
				oSocket.send((UINT8*)buff,buffLen);
				oSocket.close();
				delete buff;		
				return E_SUCCESS;	
/*_LABEL_ERROR:
				if(buff!=NULL)
					delete buff;
				return E_UNKNOWN;
*/			}
		return E_UNKNOWN;
	}


