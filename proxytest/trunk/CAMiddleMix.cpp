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

extern CACmdLnOptions options;

SINT32 CAMiddleMix::init()
	{		
		CAMsg::printMsg(LOG_INFO,"Creating Key...\n");
		if(oRSA.generateKeyPair(1024)!=E_SUCCESS)
			return E_UNKNOWN;
		return E_SUCCESS;
	}

#ifndef PROT2
SINT32 CAMiddleMix::loop()
	{
		CASocketList oSocketList;
		CASocketGroup oSocketGroup;
		int len;
		unsigned char buff[RSA_SIZE];
		CONNECTION oConnection;
		CONNECTION* pCon;
		MUXPACKET oMuxPacket;
		HCHANNEL lastId;
		lastId=1;
		unsigned char* recvBuff=NULL;
		unsigned char* infoBuff=NULL;
		
		UINT8 strTarget[255];
		options.getTargetHost(strTarget,255);
		CASocketAddr nextMix;
		nextMix.setAddr((char*)strTarget,options.getTargetPort());
		
SET_IN:		
		((CASocket*)muxOut)->create();
		((CASocket*)muxOut)->setRecvBuff(50*sizeof(MUXPACKET));
		((CASocket*)muxOut)->setSendBuff(50*sizeof(MUXPACKET));
		if(muxOut.connect(&nextMix)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix -- Exiting!\n");
				return E_UNKNOWN;
			}
		oSocketGroup.add(muxOut);

		CAMsg::printMsg(LOG_INFO," connected!\n");
		UINT16 keyLen;
		((CASocket*)muxOut)->receive((UINT8*)&keyLen,2);
		CAMsg::printMsg(LOG_INFO,"Received Key Info lenght %u\n",ntohs(keyLen));
		delete recvBuff;
		recvBuff=new unsigned char[ntohs(keyLen)+2];
		memcpy(recvBuff,&keyLen,2);
		((CASocket*)muxOut)->receive(recvBuff+2,ntohs(keyLen));
		CAMsg::printMsg(LOG_INFO,"Received Key Info...\n");
		
		
SET_OUT:		
		if(muxIn.accept(options.getServerPort())==SOCKET_ERROR)
			{
				CAMsg::printMsg(LOG_CRIT,"Error waiting for previous Mix... -- Exiting!\n");
				return E_UNKNOWN;
			}
		((CASocket*)muxIn)->setRecvBuff(50*sizeof(MUXPACKET));
		((CASocket*)muxIn)->setSendBuff(50*sizeof(MUXPACKET));

		oSocketGroup.add(muxIn);

		UINT32 keySize=oRSA.getPublicKeySize();
		UINT16 infoSize=ntohs((*(unsigned short*)recvBuff))+2;
		delete infoBuff;
		infoBuff=new unsigned char[infoSize+keySize]; 
		memcpy(infoBuff,recvBuff,infoSize);
		infoBuff[2]++; //chainlen erhoehen
		oRSA.getPublicKey(infoBuff+infoSize,&keySize);
		infoSize+=keySize;
		(*(unsigned short*)infoBuff)=htons(infoSize-2);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n",infoSize);
		#endif
		if(((CASocket*)muxIn)->send(infoBuff,infoSize)==-1)
			CAMsg::printMsg(LOG_DEBUG,"Error sending new New Key Info\n");
		else
			CAMsg::printMsg(LOG_DEBUG,"Sending new New Key Info succeded\n");
		
		for(;;)
			{
				if(oSocketGroup.select()==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(muxIn))
					{
						len=muxIn.receive(&oMuxPacket);
						if(len==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								//exit(-1);
								goto ERR_IN;
							}
						if(!oSocketList.get(oMuxPacket.channel,&oConnection))
							{
								if(len!=0)
									{
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										CASymCipher* newCipher=new CASymCipher();
										oRSA.decrypt((unsigned char*)oMuxPacket.data,buff);
#ifndef AES
										newCipher->setDecryptionKey(buff);
//										newCipher->setEncryptionKey(buff);
										newCipher->decrypt((unsigned char*)oMuxPacket.data+RSA_SIZE,
																			 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																			 DATA_SIZE-RSA_SIZE);
#else
										newCipher->setDecryptionKeyAES(buff);
//										newCipher->setEncryptionKeyAES(buff);
										newCipher->decryptAES((unsigned char*)oMuxPacket.data+RSA_SIZE,
																			 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																			 DATA_SIZE-RSA_SIZE);
#endif
										memcpy(oMuxPacket.data,buff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
										
										oMuxPacket.len=oMuxPacket.len-16;

										oSocketList.add(oMuxPacket.channel,lastId,newCipher);
										oMuxPacket.channel=lastId;
										lastId++;
										if(muxOut.send(&oMuxPacket)==SOCKET_ERROR)
											goto ERR_OUT;
									}
							}
						else
							{
								if(len==0)
									{
										muxOut.close(oConnection.outChannel);
										oSocketList.remove(oMuxPacket.channel);
									}
								else
									{
										oMuxPacket.channel=oConnection.outChannel;
#ifndef AES
										oConnection.pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#else
										oConnection.pCipher->decryptAES((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#endif
										if(muxOut.send(&oMuxPacket)==SOCKET_ERROR)
											goto ERR_OUT;
									}
							}
					}
				
				if(oSocketGroup.isSignaled(muxOut))
					{
						len=muxOut.receive(&oMuxPacket);
						if(len==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								//exit(-1);
								goto ERR_OUT;
							}
						if(oSocketList.get(&oConnection,oMuxPacket.channel))
							{
								if(len!=0)
									{
										oMuxPacket.channel=oConnection.id;
#ifndef AES
										oConnection.pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#else
										oConnection.pCipher->decryptAES((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#endif
										if(muxIn.send(&oMuxPacket)==SOCKET_ERROR)
											goto ERR_IN;
									}
								else
									{
										muxIn.close(oConnection.id);
										oSocketList.remove(oConnection.id);
									}
							}
						else
							muxOut.close(oMuxPacket.channel);
					}
			}
ERR_IN:
		oSocketGroup.remove(*(CASocket*)muxIn);
		muxIn.close();
		pCon=oSocketList.getFirst();
		while(pCon!=NULL)
			{
				muxOut.close(pCon->outChannel);
				oSocketList.remove(pCon->id);
				pCon=oSocketList.getNext();
			}
		goto SET_OUT;

ERR_OUT:
		oSocketGroup.remove(*(CASocket*)muxOut);
		muxOut.close();
		pCon=oSocketList.getFirst();
		while(pCon!=NULL)
			{
				muxIn.close(pCon->id);
				oSocketList.remove(pCon->id);
				pCon=oSocketList.getNext();
			}

		oSocketGroup.remove(*(CASocket*)muxIn);
		muxIn.close();
		goto SET_IN;

		return E_SUCCESS;
	}

#else

SINT32 CAMiddleMix::loop()
	{
		CASocketList oSocketList;
		CASocketGroup oSocketGroup;
		int len;
		unsigned char buff[RSA_SIZE];
		CONNECTION oConnection;
		CONNECTION* pCon;
		MUXPACKET oMuxPacket;
		HCHANNEL lastId;
		lastId=1;
		unsigned char* recvBuff=NULL;
		unsigned char* infoBuff=NULL;
		
		UINT8 strTarget[255];
		options.getTargetHost(strTarget,255);
		CASocketAddr nextMix;
		nextMix.setAddr((char*)strTarget,options.getTargetPort());
		
SET_IN:		
		((CASocket*)muxOut)->create();
		((CASocket*)muxOut)->setRecvBuff(50*sizeof(MUXPACKET));
		((CASocket*)muxOut)->setSendBuff(50*sizeof(MUXPACKET));
		if(muxOut.connect(&nextMix)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix -- Exiting!\n");
				return E_UNKNOWN;
			}
		oSocketGroup.add(muxOut);

		CAMsg::printMsg(LOG_INFO," connected!\n");
		UINT16 keyLen;
		((CASocket*)muxOut)->receive((UINT8*)&keyLen,2);
		CAMsg::printMsg(LOG_INFO,"Received Key Info lenght %u\n",ntohs(keyLen));
		delete recvBuff;
		recvBuff=new unsigned char[ntohs(keyLen)+2];
		memcpy(recvBuff,&keyLen,2);
		((CASocket*)muxOut)->receive(recvBuff+2,ntohs(keyLen));
		CAMsg::printMsg(LOG_INFO,"Received Key Info...\n");
		
		
SET_OUT:		
		if(muxIn.accept(options.getServerPort())==SOCKET_ERROR)
			{
				CAMsg::printMsg(LOG_CRIT,"Error waiting for previous Mix... -- Exiting!\n");
				return E_UNKNOWN;
			}
		((CASocket*)muxIn)->setRecvBuff(50*sizeof(MUXPACKET));
		((CASocket*)muxIn)->setSendBuff(50*sizeof(MUXPACKET));

		oSocketGroup.add(muxIn);

		UINT32 keySize=oRSA.getPublicKeySize();
		UINT16 infoSize=ntohs((*(unsigned short*)recvBuff))+2;
		delete infoBuff;
		infoBuff=new unsigned char[infoSize+keySize]; 
		memcpy(infoBuff,recvBuff,infoSize);
		infoBuff[2]++; //chainlen erhoehen
		oRSA.getPublicKey(infoBuff+infoSize,&keySize);
		infoSize+=keySize;
		(*(unsigned short*)infoBuff)=htons(infoSize-2);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n",infoSize);
		#endif
		if(((CASocket*)muxIn)->send(infoBuff,infoSize)==-1)
			CAMsg::printMsg(LOG_DEBUG,"Error sending new New Key Info\n");
		else
			CAMsg::printMsg(LOG_DEBUG,"Sending new New Key Info succeded\n");
		
		for(;;)
			{
				if(oSocketGroup.select()==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(muxIn))
					{
						len=muxIn.receive(&oMuxPacket);
						if(len==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								//exit(-1);
								goto ERR_IN;
							}
						if(!oSocketList.get(oMuxPacket.channel,&oConnection))
							{
								if(len!=0)
									{
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										CASymCipher* newCipher=new CASymCipher();
										oRSA.decrypt((unsigned char*)oMuxPacket.data,buff);
#ifndef AES
										newCipher->setDecryptionKey(buff);
//										newCipher->setEncryptionKey(buff);
										newCipher->decrypt((unsigned char*)oMuxPacket.data+RSA_SIZE,
																			 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																			 DATA_SIZE-RSA_SIZE);
#else
										newCipher->setDecryptionKeyAES(buff);
//										newCipher->setEncryptionKeyAES(buff);
										newCipher->decryptAES((unsigned char*)oMuxPacket.data+RSA_SIZE,
																			 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																			 DATA_SIZE-RSA_SIZE);
#endif
										memcpy(oMuxPacket.data,buff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
										
//										oMuxPacket.len=oMuxPacket.len-16;

										oSocketList.add(oMuxPacket.channel,lastId,newCipher);
										oMuxPacket.channel=lastId;
										lastId++;
										if(muxOut.send(&oMuxPacket)==SOCKET_ERROR)
											goto ERR_OUT;
									}
							}
						else
							{
								if(len==0)
									{
										muxOut.close(oConnection.outChannel);
										oSocketList.remove(oMuxPacket.channel);
									}
								else
									{
										oMuxPacket.channel=oConnection.outChannel;
#ifndef AES
										oConnection.pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#else
										oConnection.pCipher->decryptAES((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#endif
										if(muxOut.send(&oMuxPacket)==SOCKET_ERROR)
											goto ERR_OUT;
									}
							}
					}
				
				if(oSocketGroup.isSignaled(muxOut))
					{
						len=muxOut.receive(&oMuxPacket);
						if(len==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								//exit(-1);
								goto ERR_OUT;
							}
						if(oSocketList.get(&oConnection,oMuxPacket.channel))
							{
								if(len!=0)
									{
										oMuxPacket.channel=oConnection.id;
#ifndef AES
										oConnection.pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#else
										oConnection.pCipher->decryptAES((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#endif
										if(muxIn.send(&oMuxPacket)==SOCKET_ERROR)
											goto ERR_IN;
									}
								else
									{
										muxIn.close(oConnection.id);
										oSocketList.remove(oConnection.id);
									}
							}
						else
							muxOut.close(oMuxPacket.channel);
					}
			}
ERR_IN:
		oSocketGroup.remove(*(CASocket*)muxIn);
		muxIn.close();
		pCon=oSocketList.getFirst();
		while(pCon!=NULL)
			{
				muxOut.close(pCon->outChannel);
				oSocketList.remove(pCon->id);
				pCon=oSocketList.getNext();
			}
		goto SET_OUT;

ERR_OUT:
		oSocketGroup.remove(*(CASocket*)muxOut);
		muxOut.close();
		pCon=oSocketList.getFirst();
		while(pCon!=NULL)
			{
				muxIn.close(pCon->id);
				oSocketList.remove(pCon->id);
				pCon=oSocketList.getNext();
			}

		oSocketGroup.remove(*(CASocket*)muxIn);
		muxIn.close();
		goto SET_IN;

		return E_SUCCESS;
	}


#endif
