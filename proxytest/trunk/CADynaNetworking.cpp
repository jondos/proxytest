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
#ifdef DYNAMIC_MIX

#include "CADynaNetworking.hpp"
#include "CASocket.hpp"
#include "CASocketAddrINet.hpp"
#include "CAHttpClient.hpp"
#include "CACmdLnOptions.hpp"
#include "CASocketAddr.hpp"
#include "xml/DOM_Output.hpp"

CADynaNetworking::CADynaNetworking() : CAInfoServiceAware()
	{
	}

CADynaNetworking::~CADynaNetworking()
	{
	}

/**
  * LERNGRUPPE
  * Tries to determine the network configuration of this mix. BEWARE! Any old ListenerInterfaces get
  * deleted. This method does the following:
  * 1) Determine the IP of a local interface
  * 2) With the help of an InfoService determine the public IP
  * 3) Create ListenerInterfaces
  *   a) If public and local IP are the same, create one non-virtual, non-hidden ListenerInterface
  *   b) If public and local IP differ, create one virtual ListenerInterface for the public IP and one hidden ListenerInterface for the local IP
  *
  * @param a_port The port which should be used for the ListenerInterfaces
  * @returns E_SUCCESS if everything went well
  * @returns E_UNKNOWN otherwise
  */
SINT32 CADynaNetworking::updateNetworkConfiguration(UINT16 a_port)
	{
    UINT8 internalIp[255];
    UINT8 externalIp[255];
    UINT32 len = 255;

    if( resolveInternalIp( internalIp ) == E_UNKNOWN )
    {
        return E_UNKNOWN;
    }
    if( resolveExternalIp( externalIp, len ) == E_UNKNOWN )
    {
        return E_UNKNOWN;
    }

    // Beware! This resets all information abount ListenerInterfaces!
    pglobalOptions->resetNetworkConfiguration();

    DOM_Document doc;
    pglobalOptions->getMixXml(doc);
    DOM_Element elemRoot = doc.getDocumentElement();
    DOM_Element elemListeners;
    getDOMChildByName(elemRoot,(UINT8*)"ListenerInterfaces",elemListeners,false);
    if(elemListeners == NULL)
    {
        elemListeners = doc.createElement("ListenerInterfaces");
        elemRoot.appendChild( elemListeners );
    }

    if(strcmp((char*)internalIp, (char*)externalIp) == 0)
    {
        /* External IP == Internal IP, wonderful */
        createListenerInterface( elemListeners, doc, internalIp, a_port, false, false);
        return E_SUCCESS;
    }
    createListenerInterface( elemListeners, doc, internalIp, a_port, true, false);
    createListenerInterface( elemListeners, doc, externalIp, a_port, false, true);


#ifdef DEBUG
    UINT8* buff = DOM_Output::dumpToMem(doc,&len);
    CAMsg::printMsg(LOG_CRIT, (char*)buff);
#endif
    return E_SUCCESS;
	}

/**
  * LERNGRUPPE
  * Creates a ListenerInterface-XML-Node for the given information. With this node the CACmdLnOptions.addListenerInterface-method can
  * be called to add the ListenerInterface to the configuration
  * @param a_ownerDoc The Document (pglobalOptions->getMixXml(doc))
  * @param a_ip The IP address for the ListenerInterface
  * @param a_port The port for the ListenerInterface
  * @param a_bHidden Indicator if the ListenerInterface should be hidden
  * @param a_bVirtual Indicator if the ListenerInterface should be virtual
  * @return r_elemListeners 
  * @retval E_SUCCESS upon success
*/
SINT32 CADynaNetworking::createListenerInterface(DOM_Element r_elemListeners, DOM_Document a_ownerDoc,const UINT8 *a_ip, UINT32 a_port, bool a_bHidden, bool a_bVirtual)
	{
    DOM_Element elemTmp;

    DOM_Element elemListener=a_ownerDoc.createElement("ListenerInterface");
    if( a_bHidden )
    {
        elemListener.setAttribute("hidden", "True");
    }

    if( a_bVirtual )
    {
        elemListener.setAttribute("virtual", "True");
    }
    r_elemListeners.appendChild(elemListener);
    elemTmp=a_ownerDoc.createElement("Port");
    setDOMElementValue(elemTmp,a_port);
    elemListener.appendChild(elemTmp);
    elemTmp=a_ownerDoc.createElement("NetworkProtocol");
    setDOMElementValue(elemTmp,(UINT8*)"RAW/TCP");
    elemListener.appendChild(elemTmp);
    elemTmp=a_ownerDoc.createElement("IP");
    setDOMElementValue(elemTmp, a_ip);
    elemListener.appendChild(elemTmp);
#ifdef DEBUG
    UINT32 len;
    UINT8* buff = DOM_Output::dumpToMem(elemListener, &len);
    CAMsg::printMsg(LOG_CRIT, "%s\n",(char*)buff);
#endif

    pglobalOptions->addListenerInterface(elemListener);
    return E_SUCCESS;
}

