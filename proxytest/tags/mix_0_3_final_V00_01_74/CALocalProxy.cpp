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
#include "CALocalProxy.hpp"
#include "CASocketList.hpp"
#include "CASocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CAUtil.hpp"
#include "CASocketAddrINet.hpp"

extern CACmdLnOptions options;

SINT32 CALocalProxy::initOnce()
	{
		if(options.getListenerInterfaceCount()<1)
		  {
				CAMsg::printMsg(LOG_CRIT,"No Listener Interface spezified!\n");
				return E_UNKNOWN;
			}
		if(options.getListenerInterfaceCount()<1)
		  {
				CAMsg::printMsg(LOG_CRIT,"No Listener Interface specified!\n");
				return E_UNKNOWN;
			}
		UINT8 strTarget[255];
		if(options.getMixHost(strTarget,255)!=E_SUCCESS)
		  {
				CAMsg::printMsg(LOG_CRIT,"No AnonServer specified!\n");
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

SINT32 CALocalProxy::init()
	{
		ListenerInterface oListener;
		
		CASocketAddrINet socketAddrIn;
		socketIn.create();
		socketIn.setReuseAddr(true);
		options.getListenerInterface(oListener,1);
		if(((CASocketAddrINet*)oListener.addr)->isAnyIP())
			((CASocketAddrINet*)oListener.addr)->setAddr((UINT8*)"127.0.0.1",((CASocketAddrINet*)oListener.addr)->getPort());
		if(socketIn.listen(*oListener.addr)!=E_SUCCESS)
		  {
				CAMsg::printMsg(LOG_CRIT,"Cannot listen (1)\n");
				delete oListener.addr;
				return E_UNKNOWN;
			}
		delete oListener.addr;
		if(options.getSOCKSServerPort()!=(UINT16)-1)
			{
				socketAddrIn.setAddr((UINT8*)"127.0.0.1",options.getSOCKSServerPort());
				socketSOCKSIn.create();
				socketSOCKSIn.setReuseAddr(true);
				if(socketSOCKSIn.listen(socketAddrIn)!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"Cannot listen (2)\n");
						return E_UNKNOWN;
					}
			}
		CASocketAddrINet addrNext;
		UINT8 strTarget[255];
		options.getMixHost(strTarget,255);
		addrNext.setAddr(strTarget,options.getMixPort());
		CAMsg::printMsg(LOG_INFO,"Try connecting to next Mix...\n");

		((CASocket*)muxOut)->create();
		((CASocket*)muxOut)->setSendBuff(MIXPACKET_SIZE*50);
		((CASocket*)muxOut)->setRecvBuff(MIXPACKET_SIZE*50);
		if(muxOut.connect(addrNext)==E_SUCCESS)
			{
				
				CAMsg::printMsg(LOG_INFO," connected!\n");
				UINT16 size;
				((CASocket*)muxOut)->receive((UINT8*)&size,2);
				((CASocket*)muxOut)->receive(&chainlen,1);
				CAMsg::printMsg(LOG_INFO,"Chain-Length: %d\n",chainlen);
				size=ntohs(size)-1;
				UINT8* buff=new UINT8[size];
				((CASocket*)muxOut)->receiveFully(buff,size);
				arRSA=new CAASymCipher[chainlen];
				int aktIndex=0;
				for(int i=0;i<chainlen;i++)
					{
						int len=size;
						arRSA[i].setPublicKey(buff+aktIndex,(UINT32*)&len);
						size-=len;
						aktIndex+=len;
					}
				delete[] buff;
				return E_SUCCESS;
			}
		else
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix!\n");
				return E_UNKNOWN;
			}
	}

