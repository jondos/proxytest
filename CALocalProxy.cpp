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
		m_socketIn.create();
		m_socketIn.setReuseAddr(true);
		options.getListenerInterface(oListener,1);
		if(((CASocketAddrINet*)oListener.addr)->isAnyIP())
			((CASocketAddrINet*)oListener.addr)->setAddr((UINT8*)"127.0.0.1",((CASocketAddrINet*)oListener.addr)->getPort());
		if(m_socketIn.listen(*oListener.addr)!=E_SUCCESS)
		  {
				CAMsg::printMsg(LOG_CRIT,"Cannot listen (1)\n");
				delete oListener.addr;
				return E_UNKNOWN;
			}
		delete oListener.addr;
		if(options.getSOCKSServerPort()!=(UINT16)-1)
			{
				socketAddrIn.setAddr((UINT8*)"127.0.0.1",options.getSOCKSServerPort());
				m_socketSOCKSIn.create();
				m_socketSOCKSIn.setReuseAddr(true);
				if(m_socketSOCKSIn.listen(socketAddrIn)!=E_SUCCESS)
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

		((CASocket*)m_muxOut)->create();
		((CASocket*)m_muxOut)->setSendBuff(MIXPACKET_SIZE*50);
		((CASocket*)m_muxOut)->setRecvBuff(MIXPACKET_SIZE*50);
		if(m_muxOut.connect(addrNext)==E_SUCCESS)
			{
				
				CAMsg::printMsg(LOG_INFO," connected!\n");
				UINT16 size;
				((CASocket*)m_muxOut)->receive((UINT8*)&size,2);
				((CASocket*)m_muxOut)->receive(&m_chainlen,1);
				if(m_chainlen='<')//assuming XML
					{
						size=ntohs(size);
						UINT8* buff=new UINT8[size+1];
						buff[0]=m_chainlen;
						((CASocket*)m_muxOut)->receiveFully(buff+1,size-1);
						buff[size]=0;
						SINT32 ret=processKeyExchange(buff,size);
						delete []  buff;
						if(ret!=E_SUCCESS)
							return E_UNKNOWN;
					}
				else
					{
						CAMsg::printMsg(LOG_INFO,"Chain-Length: %d\n",m_chainlen);
						size=ntohs(size)-1;
						UINT8* buff=new UINT8[size];
						((CASocket*)m_muxOut)->receiveFully(buff,size);
						m_arRSA=new CAASymCipher[m_chainlen];
						int aktIndex=0;
						for(int i=0;i<m_chainlen;i++)
							{
								int len=size;
								m_arRSA[i].setPublicKey(buff+aktIndex,(UINT32*)&len);
								size-=len;
								aktIndex+=len;
							}
						delete[] buff;
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
		oSocketGroup.add(m_socketIn);
		UINT16 socksPort=options.getSOCKSServerPort();
		bool bHaveSocks=(socksPort!=0xFFFF);
		if(bHaveSocks)
			oSocketGroup.add(m_socketSOCKSIn);
		oSocketGroup.add(m_muxOut);
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
				if(oSocketGroup.isSignaled(m_socketIn))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New Connection from Browser!\n");
						#endif
						newSocket=new CASocket;
						if(m_socketIn.accept(*newSocket)!=E_SUCCESS)
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Accept Error - Connection from Browser!\n");
								#endif
								delete newSocket;
							}
						else
							{
								newCipher=new CASymCipher[m_chainlen];
								oSocketList.add(lastChannelId++,newSocket,newCipher);
								oSocketGroup.add(*newSocket);
							}
					}
				if(bHaveSocks&&oSocketGroup.isSignaled(m_socketSOCKSIn))
					{
						countRead--;
						#ifdef _DEBUG
							CAMsg::printMsg(LOG_DEBUG,"New Connection from SOCKS!\n");
						#endif
						newSocket=new CASocket;
						if(m_socketSOCKSIn.accept(*newSocket)!=E_SUCCESS)
							{
								#ifdef _DEBUG
									CAMsg::printMsg(LOG_DEBUG,"Accept Error - Connection from SOCKS!\n");
								#endif
								delete newSocket;
							}
						else
							{
								newCipher=new CASymCipher[m_chainlen];
								oSocketList.add(lastChannelId++,newSocket,newCipher);
								oSocketGroup.add(*newSocket);
							}
					}
				if(oSocketGroup.isSignaled(m_muxOut))
						{
							countRead--;	
							ret=m_muxOut.receive(pMixPacket);
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
											for(int c=0;c<m_chainlen;c++)
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
											len=tmpCon->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE-m_chainlen*16);
										else
											len=tmpCon->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE);
										if(len==SOCKET_ERROR||len==0)
											{
												//TODO delete cipher..
												CASocket* tmpSocket=oSocketList.remove(tmpCon->outChannel);
												if(tmpSocket!=NULL)
													{
														oSocketGroup.remove(*tmpSocket);
														pMixPacket->flags=CHANNEL_CLOSE;
														pMixPacket->channel=tmpCon->outChannel;
														getRandom(pMixPacket->data,DATA_SIZE);
														m_muxOut.send(pMixPacket);
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
														for(int c=0;c<m_chainlen;c++)
															{
																getRandom(buff,16);
																buff[0]&=0x7F; // Hack for RSA to ensure m < n !!!!!
																tmpCon->pCiphers[c].setKeyAES(buff);
																memcpy(buff+KEY_SIZE,pMixPacket->data,size);
																m_arRSA[c].encrypt(buff,buff);
																tmpCon->pCiphers[c].encryptAES(buff+RSA_SIZE,buff+RSA_SIZE,DATA_SIZE-RSA_SIZE);
																memcpy(pMixPacket->data,buff,DATA_SIZE);
																size-=KEY_SIZE;
																len+=KEY_SIZE;
															}
														pMixPacket->flags=CHANNEL_OPEN;
													}
												else //sonst
													{
														for(int c=0;c<m_chainlen;c++)
															tmpCon->pCiphers[c].encryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														pMixPacket->flags=CHANNEL_DATA;
													}
												if(m_muxOut.send(pMixPacket)==SOCKET_ERROR)
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
		m_socketIn.close();
		m_socketSOCKSIn.close();
		m_muxOut.close();
		if(m_arRSA!=NULL)
			delete[] m_arRSA;
		m_arRSA=NULL;
		return E_SUCCESS;
	}

