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
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocketAddrUnix.hpp"
#include "CAThread.hpp"

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
		memset(strTarget,0,255);
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
				((CASocketAddrINet*)pAddrNext)->setAddr(strTarget,options.getTargetPort());
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
		memset(path,0,255);
		if(options.getServerHost(path,255)==E_SUCCESS&&path[0]=='/') //unix domain
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
				if(path[0]==0) //empty host
					((CASocketAddrINet*)pAddrListen)->setPort(options.getServerPort());
				else	
					((CASocketAddrINet*)pAddrListen)->setAddr(path,options.getServerPort());
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
		m_pMiddleMixChannelList=new CAMiddleMixChannelList();
		return E_SUCCESS;
	}

	
THREAD_RETURN loopDownStream(void *p)
	{
		CAMiddleMix* pMix=(CAMiddleMix*)p;
		HCHANNEL channelIn;
		CASymCipher* pCipher;
		MIXPACKET* pMixPacket=new MIXPACKET;
		SINT32 ret;
		CASingleSocketGroup oSocketGroup;
		oSocketGroup.add(pMix->m_MuxOut);
		for(;;)
			{
				ret=oSocketGroup.select(false,1000);
				if(ret!=1)
					{
						if(ret==E_TIMEDOUT)
							continue;
						else
							{
								CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Fehler bei select() -- goto ERR!\n");
								goto ERR;
							}
					}
				else
					{
						ret=pMix->m_MuxOut.receive(pMixPacket);
						if(ret==SOCKET_ERROR)
								{
									CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Fehler bei receive() -- goto ERR!\n");
									goto ERR;
								}
						if(pMix->m_pMiddleMixChannelList->getOutToIn(&channelIn,pMixPacket->channel,&pCipher)==E_SUCCESS)
							{//connection found
								if(pMixPacket->flags!=CHANNEL_CLOSE)
									{
										pMixPacket->channel=channelIn;
										pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										pCipher->unlock();
										if(pMix->m_MuxIn.send(pMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
								else
									{//connection should be closed
										pCipher->unlock();
										if(pMix->m_MuxIn.close(channelIn)==SOCKET_ERROR)
											goto ERR;
										pMix->m_pMiddleMixChannelList->remove(channelIn);
									}
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Exiting!\n");
		delete pMixPacket;
		pMix->m_MuxIn.close();
		pMix->m_MuxOut.close();
		THREAD_RETURN_SUCCESS;		
	}


SINT32 CAMiddleMix::loop()
	{
		MIXPACKET* pMixPacket=new MIXPACKET;
		HCHANNEL channelOut;
		CASymCipher* pCipher;
		SINT32 ret;
		UINT8* tmpRSABuff=new UINT8[RSA_SIZE];
		CASingleSocketGroup oSocketGroup;
		oSocketGroup.add(m_MuxIn);
		m_MuxIn.setCrypt(true);
		m_MuxOut.setCrypt(true);
		CAThread oThread;
		oThread.setMainLoop(loopDownStream);
		oThread.start(this);
		for(;;)
			{
				ret=oSocketGroup.select(false,1000);
				if(ret!=1)
					{
						if(ret==E_TIMEDOUT)
							continue;
						else
							{
								CAMsg::printMsg(LOG_CRIT,"loopUpStream -- Fehler bei select() -- goto ERR!\n");
								goto ERR;
							}
					}
				else
					{
						ret=m_MuxIn.receive(pMixPacket);
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								goto ERR;
							}
						if(m_pMiddleMixChannelList->getInToOut(pMixPacket->channel,&channelOut,&pCipher)!=E_SUCCESS)
							{//new connection
								if(pMixPacket->flags==CHANNEL_OPEN)
									{
										#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										pCipher=new CASymCipher();
										m_RSA.decrypt(pMixPacket->data,tmpRSABuff);
										pCipher->setKeyAES(tmpRSABuff);
										pCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																				pMixPacket->data+RSA_SIZE-KEY_SIZE,
																				DATA_SIZE-RSA_SIZE);
										memcpy(pMixPacket->data,tmpRSABuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
										m_pMiddleMixChannelList->add(pMixPacket->channel,pCipher,&channelOut);
										pMixPacket->channel=channelOut;
										if(m_MuxOut.send(pMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
							}
						else
							{//established connection
								if(pMixPacket->flags==CHANNEL_CLOSE)
									{
										pCipher->unlock();
										if(m_MuxOut.close(channelOut)==SOCKET_ERROR)
											goto ERR;
										m_pMiddleMixChannelList->remove(pMixPacket->channel);
									}
								else
									{
										pMixPacket->channel=channelOut;
										pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										pCipher->unlock();
										if(m_MuxOut.send(pMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"Preparing for restart...\n");
		m_MuxIn.close();
		m_MuxOut.close();
		oThread.join();		

		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		delete tmpRSABuff;
		delete pMixPacket;
		return E_UNKNOWN;
	}
SINT32 CAMiddleMix::clean()
	{
		m_MuxIn.close();
		m_MuxOut.close();
		m_RSA.destroy();
		if(m_pMiddleMixChannelList!=NULL)
			delete m_pMiddleMixChannelList;
		m_pMiddleMixChannelList=NULL;
		return E_SUCCESS;
	}
