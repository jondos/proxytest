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



#ifndef PROT2
SINT32 CALastMix::init()
	{
		pRSA=new CAASymCipher();
		pRSA->generateKeyPair(1024);
		CAMsg::printMsg(LOG_INFO,"Waiting for Connection from previous Mix...\n");
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
		if(((CASocket*)muxIn)->send(buff,messageSize+2)!=E_SUCCESS)
		{
		    delete buff;
		    return E_UNKOWN;
		}
		delete buff;
		UINT8 strTarget[255];
		options.getTargetHost(strTarget,255);
		addrSquid.setAddr((char*)strTarget,options.getTargetPort());

		options.getSOCKSHost(strTarget,255);
		addrSocks.setAddr((char*)strTarget,options.getSOCKSPort());
		
		return E_SUCCESS;
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
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"Select Error\n");
										#endif
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
#ifndef AES
													newCipher->setDecryptionKey(buff);
//													newCipher->setEncryptionKey(buff);
													newCipher->decrypt((unsigned char*)oMuxPacket.data+RSA_SIZE,
																						 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																						 DATA_SIZE-RSA_SIZE);
#else
													newCipher->setDecryptionKeyAES(buff);
//													newCipher->setEncryptionKeyAES(buff);
													newCipher->decryptAES((unsigned char*)oMuxPacket.data+RSA_SIZE,
																						 (unsigned char*)oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																						 DATA_SIZE-RSA_SIZE);
