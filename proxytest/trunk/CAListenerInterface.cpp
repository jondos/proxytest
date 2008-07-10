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
#include "CAListenerInterface.hpp"
#include "CAUtil.hpp"
#include "CAMsg.hpp"
#include "CASocketAddrINet.hpp"
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
	#include "CASocketAddrUnix.hpp"
#endif

#ifndef ONLY_LOCAL_PROXY
const char* CAListenerInterface::XML_ELEMENT_CONTAINER_NAME = "ListenerInterfaces";
const char* CAListenerInterface::XML_ELEMENT_NAME = "ListenerInterface";
#endif

CAListenerInterface::CAListenerInterface(void)
	{
		m_bHidden=false;
		m_bVirtual=false;
		m_strHostname=NULL;
		m_pAddr=NULL;
		m_Type=UNKNOWN_NETWORKTYPE;
	}

CAListenerInterface::~CAListenerInterface(void)
	{
		delete m_pAddr;
		m_pAddr = NULL;
		delete[] m_strHostname;
		m_strHostname = NULL;
	}

CAListenerInterface* CAListenerInterface::getInstance(NetworkType type,const UINT8* file)
	{
		#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
			if(	(type!=RAW_UNIX&&type!=SSL_UNIX)||
					file==NULL)
				return NULL;
			CAListenerInterface* pListener=new CAListenerInterface();
			pListener->m_Type=type;
			pListener->m_pAddr=new CASocketAddrUnix();
			((CASocketAddrUnix*)pListener->m_pAddr)->setPath((const char*)file);
			return pListener;
		#else
			return NULL;
		#endif
	}

CAListenerInterface* CAListenerInterface::getInstance(NetworkType type,const UINT8* hostnameOrIP,UINT16 port)
	{
		if(	(type!=RAW_TCP&&type!=SSL_TCP))
			return NULL;
		CAListenerInterface* pListener=new CAListenerInterface();
		pListener->m_Type=type;
		pListener->m_pAddr=new CASocketAddrINet();
		if(((CASocketAddrINet*)pListener->m_pAddr)->setAddr(hostnameOrIP,port)!=E_SUCCESS)
			{
				delete pListener;
				pListener=NULL;
			}
		return pListener;
	}

#ifndef ONLY_LOCAL_PROXY
CAListenerInterface** CAListenerInterface::getInstance(DOMElement* a_elemListenerInterfaces, 
													  UINT32& r_length)
{
	CAListenerInterface** interfaces = NULL;
	if(a_elemListenerInterfaces!=NULL)
	{
		DOMNodeList* nlListenerInterfaces =
			getElementsByTagName(a_elemListenerInterfaces,CAListenerInterface::XML_ELEMENT_NAME);
		r_length=nlListenerInterfaces->getLength();
		if(r_length>0)
		{
			interfaces=new CAListenerInterface*[r_length];
			UINT32 aktInterface=0;
			for(UINT32 i=0;i<r_length;i++)
			{
				DOMNode* elemListenerInterface;
				elemListenerInterface=nlListenerInterfaces->item(i);
				CAListenerInterface* pListener=CAListenerInterface::getInstance(elemListenerInterface);
				if(pListener!=NULL)
				{
					interfaces[aktInterface++]=pListener;
				}
			}
			r_length=aktInterface;
		}
	}	
	else
	{
		r_length = 0;
		interfaces=NULL;
	}
			
	return interfaces;
}


SINT32	CAListenerInterface::toDOMElement(DOMElement* & elemListenerInterface,XERCES_CPP_NAMESPACE::DOMDocument* ownerDoc) const
	{
		elemListenerInterface=createDOMElement(ownerDoc,"ListenerInterface");
		DOMElement* elem=createDOMElement(ownerDoc,"Type");
		elemListenerInterface->appendChild(elem);
		setDOMElementValue(elem,(UINT8*)"RAW/TCP");
		elem=createDOMElement(ownerDoc,"Port");
		elemListenerInterface->appendChild(elem);
		UINT32 port=((CASocketAddrINet*)m_pAddr)->getPort();
		setDOMElementValue(elem,port);
		elem=createDOMElement(ownerDoc,"Host");
		elemListenerInterface->appendChild(elem);
		UINT8 ip[50];
		if(m_strHostname!=NULL)
			setDOMElementValue(elem,m_strHostname);
		else 
			{
				((CASocketAddrINet*)m_pAddr)->getIPAsStr(ip,50);
				setDOMElementValue(elem,ip);
			}
		elem=createDOMElement(ownerDoc,"IP");
		elemListenerInterface->appendChild(elem);
		((CASocketAddrINet*)m_pAddr)->getIPAsStr(ip,50);
		setDOMElementValue(elem,ip);
		return E_SUCCESS;
	}

