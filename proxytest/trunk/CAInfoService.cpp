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
#include "CAMsg.hpp"
#include "CASocketAddrINet.hpp"
#include "CAUtil.hpp"
extern CACmdLnOptions options;

/**
	This class implements the Output Stream interface required by XML::Output. 
	It stores all the data in a memory buffer.
*/
class BufferOutputStream:public XMLOutputStream
	{
		public:
			/** Constructs a new buffered Output Stream.
			@param initsize	the inital size of the buffer that holds the streamed data
							If initsize=0 then the default value is 1 kilo byte (so initsize=1024)
			@param grow the number of bytes by which the buffer is increased, if needed. 
									If grow=0 then the initial size is used (so grow=initsize)
			**/
			BufferOutputStream(UINT32 initsize=0,UINT32 grow=0)
				{
					if(initsize==0)
						initsize=1024;
					m_buffer=(UINT8*)malloc(initsize);
					if(m_buffer==NULL)
						m_size=0;
					else
						m_size=initsize;
					m_used=0;
					if(grow>0)
						m_grow=grow;
					else
						m_grow=initsize;
				}
			
			/** Releases the memory for the buffer.**/
			virtual ~BufferOutputStream()
				{
					if(m_buffer!=NULL)
						free(m_buffer);
				}

	/** write up to bufLen characters to an output source (memory buffer).
		@param buf		source buffer
		@param bufLen	number of characters to write
		@return 		the number of characters actually written - if this
						number is less than bufLen, there was an error (which is acctually E_UNKNOWN)
	*/			
			int write(const char *buf, size_t bufLen)
				{
					if(m_size-m_used<bufLen)
						{
							UINT8* tmp=(UINT8*)realloc(m_buffer,m_size+m_grow);
							if(tmp==NULL)
								{
									return E_UNKNOWN;	
								}
							m_size+=m_grow;
							m_buffer=tmp;
						}
					memcpy(m_buffer+m_used,buf,bufLen);
					m_used+=bufLen;
					return bufLen;
				}

			/** Gets the buffer.
			@return a pointer to the buffer which holds the already streamed data. May be NULL.
			*/
			UINT8* getBuff()
				{
					return m_buffer;
				}
			
			/** Gets the number of bytes which are stored in the output buffer.
			@return number of bytes writen to the stream.
			*/
			UINT32 getBufferSize()
				{
					return m_used;	
				}

			/** Resets the stream. All data is trashed and the next output will be writen to the beginn of the buffer.
			@return always E_SUCCESS
			*/
			SINT32 reset()
				{
					m_used=0;
					return E_SUCCESS;
				}

		private:
			UINT8* m_buffer; ///The buffer which stores the data
			UINT32 m_size; ///The size of the buffer in which the streaming data is stored 
			UINT32 m_used; ///The number of bytes already used of the buffer
			UINT32 m_grow; ///The number of bytes by which the buffer should grow if neccesary
	};

