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

extern CACmdLnOptions options;

SINT32 CAMiddleMix::init()
	{		
		CAMsg::printMsg(LOG_INFO,"Creating Key...\n");
		if(mRSA.generateKeyPair(1024)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Init: Error generating Key-Pair...\n");
				return E_UNKNOWN;		
			}
		
		UINT8 strTarget[255];
		options.getTargetHost(strTarget,255);
		CASocketAddrINet nextMix;
		nextMix.setAddr((char*)strTarget,options.getTargetPort());
		
		if(((CASocket*)muxOut)->create()!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Init: Cannot create SOCKET for outgoing conncetion...\n");
				return E_UNKNOWN;
			}
		((CASocket*)muxOut)->setRecvBuff(50*MIXPACKET_SIZE);
		((CASocket*)muxOut)->setSendBuff(50*MIXPACKET_SIZE);
#define RETRIES 100
#define RETRYTIME 30
		CAMsg::printMsg(LOG_INFO,"Init: Try to connect to next Mix: %s...\n",strTarget);
		if(muxOut.connect(nextMix,RETRIES,RETRYTIME)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix -- Exiting!\n");
				return E_UNKNOWN;
			}
//		mSocketGroup.add(muxOut);
		CAMsg::printMsg(LOG_INFO," connected!\n");
		if(((CASocket*)muxOut)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)muxOut)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}
		
//		int len;
//		unsigned char buff[RSA_SIZE];
//		CONNECTION oConnection;
//		CONNECTION* pCon;
		unsigned char* recvBuff=NULL;
		unsigned char* infoBuff=NULL;
		
		UINT16 keyLen;
		if(((CASocket*)muxOut)->receiveFully((UINT8*)&keyLen,2)!=E_SUCCESS)
			return E_UNKNOWN;
		CAMsg::printMsg(LOG_INFO,"Received Key Info lenght %u\n",ntohs(keyLen));
		recvBuff=new unsigned char[ntohs(keyLen)+2];
		if(recvBuff==NULL)
			return E_UNKNOWN;
		memcpy(recvBuff,&keyLen,2);
		if(((CASocket*)muxOut)->receiveFully(recvBuff+2,ntohs(keyLen))!=E_SUCCESS)
			{
				delete recvBuff;
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_INFO,"Received Key Info...\n");
		
		if(muxIn.accept(options.getServerPort())==SOCKET_ERROR)
			{
				CAMsg::printMsg(LOG_CRIT,"Error waiting for previous Mix... -- Exiting!\n");				
				delete recvBuff;
				return E_UNKNOWN;
			}
		((CASocket*)muxIn)->setRecvBuff(50*MIXPACKET_SIZE);
		((CASocket*)muxIn)->setSendBuff(50*MIXPACKET_SIZE);
		if(((CASocket*)muxIn)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)muxIn)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}

		UINT32 keySize=mRSA.getPublicKeySize();
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
		mRSA.getPublicKey(infoBuff+infoSize,&keySize);
		infoSize+=keySize;
		(*(UINT16*)infoBuff)=htons(infoSize-2);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n",infoSize);
		#endif
		if(((CASocket*)muxIn)->send(infoBuff,infoSize)==-1)
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

SINT32 CAMiddleMix::clean()
	{
		muxIn.close();
		muxOut.close();
		mRSA.destroy();
		return E_SUCCESS;
	}
