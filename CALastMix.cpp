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
		if(options.getServerHost(path,255)==E_SUCCESS) //unix domain
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
		UINT8 strTarget[255];
		options.getTargetHost(strTarget,255);
		maddrSquid.setAddr((char*)strTarget,options.getTargetPort());

		options.getSOCKSHost(strTarget,255);
		maddrSocks.setAddr((char*)strTarget,options.getSOCKSPort());
		delete buff;
		return E_SUCCESS;
	}

#define _CONNECT_TIMEOUT 5000 //5 Seconds...
#define _SEND_TIMEOUT (UINT32)5000 //5 Seconds...

SINT32 CALastMix::loop()
	{
		CASocketList  oSocketList;
		CASocketGroup oSocketGroup;
		CASingleSocketGroup oSocketGroupMuxIn;
		MIXPACKET* pMixPacket=new MIXPACKET;
		SINT32 ret;
		SINT32 countRead;
		CONNECTION oConnection;
		UINT8 rsaBuff[RSA_SIZE];
		CONNECTION* tmpCon;
		HCHANNEL tmpID;

		oSocketGroup.add(muxIn);
		oSocketGroupMuxIn.add(muxIn);
		muxIn.setCrypt(true);
		for(;;)
			{
LOOP_START:
				if((countRead=oSocketGroup.select())==SOCKET_ERROR)
					{
						sleep(1);
						continue;
					}
				if(oSocketGroup.isSignaled(muxIn))
					{
						countRead--;
						ret=muxIn.receive(pMixPacket);
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Channel to previous mix closed -- Restarting!\n");
								goto ERR;
							}
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
												tmpSocket->create();
												tmpSocket->setRecvBuff(50000);
												tmpSocket->setSendBuff(5000);
												ret=tmpSocket->connect(maddrSquid,_CONNECT_TIMEOUT);
											}	
										if(ret!=E_SUCCESS)
										    {
	    										#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"Cannot connect to Squid!\n");
													#endif
													delete tmpSocket;
													delete newCipher;
													if(muxIn.close(pMixPacket->channel)==SOCKET_ERROR)
														goto ERR;
										    }
										else
										    {    
#ifdef _ASYNC
//													tmpSocket->setASyncSend(true,-1,0,10000,this);
#endif													
//													if(tmpSocket->setSendTimeOut(_SEND_TIMEOUT)!=E_SUCCESS)
//														CAMsg::printMsg(LOG_DEBUG,"Could not SEND Timeout!!");
													UINT16 payLen=ntohs(pMixPacket->payload.len);
													#ifdef _DEBUG
														pMixPacket->payload.data[ntohs(pMixPacket->payload.len)]=0;
														CAMsg::printMsg(LOG_DEBUG,"%u\n%s",ntohs(pMixPacket->payload.len),pMixPacket->payload.data);
													#endif
													if(payLen>PAYLOAD_SIZE||tmpSocket->sendTimeOut(pMixPacket->payload.data,payLen,_SEND_TIMEOUT)==SOCKET_ERROR)
														{
															#ifdef _DEBUG
																CAMsg::printMsg(LOG_DEBUG,"Error sending Data to Squid!");
															#endif
															tmpSocket->close();
															delete tmpSocket;
															delete newCipher;
															if(muxIn.close(pMixPacket->channel)==SOCKET_ERROR)
																goto ERR;
														}
													else
														{
															oSocketList.add(pMixPacket->channel,tmpSocket,newCipher);
															oSocketGroup.add(*tmpSocket);
														}
										    }
									}
								else
									{
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_CRIT,"Should never be here!!! New Channel wich Channel detroy packet!\n");
										#endif
									}
							}
						else
							{
								if(pMixPacket->flags==CHANNEL_CLOSE)
									{
										oSocketGroup.remove(*(oConnection.pSocket));
										oConnection.pSocket->close();
										deleteResume(pMixPacket->channel);
										oSocketList.remove(pMixPacket->channel);
										delete oConnection.pSocket;
										delete oConnection.pCipher;
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u\n",pMixPacket->channel);
										#endif
									}
								else if(pMixPacket->flags==CHANNEL_SUSPEND)
									{
										oSocketGroup.remove(*(oConnection.pSocket));
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_DEBUG,"Suspending Channel: %u\n",pMixPacket->channel);
										#endif
									}
								else if(pMixPacket->flags==CHANNEL_RESUME)
									{
										oSocketGroup.add(*(oConnection.pSocket));
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_DEBUG,"Resumeing Channel: %u\n",pMixPacket->channel);
										#endif
									}
								else
									{
										oConnection.pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										#ifdef _DEBUG
											pMixPacket->payload.data[ntohs(pMixPacket->payload.len)]=0;
											CAMsg::printMsg(LOG_DEBUG,"%u\n%s",ntohs(pMixPacket->payload.len),pMixPacket->payload.data);
										#endif
										ret=ntohs(pMixPacket->payload.len);
										if(ret>=0&&ret<=PAYLOAD_SIZE)
											ret=oConnection.pSocket->sendTimeOut(pMixPacket->payload.data,ret,_SEND_TIMEOUT);
										else
											ret=SOCKET_ERROR;
										if(ret==SOCKET_ERROR)
											{
												oSocketGroup.remove(*(oConnection.pSocket));
												oConnection.pSocket->close();
												delete oConnection.pSocket;
												delete oConnection.pCipher;
												oSocketList.remove(pMixPacket->channel);
												deleteResume(pMixPacket->channel);
												if(muxIn.close(pMixPacket->channel)==SOCKET_ERROR)
													goto ERR;
											}
	//we let the queu grow as much as memor is available...
	/*									
										else if(ret==E_QUEUEFULL)
											{
												EnterCriticalSection(&csResume);
												oSuspendList.add(oMuxPacket.channel,oConnection.pSocket,NULL);
												oMuxPacket.flags=CHANNEL_SUSPEND;
												CAMsg::printMsg(LOG_INFO,"Suspending channel %u\n",oMuxPacket.channel);
												muxIn.send(&oMuxPacket);
												LeaveCriticalSection(&csResume);
											}
	*/
									}
	
							}
					}
				if(countRead>0)
					{
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL&&countRead>0)
							{
								if(oSocketGroup.isSignaled(*(tmpCon->pSocket)))
									{
										if(oSocketGroupMuxIn.select(true,100)!=1)
											goto LOOP_START;
										countRead--;
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"Receiving Data from Squid!");
										#endif
										ret=tmpCon->pSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE);
										if(ret==SOCKET_ERROR||ret==0)
											{
												#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"Closing Connection from Squid!\n");
												#endif
												oSocketGroup.remove(*(tmpCon->pSocket));
												tmpCon->pSocket->close();
												delete tmpCon->pSocket;
												delete tmpCon->pCipher;
												tmpID=tmpCon->id;
												oSocketList.remove(tmpID);
												deleteResume(tmpID);
												oSocketList.remove(tmpID);
												if(muxIn.close(tmpID)==SOCKET_ERROR)
													goto ERR;
											}
										else 
											{
												pMixPacket->channel=tmpCon->id;
												pMixPacket->flags=CHANNEL_DATA;
												pMixPacket->payload.len=htons((UINT16)ret);
												pMixPacket->payload.type=0;
												tmpCon->pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
												if(muxIn.send(pMixPacket)==SOCKET_ERROR)
													{
														CAMsg::printMsg(LOG_CRIT,"Mux Data Sending Error - Restarting!\n");
														goto ERR;
													}
											}
									}
								tmpCon=oSocketList.getNext();
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		tmpCon=oSocketList.getFirst();
		while(tmpCon!=NULL)
			{
				delete tmpCon->pCipher;
				tmpCon->pSocket->close();
				delete tmpCon->pSocket;
				tmpCon=tmpCon->next;
			}
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

void CALastMix::resume(CASocket* pSocket)
	{
		EnterCriticalSection(&csResume);
		CONNECTION oConnection;
		if(oSuspendList.get(&oConnection,pSocket))
			{
				MIXPACKET oMixPacket;
				oMixPacket.flags=CHANNEL_RESUME;
				oMixPacket.channel=oConnection.id;
				muxIn.send(&oMixPacket);
				oSuspendList.remove(oConnection.id);
			}
		LeaveCriticalSection(&csResume);
	}
void CALastMix::deleteResume(HCHANNEL id)
{
		EnterCriticalSection(&csResume);
		oSuspendList.remove(id);
		LeaveCriticalSection(&csResume);
}