THREAD_RETURN InfoLoop(void *p)
	{
		CAInfoService* pInfoService=(CAInfoService*)p;
		CASocket oSocket;
		CASocketAddrINet oAddr;
		CASignature* pSignature=pInfoService->getSignature();
		UINT8* buff=new UINT8[1024];
		UINT8 buffHeader[255];
		options.getInfoServerHost(buff,1024);
		oAddr.setAddr(buff,options.getInfoServerPort());
		BufferOutputStream oBufferStream(1024,1024);
		XMLOutput oxmlOut(oBufferStream);
		SINT32 tmpUser,tmpRisk,tmpTraffic;
		UINT32 tmpPackets;
		UINT32 buffLen;
		UINT8 strAnonServer[255];
		if(options.getServerHost((UINT8*)strAnonServer,255)!=E_SUCCESS)
			CASocketAddrINet::getLocalHostIP(buff);
		else
			{
				CASocketAddrINet oAddr;
				oAddr.setAddr(strAnonServer,0);
				oAddr.getIP(buff);
			}
		sprintf((char*)strAnonServer,"%u.%u.%u.%u%%3A%u",buff[0],buff[1],buff[2],buff[3],options.getServerPort());
		int helocount=10;

		//Must be changed to UINT64 ....
		UINT32 avgTraffic=0;		
		UINT32 lastMixedPackets=0;
		UINT32 minuts=1;
		UINT32 diffTraffic;
		double dTmp;
		while(pInfoService->getRun())
			{
				if(oSocket.connect(oAddr)==E_SUCCESS)
					{
						oBufferStream.reset();
						oxmlOut.BeginDocument("1.0","UTF-8",true);
						oxmlOut.BeginElementAttrs("MixCascadeStatus");
						oxmlOut.WriteAttr("id",(char*)strAnonServer);
						tmpUser=tmpPackets=tmpRisk=tmpTraffic=-1;
						pInfoService->getLevel(&tmpUser,&tmpRisk,&tmpTraffic);
						pInfoService->getMixedPackets((UINT32*)&tmpPackets);
						avgTraffic=tmpPackets/minuts;
						minuts++;
						diffTraffic=tmpPackets-lastMixedPackets;
						if(avgTraffic==0)
							{
								if(diffTraffic==0)
									tmpTraffic=0;
								else
									tmpTraffic=100;
							}
						else
							{
								dTmp=(double)diffTraffic/(double)avgTraffic;
								tmpTraffic=min(SINT32(50.*dTmp),100);
							}
						lastMixedPackets=tmpPackets;
					
						oxmlOut.WriteAttr("nrOfActiveUsers",(int)tmpUser);
						oxmlOut.WriteAttr("currentRisk",(int)tmpRisk);
						oxmlOut.WriteAttr("trafficSituation",(int)tmpTraffic);
						oxmlOut.WriteAttr("mixedPackets",(int)tmpPackets);
						oxmlOut.EndAttrs();
						oxmlOut.EndElement();
						oxmlOut.EndDocument();
						buffLen=1024;
						pSignature->signXML(oBufferStream.getBuff(),oBufferStream.getBufferSize(),(UINT8*)buff,&buffLen);
						sprintf((char*)buffHeader,"POST /feedback HTTP/1.0\r\nContent-Length: %u\r\n\r\n",buffLen);
						oSocket.send(buffHeader,strlen((char*)buffHeader));
						oSocket.send(buff,buffLen);
					}
				oSocket.close();	
				if(helocount==0)
					{
						pInfoService->sendHelo();
						helocount=10;
					}
				else 
				 helocount--;
				sSleep(60);
			}
		delete []buff;
		THREAD_RETURN_SUCCESS;
	}

CAInfoService::CAInfoService(CAFirstMix* pFirstMix,UINT32 numberOfMixes)
	{
//		iUser=iRisk=iTraffic=-1;
		m_pFirstMix=pFirstMix;
		m_NumberOfMixes=numberOfMixes;
		m_bRun=false;
		m_pSignature=NULL;
	}

CAInfoService::~CAInfoService()
	{
		stop();
	}
/*
SINT32 CAInfoService::setLevel(SINT32 user,SINT32 risk,SINT32 traffic)
	{
		csLevel.lock();
		iUser=user;
		iRisk=risk;
		iTraffic=traffic;
		csLevel.unlock();
		return E_SUCCESS;
	}
*/
/*
SINT32 CAInfoService::setMixedPackets(UINT32 nPackets)
	{
		EnterCriticalSection(&csLevel);
		m_MixedPackets=nPackets;
		LeaveCriticalSection(&csLevel);
		return E_SUCCESS;
	}
*/

SINT32 CAInfoService::setSignature(CASignature* pSig)
	{
		m_pSignature=pSig;
		return E_SUCCESS;
	}

SINT32 CAInfoService::getLevel(SINT32* puser,SINT32* prisk,SINT32* ptraffic)
	{
/*		csLevel.lock();
		if(puser!=NULL)
			*puser=iUser;
		if(ptraffic!=NULL)
			*ptraffic=iTraffic;
		if(prisk!=NULL)
			*prisk=iRisk;
		csLevel.unlock();*/
		return m_pFirstMix->getLevel(puser,prisk,ptraffic);
	}

SINT32 CAInfoService::getMixedPackets(UINT32* ppackets)
	{
		//EnterCriticalSection(&csLevel);
		if(m_pFirstMix!=NULL)
			return m_pFirstMix->getMixedPackets(ppackets);
		return E_UNKNOWN;
		//LeaveCriticalSection(&csLevel);
		//return E_SUCCESS;
	}

SINT32 CAInfoService::start()
	{
		if(m_pSignature==NULL)
			return E_UNKNOWN;
		m_bRun=true;
		m_threadRunLoop.setMainLoop(InfoLoop);
		return m_threadRunLoop.start(this);
	}

SINT32 CAInfoService::stop()
	{
		if(m_bRun)
			{
				m_bRun=false;
				m_threadRunLoop.join();
			}
		return E_SUCCESS;
	}

