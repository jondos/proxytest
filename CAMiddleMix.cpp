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
#include "CASingleSocketGroup.hpp"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocketAddrUnix.hpp"
#include "CAThread.hpp"
#include "CAInfoService.hpp"
#include "CAUtil.hpp"
#include "xml/DOM_Output.hpp"

extern CACmdLnOptions options;

SINT32 CAMiddleMix::initOnce()
	{
		CAMsg::printMsg(LOG_DEBUG,"Starting MiddleMix InitOnce\n");
		int handle;
		SINT32 len;
		UINT8* fileBuff=new UINT8[2048];
		if(fileBuff==NULL||options.getKeyFileName(fileBuff,2048)!=E_SUCCESS)
			goto END;
		handle=open((char*)fileBuff,O_BINARY|O_RDONLY);
		if(handle==-1)
			goto END;
		len=read(handle,fileBuff,2048);
		close(handle);
		if(len<1)
			goto END;
		m_pSignature=new CASignature();
		if(m_pSignature->setSignKey(fileBuff,len,SIGKEY_XML)!=E_SUCCESS)
			{
				delete m_pSignature;
				m_pSignature=NULL;
				goto END;
			}
END:		
		delete []fileBuff;
		return E_SUCCESS;
	}

SINT32 CAMiddleMix::proccessKeyExchange()
	{
		UINT8* recvBuff=NULL;
//		UINT8* infoBuff=NULL;
		
		UINT16 len;
		if(((CASocket*)*m_pMuxOut)->receiveFully((UINT8*)&len,2)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Error receiving Key Info lenght!\n");
				return E_UNKNOWN;
			}
		len=ntohs(len);
		CAMsg::printMsg(LOG_INFO,"Received Key Info lenght %u\n",len);
		recvBuff=new UINT8[len+1]; //for the \0 at the end
		if(recvBuff==NULL)
			return E_UNKNOWN;
		if(((CASocket*)*m_pMuxOut)->receiveFully(recvBuff,len)!=E_SUCCESS)
			{
				delete recvBuff;
				return E_UNKNOWN;
			}
		recvBuff[len]=0; //make a string

		CAMsg::printMsg(LOG_INFO,"Received Key Info...\n");
		
				
		CAMsg::printMsg(LOG_INFO,"%s\n",recvBuff);
		MemBufInputSource oInput(recvBuff,len,"tmpID");
		DOMParser oParser;
		oParser.parse(oInput);		
		DOM_Document doc=oParser.getDocument();
		delete recvBuff;

		DOM_Element root=doc.getDocumentElement();
		
		DOMString tmpDOMStr=root.getAttribute("count");
		char* tmpStr=tmpDOMStr.transcode();
		if(tmpStr==NULL)
			{//TODO all deletes...
				return E_UNKNOWN;
			}
		UINT32 count=atol(tmpStr);
		delete tmpStr;
		count++;
		setDOMElementAttribute(root,"count",count);
		
		DOM_Element mixNode=doc.createElement(DOMString("Mix"));
		UINT8 tmpBuff[50];
		options.getMixId(tmpBuff,50); //the mix id...
		mixNode.setAttribute("id",DOMString((char*)tmpBuff));

		DOM_DocumentFragment* pDocFragment=NULL;
		m_RSA.getPublicKeyAsDocumentFragment(pDocFragment); //the key
		mixNode.appendChild(doc.importNode(*pDocFragment,true));
		delete pDocFragment;
		root.appendChild(mixNode);
//		delete mixNode;

		recvBuff=new UINT8[2048];
		UINT32 outlen=2048;
		DOM_Output::dumpToMem(doc,recvBuff+2,&outlen);
		#ifdef _DEBUG
			CAMsg::printMsg(LOG_DEBUG,"New Key Info size: %u\n",outlen);
		#endif
		len=htons(outlen);
		memcpy(recvBuff,&len,2);
		if(((CASocket*)*m_pMuxIn)->send(recvBuff,outlen+2)==-1)
			{
				CAMsg::printMsg(LOG_DEBUG,"Error sending new New Key Info\n");
				return E_UNKNOWN;
			}
		else
			CAMsg::printMsg(LOG_DEBUG,"Sending new New Key Info succeded\n");
		delete recvBuff;
//		delete doc;
		return E_SUCCESS;
	}

