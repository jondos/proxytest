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

extern CACmdLnOptions options;

SINT32 CALocalProxy::init()
	{
		CASocketAddr socketAddrIn("127.0.0.1",options.getServerPort());
		socketIn.create();
		socketIn.setReuseAddr(true);
		if(socketIn.listen(&socketAddrIn)==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
					return E_UNKNOWN;
		    }
		if(options.getSOCKSServerPort()!=(UINT16)-1)
			{
				socketAddrIn.setAddr("127.0.0.1",options.getSOCKSServerPort());
				socketSOCKSIn.create();
				socketSOCKSIn.setReuseAddr(true);
				if(socketSOCKSIn.listen(&socketAddrIn)==SOCKET_ERROR)
						{
							CAMsg::printMsg(LOG_CRIT,"Cannot listen\n");
							return E_UNKNOWN;
						}
			}
		CASocketAddr addrNext;
		UINT8 strTarget[255];
		options.getTargetHost(strTarget,255);
		addrNext.setAddr((char*)strTarget,options.getTargetPort());
		CAMsg::printMsg(LOG_INFO,"Try connectiong to next Mix...");

		((CASocket*)muxOut)->create();
		((CASocket*)muxOut)->setSendBuff(sizeof(MUXPACKET)*50);
		((CASocket*)muxOut)->setRecvBuff(sizeof(MUXPACKET)*50);
		if(muxOut.connect(&addrNext)==E_SUCCESS)
			{
				
				CAMsg::printMsg(LOG_INFO," connected!\n");
				UINT16 size;
				((CASocket*)muxOut)->receive((UINT8*)&size,2);
				((CASocket*)muxOut)->receive(&chainlen,1);
				CAMsg::printMsg(LOG_INFO,"Chain-Length: %d\n",chainlen);
				size=ntohs(size)-1;
				UINT8* buff=new UINT8[size];
				((CASocket*)muxOut)->receive(buff,size);
				arRSA=new CAASymCipher[chainlen];
				int aktIndex=0;
				for(int i=0;i<chainlen;i++)
					{
						int len=size;
						arRSA[i].setPublicKey(buff+aktIndex,(UINT32*)&len);
						size-=len;
						aktIndex+=len;
					}
				
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
		MUXPACKET oMuxPacket;
		memset(&oMuxPacket,0,sizeof(oMuxPacket));
		int len,ret;
		CASocket* newSocket;//,*tmpSocket;
		CASymCipher* newCipher;
		int countRead;
		CONNECTION oConnection;		
		for(;;)
			{
				if((countRead=oSocketGroup.select())==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(socketIn))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New Connection from Browser!\n");
						#endif
						newSocket=new CASocket;
						if(socketIn.accept(*newSocket)==SOCKET_ERROR)
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
						if(socketSOCKSIn.accept(*newSocket)==SOCKET_ERROR)
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
							ret=muxOut.receive(&oMuxPacket);
							if(ret==SOCKET_ERROR)
								{
									CAMsg::printMsg(LOG_CRIT,"Mux-Channel Receiving Data Error - Exiting!\n");									
									exit(-1);
								}

							if(!oSocketList.get(oMuxPacket.channel,&oConnection))
								{
									CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- Channel-Id no valid!\n");										
								}
							else
								{
									for(int c=0;c<chainlen;c++)
										oConnection.pCipher[c].decryptAES2((unsigned char*)oMuxPacket.data,oMuxPacket.data,DATA_SIZE);
									if(oMuxPacket.flags==CHANNEL_CLOSE)
										{
											#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ... ",oMuxPacket.channel);
											#endif
											//TODO: deleting cipher...
											/*tmpSocket=*/oSocketList.remove(oMuxPacket.channel);
											if(oConnection.pSocket!=NULL)
												{
													oSocketGroup.remove(*oConnection.pSocket);
													oConnection.pSocket->close();
													#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"closed!\n");
													#endif
													delete oConnection.pSocket;
													delete oConnection.pCipher;
												}
										}
									else
										{
											#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!");
											#endif
											oConnection.pSocket->send(oMuxPacket.payload.data,ntohs(oMuxPacket.payload.len));
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
										if(!tmpCon->pCipher[0].isEncyptionKeyValid())
											len=tmpCon->pSocket->receive(oMuxPacket.payload.data,PAYLOAD_SIZE-chainlen*16);
										else
											len=tmpCon->pSocket->receive(oMuxPacket.payload.data,PAYLOAD_SIZE);
										if(len==SOCKET_ERROR||len==0)
											{
												//TODO delete cipher..
												CASocket* tmpSocket=oSocketList.remove(tmpCon->id);
												if(tmpSocket!=NULL)
													{
														oSocketGroup.remove(*tmpSocket);
														muxOut.close(tmpCon->id);
														tmpSocket->close();
														delete tmpSocket;
													}
											}
										else 
											{
												oMuxPacket.channel=tmpCon->id;
												oMuxPacket.payload.len=htons(len);
												if(bHaveSocks&&tmpCon->pSocket->getLocalPort()==socksPort)
													{
														oMuxPacket.payload.type=MUX_SOCKS;
													}
												else
													{
														oMuxPacket.payload.type=MUX_HTTP;
													}
												if(!tmpCon->pCipher[0].isEncyptionKeyValid()) //First time --> rsa key
													{
														//Has to bee optimized!!!!
														unsigned char buff[DATA_SIZE];
														int size=DATA_SIZE-16;
														//tmpCon->pCipher->generateEncryptionKey(); //generate Key
														for(int c=0;c<chainlen;c++)
															{
																RAND_bytes(buff,16);
																tmpCon->pCipher[c].setKeyAES(buff);
																memcpy(buff+KEY_SIZE,oMuxPacket.data,size);
																arRSA[c].encrypt(buff,buff);
																tmpCon->pCipher[c].decryptAES(buff+RSA_SIZE,buff+RSA_SIZE,DATA_SIZE-RSA_SIZE);
																memcpy(oMuxPacket.data,buff,DATA_SIZE);
																size-=KEY_SIZE;
																len+=KEY_SIZE;
															}
														oMuxPacket.flags=CHANNEL_OPEN;
													}
												else //sonst
													{
														for(int c=0;c<chainlen;c++)
															tmpCon->pCipher[c].decryptAES((unsigned char*)oMuxPacket.data,oMuxPacket.data,DATA_SIZE);
														oMuxPacket.flags=CHANNEL_DATA;
													}
												if(muxOut.send(&oMuxPacket)==SOCKET_ERROR)
													{
														CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Exiting!\n");									
														exit(-1);
													}
											}
										break;
									}
								tmpCon=oSocketList.getNext();
							}
					}
			}
		return E_SUCCESS;
	}