SINT32 CAInfoService::sendHelo()
	{
		CASocket oSocket;
		CASocketAddrINet oAddr;
		UINT8 hostname[255];
		UINT8 id[50];
		UINT8 buffHeader[255];
		if(options.getInfoServerHost(hostname,255)!=E_SUCCESS)
			return E_UNKNOWN;
		oAddr.setAddr(hostname,options.getInfoServerPort());
		if(oSocket.connect(oAddr)==E_SUCCESS)
			{
				BufferOutputStream oBufferStream(1024,1024);
				UINT8* buff=new UINT8[1024];
				if(buff==NULL)
					return E_UNKNOWN;
				XMLOutput oxmlOut(oBufferStream);
				oBufferStream.reset();
				UINT buffLen;
				oxmlOut.BeginDocument("1.0","UTF-8",true);
				oxmlOut.BeginElementAttrs("MixCascade");
				
				if(options.getServerHost(hostname,255)!=E_SUCCESS)
					{
						CASocketAddrINet::getLocalHostIP(buff);
						CASocketAddrINet::getLocalHostName(hostname,255);
					}
				else
					{
						CASocketAddrINet oAddr;
						oAddr.setAddr(hostname,0);
						oAddr.getIP(buff);
					}
				sprintf((char*)id,"%u.%u.%u.%u%%3A%u",buff[0],buff[1],buff[2],buff[3],options.getServerPort());
				oxmlOut.WriteAttr("id",(char*)id);
				oxmlOut.EndAttrs();
				if(options.getCascadeName(buff,1024)!=E_SUCCESS)
					{
						delete []buff;
						return E_UNKNOWN;
					}
				oxmlOut.WriteElement("Name",(char*)buff);
				oxmlOut.WriteElement("IP",(char*)hostname);
				oxmlOut.WriteElement("Port",(int)options.getServerPort());
        if(options.getProxySupport())
        	oxmlOut.WriteElement("ProxyPort",(int)443);
				oxmlOut.BeginElementAttrs("Mixes");
				oxmlOut.WriteAttr("count",(int)m_NumberOfMixes);
				oxmlOut.EndAttrs();
				oxmlOut.EndElement();
				oxmlOut.EndElement();
				oxmlOut.EndDocument();
				buffLen=1024;
				if(m_pSignature->signXML(oBufferStream.getBuff(),oBufferStream.getBufferSize(),(UINT8*)buff,&buffLen)!=E_SUCCESS)
					{delete []buff;return E_UNKNOWN;}
				sprintf((char*)buffHeader,"POST /helo HTTP/1.0\r\nContent-Length: %u\r\n\r\n",buffLen);
				oSocket.send(buffHeader,strlen((char*)buffHeader));
				oSocket.send(buff,buffLen);
				oSocket.close();
				delete []buff;		
				return E_SUCCESS;	
/*_LABEL_ERROR:
				if(buff!=NULL)
					delete buff;
				return E_UNKNOWN;
*/			}
		return E_UNKNOWN;
	}

SINT32 CAInfoService::test()
	{
		BufferOutputStream oBufferStream(1024,1024);
		XMLOutput oxmlOut(oBufferStream);
		CASignature oSignature;
		UINT8 fileBuff[2048];
		int handle=open("G:/proxytest/Debug/privkey.xml",O_BINARY|O_RDONLY);
		UINT32 len=read(handle,fileBuff,2048);
		close(handle);
		oSignature.setSignKey(fileBuff,len,SIGKEY_XML);
		UINT8 buff[1024];
		UINT32 buffLen;
#ifdef _WIN32
		_CrtMemState s1, s2, s3;
		_CrtMemCheckpoint( &s1 );
#endif
		for(int i=0;i<1000;i++)
			{
				oBufferStream.reset();
				oxmlOut.BeginDocument("1.0","UTF-8",true);
				oxmlOut.BeginElementAttrs("MixCascadeStatus");
				oxmlOut.WriteAttr("id","errwrzteutuztuit");
				int tmpUser,tmpPackets,tmpRisk,tmpTraffic;
			
				oxmlOut.WriteAttr("nrOfActiveUsers",(int)tmpUser);
				oxmlOut.WriteAttr("currentRisk",(int)tmpRisk);
				oxmlOut.WriteAttr("trafficSituation",(int)tmpTraffic);
				oxmlOut.WriteAttr("mixedPackets",(int)tmpPackets);
				oxmlOut.EndAttrs();
				oxmlOut.BeginElementAttrs("Mixes");
				oxmlOut.WriteAttr("count",(int)3);
				oxmlOut.EndAttrs();
				oxmlOut.EndElement();
				oxmlOut.EndElement();
				oxmlOut.EndDocument();
				oSignature.signXML(oBufferStream.getBuff(),oBufferStream.getBufferSize(),(UINT8*)buff,&buffLen);
			}
#ifdef _WIN32
		_CrtMemCheckpoint( &s2 );
		if ( _CrtMemDifference( &s3, &s1, &s2 ) )
      _CrtMemDumpStatistics( &s3 );
#endif
		return E_SUCCESS;
	}
