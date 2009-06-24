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
#include "CABase64.hpp"
#include "xml/DOM_Output.hpp"
#ifndef NEW_MIX_TYPE
extern CACmdLnOptions* pglobalOptions;
// signals the main loop whether to capture or replay packets
bool CALocalProxy::bCapturePackets;
bool CALocalProxy::bReplayPackets;
int CALocalProxy::iCapturedPackets;

// Signal handler for SIGUSR1.
// This starts the capture with the next channel-open message.
void SIGUSR1_handler(int signum)
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting to capture packets for replay attack.\n");
		CALocalProxy::bCapturePackets = true;
		CALocalProxy::iCapturedPackets = 0;
	}

// Signal handler for SIGUSR2.
// This replays the currently captured packets.
void SIGUSR2_handler(int signum)
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting replay of packets.\n");
		CALocalProxy::bReplayPackets = true;
	}


SINT32 CALocalProxy::start()
	{
		if(initOnce()!=E_SUCCESS)
			return E_UNKNOWN;
    for(;;)
    {
				CAMsg::printMsg(LOG_DEBUG, "CALocalProxy main: before init()\n");
        if(init() == E_SUCCESS)
        {
					CAMsg::printMsg(LOG_DEBUG, "CALocalProxy main: init() returned success\n");
          CAMsg::printMsg(LOG_INFO, "The local proxy is now on-line.\n");
					loop();
					CAMsg::printMsg(LOG_DEBUG, "CAMix local proxy: loop() returned, maybe connection lost.\n");
        }
        else
        {
            CAMsg::printMsg(LOG_DEBUG, "init() failed, maybe no connection.\n");
        }

				CAMsg::printMsg(LOG_DEBUG, "CALocalProxy main: before clean()\n");
				clean();
				CAMsg::printMsg(LOG_DEBUG, "CAMix LocalProxy: after clean()\n");
        sSleep(20);
    }
}


