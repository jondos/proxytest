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
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CASocketAddrUnix.hpp"
#include "CASocketAddrINet.hpp"


extern CACmdLnOptions options;

/*******************************************************************************/
// ----------START NEW VERSION -----------------------
//---------------------------------------------------------
/********************************************************************************/

SINT32 CALastMix::initOnce()
	{
		UINT32 cntTargets=options.getTargetCount();
		if(cntTargets==0)
			return E_UNKNOWN;
		CASocketAddrINet oAddr;
		for(UINT32 i=1;i<=cntTargets;i++)
			{
				options.getTargetAddr(oAddr,i);
				m_oCacheLB.add(oAddr);
			}
		UINT8 strTarget[255];
		options.getSOCKSHost(strTarget,255);
		maddrSocks.setAddr(strTarget,options.getSOCKSPort());
		return E_SUCCESS;
	}

SINT32 CALastMix::init()
	{
		if(mRSA.generateKeyPair(1024)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not generate a valid key pair\n");
				return E_UNKNOWN;
			}
		CAMsg::printMsg(LOG_INFO,"Waiting for Connection from previous Mix...\n");
		CASocketAddr* pAddrListen;
		UINT8 path[255];
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
				((CASocketAddrINet*)pAddrListen)->setAddr(path,options.getServerPort());
			}
		if(muxIn.accept(*pAddrListen)==SOCKET_ERROR)
		    {
					delete pAddrListen;
					CAMsg::printMsg(LOG_CRIT," failed!\n");
					return E_UNKNOWN;
		    }
		delete pAddrListen;
		((CASocket*)muxIn)->setRecvBuff(500*MIXPACKET_SIZE);
		((CASocket*)muxIn)->setSendBuff(500*MIXPACKET_SIZE);
		if(((CASocket*)muxIn)->setSendLowWat(MIXPACKET_SIZE)!=E_SUCCESS)
			CAMsg::printMsg(LOG_INFO,"SOCKET Option SENDLOWWAT not set!\n");
		if(((CASocket*)muxIn)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)muxIn)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}
		
		CAMsg::printMsg(LOG_INFO,"connected!\n");
		CAMsg::printMsg(LOG_INFO,"Sending Infos (chain length and RSA-Key, RSA-Keysize %u)\n",mRSA.getPublicKeySize());
		UINT32 keySize=mRSA.getPublicKeySize();
		UINT16 messageSize=keySize+1;
		UINT8* buff=new UINT8[messageSize+2];
		UINT16 tmp=htons(messageSize);
		memcpy(buff,&tmp,2);
		buff[2]=1; //chainlen
		mRSA.getPublicKey(buff+3,&keySize);
		if(((CASocket*)muxIn)->send(buff,messageSize+2)!=messageSize+2)
			{
				CAMsg::printMsg(LOG_ERR,"Error sending Key-Info!\n");
				delete buff;
				return E_UNKNOWN;
			}
		delete buff;
		return E_SUCCESS;
	}

#define _CONNECT_TIMEOUT 5000 //5 Seconds...
#define _SEND_TIMEOUT (UINT32)5000 //5 Seconds...