SINT32 CAMiddleMix::init()
	{		
		CAMsg::printMsg(LOG_INFO,"Creating Key...\n");
		if(m_RSA.generateKeyPair(1024)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Init: Error generating Key-Pair...\n");
				return E_UNKNOWN;		
			}
		
		UINT8 strTarget[255];
		memset(strTarget,0,255);
		UINT8 path[255];
		CASocketAddr* pAddrNext;
		options.getTargetHost(strTarget,255);
		if(strTarget[0]=='/') //unix domain
			{
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
				pAddrNext=new CASocketAddrUnix();
				((CASocketAddrUnix*)pAddrNext)->setPath((char*)strTarget);
#else
				CAMsg::printMsg(LOG_CRIT,"I do not understand the Unix Domain Protocol!\n");
				return E_UNKNOWN;
#endif
			}
		else
			{
				pAddrNext=new CASocketAddrINet();
				((CASocketAddrINet*)pAddrNext)->setAddr(strTarget,options.getTargetPort());
			}

		
		m_pMuxOut=new CAMuxSocket();

		if(((CASocket*)*m_pMuxOut)->create(pAddrNext->getType())!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Init: Cannot create SOCKET for outgoing conncetion...\n");
				return E_UNKNOWN;
			}
		((CASocket*)*m_pMuxOut)->setRecvBuff(50*MIXPACKET_SIZE);
		((CASocket*)*m_pMuxOut)->setSendBuff(50*MIXPACKET_SIZE);
#define RETRIES 100
#define RETRYTIME 30
		CAMsg::printMsg(LOG_INFO,"Init: Try to connect to next Mix: %s...\n",strTarget);
		if(m_pMuxOut->connect(*pAddrNext,RETRIES,RETRYTIME)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Cannot connect to next Mix -- Exiting!\n");
				return E_UNKNOWN;
			}
//		mSocketGroup.add(muxOut);
		CAMsg::printMsg(LOG_INFO," connected!\n");
		if(((CASocket*)*m_pMuxOut)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)*m_pMuxOut)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}
		
		CASocketAddr* pAddrListen;
		memset(path,0,255);
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
				if(path[0]==0) //empty host
					((CASocketAddrINet*)pAddrListen)->setPort(options.getServerPort());
				else	
					((CASocketAddrINet*)pAddrListen)->setAddr(path,options.getServerPort());
			}

		m_pMuxIn=new CAMuxSocket();
		if(m_pMuxIn->accept(*pAddrListen)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Error waiting for previous Mix... -- Exiting!\n");				
				return E_UNKNOWN;
			}
		((CASocket*)*m_pMuxIn)->setRecvBuff(50*MIXPACKET_SIZE);
		((CASocket*)*m_pMuxIn)->setSendBuff(50*MIXPACKET_SIZE);
		if(((CASocket*)*m_pMuxIn)->setKeepAlive((UINT32)1800)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_INFO,"Socket option TCP-KEEP-ALIVE returned an error - so not set!\n");
				if(((CASocket*)*m_pMuxIn)->setKeepAlive(true)!=E_SUCCESS)
					CAMsg::printMsg(LOG_INFO,"Socket option KEEP-ALIVE returned an error - so also not set!\n");
			}
		

		proccessKeyExchange();

		m_pMiddleMixChannelList=new CAMiddleMixChannelList();
		
		return E_SUCCESS;
	}

	