SINT32 CALocalProxy::initOnce()
	{
		if(pglobalOptions->getListenerInterfaceCount()<1)
		  {
				CAMsg::printMsg(LOG_CRIT,"No Listener Interface spezified!\n");
				return E_UNKNOWN;
			}

		bCapturePackets = bReplayPackets = false;
		#if defined(REPLAY_DETECTION)&&!defined(_WIN32)
			// Set up signal handling for the replay attack.
			signal(SIGUSR1, SIGUSR1_handler);
			signal(SIGUSR2, SIGUSR2_handler);
		#endif

		UINT8 strTarget[255];
		if(pglobalOptions->getMixHost(strTarget,255)!=E_SUCCESS)
		  {
				CAMsg::printMsg(LOG_CRIT,"No AnonServer specified!\n");
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

SINT32 CALocalProxy::init()
	{
		CAListenerInterface* pListener;
		m_nFlowControlDownstreamSendMe=FLOW_CONTROL_SENDME_SOFT_LIMIT;
		m_bWithNewFlowControl=false;
		m_bWithEnhancedChannelEncryption=false;
		m_bWithFirstMixSymmetric=false;
		m_socketIn.create();
		m_socketIn.setReuseAddr(true);
		pListener=pglobalOptions->getListenerInterface(1);
		if(pListener==NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"No listener specified\n");
				return E_UNKNOWN;
			}
		CASocketAddrINet* pSocketAddrIn=(CASocketAddrINet*)pListener->getAddr();
		delete pListener;
		pListener = NULL;
		if(pSocketAddrIn->isAnyIP())
			pSocketAddrIn->setAddr((UINT8*)"127.0.0.1",pSocketAddrIn->getPort());
		if(m_socketIn.listen(*pSocketAddrIn)!=E_SUCCESS)
		  {
				CAMsg::printMsg(LOG_CRIT,"Cannot listen (1)\n");
				delete pSocketAddrIn;
				pSocketAddrIn = NULL;
				return E_UNKNOWN;
			}
		delete pSocketAddrIn;
		pSocketAddrIn = NULL;
/*		if(pglobalOptions->getSOCKSServerPort()!=(UINT16)-1)
			{
				socketAddrIn.setAddr((UINT8*)"127.0.0.1",pglobalOptions->getSOCKSServerPort());
				m_socketSOCKSIn.create();
				m_socketSOCKSIn.setReuseAddr(true);
				if(m_socketSOCKSIn.listen(socketAddrIn)!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"Cannot listen (2)\n");
						return E_UNKNOWN;
					}
			}*/
		CASocketAddrINet addrNext;
		UINT8 strTarget[255];
		pglobalOptions->getMixHost(strTarget,255);
		addrNext.setAddr(strTarget,pglobalOptions->getMixPort());
		CAMsg::printMsg(LOG_INFO,"Try connecting to next Mix...\n");

		m_muxOut.getCASocket()->create();
		m_muxOut.getCASocket()->setSendBuff(MIXPACKET_SIZE*50);
		m_muxOut.getCASocket()->setRecvBuff(MIXPACKET_SIZE*50);
		if(m_muxOut.connect(addrNext)==E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO," connected!\n");
				UINT16 size;
				UINT8 byte;
				m_muxOut.getCASocket()->receiveFully((UINT8*)&size,2);
				m_muxOut.getCASocket()->receiveFully((UINT8*)&byte,1);
				CAMsg::printMsg(LOG_INFO,"Received Key Info!\n");
				size=ntohs(size);
#ifdef _DEBUG
				CAMsg::printMsg(LOG_INFO,"Key Info size is:%u\n",size);
#endif
				if(byte=='<')//assuming XML
					{
#ifdef _DEBUG
						CAMsg::printMsg(LOG_INFO,"Key Info is XML!\n");
#endif
						UINT8* buff=new UINT8[size+1];
						buff[0]=byte;
						m_muxOut.getCASocket()->receiveFully(buff+1,size-1);
						buff[size]=0;
#ifdef _DEBUG
						CAMsg::printMsg(LOG_INFO,"Key Info is:\n");
						CAMsg::printMsg(LOG_INFO,"%s\n",buff);
#endif
						SINT32 ret=processKeyExchange(buff,size);
						delete[]  buff;
						buff = NULL;
						if(ret!=E_SUCCESS)
							return E_UNKNOWN;
					}
				else
					{
						return E_UNKNOWN;
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
		CASocketGroup oSocketGroup(false);
		oSocketGroup.add(m_socketIn);
		UINT16 socksPort=pglobalOptions->getSOCKSServerPort();
		bool bHaveSocks=(socksPort!=0xFFFF);
		if(bHaveSocks)
			oSocketGroup.add(m_socketSOCKSIn);
		oSocketGroup.add(m_muxOut);
		MIXPACKET* pMixPacket=new MIXPACKET;

		memset(pMixPacket,0,MIXPACKET_SIZE);
		SINT32 len,ret;
		CASocket* newSocket;//,*tmpSocket;
		CASymCipher* newCipher;
		int countRead;
		CONNECTION oConnection;

		// Temporary storage for packets that could be replayed.
		MIXPACKET* pReplayMixPackets=new MIXPACKET[REPLAY_COUNT];
		memset(pReplayMixPackets,0,MIXPACKET_SIZE*REPLAY_COUNT);
		// channel ID from which packets are captured
		unsigned uCapturedChannel = 0;


		for(;;)
			{
				// Add timeout to select to allow for a replay attack to take place.
				if((countRead=oSocketGroup.select(100))==SOCKET_ERROR)
					{
						sSleep(1);
						continue;
					}

				// Start a replay attack, if a packet is present
				if(bReplayPackets)
					{
						if(iCapturedPackets > 0)
							{
								for(int i = 0; i < iCapturedPackets; i++)
									{
										memcpy(pMixPacket,&pReplayMixPackets[i],MIXPACKET_SIZE);
										CAMsg::printMsg(LOG_DEBUG,"Replaying packet #%d: %X %X %X %X\n", (i+1), *pMixPacket->data, *(pMixPacket->data+1), *(pMixPacket->data+2), *(pMixPacket->data+3));
										if(m_muxOut.send(pMixPacket)==SOCKET_ERROR)
											{
												CAMsg::printMsg(LOG_CRIT,"Mux-Channel Sending Data Error - Exiting!\n");
												ret=E_UNKNOWN;
												goto MIX_CONNECTION_ERROR;
											}
									}
								memset(pMixPacket,0,MIXPACKET_SIZE);
							}
						else
							{
								CAMsg::printMsg(LOG_DEBUG,"No captured packets found.\n");
							}
					CAMsg::printMsg(LOG_DEBUG,"Replay finished.\n");
					bReplayPackets = false;
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
								newSocket = NULL;
							}
						else
							{
								newCipher=new CASymCipher[m_chainlen];
								oSocketList.add(newSocket,newCipher);
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
								newSocket = NULL;
							}
						else
							{
								newCipher=new CASymCipher[m_chainlen];
								oSocketList.add(newSocket,newCipher);
								oSocketGroup.add(*newSocket);
							}
					}
				//Recevie from next Mix
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
													oConnection.pSocket = NULL;
													delete [] oConnection.pCiphers;
													oConnection.pCiphers = NULL;

													// stop capturing packets when the channel is closed
													if(pMixPacket->channel == uCapturedChannel)
														{
															CAMsg::printMsg(LOG_DEBUG,"Stopping to capture packets.\n");
															bCapturePackets = false;
														}
												}
										}
									else
										{
											for(UINT32 c=0;c<m_chainlen;c++)
												oConnection.pCiphers[c].crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
											UINT32 truelen=ntohs(pMixPacket->payload.len);
											truelen=truelen&PAYLOAD_LEN_MASK;
											oConnection.pSocket->send(pMixPacket->payload.data,truelen);
											#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser (%u bytes)!\n",truelen);
											#endif
											if(m_bWithNewFlowControl)
											{
												if(oConnection.currentSendMeCounter+1>=m_nFlowControlDownstreamSendMe)
													{
														//send a SEND_ME MixPacket
														pMixPacket->flags=CHANNEL_DATA;
														pMixPacket->channel=oConnection.outChannel;
														getRandom(pMixPacket->data,DATA_SIZE);
														pMixPacket->payload.len=htons(NEW_FLOW_CONTROL_FLAG);
														pMixPacket->payload.type=MIX_PAYLOAD_HTTP;
														for(UINT32 c=0;c<m_chainlen;c++)
															oConnection.pCiphers[c].crypt1(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														m_muxOut.send(pMixPacket);
														#ifdef _DEBUG
															CAMsg::printMsg(LOG_DEBUG,"sent sendme!\n");
														#endif
														oSocketList.addSendMeCounter(oConnection.outChannel,-(m_nFlowControlDownstreamSendMe-1));
													}
													else
													{
														oSocketList.addSendMeCounter(oConnection.outChannel,1);
													}
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
								if(oSocketGroup.isSignaled(*tmpCon->pSocket))
									{
										countRead--;
										getRandom(pMixPacket->payload.data,PAYLOAD_SIZE);
										if(!tmpCon->pCiphers[0].isKeyValid())
											len=tmpCon->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE-m_chainlen*m_SymChannelEncryptedKeySize);
										else
											len=tmpCon->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE);
										if(len==SOCKET_ERROR||len==0)
											{
												CAMsg::printMsg(LOG_DEBUG,"client side close channel - upstream sent: %u\n",tmpCon->upstreamBytes);
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
														tmpSocket = NULL;
														delete [] tmpCon->pCiphers;
														tmpCon->pCiphers = NULL;
													}
											}
										else
											{
												pMixPacket->channel=tmpCon->outChannel;
												tmpCon->upstreamBytes+=len;
												pMixPacket->payload.len=htons((UINT16)len);
												if(bHaveSocks&&tmpCon->pSocket->getLocalPort()==socksPort)
													{
														pMixPacket->payload.type=MIX_PAYLOAD_SOCKS;
													}
												else
													{
														pMixPacket->payload.type=MIX_PAYLOAD_HTTP;
													}
												if(!tmpCon->pCiphers[0].isKeyValid()) //First time --> rsa key
													{
														//Has to bee optimized!!!!
														UINT8 buff[DATA_SIZE];
														//UINT32 size=DATA_SIZE-KEY_SIZE;
														//tmpCon->pCipher->generateEncryptionKey(); //generate Key
														for(UINT32 c=0;c<m_chainlen;c++)
															{
																getRandom(buff,m_SymChannelKeySize);
																buff[0]&=0x7F; // Hack for RSA to ensure m < n !!!!!
																tmpCon->pCiphers[c].setKeys(buff,m_SymChannelKeySize);
																/*
																	PROTOCOL CHANGE:
																	This is a change in the protocol between JAP and the mixes:
																	In the RSA encrypted part of the channel-open packet apart
																	from the symmetric key for each mix, we now include a
																	TIMESTAMP_SIZE bytes wide timestamp. This reduces the storage
																	space for symmetrical keys in the RSA_SIZE wide part of the
																	packet (from 8 to 7).
																*/
																#ifdef WITH_TIMESTAMP
																	//TODO insert timesampt
																	//currentTimestamp(buff+KEY_SIZE,true);
																#endif
																if(m_bWithFirstMixSymmetric&&c==m_chainlen-1)
																	{
																		memcpy(buff+m_SymChannelKeySize,pMixPacket->data,DATA_SIZE-m_SymChannelKeySize);
																		m_pSymCipher->crypt1(buff,buff,m_SymChannelKeySize);
																		UINT8 iv[16];
																		memset(iv,0xFF,16);
																		tmpCon->pCiphers[c].setIV2(iv);
																		tmpCon->pCiphers[c].crypt1(buff+m_SymChannelKeySize,buff+m_SymChannelKeySize,DATA_SIZE-m_SymChannelKeySize);
																	}
																else
																	{
																		memcpy(buff+m_SymChannelKeySize,pMixPacket->data,DATA_SIZE-m_SymChannelEncryptedKeySize);
																		if(m_bWithEnhancedChannelEncryption)
																		{
																			UINT8 tmpBuff[1000];
																			UINT32 lk=1000;
																			m_arRSA[c].encryptOAEP(buff,128-42,tmpBuff,&lk);
																			tmpCon->pCiphers[c].crypt1(buff+RSA_SIZE-42,tmpBuff+RSA_SIZE,DATA_SIZE-RSA_SIZE);
																			memcpy(buff,tmpBuff,DATA_SIZE);
																		}
																		else
																		{
																			m_arRSA[c].encrypt(buff,buff);
																			tmpCon->pCiphers[c].crypt1(buff+RSA_SIZE,buff+RSA_SIZE,DATA_SIZE-RSA_SIZE);
																		}
																		// Does RSA_SIZE need to be increased by RSA_SIZE/KEY_SIZE*TIMESTAMP_SIZE?
																	}
																memcpy(pMixPacket->data,buff,DATA_SIZE);
																//size-=KEY_SIZE;
																//len+=KEY_SIZE;
															}
														pMixPacket->flags=CHANNEL_OPEN;
													}
												else //sonst
													{
														for(UINT32 c=0;c<m_chainlen;c++)
															tmpCon->pCiphers[c].crypt1(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														pMixPacket->flags=CHANNEL_DATA;
													}

												// Capture the this packet for future replay
												if(bCapturePackets)
													{
														if(!iCapturedPackets && (pMixPacket->flags == CHANNEL_OPEN))
															{
																CAMsg::printMsg(LOG_DEBUG,"Captured channel-open packet: %X %X %X %X\n", *pMixPacket->data, *(pMixPacket->data+1), *(pMixPacket->data+2), *(pMixPacket->data+3));
																memcpy(&pReplayMixPackets[iCapturedPackets++],pMixPacket,MIXPACKET_SIZE);
																uCapturedChannel = pMixPacket->channel;
															}
														else if(iCapturedPackets && (uCapturedChannel == pMixPacket->channel))
															{
																CAMsg::printMsg(LOG_DEBUG,"Captured data packet: %X %X %X %X\n", *pMixPacket->data, *(pMixPacket->data+1), *(pMixPacket->data+2), *(pMixPacket->data+3));
																memcpy(&pReplayMixPackets[iCapturedPackets++],pMixPacket,MIXPACKET_SIZE);
															}
													}
												if(iCapturedPackets >= REPLAY_COUNT)
													{
														CAMsg::printMsg(LOG_DEBUG,"Storage full, stopping to capture packets.\n");
														bCapturePackets = false;
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
				tmpCon->pCiphers = NULL;
				delete tmpCon->pSocket;
				tmpCon->pSocket = NULL;
				tmpCon=tmpCon->next;
			}
		delete pMixPacket;
		pMixPacket = NULL;
		if(ret==E_SUCCESS)
			return E_SUCCESS;
		if(pglobalOptions->getAutoReconnect())
			return E_UNKNOWN;
		else
			exit(-1);
	}

SINT32 CALocalProxy::clean()
	{
		m_socketIn.close();
		m_socketSOCKSIn.close();
		m_muxOut.close();

		delete[] m_arRSA;
		m_arRSA=NULL;

		delete m_pSymCipher;
		m_pSymCipher=NULL;

		return E_SUCCESS;
	}

SINT32 CALocalProxy::processKeyExchange(UINT8* buff,UINT32 len)
	{
		m_SymChannelEncryptedKeySize=16;
		m_SymChannelKeySize=16;
		CAMsg::printMsg(LOG_INFO,"Login process and key exchange started...\n");
		//Parsing KeyInfo received from Mix n+1
		XERCES_CPP_NAMESPACE::DOMDocument* doc=parseDOMDocument(buff,len);
		if(doc==NULL)
			{
				CAMsg::printMsg(LOG_INFO,"Error parsing Key Info from Mix!\n");
				return E_UNKNOWN;
			}


		DOMElement* root=doc->getDocumentElement();
		DOMElement* elemVersion=NULL;
		getDOMChildByName(root,"MixProtocolVersion",elemVersion,false);
		UINT8 strVersion[255];
		UINT32 tmpLen=255;
		if(getDOMElementValue(elemVersion,strVersion,&tmpLen)==E_SUCCESS)
			{
#ifdef _DEBUG
				CAMsg::printMsg(LOG_INFO,"XML MixProtocolVersion value:%s\n",strVersion);
#endif
				if(tmpLen==3&&memcmp(strVersion,"0.4",3)==0)
					{
						CAMsg::printMsg(LOG_INFO,"MixCascadeProtocolVersion: 0.4\n");
						m_MixCascadeProtocolVersion=MIX_CASCADE_PROTOCOL_VERSION_0_4;
						m_bWithFirstMixSymmetric=true;
					}
				else if(tmpLen==3&&memcmp(strVersion,"0.3",3)==0)
					{
						CAMsg::printMsg(LOG_INFO,"MixCascadeProtocolVersion: 0.3\n");
						m_MixCascadeProtocolVersion=MIX_CASCADE_PROTOCOL_VERSION_0_3;
					}
				else if(tmpLen==3&&memcmp(strVersion,"0.2",3)==0)
					{
						CAMsg::printMsg(LOG_INFO,"MixCascadeProtocolVersion: 0.2\n");
						m_MixCascadeProtocolVersion=MIX_CASCADE_PROTOCOL_VERSION_0_2;
					}
				else if(tmpLen==4&&memcmp(strVersion,"0.10",4)==0)
					{
						CAMsg::printMsg(LOG_INFO,"MixCascadeProtocolVersion: 0.10\n");
						m_MixCascadeProtocolVersion=MIX_CASCADE_PROTOCOL_VERSION_0_1_0;
						m_bWithEnhancedChannelEncryption=true;
						m_SymChannelKeySize=32;
						m_SymChannelEncryptedKeySize=m_SymChannelKeySize+42;
						m_bWithFirstMixSymmetric=true;
					}
				else
					{
						CAMsg::printMsg(LOG_ERR,"Unsupported MixProtocolVersion!\n");
						return E_UNKNOWN;
					}
			}
		else
		{
#ifdef _DEBUG
			CAMsg::printMsg(LOG_ERR,"No MixProtocolVersion given!\n");
			return E_UNKNOWN;
#endif
		}
		DOMElement* elemMixes=NULL;
		getDOMChildByName(root,"Mixes",elemMixes,false);
		SINT32 chainlen=-1;
		if(elemMixes==NULL||getDOMElementAttribute(elemMixes,"count",&chainlen)!=E_SUCCESS)
			{
#ifdef _DEBUG
				CAMsg::printMsg(LOG_ERR,"No count of Mixes given!\n");
				return E_UNKNOWN;
#endif
			}
		m_chainlen=(UINT32)chainlen;
#ifdef _DEBUG
		CAMsg::printMsg(LOG_INFO,"Number of Mixes is: %u\n",m_chainlen);
#endif
		if(m_chainlen==0)
			{
#ifdef _DEBUG
				CAMsg::printMsg(LOG_ERR,"Number of Mixes is 0!\n");
				return E_UNKNOWN;
#endif
			}
		UINT32 i=0;
		m_arRSA=new CAASymCipher[m_chainlen];
		DOMNode* child=elemMixes->getLastChild();
		bool bIsLast=true;
		while(child!= NULL&&chainlen>0)
			{
				if(equals(child->getNodeName(),"Mix"))
					{
						DOMNode* nodeKey=child->getFirstChild();
						if(m_arRSA[i++].setPublicKeyAsDOMNode(nodeKey)!=E_SUCCESS)
							{
#ifdef _DEBUG
								CAMsg::printMsg(LOG_ERR,"Error in parsing the public key of a Mix\n");
								return E_UNKNOWN;
#endif
							}
						if(bIsLast)
						{
							DOMNode* tmpNode=NULL;
							getDOMChildByName(child,"MixProtocolVersion",tmpNode);
							UINT8 tmpBuff[255];
							UINT32 tmpBuffLen=255;
							if(getDOMElementValue(tmpNode,tmpBuff,&tmpBuffLen)==E_SUCCESS)
							{
								CAMsg::printMsg(LOG_DEBUG,"Last Mix Protcol Version %s\n",tmpBuff);
								if(strcmp("0.6",(char*)tmpBuff)==0)
								{
									m_bWithNewFlowControl=true;
									CAMsg::printMsg(LOG_DEBUG,"Last Mix uses new flow control\n");
								}
							}
							if(m_bWithNewFlowControl)
							{
								getDOMChildByName(child,"FlowControl",tmpNode);
								DOMElement* nodeDownstreamSendMe=NULL;
								getDOMChildByName(tmpNode,"DownstreamSendMe",nodeDownstreamSendMe);
								getDOMElementValue(nodeDownstreamSendMe,m_nFlowControlDownstreamSendMe,m_nFlowControlDownstreamSendMe);
								CAMsg::printMsg(LOG_DEBUG,"Last Mix new flow control downstream sewnd me: %u\n",m_nFlowControlDownstreamSendMe);
							}
							bIsLast=false;
						}
						chainlen--;
					}
				child=child->getPreviousSibling();
			}
		if(chainlen!=0)
			{
#ifdef _DEBUG
				CAMsg::printMsg(LOG_ERR,"Expected information for %u Mixes but found only %u!\n",m_chainlen,m_chainlen-chainlen);
				return E_UNKNOWN;
#endif
			}
		//Now sending SymKeys....
		if(m_MixCascadeProtocolVersion==MIX_CASCADE_PROTOCOL_VERSION_0_2)
			{
				MIXPACKET oPacket;
				getRandom((UINT8*)&oPacket,MIXPACKET_SIZE);
				oPacket.flags=0;
				oPacket.channel=0;
				UINT8 keys[32];
				getRandom(keys,32);
				m_muxOut.setReceiveKey(keys,16);
				m_muxOut.setSendKey(keys+16,16);
				memcpy(oPacket.data,"KEYPACKET",9);
				memcpy(oPacket.data+9,keys,32);
				m_arRSA[m_chainlen-1].encrypt(oPacket.data,oPacket.data);
				m_muxOut.send(&oPacket);
				m_muxOut.setCrypt(true);
			}
		else
			{
				const char* XML_HEADER="<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
				const UINT32 XML_HEADER_SIZE=strlen(XML_HEADER);
				const char* XML_JAP_KEY_TEMPLATE="<JAPKeyExchange version=\"0.1\"><LinkEncryption>%s</LinkEncryption><MixEncryption>%s</MixEncryption></JAPKeyExchange>";
	      //DOM_Document doc=DOM_Document::createDocument();
   			//DOM_Element e = doc.createElement("JAPKeyExchange");
				//doc.appendChild(e);
				//e.setAttribute("version", "0.1");
				//DOM_Element elemLinkEnc = doc.createElement("LinkEncryption");
				UINT8* buff=new UINT8[9000];
				UINT8 linkKeys[64];
				getRandom(linkKeys,64);
				UINT8 outBuffLinkKey[512];
				UINT32 outlenLinkKey=512;
				CABase64::encode(linkKeys,64,outBuffLinkKey,&outlenLinkKey);
				outBuffLinkKey[outlenLinkKey]=0;
				//setDOMElementValue(elemLinkEnc,outBuff);
				//e.appendChild(elemLinkEnc);
				//DOM_Element elemMixEnc = doc.createElement("MixEncryption");
				UINT8 mixKeys[32];
				getRandom(mixKeys,32);
				m_pSymCipher=new CASymCipher();
				m_pSymCipher->setKey(mixKeys);
				m_pSymCipher->setIVs(mixKeys+16);
				UINT8 outBuffMixKey[512];
				UINT32 outlenMixKey=512;
				CABase64::encode(mixKeys,32,outBuffMixKey,&outlenMixKey);
				outBuffMixKey[outlenMixKey]=0;
				//setDOMElementValue(elemMixEnc,outBuff);
				//e.appendChild(elemMixEnc);
				sprintf((char*)buff,XML_JAP_KEY_TEMPLATE,outBuffLinkKey,outBuffMixKey);
				UINT32 encbufflen;
				UINT8* encbuff=encryptXMLElement(buff,strlen((char*)buff),encbufflen,&m_arRSA[m_chainlen-1]);
				UINT16 size2=htons((UINT16)(encbufflen+XML_HEADER_SIZE));
				SINT32 ret=m_muxOut.getCASocket()->send((UINT8*)&size2,2);
				ret=m_muxOut.getCASocket()->send((UINT8*)XML_HEADER,XML_HEADER_SIZE);
				ret=m_muxOut.getCASocket()->send(encbuff,encbufflen);
				delete[] encbuff;
				encbuff = NULL;
				delete[] buff;
				buff = NULL;
				// Checking Signature send from Mix
				ret=m_muxOut.getCASocket()->receiveFully((UINT8*)&size2,2);
				size2=ntohs(size2);
				UINT8* xmlbuff=new UINT8[size2];
				ret=m_muxOut.getCASocket()->receiveFully(xmlbuff,size2);
				delete[] xmlbuff;
				xmlbuff = NULL;
				m_muxOut.setSendKey(linkKeys,32);
				m_muxOut.setReceiveKey(linkKeys+32,32);
				m_muxOut.setCrypt(true);
			}
		if (doc != NULL)
		{
			doc->release();
			doc = NULL;
		}
		CAMsg::printMsg(LOG_INFO,"Login process and key exchange finished!\n");
		return E_SUCCESS;
	}
#endif //!NEW_MIX_TYPE