/**
  * LERNGRUPPE
  * Returns a working ListenerInterface for this mix
  * @return A working ListenerInterface or NULL if none found
  */
CAListenerInterface *CADynaNetworking::getWorkingListenerInterface()
{
    UINT32 interfaces = pglobalOptions->getListenerInterfaceCount();
    CAListenerInterface *pListener = NULL;
    for( UINT32 i = 1; i <= interfaces; i++ )
    {
        pListener = pglobalOptions->getListenerInterface(i);
        if(!pListener->isVirtual())
            break;
        delete pListener;
        pListener=NULL;
    }
    return pListener;
}

/**
  * LERNGRUPPE
  * This CAThread-Method accepts connections on a ListenerInterface-Socket, reads an
  * echoRequest from it and echoes it back to the caller (InfoService)
  */
static THREAD_RETURN ConnectivityLoop(void *p)
{
	INIT_STACK;
	BEGIN_STACK("CADynaNetworking::ConnectivityLoop");
	
    CADynaNetworking* parent = (CADynaNetworking*)p;
    char buff[1024];
    UINT32 len = 1024;
    SINT32 rLen;
    char *pechoRequest = NULL;

#ifdef DEBUG
    CAMsg::printMsg(LOG_DEBUG, "CADynaNetworking - ConnectivityLoop() started\n");
#endif
    CAListenerInterface *pListener = parent->getWorkingListenerInterface();
    if(pListener == NULL)
    {
        CAMsg::printMsg(LOG_DEBUG, "CADynaNetworking - ConnectivityLoop - No valid ListenerInterface found, aborting\n");
        THREAD_RETURN_ERROR;
    }

    CASocket serverSocket;
    CASocket socket;
    CASocketAddrINet *address = (CASocketAddrINet*)pListener->getAddr();
    if( serverSocket.listen( *address )  != E_SUCCESS )
    {
        CAMsg::printMsg(LOG_ERR, "CADynaNetworking - ConnectivityLoop - Unable to listen to standard port, aborting\n");
        goto EXIT;
    }
#ifdef DEBUG
    CAMsg::printMsg(LOG_ERR, "CADynaNetworking - ConnectivityLoop - Ok, listening...\n");
#endif
    if( serverSocket.accept(socket) != E_SUCCESS)
    {
        CAMsg::printMsg(LOG_ERR, "CADynaNetworking - ConnectivityLoop - Unable to accept on standard port, aborting\n");
        goto EXIT;
    }
#ifdef DEBUG
    CAMsg::printMsg(LOG_ERR, "CADynaNetworking - ConnectivityLoop - Ok, i accepted\n");
#endif
    rLen = socket.receive( (UINT8*)buff, len );
#ifdef DEBUG
    CAMsg::printMsg(LOG_DEBUG, "CADynaNetworking - ConnectivityLoop - Received %i Bits: (%s)\n", rLen, buff);
#endif
    pechoRequest = new char[rLen+1];
    strncpy(pechoRequest, buff, rLen+1);
#ifdef DEBUG
    CAMsg::printMsg(LOG_DEBUG, "CADynaNetworking - ConnectivityLoop - echoRequest: (%s)\n", pechoRequest);
    CAMsg::printMsg(LOG_DEBUG, "CADynaNetworking - ConnectivityLoop - Will send: %i chars, %s\n", strlen(pechoRequest), pechoRequest);

#endif
    socket.send( (const UINT8*)pechoRequest, rLen);
    delete pechoRequest;
    pechoRequest = NULL;
EXIT:
	FINISH_STACK("CADynaNetworking::ConnectivityLoop");

    delete address;
    address = NULL;
    socket.close();
    serverSocket.close();
		THREAD_RETURN_SUCCESS;
}