SINT32 CALocalProxy::processKeyExchange(UINT8* buff,UINT32 len)
	{
		//Parsing KeyInfo received from Mix n+1
		MemBufInputSource oInput(buff,len,"localoproxy");
		DOMParser oParser;
		oParser.parse(oInput);		
		DOM_Document doc=oParser.getDocument();
		if(doc.isNull())
			{
				CAMsg::printMsg(LOG_INFO,"Error parsing Key Info from Mix!\n");
				return E_UNKNOWN;
			}

		DOM_Element root=doc.getDocumentElement();
		DOM_Element elemMixes;
		getDOMChildByName(root,(UINT8*)"Mixes",elemMixes,false);
		int chainlen=-1;
		if(elemMixes==NULL||getDOMElementAttribute(elemMixes,"count",&chainlen)!=E_SUCCESS)
			return E_UNKNOWN;
		m_chainlen=(UINT32)chainlen;
		UINT32 i=0;
		m_arRSA=new CAASymCipher[m_chainlen];
		DOM_Node child=elemMixes.getFirstChild();
		while(child!=NULL&&chainlen>0)
			{
				if(child.getNodeName().equals("Mix"))
					{
						if(m_arRSA[i++].setPublicKeyAsDOMNode(child.getFirstChild())!=E_SUCCESS)
							return E_UNKNOWN;						
						chainlen--;
					}
				child=child.getNextSibling();
			}
		if(chainlen!=0)
			return E_UNKNOWN;
		//Now sending SymKeys....
		MIXPACKET oPacket;
		oPacket.flags=0;
		oPacket.channel=0;
		UINT8 keys[32];
		getRandom(keys,32);
		m_muxOut.setReceiveKey(keys,16);
		m_muxOut.setSendKey(keys+16,16);
		getRandom(oPacket.data,DATA_SIZE);
		memcpy(oPacket.data,"KEYPACKET",9);
		memcpy(oPacket.data+9,keys,32);
		m_arRSA[0].encrypt(oPacket.data,oPacket.data);
		m_muxOut.send(&oPacket);
		m_muxOut.setCrypt(true);
		return E_SUCCESS;
	}