CAListenerInterface* CAListenerInterface::getInstance(const DOMNode* elemListenerInterface)
	{
		if(	elemListenerInterface==NULL||
				elemListenerInterface->getNodeType()!=DOMNode::ELEMENT_NODE)//||
				//!elemListenerInterface.getNodeName().equals("ListenerInterface"))
			return NULL;
		CAListenerInterface* pListener=new CAListenerInterface();
		getDOMElementAttribute(elemListenerInterface,"hidden",pListener->m_bHidden);
		getDOMElementAttribute(elemListenerInterface,"virtual",pListener->m_bVirtual);
		DOMNode* elemType=NULL;
		getDOMChildByName(elemListenerInterface,"NetworkProtocol",elemType,false);
		if (elemType == NULL)
		{
			getDOMChildByName(elemListenerInterface,"Type",elemType,false);
		}
		UINT32 tmpLen = 255;
		UINT8 tmpBuff[255];
		if (elemType != NULL)
		{
			if(getDOMElementValue(elemType,tmpBuff,&tmpLen)!=E_SUCCESS)
				goto ERR;
			strtrim(tmpBuff);
			if(strcmp((char*)tmpBuff,"RAW/TCP")==0)
				pListener->m_Type=RAW_TCP;
			else if(strcmp((char*)tmpBuff,"RAW/UNIX")==0)
				pListener->m_Type=RAW_UNIX;
			else if(strcmp((char*)tmpBuff,"SSL/TCP")==0)
				pListener->m_Type=SSL_TCP;
			else if(strcmp((char*)tmpBuff,"SSL/UNIX")==0)
				pListener->m_Type=SSL_UNIX;
			else if (strcmp((char*)tmpBuff,"HTTP/TCP")==0)
			{
				pListener->m_Type=HTTP_TCP;
			}
			else
				goto ERR;
		}
		else
		{
			// infoservice old style <= config version 0.61
			pListener->m_Type=HTTP_TCP;
		}
		if(pListener->m_Type==SSL_TCP||pListener->m_Type==RAW_TCP
		   ||pListener->m_Type==HTTP_TCP)
			{ 
				DOMNode* elemIP=NULL;
				DOMElement* elemPort=NULL;
				DOMNode* elemHost=NULL;
				getDOMChildByName(elemListenerInterface,"Port",elemPort,false);
				UINT32 port;
				if(getDOMElementValue(elemPort,&port)!=E_SUCCESS)
					goto ERR;
				pListener->m_pAddr=new CASocketAddrINet;
				getDOMChildByName(elemListenerInterface,"IP",elemIP,false);
				if(elemIP!=NULL)
					{
						UINT8 buffIP[50];
						UINT32 buffIPLen=50;
						if(getDOMElementValue(elemIP,buffIP,&buffIPLen)!=E_SUCCESS)
							goto ERR;
						if(((CASocketAddrINet*)pListener->m_pAddr)->setAddr(buffIP,(UINT16)port)!=E_SUCCESS)
							goto ERR;
					}
				getDOMChildByName(elemListenerInterface,"Host",elemHost,false);
				tmpLen=255;										
				if(getDOMElementValue(elemHost,tmpBuff,&tmpLen)==E_SUCCESS)
					{
						tmpBuff[tmpLen]=0;
						if(elemIP==NULL&&((CASocketAddrINet*)pListener->m_pAddr)->setAddr(tmpBuff,(UINT16)port)!=E_SUCCESS)
							goto ERR;
						pListener->m_strHostname=new UINT8[tmpLen+1];
						memcpy(pListener->m_strHostname,tmpBuff,tmpLen);
						pListener->m_strHostname[tmpLen]=0;
					}
				else if(elemIP==NULL)
					goto ERR;
			}
		else
			#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
				{
					DOMElement* elemFile=NULL;
					getDOMChildByName(elemListenerInterface,"File",elemFile,false);
					tmpLen=255;
					if(getDOMElementValue(elemFile,tmpBuff,&tmpLen)!=E_SUCCESS)
						goto ERR;
					tmpBuff[tmpLen]=0;
					strtrim(tmpBuff);
					pListener->m_pAddr=new CASocketAddrUnix;
					if(((CASocketAddrUnix*)pListener->m_pAddr)->setPath((char*)tmpBuff)!=E_SUCCESS)
						goto ERR;
					pListener->m_strHostname=NULL;
				}
			#else
				goto ERR;
			#endif
		return pListener;
ERR:
		delete pListener;
		pListener = NULL;
		return NULL;
}
#endif //ONLY_LOCAL_PROXY
