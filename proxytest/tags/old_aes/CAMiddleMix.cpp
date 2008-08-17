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
#include "CAMiddleMix.hpp"
#include "CASocketList.hpp"
#include "CASocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocketAddrUnix.hpp"

extern CACmdLnOptions options;

SINT32 CAMiddleMix::init()
	{		
		CAMsg::printMsg(LOG_INFO,"Creating Key...\n");
		if(m_RSA.generateKeyPair(1024)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Init: Error generating Key-Pair...\n");
				return E_UNKNOWN;		
			}
		
		UINT8 strTarget[255];
		UINT8 path[255];
		CASocketAddr* pAddrNext;
		options.getTargetHost(strTarget,255);
		if(strTarget[0]=='/') //unix domain
			{
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
				pAddrNext=new CASocketAddrUnix();
				((CASocketAddrUnix*)pAddrNext)->setPath((char*)strTarget);
#else
				CAMsg::printMsg(LOG_CRIT,"I do not understand the Unix Domain Protocol!\n");
				return E_UNKNOWN;
#endif
			}
		else
			{
				pAddrNext=new CASocketAddrINet();
				((CASocketAddrINet*)pAddrNext)->setAddr((char*)strTarget,options.getTargetPort());
			}

		
		
		if(((CASocket*)m_MuxOut)->create(pAddrNext->getType())!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Init: Cannot create SOCKET for outgoing conncetion...\n");
				return E_UNKNOWN;
			}
		((CASocket*)m_MuxOut)->setRecvBuff(50*MIXPACKET_SIZE);
		((CASocket*)m_MuxOut)->setSendBuff(50*MIXPACKET_SIZE);
#define RETRIES 100
#define RETRYTIME 30
		CAMsg::printMsg(LOG_INFO,"Init: Try to connect to next Mix: %s...\n",strTarget);
		if(m_MuxOut.connect(*pAddrNext,RETRIES,RETRYTIME)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix -- Exiting!\n");
				return E_UNKNOWN;
			}
//		mSocketGroup.add(muxOut);
		CAMsg::printMsg(LOG_INFO," connected!\n");
		if(((CASocket*)m_MuxOut)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)m_MuxOut)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}
		
		unsigned char* recvBuff=NULL;
		unsigned char* infoBuff=NULL;
		
		UINT16 keyLen;
		if(((CASocket*)m_MuxOut)->receiveFully((UINT8*)&keyLen,2)!=E_SUCCESS)
			return E_UNKNOWN;
		CAMsg::printMsg(LOG_INFO,"Received Key Info lenght %u\n",ntohs(keyLen));
		recvBuff=new unsigned char[ntohs(keyLen)+2];
		if(recvBuff==NULL)
			return E_UNKNOWN;
		memcpy(recvBuff,&keyLen,2);
		if(((CASocket*)m_MuxOut)->receiveFully(recvBuff+2,ntohs(keyLen))!=E_SUCCESS)
			{
				delete recvBuff;
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_INFO,"Received Key Info...\n");
		
		CASocketAddr* pAddrListen;
		if(options.getServerPath(path,255)==E_SUCCESS) //unix domain
			{
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
				pAddrListen=new CASocketAddrUnix();
				((CASocketAddrUnix*)pAddrListen)->setPath((char*)path);
#else
				CAMsg::printMsg(LOG_CRIT,"I do not understand the Unix Domain Protocol!\n");
				return E_UNKNOWN;
#endif
			}
		else
			{
				pAddrListen=new CASocketAddrINet();
				((CASocketAddrINet*)pAddrListen)->setPort(options.getServerPort());
			}



		if(m_MuxIn.accept(*pAddrListen)==SOCKET_ERROR)
			{
				CAMsg::printMsg(LOG_CRIT,"Error waiting for previous Mix... -- Exiting!\n");				
				delete recvBuff;
				return E_UNKNOWN;
			}
		((CASocket*)m_MuxIn)->setRecvBuff(50*MIXPACKET_SIZE);
		((CASocket*)m_MuxIn)->setSendBuff(50*MIXPACKET_SIZE);
		if(((CASocket*)m_MuxIn)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)m_MuxIn)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}

		UINT32 keySize=m_RSA.getPublicKeySize();
		UINT16 infoSize;
		memcpy(&infoSize,recvBuff,2);
		infoSize=ntohs(infoSize)+2;
		if(infoSize>keyLen)
			{
				delete recvBuff;
				return E_UNKNOWN;
			}
		infoBuff=new unsigned char[infoSize+keySize]; 
		if(infoBuff==NULL)
			{
				delete recvBuff;
				return E_UNKNOWN;
			}
		memcpy(infoBuff,recvBuff,infoSize);
		delete recvBuff;
		
		infoBuff[2]++; //chainlen erhoehen
		m_RSA.getPublicKey(infoBuff+infoSize,&keySize);
		infoSize+=keySize;
		(*(UINT16*)infoBuff)=htons(infoSize-2);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n",infoSize);
		#endif
		if(((CASocket*)m_MuxIn)->send(infoBuff,infoSize)==-1)
			{
				CAMsg::printMsg(LOG_DEBUG,"Error sending new New Key Info\n");
				delete infoBuff;
				return E_UNKNOWN;
			}
		else
			CAMsg::printMsg(LOG_DEBUG,"Sending new New Key Info succeded\n");
		delete infoBuff;
		return E_SUCCESS;
	}