#endif
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
#ifndef AES
										oConnection.pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#else
										oConnection.pCipher->decryptAES((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#endif
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
#ifndef AES
												tmpCon->pCipher->decrypt((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#else
												tmpCon->pCipher->decryptAES((unsigned char*)oMuxPacket.data,(unsigned char*)oMuxPacket.data,DATA_SIZE);
#endif
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

#else

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
		if(muxIn.accept(options.getServerPort())==SOCKET_ERROR)
		    {
					CAMsg::printMsg(LOG_CRIT," failed!\n");
					return E_UNKNOWN;
		    }
		((CASocket*)muxIn)->setRecvBuff(500*MUXPACKET_SIZE);
		((CASocket*)muxIn)->setSendBuff(500*MUXPACKET_SIZE);
		if(((CASocket*)muxIn)->setSendLowWat(MUXPACKET_SIZE)!=E_SUCCESS)
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

SINT32 CALastMix::loop()
	{
		CASocketList  oSocketList;
		CASocketGroup oSocketGroup;
		CASocketGroup oSocketGroupMuxIn;
		MUXPACKET oMuxPacket;
		SINT32 ret;
		SINT32 countRead;
		CONNECTION oConnection;
		UINT8 rsaBuff[RSA_SIZE];
		CONNECTION* tmpCon;
		
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
						ret=muxIn.receive(&oMuxPacket);
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Channel to previous mix closed -- Restarting!\n");
								goto ERR;
							}
						if(!oSocketList.get(oMuxPacket.channel,&oConnection))
							{
								if(oMuxPacket.flags==CHANNEL_OPEN)
									{
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
		
										CASymCipher* newCipher=new CASymCipher();
										mRSA.decrypt(oMuxPacket.data,rsaBuff);
										newCipher->setKeyAES(rsaBuff);
										newCipher->decryptAES(oMuxPacket.data+RSA_SIZE,
																					oMuxPacket.data+RSA_SIZE-KEY_SIZE,
																					DATA_SIZE-RSA_SIZE);
										memcpy(oMuxPacket.data,rsaBuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
												
										CASocket* tmpSocket=new CASocket;										
										int ret;
										if(oMuxPacket.payload.type==MUX_SOCKS)
											ret=tmpSocket->connect(&maddrSocks); //may be block
										else
											ret=tmpSocket->connect(&maddrSquid);	//may be block
										if(ret!=E_SUCCESS)
										    {
	    										#ifdef _DEBUG
														CAMsg::printMsg(LOG_DEBUG,"Cannot connect to Squid!\n");
													#endif
													muxIn.close(oMuxPacket.channel);
													delete tmpSocket;
													delete newCipher;
										    }
										else
										    {    
#ifdef _ASYNC
//													tmpSocket->setASyncSend(true,-1,this);
#endif													
													int payLen=ntohs(oMuxPacket.payload.len);
													#ifdef _DEBUG
														oMuxPacket.payload.data[ntohs(oMuxPacket.payload.len)]=0;
														CAMsg::printMsg(LOG_DEBUG,"%u\n%s",ntohs(oMuxPacket.payload.len),oMuxPacket.payload.data);
													#endif
													if(payLen>PAYLOAD_SIZE||tmpSocket->send(oMuxPacket.payload.data,payLen)==SOCKET_ERROR)
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
								else
									{
										#ifdef _DEBUG
											CAMsg::printMsg(LOG_CRIT,"Should never be here!!! New Channel wich Channel detroy packet!\n");
										#endif
									}
							}
						else
							{
								if(oMuxPacket.flags==CHANNEL_CLOSE)
									{
										oSocketGroup.remove(*(oConnection.pSocket));
										oConnection.pSocket->close();
										deleteResume(oMuxPacket.channel);
										muxIn.close(oMuxPacket.channel);
										oSocketList.remove(oMuxPacket.channel);
										delete oConnection.pSocket;
										delete oConnection.pCipher;
									}
								else if(oMuxPacket.flags==CHANNEL_SUSPEND)
									{
										oSocketGroup.remove(*(oConnection.pSocket));
									}
								else if(oMuxPacket.flags==CHANNEL_RESUME)
									{
										oSocketGroup.add(*(oConnection.pSocket));
									}
								else
									{
										oConnection.pCipher->decryptAES(oMuxPacket.data,oMuxPacket.data,DATA_SIZE);
										#ifdef _DEBUG
											oMuxPacket.payload.data[ntohs(oMuxPacket.payload.len)]=0;
											CAMsg::printMsg(LOG_DEBUG,"%u\n%s",ntohs(oMuxPacket.payload.len),oMuxPacket.payload.data);
										#endif
										ret=oConnection.pSocket->send(oMuxPacket.payload.data,ntohs(oMuxPacket.payload.len));
										if(ret==SOCKET_ERROR)
											{
												oSocketGroup.remove(*(oConnection.pSocket));
												oConnection.pSocket->close();
												muxIn.close(oMuxPacket.channel);
												oSocketList.remove(oMuxPacket.channel);
												delete oConnection.pSocket;
												delete oConnection.pCipher;
												deleteResume(oMuxPacket.channel);
											}
										else if(ret==E_QUEUEFULL)
											{
												EnterCriticalSection(&csResume);
												oSuspendList.add(oMuxPacket.channel,oConnection.pSocket,NULL);
												oMuxPacket.flags=CHANNEL_SUSPEND;
												CAMsg::printMsg(LOG_INFO,"Suspending channel %u\n",oMuxPacket.channel);
												muxIn.send(&oMuxPacket);
												LeaveCriticalSection(&csResume);
											}
									}
							}
					}
				if(countRead>0)
					{
						tmpCon=oSocketList.getFirst();
						while(tmpCon!=NULL&&countRead>0)
							{
								if(oSocketGroupMuxIn.select(true,0)!=1)
									goto LOOP_START;
								if(oSocketGroup.isSignaled(*(tmpCon->pSocket)))
									{
										countRead--;
										#ifdef _DEBUG
										    CAMsg::printMsg(LOG_DEBUG,"Receiving Data from Squid!");
										#endif
										ret=tmpCon->pSocket->receive(oMuxPacket.payload.data,PAYLOAD_SIZE);
										if(ret==SOCKET_ERROR||ret==0)
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
												deleteResume(tmpCon->id);
											}
										else 
											{
												oMuxPacket.channel=tmpCon->id;
												oMuxPacket.flags=CHANNEL_DATA;
												oMuxPacket.payload.len=htons((UINT16)ret);
												oMuxPacket.payload.type=0;
												tmpCon->pCipher->decryptAES2(oMuxPacket.data,oMuxPacket.data,DATA_SIZE);
												if(muxIn.send(&oMuxPacket)==SOCKET_ERROR)
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
		tmpCon=oSocketList.getFirst();
		while(tmpCon!=NULL)
			{
				delete tmpCon->pCipher;
				tmpCon->pSocket->close();
				delete tmpCon->pSocket;
				tmpCon=tmpCon->next;
			}
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
				MUXPACKET oMuxPacket;
				oMuxPacket.flags=CHANNEL_RESUME;
				oMuxPacket.channel=oConnection.id;
				muxIn.send(&oMuxPacket);
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

#endif