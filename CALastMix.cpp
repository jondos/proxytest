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
#include "CALastMix.hpp"
#include "CASocketList.hpp"
#include "CASocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"

extern CACmdLnOptions options;

SINT32 CALastMix::init()
	{
		pRSA=new CAASymCipher();
		pRSA->generateKeyPair(1024);
		CAMsg::printMsg(LOG_INFO,"Waiting for Connection from previous Mix...");
		if(muxIn.accept(options.getServerPort())==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT," failed!\n");
					return E_UNKNOWN;
		    }
		((CASocket*)muxIn)->setRecvBuff(50*sizeof(MUXPACKET));
		((CASocket*)muxIn)->setSendBuff(50*sizeof(MUXPACKET));
		
		CAMsg::printMsg(LOG_INFO,"connected!\n");
		CAMsg::printMsg(LOG_INFO,"Sending Infos (chain length and RSA-Key)\n");
		UINT32 keySize=pRSA->getPublicKeySize();
		UINT16 messageSize=keySize+1;
		UINT8* buff=new UINT8[messageSize+2];
		(*(UINT16*)buff)=htons(messageSize);
		buff[2]=1; //chainlen
		pRSA->getPublicKey(buff+3,&keySize);
		((CASocket*)muxIn)->send(buff,messageSize+2);
		
		UINT8 strTarget[255];
		options.getTargetHost(strTarget,255);
		addrSquid.setAddr((char*)strTarget,options.getTargetPort());

		options.getSOCKSHost(strTarget,255);
		addrSocks.setAddr((char*)strTarget,options.getSOCKSPort());
		
		return E_SUCCESS();
	}

SINT32 CALastMix::loop()
	{
		CASocketList  oSocketList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(muxIn);
		MUXPACKET oMuxPacket;
		int len;
		int countRead;
		CONNECTION oConnection;
		unsigned char buff[RSA_SIZE];
		for(;;)
			{
				if((countRead=oSocketGroup.select())==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(muxIn))
					{
						countRead--;
						len=muxIn.receive(&oMuxPacket);
						if(len==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Channel to previous mix closed -- Exiting!\n");
								exit(-1);
							}
						if(!oSocketList.get(oMuxPacket.channel,&oConnection))
							{
								if(len!=0)
									{
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										CASocket* tmpSocket=new CASocket;
										int ret;
										if(oMuxPacket.type==MUX_SOCKS)
											ret=tmpSocket->connect(&addrSocks);
										else
											ret=tmpSocket->connect(&addrSquid);	
										if(ret!=E_SUCCESS)
										    {
	    										#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"Cannot connect to Squid!\n");
													#endif
													muxIn.close(oMuxPacket.channel);
													delete tmpSocket;
										    }
										else
										    {    
													CASymCipher* newCipher=new CASymCipher();
													pRSA->decrypt((unsigned char*)oMuxPacket.data,buff);
													newCipher->setDecryptionKey(buff);
													newCipher->setEncryptionKey(buff);
													newCipher->decrypt((unsigned char*)oMuxPacket.data+RSA_SIZE,
																						 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																						 DATA_SIZE-RSA_SIZE);
													memcpy(oMuxPacket.data,buff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
													
													#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"%s",oMuxPacket.data);
													#endif
													if(tmpSocket->send(oMuxPacket.data,len-KEY_SIZE)==SOCKET_ERROR)
														{
															#ifdef _DEBUG
																CAMsg::printMsg(LOG_DEBUG,"Error sending Data to Squid!");
															#endif
															tmpSocket->close();
															muxIn.close(oMuxPacket.channel);
															delete tmpSocket;
															delete newCipher;
														}
													else
														{
															oSocketList.add(oMuxPacket.channel,tmpSocket,newCipher);
															oSocketGroup.add(*tmpSocket);
														}
										    }
									}
							}
						else
							{
								if(len==0)
									{
										oSocketGroup.remove(*(oConnection.pSocket));
										oConnection.pSocket->close();
										muxIn.close(oMuxPacket.channel);
										oSocketList.remove(oMuxPacket.channel);
										delete oConnection.pSocket;
										delete oConnection.pCipher;
									}
								else
									{
										oConnection.pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
										len=oConnection.pSocket->send(oMuxPacket.data,len);
										if(len==SOCKET_ERROR)
											{
												oSocketGroup.remove(*(oConnection.pSocket));
												oConnection.pSocket->close();
												muxIn.close(oMuxPacket.channel);
												oSocketList.remove(oMuxPacket.channel);
												delete oConnection.pSocket;
												delete oConnection.pCipher;
											}
									}
							}
					}
				if(countRead>0)
					{
						CONNECTION* tmpCon;
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL&&countRead>0)
							{
								if(oSocketGroup.isSignaled(*(tmpCon->pSocket)))
									{
										countRead--;
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"Receiving Data from Squid!");
										#endif
										len=tmpCon->pSocket->receive(oMuxPacket.data,1000);
										if(len==SOCKET_ERROR||len==0)
											{
												#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"Closing Connection from Squid!\n");
												#endif
												oSocketGroup.remove(*(tmpCon->pSocket));
												tmpCon->pSocket->close();
												muxIn.close(tmpCon->id);
												delete tmpCon->pSocket;
												delete tmpCon->pCipher;
												oSocketList.remove(tmpCon->id);
												break;
											}
										else 
											{
												oMuxPacket.channel=tmpCon->id;
												oMuxPacket.len=(unsigned short)len;
												tmpCon->pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
												if(muxIn.send(&oMuxPacket)==SOCKET_ERROR)
													{
														CAMsg::printMsg(LOG_CRIT,"Mux Data Sending Error - Exiting!\n");
														exit(-1);
													}
											}
									}
								tmpCon=oSocketList.getNext();
							}
					}
			}
		return E_SUCCESS;
	}