#ifdef _ASYNC
//Bug: What if Upstream deletes a channel, if if proccess a packet for this channel
// (pChipher could become invaild)!!! 
	
THREAD_RETURN loopDownStream(void *p)
	{
		CAMiddleMix* pMix=(CAMiddleMix*)p;
		CONNECTION oConnection;
		MIXPACKET oMixPacket;
		SINT32 ret;
		for(;;)
			{
				ret=pMix->m_MuxOut.receive(&oMixPacket);
				if(ret==SOCKET_ERROR)
						{
							CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
							goto ERR;
						}
				if(pMix->m_oSocketList.get(&oConnection,oMixPacket.channel))
					{
						if(oMixPacket.flags!=CHANNEL_CLOSE)
							{
								oMixPacket.channel=oConnection.id;
								oConnection.pCipher->decryptAES2(oMixPacket.data,oMixPacket.data,DATA_SIZE);
								if(pMix->m_MuxIn.send(&oMixPacket)==SOCKET_ERROR)
									goto ERR;
							}
						else
							{
								if(pMix->m_MuxIn.close(oConnection.id)==SOCKET_ERROR)
									goto ERR;
								delete oConnection.pCipher;
								pMix->m_oSocketList.remove(oConnection.id);
							}
					}
/*				else
					{
						if(pMix->m_MuxOut.close(oMixPacket.channel)==SOCKET_ERROR)
							goto ERR;
					}
*/			}
ERR:
		THREAD_RETURN_SUCCESS;		
	}