THREAD_RETURN loopDownStream(void *p)
	{
		CAMiddleMix* pMix=(CAMiddleMix*)p;
		HCHANNEL channelIn;
		CASymCipher* pCipher;
		MIXPACKET* pMixPacket=new MIXPACKET;
		SINT32 ret;
		CASingleSocketGroup oSocketGroup;
		oSocketGroup.add(*(pMix->m_pMuxOut));
		for(;;)
			{
				ret=oSocketGroup.select(false,1000);
				if(ret!=1)
					{
						if(ret==E_TIMEDOUT)
							continue;
						else
							{
								CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Fehler bei select() -- goto ERR!\n");
								goto ERR;
							}
					}
				else
					{
						ret=pMix->m_pMuxOut->receive(pMixPacket);
						if(ret==SOCKET_ERROR)
								{
									CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Fehler bei receive() -- goto ERR!\n");
									goto ERR;
								}
						if(pMix->m_pMiddleMixChannelList->getOutToIn(&channelIn,pMixPacket->channel,&pCipher)==E_SUCCESS)
							{//connection found
								if(pMixPacket->flags!=CHANNEL_CLOSE)
									{
										pMixPacket->channel=channelIn;
										pCipher->decryptAES2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										pCipher->unlock();
										if(pMix->m_pMuxIn->send(pMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
								else
									{//connection should be closed
										pCipher->unlock();
										if(pMix->m_pMuxIn->close(channelIn)==SOCKET_ERROR)
											goto ERR;
										pMix->m_pMiddleMixChannelList->remove(channelIn);
									}
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"loopDownStream -- Exiting!\n");
		delete pMixPacket;
		pMix->m_pMuxIn->close();
		pMix->m_pMuxOut->close();
		THREAD_RETURN_SUCCESS;		
	}


SINT32 CAMiddleMix::loop()
	{
		CAInfoService* pInfoService=NULL;
		if(m_pSignature!=NULL&&options.isInfoServiceEnabled())
			{
				pInfoService=new CAInfoService();
				pInfoService->setSignature(m_pSignature);
				pInfoService->sendHelo();
				pInfoService->start();
			}
		MIXPACKET* pMixPacket=new MIXPACKET;
		HCHANNEL channelOut;
		CASymCipher* pCipher;
		SINT32 ret;
		UINT8* tmpRSABuff=new UINT8[RSA_SIZE];
		CASingleSocketGroup oSocketGroup;
		oSocketGroup.add(*m_pMuxIn);
		m_pMuxIn->setCrypt(true);
		m_pMuxOut->setCrypt(true);
		CAThread oThread;
		oThread.setMainLoop(loopDownStream);
		oThread.start(this);
		for(;;)
			{
				ret=oSocketGroup.select(false,1000);
				if(ret!=1)
					{
						if(ret==E_TIMEDOUT)
							continue;
						else
							{
								CAMsg::printMsg(LOG_CRIT,"loopUpStream -- Fehler bei select() -- goto ERR!\n");
								goto ERR;
							}
					}
				else
					{
						ret=m_pMuxIn->receive(pMixPacket);
						if(ret==SOCKET_ERROR)
							{
								CAMsg::printMsg(LOG_CRIT,"Fehler beim Empfangen -- Exiting!\n");
								goto ERR;
							}
						if(m_pMiddleMixChannelList->getInToOut(pMixPacket->channel,&channelOut,&pCipher)!=E_SUCCESS)
							{//new connection
								if(pMixPacket->flags==CHANNEL_OPEN_OLD||pMixPacket->flags==CHANNEL_OPEN_NEW)
									{
										#ifdef _DEBUG
												CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
										#endif
										pCipher=new CASymCipher();
										m_RSA.decrypt(pMixPacket->data,tmpRSABuff);
										pCipher->setKeyAES(tmpRSABuff);
										pCipher->decryptAES(pMixPacket->data+RSA_SIZE,
																				pMixPacket->data+RSA_SIZE-KEY_SIZE,
																				DATA_SIZE-RSA_SIZE);
										memcpy(pMixPacket->data,tmpRSABuff+KEY_SIZE,RSA_SIZE-KEY_SIZE);
										m_pMiddleMixChannelList->add(pMixPacket->channel,pCipher,&channelOut);
										pMixPacket->channel=channelOut;
										if(m_pMuxOut->send(pMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
							}
						else
							{//established connection
								if(pMixPacket->flags==CHANNEL_CLOSE)
									{
										pCipher->unlock();
										if(m_pMuxOut->close(channelOut)==SOCKET_ERROR)
											goto ERR;
										m_pMiddleMixChannelList->remove(pMixPacket->channel);
									}
								else
									{
										pMixPacket->channel=channelOut;
										pCipher->decryptAES(pMixPacket->data,pMixPacket->data,DATA_SIZE);
										pCipher->unlock();
										if(m_pMuxOut->send(pMixPacket)==SOCKET_ERROR)
											goto ERR;
									}
							}
					}
			}
ERR:
		CAMsg::printMsg(LOG_CRIT,"Preparing for restart...\n");
		m_pMuxIn->close();
		m_pMuxOut->close();
		oThread.join();		

		CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
		delete tmpRSABuff;
		delete pMixPacket;
		if(pInfoService!=NULL)
			delete pInfoService;
		return E_UNKNOWN;
	}
SINT32 CAMiddleMix::clean()
	{
		if(m_pMuxIn!=NULL)
			{
				m_pMuxIn->close();
				delete m_pMuxIn;
			}
		m_pMuxIn=NULL;
		if(m_pMuxOut!=NULL)
			{
				m_pMuxOut->close();
				delete m_pMuxOut;
			}
		m_pMuxOut=NULL;
		m_RSA.destroy();
		if(m_pMiddleMixChannelList!=NULL)
			delete m_pMiddleMixChannelList;
		m_pMiddleMixChannelList=NULL;
		return E_SUCCESS;
	}
