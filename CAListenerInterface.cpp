#include "StdAfx.h"
#include "CAListenerInterface.hpp"
#include "CAUtil.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocketAddrUnix.hpp"

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
		delete m_strHostname;
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

SINT32	CAListenerInterface::toDOMFragment(DOM_DocumentFragment& fragment,DOM_Document& ownerDoc) const
	{
		fragment=ownerDoc.createDocumentFragment();
		DOM_Element elemListenerInterface=ownerDoc.createElement("ListenerInterface");
		fragment.appendChild(elemListenerInterface);
		DOM_Element elem=ownerDoc.createElement("Type");
		elemListenerInterface.appendChild(elem);
		setDOMElementValue(elem,(UINT8*)"RAW/TCP");
		elem=ownerDoc.createElement("Port");
		elemListenerInterface.appendChild(elem);
		UINT32 port=((CASocketAddrINet*)m_pAddr)->getPort();
		setDOMElementValue(elem,port);
		elem=ownerDoc.createElement("Host");
		elemListenerInterface.appendChild(elem);
		UINT8 ip[50];
		if(m_strHostname!=NULL)
			setDOMElementValue(elem,m_strHostname);
		else 
			{
				((CASocketAddrINet*)m_pAddr)->getIPAsStr(ip,50);
				setDOMElementValue(elem,ip);
			}
		elem=ownerDoc.createElement("IP");
		elemListenerInterface.appendChild(elem);
		((CASocketAddrINet*)m_pAddr)->getIPAsStr(ip,50);
		setDOMElementValue(elem,ip);
		return E_SUCCESS;
	}

CAListenerInterface* CAListenerInterface::getInstance(const DOM_Node& elemListenerInterface)
	{
		if(	elemListenerInterface==NULL||
				elemListenerInterface.getNodeType()!=DOM_Node::ELEMENT_NODE||
				!elemListenerInterface.getNodeName().equals("ListenerInterface"))
			return NULL;
		CAListenerInterface* pListener=new CAListenerInterface();
		getDOMElementAttribute(elemListenerInterface,"hidden",pListener->m_bHidden);
		getDOMElementAttribute(elemListenerInterface,"virtual",pListener->m_bVirtual);
		DOM_Element elemType;
		getDOMChildByName(elemListenerInterface,(UINT8*)"NetworkProtocol",elemType,false);
		UINT8 tmpBuff[255];
		UINT32 tmpLen=255;
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
		else
			goto ERR;
		if(pListener->m_Type==SSL_TCP||pListener->m_Type==RAW_TCP)
			{
				DOM_Element elemIP;
				DOM_Element elemPort;
				DOM_Element elemHost;
				getDOMChildByName(elemListenerInterface,(UINT8*)"Port",elemPort,false);
				UINT32 port;
				if(getDOMElementValue(elemPort,&port)!=E_SUCCESS)
					goto ERR;
				pListener->m_pAddr=new CASocketAddrINet;
				getDOMChildByName(elemListenerInterface,(UINT8*)"IP",elemIP,false);
				if(elemIP!=NULL)
					{
						UINT8 buffIP[50];
						UINT32 buffIPLen=50;
						if(getDOMElementValue(elemIP,buffIP,&buffIPLen)!=E_SUCCESS)
							goto ERR;
						if(((CASocketAddrINet*)pListener->m_pAddr)->setAddr(buffIP,port)!=E_SUCCESS)
							goto ERR;
					}
				getDOMChildByName(elemListenerInterface,(UINT8*)"Host",elemHost,false);
				tmpLen=255;										
				if(getDOMElementValue(elemHost,tmpBuff,&tmpLen)==E_SUCCESS)
					{
						tmpBuff[tmpLen]=0;
						if(elemIP==NULL&&((CASocketAddrINet*)pListener->m_pAddr)->setAddr(tmpBuff,port)!=E_SUCCESS)
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
					DOM_Element elemFile;
					getDOMChildByName(elemListenerInterface,(UINT8*)"File",elemFile,false);
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
		return NULL;
}