SINT32 CAMiddleMix::loop()
	{
		CONNECTION oConnection;
		MIXPACKET oMixPacket;
		HCHANNEL lastId=1;
		SINT32 ret;
		UINT8 tmpRSABuff[RSA_SIZE];
		m_MuxIn.setCrypt(true);
		m_MuxOut.setCrypt(true);
		m_oSocketList.clear();
		m_oSocketList.setThreadSafe(true);
		#ifdef _WIN32
		 _beginthread(loopDownStream,0,this);
		#else
		 pthread_t othread;
		 pthread_create(&othread,NULL,loopDownStream,this);
		#endif

		for(;;)
			{
				ret=m_MuxIn.receive(&oMixPacket);
				if(ret==SOCKET_ERROR)
					{
						CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
						goto ERR;
					}
				if(!m_oSocketList.get(oMixPacket.channel,&oConnection))
					{
						if(oMixPacket.flags==CHANNEL_OPEN)
							{
								#ifdef _DEBUG
										CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
								#endif
								CASymCipher* newCipher=new CASymCipher();
								m_RSA.decrypt((unsigned char*)oMixPacket.data,tmpRSABuff);
								newCipher->setKeyAES(tmpRSABuff);
								newCipher->decryptAES(oMixPacket.data+RSA_SIZE,
																			oMixPacket.data+RSA_SIZE-KEY_SIZE,
																			DATA_SIZE-RSA_SIZE);
								memcpy(oMixPacket.data,tmpRSABuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
								m_oSocketList.add(oMixPacket.channel,lastId,newCipher);
								oMixPacket.channel=lastId;
								lastId++;
								if(m_MuxOut.send(&oMixPacket)==SOCKET_ERROR)
									goto ERR;
							}
					}
				else
					{
						if(oMixPacket.flags==CHANNEL_CLOSE)
							{
								if(m_MuxOut.close(oConnection.outChannel)==SOCKET_ERROR)
									goto ERR;
								delete oConnection.pCipher;
								m_oSocketList.remove(oMixPacket.channel);
							}
						else
							{
								oMixPacket.channel=oConnection.outChannel;
								oConnection.pCipher->decryptAES(oMixPacket.data,oMixPacket.data,DATA_SIZE);
								if(m_MuxOut.send(&oMixPacket)==SOCKET_ERROR)
									goto ERR;
							}
					}
			}
ERR:
		m_MuxIn.close();
		m_MuxOut.close();
//todo wait for other thread
		
//
		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		CONNECTION* pCon=m_oSocketList.getFirst();
		while(pCon!=NULL)
			{
				delete pCon->pCipher;
				pCon=m_oSocketList.getNext();
			}
		return E_UNKNOWN;
	}
#else
SINT32 CAMiddleMix::loop()
	{
		CONNECTION oConnection;
		MIXPACKET oMixPacket;
		HCHANNEL lastId=1;
		SINT32 ret;
		UINT8 tmpRSABuff[RSA_SIZE];
		CASocketList oSocketList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(muxIn);
		oSocketGroup.add(muxOut);
		muxIn.setCrypt(true);
		muxOut.setCrypt(true);
		for(;;)
			{
				if(oSocketGroup.select()==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(muxIn))
					{
						ret=muxIn.receive(&oMixPacket,0);
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								goto ERR;
							}
						if(ret==E_AGAIN)
							goto NEXT;
						if(!oSocketList.get(oMixPacket.channel,&oConnection))
							{
								if(oMixPacket.flags==CHANNEL_OPEN)
									{
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										CASymCipher* newCipher=new CASymCipher();
										mRSA.decrypt((unsigned char*)oMixPacket.data,tmpRSABuff);
										newCipher->setKeyAES(tmpRSABuff);
										newCipher->decryptAES(oMixPacket.data+RSA_SIZE,
																					oMixPacket.data+RSA_SIZE-KEY_SIZE,
																					DATA_SIZE-RSA_SIZE);
										memcpy(oMixPacket.data,tmpRSABuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
										oSocketList.add(oMixPacket.channel,lastId,newCipher);
										oMixPacket.channel=lastId;
										lastId++;
										if(muxOut.send(&oMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
							}
						else
							{
								if(oMixPacket.flags==CHANNEL_CLOSE)
									{
										if(muxOut.close(oConnection.outChannel)==SOCKET_ERROR)
											goto ERR;
										delete oConnection.pCipher;
										oSocketList.remove(oMixPacket.channel);
									}
								else
									{
										oMixPacket.channel=oConnection.outChannel;
										oConnection.pCipher->decryptAES(oMixPacket.data,oMixPacket.data,DATA_SIZE);
										if(muxOut.send(&oMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
							}
					}
NEXT:				
				if(oSocketGroup.isSignaled(muxOut))
					{
						ret=muxOut.receive(&oMixPacket,0);
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								goto ERR;
							}
						if(ret==E_AGAIN)
							continue;
						if(oSocketList.get(&oConnection,oMixPacket.channel))
							{
								if(oMixPacket.flags!=CHANNEL_CLOSE)
									{
										oMixPacket.channel=oConnection.id;
										oConnection.pCipher->decryptAES2(oMixPacket.data,oMixPacket.data,DATA_SIZE);
										if(muxIn.send(&oMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
								else
									{
										if(muxIn.close(oConnection.id)==SOCKET_ERROR)
											goto ERR;
										delete oConnection.pCipher;
										oSocketList.remove(oConnection.id);
									}
							}
						else
							{
								if(muxOut.close(oMixPacket.channel)==SOCKET_ERROR)
									goto ERR;
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		CONNECTION* pCon=oSocketList.getFirst();
		while(pCon!=NULL)
			{
				delete pCon->pCipher;
				pCon=oSocketList.getNext();
			}
		return E_UNKNOWN;
	}
#endif
SINT32 CAMiddleMix::clean()
	{
		m_MuxIn.close();
		m_MuxOut.close();
		m_RSA.destroy();
		return E_SUCCESS;
	}