/**
  * Tests if this mix can communicate with other mixes or a JAP
  * 1) Try to listen on the mixport (ConnectivityLoop)
  * 2) Send request containing this mix' port to the InfoService
  * 3) Wait for the ping echoRequest from the InfoService (ConnectivityLoop)
  * 4) Send ping reply to the InfoService
  * 5) Wait for the InfoService's reply
  *
  * @retval E_SUCCESS if all went well an we could be reached
  * @retval E_UNKNOWN otherwise
*/
SINT32 CADynaNetworking::verifyConnectivity()
{
    SINT32 ret = E_UNKNOWN;
    DOM_Element elemRoot;
    DOM_Node elemRes;
    UINT8 res[255];
    UINT32 len=255;

    CAThread* m_pthreadConnectivtyLoop = new CAThread((UINT8*)"ConnectivityThread");
    m_pthreadConnectivtyLoop->setMainLoop(ConnectivityLoop);
    // 1, 3+4)
    m_pthreadConnectivtyLoop->start(this);

    //@todo FIXME We have a race condition here between the sending of the request to the infoservice and our m_pthreadConnectivtyLoop thread listenting
    // on the port!
    msSleep(1000);
    // 2 + 5)
    CAListenerInterface *pListener = getWorkingListenerInterface();

    if(pListener == NULL)
    {
        CAMsg::printMsg( LOG_DEBUG, "Didn't find a useable ListenerInterface, exiting\n");
        // Well, if we can't get a working ListenerInterface, the Thread should't either and return to us
        m_pthreadConnectivtyLoop->join();
        return E_UNKNOWN;
    }
    CASocketAddrINet *addr = (CASocketAddrINet*)pListener->getAddr();    
    UINT16 port = addr->getPort();
    delete pListener;
    pListener = NULL;
    // End Fixme

    CASocket tmpSock;
    
    if( sendConnectivityRequest( &elemRoot, port) != E_SUCCESS )
    {
        CAMsg::printMsg(LOG_DEBUG, "CADynaNetworking - verifyConnectivity - Unable to query Infoservice for connectivity! This might becomes a problem soon!\n");
       // Release the Thread, maybe it is still blocking in accept
	/** @todo that doesn't look really nice, maybe there is a better way to do this? */
        tmpSock.connect(*addr);
        tmpSock.close();
        m_pthreadConnectivtyLoop->join();
	goto error;
    }
    /** @todo that doesn't look really nice, maybe there is a better way to do this? */
    tmpSock.connect(*addr);
    tmpSock.close();
    m_pthreadConnectivtyLoop->join();

    if(elemRoot == NULL)
    {
        CAMsg::printMsg(LOG_ERR, "CADynaNetworking - verifyConnectivity - InfoService answer was not parsable\n");
	goto error;
    }
    
    if(getDOMChildByName(elemRoot,(UINT8*)"Result",elemRes)!=E_SUCCESS)
    {
        CAMsg::printMsg(LOG_ERR, "CADynaNetworking - verifyConnectivity - InfoService answer was not parsable\n");
	goto error;
    }

    if( getDOMElementValue(elemRes, res, &len)  != E_SUCCESS )
    {
        CAMsg::printMsg(LOG_ERR, "CADynaNetworking - verifyConnectivity - InfoService answer was not parsable\n");
        goto error;
    }

    if( strcmp( (const char*)res, "OK" ) == 0)
        ret = E_SUCCESS;
error:
    delete m_pthreadConnectivtyLoop;
    m_pthreadConnectivtyLoop = NULL;
    delete addr;
    addr = NULL;
    return ret;
}

/**
  * LERNGRUPPE
  * Sends a connectivity request to the InfoService
  * @param a_strRoot The resulting XML Documents root-element's name
  * @param a_port The port to be tested
  * @return r_elemRoot The XML structure to be filled with the result
  * @retval E_SUCCESS
  */
SINT32 CADynaNetworking::sendConnectivityRequest( DOM_Element *r_elemRoot, UINT32 a_port )
{
    DOM_Document doc = DOM_Document::createDocument();
    DOM_Element elemRoot = doc.createElement("ConnectivityCheck");
    doc.appendChild(elemRoot);
    DOM_Element elemPort = doc.createElement("Port");
    setDOMElementValue(elemPort,(UINT32)a_port);
    elemRoot.appendChild(elemPort);

    UINT32 lenOut;
    UINT8* buffOut = DOM_Output::dumpToMem(doc,&lenOut);
    sendRandomInfoserviceRequest( (UINT8*)"/connectivity", r_elemRoot, buffOut, lenOut);

    return E_SUCCESS;
}

/**
  * LERNGRUPPE
  * Tries to determine the internal IP of this mix.
  *
  * @return r_strIp The interal IP
  * @retval E_SUCCESS if all went well
  * @retval E_UNKOWN otherwiseDOM_Document::createDocument();
  */