SINT32 CALastMix::loop()
	{
		CASocketList  oSocketList;
		CASocketGroup osocketgroupCacheRead;
		CASocketGroup osocketgroupCacheWrite;
		CASingleSocketGroup osocketgroupMixIn;
		MIXPACKET* pMixPacket=new MIXPACKET;
		SINT32 ret;
		SINT32 countRead;
		CONNECTION oConnection;
		UINT8 rsaBuff[RSA_SIZE];
		CONNECTION* tmpCon;
		HCHANNEL tmpID;
		CAQueue oqueueMixIn;
		UINT8* tmpBuff=new UINT8[MIXPACKET_SIZE];
		osocketgroupMixIn.add(muxIn);
		((CASocket*)muxIn)->setNonBlocking(true);
		muxIn.setCrypt(true);
		bool bAktiv;
		for(;;)
			{
				bAktiv=false;
//Step one.. reading from previous Mix
// reading maximal number of current channels packets
				countRead=osocketgroupMixIn.select(false,0);
				if(countRead==1)
					{
						bAktiv=true;
						UINT32 channels=oSocketList.getSize()+1;
						for(UINT32 k=0;k<channels;k++)
							{
								ret=muxIn.receive(pMixPacket,0);
								if(ret==SOCKET_ERROR)
									{
										CAMsg::printMsg(LOG_CRIT,"Channel to previous mix closed -- Restarting!\n");
										goto ERR;
									}
								if(ret!=MIXPACKET_SIZE)
									break;
								//else one packet received
								if(!oSocketList.get(pMixPacket->channel,&oConnection))
									{
										if(pMixPacket->flags==CHANNEL_OPEN)
											{
												#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
												#endif
				
												CASymCipher* newCipher=new CASymCipher();
												mRSA.decrypt(pMixPacket->data,rsaBuff);
												newCipher->setKeyAES(rsaBuff);
												newCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																							pMixPacket->data+RSA_SIZE-KEY_SIZE,
																							DATA_SIZE-RSA_SIZE);
												memcpy(pMixPacket->data,rsaBuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
														
												CASocket* tmpSocket=new CASocket;										
												int ret;
												if(pMixPacket->payload.type==MIX_PAYLOAD_SOCKS)
													ret=tmpSocket->connect(maddrSocks,_CONNECT_TIMEOUT); 
												else
													{
														UINT32 count=0;
														do
															{
																tmpSocket->close();
																tmpSocket->create();
																tmpSocket->setRecvBuff(50000);
																tmpSocket->setSendBuff(5000);
																ret=tmpSocket->connect(*m_oCacheLB.get(),_CONNECT_TIMEOUT);
																count++;
															}
														while(ret!=E_SUCCESS&&count<m_oCacheLB.getElementCount());
													}	
												if(ret!=E_SUCCESS)
														{
	    												#ifdef _DEBUG
																CAMsg::printMsg(LOG_DEBUG,"Cannot connect to Squid!\n");
															#endif
															delete tmpSocket;
															delete newCipher;
															muxIn.close(pMixPacket->channel,tmpBuff);
															oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
														}
												else
														{    
															UINT16 payLen=ntohs(pMixPacket->payload.len);
															if(payLen>PAYLOAD_SIZE||tmpSocket->sendTimeOut(pMixPacket->payload.data,payLen,_SEND_TIMEOUT)==SOCKET_ERROR)
																{
																	#ifdef _DEBUG
																		CAMsg::printMsg(LOG_DEBUG,"Error sending Data to Squid!");
																	#endif
																	tmpSocket->close();
																	delete tmpSocket;
																	delete newCipher;
																	muxIn.close(pMixPacket->channel,tmpBuff);
																	oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
																}
															else
																{
																	tmpSocket->setNonBlocking(true);
																	oSocketList.add(pMixPacket->channel,tmpSocket,newCipher,new CAQueue());
																	osocketgroupCacheRead.add(*tmpSocket);
																}
														}
											}
									}
								else
									{
										if(pMixPacket->flags==CHANNEL_CLOSE)
											{
												osocketgroupCacheRead.remove(*(oConnection.pSocket));
												osocketgroupCacheWrite.remove(*(oConnection.pSocket));
												oConnection.pSocket->close();
												oSocketList.remove(pMixPacket->channel);
												delete oConnection.pSocket;
												delete oConnection.pCipher;
												delete oConnection.pSendQueue;										
											}
										else if(pMixPacket->flags==CHANNEL_SUSPEND)
											{
												osocketgroupCacheRead.remove(*(oConnection.pSocket));
											}
										else if(pMixPacket->flags==CHANNEL_RESUME)
											{
												osocketgroupCacheRead.add(*(oConnection.pSocket));
											}
										else
											{
												oConnection.pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
												ret=ntohs(pMixPacket->payload.len);
												if(ret>=0&&ret<=PAYLOAD_SIZE)
													ret=oConnection.pSendQueue->add(pMixPacket->payload.data,ret);
												else
													ret=SOCKET_ERROR;
												if(ret==SOCKET_ERROR)
													{
														osocketgroupCacheRead.remove(*(oConnection.pSocket));
														osocketgroupCacheWrite.remove(*(oConnection.pSocket));
														oConnection.pSocket->close();
														delete oConnection.pSocket;
														delete oConnection.pCipher;
														delete oConnection.pSendQueue;
														oSocketList.remove(pMixPacket->channel);
														muxIn.close(pMixPacket->channel,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
													}
												else
													{
														osocketgroupCacheWrite.add(*(oConnection.pSocket));
													}
											}
									}
							}
					}
//end Step 1

//Step 2 Sending to Cache...
				countRead=osocketgroupCacheWrite.select(true,0);
				if(countRead>0)
					{
						bAktiv=true;
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL&&countRead>0)
							{
								if(osocketgroupCacheWrite.isSignaled(*(tmpCon->pSocket)))
									{
										countRead--;
										SINT32 len=1000;
										tmpCon->pSendQueue->peek(tmpBuff,(UINT32*)&len);
										len=tmpCon->pSocket->send(tmpBuff,len);
										if(len>0)
											{
												tmpCon->pSendQueue->remove((UINT32*)&len);
												if(tmpCon->pSendQueue->isEmpty())
													{
														osocketgroupCacheWrite.remove(*(tmpCon->pSocket));
													}
											}
										else
											{
												if(len==SOCKET_ERROR)
													{ //do something if send error
														osocketgroupCacheRead.remove(*(tmpCon->pSocket));
														osocketgroupCacheWrite.remove(*(tmpCon->pSocket));
														tmpCon->pSocket->close();
														delete tmpCon->pSocket;
														delete tmpCon->pCipher;
														delete tmpCon->pSendQueue;
														muxIn.close(tmpCon->id,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
														oSocketList.remove(tmpCon->id);											 
													}
											}
									}
								tmpCon=oSocketList.getNext();
							}
					}
//End Step 2

//Step 3 Reading from Cache....
#define MAX_MIXIN_SEND_QUEUE_SIZE 1000000
				countRead=osocketgroupCacheRead.select(false,0);
				if(countRead>0)
					{
						bAktiv=true;
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL&&countRead>0)
							{
								if(osocketgroupCacheRead.isSignaled(*(tmpCon->pSocket)))
									{
										countRead--;
										if(oqueueMixIn.getSize()<MAX_MIXIN_SEND_QUEUE_SIZE)
											{
												ret=tmpCon->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE);
												if(ret==SOCKET_ERROR||ret==0)
													{
														osocketgroupCacheRead.remove(*(tmpCon->pSocket));
														osocketgroupCacheWrite.remove(*(tmpCon->pSocket));
														tmpCon->pSocket->close();
														delete tmpCon->pSocket;
														delete tmpCon->pCipher;
														delete tmpCon->pSendQueue;
														tmpID=tmpCon->id;
														oSocketList.remove(tmpID);
														muxIn.close(tmpID,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);			
													}
												else 
													{
														pMixPacket->channel=tmpCon->id;
														pMixPacket->flags=CHANNEL_DATA;
														pMixPacket->payload.len=htons((UINT16)ret);
														pMixPacket->payload.type=0;
														tmpCon->pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
														muxIn.send(pMixPacket,tmpBuff);
														oqueueMixIn.add(tmpBuff,MIXPACKET_SIZE);
													}
											}
										else
											break;
									}
								tmpCon=oSocketList.getNext();
							}
					}
//end Step 3

//Step 4 Writing to previous Mix
				if(!oqueueMixIn.isEmpty()&&osocketgroupMixIn.select(true,0)==1)
					{
						countRead=oSocketList.getSize()+1;
						while(countRead>0&&!oqueueMixIn.isEmpty())
							{
								bAktiv=true;
								countRead--;
								UINT32 len=MIXPACKET_SIZE;
								oqueueMixIn.peek(tmpBuff,&len);
								ret=((CASocket*)muxIn)->send(tmpBuff,len);
								if(ret>0)
									{
										oqueueMixIn.remove((UINT32*)&ret);
									}
								else if(ret==E_AGAIN)
									break;
								else
									goto ERR;
							}
					}
//end step 4
				if(!bAktiv)
					msleep(100);
			}




ERR:
		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		tmpCon=oSocketList.getFirst();
		while(tmpCon!=NULL)
			{
				delete tmpCon->pCipher;
				delete tmpCon->pSendQueue;
				tmpCon->pSocket->close();
				delete tmpCon->pSocket;
				tmpCon=tmpCon->next;
			}
		delete tmpBuff;
		delete pMixPacket;
		return E_UNKNOWN;
	}


SINT32 CALastMix::clean()
	{
		muxIn.close();
		mRSA.destroy();
		oSuspendList.clear();
		return E_SUCCESS;
	}