SINT32 CALocalProxy::loop()
	{
		CASocketList  oSocketList;
		CASocketGroup oSocketGroup;
		oSocketGroup.add(socketIn);
		UINT16 socksPort=options.getSOCKSServerPort();
		bool bHaveSocks=(socksPort!=0xFFFF);
		if(bHaveSocks)
			oSocketGroup.add(socketSOCKSIn);
		oSocketGroup.add(muxOut);
		HCHANNEL lastChannelId=1;
		MIXPACKET* pMixPacket=new MIXPACKET;

		memset(pMixPacket,0,MIXPACKET_SIZE);
		SINT32 len,ret;
		CASocket* newSocket;//,*tmpSocket;
		CASymCipher* newCipher;
		int countRead;
		CONNECTION oConnection;
		for(;;)
			{
				if((countRead=oSocketGroup.select())==SOCKET_ERROR)
					{
						sSleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(socketIn))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New Connection from Browser!\n");
						#endif
						newSocket=new CASocket;
						if(socketIn.accept(*newSocket)!=E_SUCCESS)
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Accept Error - Connection from Browser!\n");
								#endif
								delete newSocket;
							}
						else
							{
								newCipher=new CASymCipher[chainlen];
								oSocketList.add(lastChannelId++,newSocket,newCipher);
								oSocketGroup.add(*newSocket);
							}
					}
				if(bHaveSocks&&oSocketGroup.isSignaled(socketSOCKSIn))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New Connection from SOCKS!\n");
						#endif
						newSocket=new CASocket;
						if(socketSOCKSIn.accept(*newSocket)!=E_SUCCESS)
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Accept Error - Connection from SOCKS!\n");
								#endif
								delete newSocket;
							}
						else
							{
								newCipher=new CASymCipher[chainlen];
								oSocketList.add(lastChannelId++,newSocket,newCipher);
								oSocketGroup.add(*newSocket);
							}
					}
				if(oSocketGroup.isSignaled(muxOut))
						{
							countRead--;	
							ret=muxOut.receive(pMixPacket);
							if(ret==SOCKET_ERROR)
								{
									CAMsg::printMsg(LOG_CRIT,"Mux-Channel Receiving Data Error - Exiting!\n");									
									ret=E_UNKNOWN;
									goto MIX_CONNECTION_ERROR;
								}

							if(oSocketList.get(pMixPacket->channel,&oConnection)==E_SUCCESS)
								{
									if(pMixPacket->flags==CHANNEL_CLOSE)
										{
											#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ... ",pMixPacket->channel);
											#endif
											/*tmpSocket=*/oSocketList.remove(pMixPacket->channel);
											if(oConnection.pSocket!=NULL)
												{
													oSocketGroup.remove(*oConnection.pSocket);
													oConnection.pSocket->close();
													#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"closed!\n");
													#endif
													delete oConnection.pSocket;
													delete [] oConnection.pCiphers;
												}
										}
									else
										{
											for(int c=0;c<chainlen;c++)
												oConnection.pCiphers[c].decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
											#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!");
											#endif
											oConnection.pSocket->send(pMixPacket->payload.data,ntohs(pMixPacket->payload.len));
										}
								}
						}
				if(countRead>0)
					{
						CONNECTION* tmpCon;
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL&&countRead>0)
							{
								if(oSocketGroup.isSignaled(*tmpCon->pSocket))
									{
										countRead--;
										if(!tmpCon->pCiphers[0].isEncyptionKeyValid())
											len=tmpCon->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE-chainlen*16);
										else
											len=tmpCon->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE);
										if(len==SOCKET_ERROR||len==0)
											{
												//TODO delete cipher..
												CASocket* tmpSocket=oSocketList.remove(tmpCon->outChannel);
												if(tmpSocket!=NULL)
													{
														oSocketGroup.remove(*tmpSocket);
														muxOut.close(tmpCon->outChannel);
														tmpSocket->close();
														delete tmpSocket;
													}
											}
										else 
											{
												pMixPacket->channel=tmpCon->outChannel;
												pMixPacket->payload.len=htons(len);
												if(bHaveSocks&&tmpCon->pSocket->getLocalPort()==socksPort)
													{
														pMixPacket->payload.type=MIX_PAYLOAD_SOCKS;
													}
												else
													{
														pMixPacket->payload.type=MIX_PAYLOAD_HTTP;
													}
												if(!tmpCon->pCiphers[0].isEncyptionKeyValid()) //First time --> rsa key
													{
														//Has to bee optimized!!!!
														unsigned char buff[DATA_SIZE];
														int size=DATA_SIZE-16;
														//tmpCon->pCipher->generateEncryptionKey(); //generate Key
														for(int c=0;c<chainlen;c++)
															{
																getRandom(buff,16);
																buff[0]&=0x7F; // Hack for RSA to ensure m < n !!!!!
																tmpCon->pCiphers[c].setKeyAES(buff);
																memcpy(buff+KEY_SIZE,pMixPacket->data,size);
																arRSA[c].encrypt(buff,buff);
																tmpCon->pCiphers[c].encryptAES(buff+RSA_SIZE,buff+RSA_SIZE,DATA_SIZE-RSA_SIZE);
																memcpy(pMixPacket->data,buff,DATA_SIZE);
																size-=KEY_SIZE;
																len+=KEY_SIZE;
															}
														pMixPacket->flags=CHANNEL_OPEN_OLD;
													}
												else //sonst
													{
														for(int c=0;c<chainlen;c++)
															tmpCon->pCiphers[c].encryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														pMixPacket->flags=CHANNEL_DATA;
													}
												if(muxOut.send(pMixPacket)==SOCKET_ERROR)
													{
														CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Exiting!\n");									
														ret=E_UNKNOWN;
														goto MIX_CONNECTION_ERROR;
													}
											}
										break;
									}
								tmpCon=oSocketList.getNext();
							}
					}
			}
MIX_CONNECTION_ERROR:
		CONNECTION* tmpCon=oSocketList.getFirst();
		while(tmpCon!=NULL)
			{
				delete [] tmpCon->pCiphers;
				delete tmpCon->pSocket;
				tmpCon=tmpCon->next;
			}
		delete pMixPacket;
		if(ret==E_SUCCESS)
			return E_SUCCESS;
		if(options.getAutoReconnect())
			return E_UNKNOWN;
		else
			exit(-1);
	}

SINT32 CALocalProxy::clean()
	{
		socketIn.close();
		socketSOCKSIn.close();
		muxOut.close();
		if(arRSA!=NULL)
			delete[] arRSA;
		arRSA=NULL;
		return E_SUCCESS;
	}