SINT32 CADynaNetworking::resolveInternalIp(UINT8* r_ip)
{
    struct in_addr in;
    struct hostent *hent;
    char hostname[256];
    UINT32 internalIp;
    if (gethostname(hostname, sizeof(hostname)) < 0)
    {
        return E_UNKNOWN;
    }
    /* Maybe we are lucky ;-) */
		in.s_addr=inet_addr(hostname);
    if (in.s_addr==INADDR_NONE)
    {
        hent = (struct hostent *)gethostbyname(hostname);
        if(hent)
        {
            assert(hent->h_length == 4);
            memcpy( &in.s_addr, hent->h_addr, hent->h_length );

            if( isInternalIp( ntohl(in.s_addr) ) )
            {
                /* Maybe it resolved to 127.0.0.1 or sth., we ask the interface itself*/
                if( getInterfaceIp(&internalIp) == E_SUCCESS)
                {
                    in.s_addr = internalIp;
                }
            }
        }
        else
        {
            if( getInterfaceIp(&internalIp) == E_SUCCESS )
            {
                in.s_addr = internalIp;
            }
        }
    }
    char* tmp = inet_ntoa(in);
    strcpy((char*)r_ip, tmp);
    return E_SUCCESS;
}

/**
  * LERNGRUPPE
  * Tries to determine the internal IP by using a pseudo-connection to an 
  * internet site and then using the source-part
  *
  * @return r_strIp The interal IP
  * @retval E_SUCCESS if all went well
  * @retval E_UNKOWN otherwise
  */
SINT32 CADynaNetworking::getInterfaceIp(UINT32 *r_addr)
{
    CASocket *sock = new CASocket();
    sock->create();
    CASocketAddrINet test;
    // FIXME We could use an InfoService here
    test.setAddr((const UINT8*)"18.0.0.1", 0);
    sock->setSendTimeOut(1);
    sock->connect( test );
    sock->getLocalIP(r_addr);
#ifdef DEBUG
    in_addr t;
    t.s_addr = *r_addr;
    CAMsg::printMsg(LOG_DEBUG, "Local IP is: %s\n", inet_ntoa(t));
#endif
    sock->close();

    delete sock;
    sock = NULL;
    return E_SUCCESS;
}

/**
  * LERNGRUPPE
  * Tries to determine the public IP of this mix.
  *
  * @return r_strIp The public IP
  * @retval E_SUCCESS if all went well
  * @retval E_UNKOWN otherwise
  */
SINT32 CADynaNetworking::resolveExternalIp(UINT8 *r_ip, UINT32 a_len)
{
    UINT8 request[255];

    sprintf((char*) request, "/echoip");

    DOM_Element elemRoot;
    if( sendRandomInfoserviceGetRequest(request, &elemRoot) != E_SUCCESS )
    {
        CAMsg::printMsg(LOG_DEBUG, "CAInfoService::getPublicIp - Unable to query Infoservice for public IP! This will be a problem soon!\n");
        return E_UNKNOWN;
    }

    if(elemRoot == NULL)
    {
        CAMsg::printMsg(LOG_ERR, "CAInfoService::getPublicIp - Answer was not parsable\n");
        return E_UNKNOWN;
    }
    DOM_Node elemIp;

    if(getDOMChildByName(elemRoot,(UINT8*)"IP",elemIp)!=E_SUCCESS)
    {
        CAMsg::printMsg(LOG_ERR, "CAInfoService::getPublicIp - Answer was not parsable!\n");
        return E_UNKNOWN;
    }

    if( getDOMElementValue(elemIp, r_ip, &a_len)  != E_SUCCESS )
    {
        CAMsg::printMsg(LOG_ERR, "CAInfoService::getPublicIp - Answer was not parsable!\n");
        return E_UNKNOWN;
    }

    CAMsg::printMsg(LOG_DEBUG, "Public IP is: %s\n", (char*)r_ip);
    return E_SUCCESS;
}

bool CADynaNetworking::isInternalIp(UINT32 a_ip)
{
    if (((a_ip & 0xff000000) == 0x0a000000) || /*       10/8 */
            ((a_ip & 0xff000000) == 0x00000000) || /*        0/8 */
            ((a_ip & 0xff000000) == 0x7f000000) || /*      127/8 */
            ((a_ip & 0xffff0000) == 0xa9fe0000) || /* 169.254/16 */
            ((a_ip & 0xfff00000) == 0xac100000) || /*  172.16/12 */
            ((a_ip & 0xffff0000) == 0xc0a80000))   /* 192.168/16 */
        return true;
    return false;
}

#endif //DYNAMIC_MIX
