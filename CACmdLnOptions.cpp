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
#include "CACmdLnOptions.hpp"
#include "CAUtil.hpp"
#include "CAMix.hpp"
#include "CAMsg.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocket.hpp"
#include "CAXMLBI.hpp"
#include "xml/DOM_Output.hpp"
#include "CABase64.hpp"
#include "CADynaNetworking.hpp"
#ifdef LOG_CRIME
	#include "tre/regex.h"
#endif

CACmdLnOptions::CACmdLnOptions()
  {
		m_bDaemon=false;
		m_bSyslog=false;
		m_bSocksSupport = false;
		m_bLocalProxy=m_bFirstMix=m_bLastMix=m_bMiddleMix=false;
#ifndef ONLY_LOCAL_PROXY
		m_bIsRunReConfigure=false;
		m_addrInfoServices = NULL;
		m_addrInfoServicesSize=0;
		m_pcsReConfigure=new CAMutex();
		m_pSignKey=NULL;
		m_pOwnCertificate=NULL;
		m_OpCerts=NULL;
		m_pPrevMixCertificate=NULL;
		m_pNextMixCertificate=NULL;
		m_bCompressedLogs=false;
		m_pLogEncryptionCertificate=NULL;
		m_bIsEncryptedLogEnabled=false;
		m_docMixInfo=NULL;
#endif //ONLY_LOCAL_PROXY
		m_iTargetPort=m_iSOCKSPort=m_iSOCKSServerPort=0xFFFF;
		m_strTargetHost=m_strSOCKSHost=NULL;
		m_strUser=m_strCascadeName=m_strLogDir=m_strEncryptedLogDir=NULL;
		m_maxNrOfUsers = 0;
		m_arTargetInterfaces=NULL;
		m_cnTargets=0;
		m_arListenerInterfaces=NULL;
		m_cnListenerInterfaces=0;
		m_arStrVisibleAddresses=NULL;
		m_cnVisibleAddresses=0;
		m_nrOfOpenFiles=-1;
		m_strMixID=NULL;
		m_bAutoReconnect=false;
		m_strConfigFile=NULL;
		m_strPidFile=NULL;
#ifdef PAYMENT
		m_pBI=NULL;
		m_strDatabaseHost=NULL;
		m_strDatabaseName=NULL;
		m_strDatabaseUser=NULL;
		m_strDatabasePassword=NULL;
		m_strAiID=NULL;
#endif
#ifdef DYNAMIC_MIX
		m_strLastCascadeProposal = NULL;
#endif
 }

CACmdLnOptions::~CACmdLnOptions()
	{
		cleanup();
	}

/** This is the final cleanup, which deletes every resource (including any locks necessary to synchronise read/write to properties).
*/
SINT32 CACmdLnOptions::cleanup()
	{
		clean();
#ifndef ONLY_LOCAL_PROXY
		delete m_pcsReConfigure;
		m_pcsReConfigure=NULL;
#endif
		return E_SUCCESS;
	}

/** Deletes all information about the target interfaces.
	*/
SINT32 CACmdLnOptions::clearTargetInterfaces()
	{
		if(m_arTargetInterfaces!=NULL)
			{
				for(UINT32 i=0;i<m_cnTargets;i++)
					delete m_arTargetInterfaces[i].addr;
				delete[] m_arTargetInterfaces;
			}
		m_cnTargets=0;
		m_arTargetInterfaces=NULL;
		return E_SUCCESS;
	}

/** Deletes all information about the listener interfaces.
	*/
SINT32 CACmdLnOptions::clearListenerInterfaces()
	{
		if(m_arListenerInterfaces!=NULL)
			{
				for(UINT32 i=0;i<m_cnListenerInterfaces;i++)
					{
						delete m_arListenerInterfaces[i];
					}
				delete[] m_arListenerInterfaces;
			}
		m_cnListenerInterfaces=0;
		m_arListenerInterfaces=NULL;
		return E_SUCCESS;
	}

#ifndef ONLY_LOCAL_PROXY
/** Deletes all information about the visible addresses.
	*/
SINT32 CACmdLnOptions::clearVisibleAddresses()
	{
		if(m_arStrVisibleAddresses!=NULL)
			{
				for(UINT32 i=0;i<m_cnVisibleAddresses;i++)
					delete[] m_arStrVisibleAddresses[i];
				delete[] m_arStrVisibleAddresses;
			}
		m_cnVisibleAddresses=0;
		m_arStrVisibleAddresses=NULL;
		return E_SUCCESS;
	}

/** Add all the visible addresses to the list of visible addresses found in the XML description of the \<Proxy\> element given.
	* The structur is as follows:
	*@verbatim
	* <Proxy>
	*      <VisibleAddresses> <!-- Describes the visible addresses from the 'outside world' -->
 *       <VisibleAddress>
  *        <Host> <!-- Host or IP -->
   *       </Host>
   *     </VisibleAddress>
   *   </VisibleAddresses>
*
	* </Proxy>
	@endverbatim
	*/
SINT32 CACmdLnOptions::addVisibleAddresses(DOM_Node& nodeProxy)
	{
		if(nodeProxy==NULL)
			return E_UNKNOWN;
		if(!nodeProxy.getNodeName().equals("Proxy"))
			return E_UNKNOWN;
		DOM_Node elemVisAdresses;
		getDOMChildByName(nodeProxy,(UINT8*)"VisibleAddresses",elemVisAdresses);
		DOM_Node elemVisAddress;
		getDOMChildByName(elemVisAdresses,(UINT8*)"VisibleAddress",elemVisAddress);
		while(elemVisAddress!=NULL)
			{
				if(elemVisAddress.getNodeName().equals("VisibleAddress"))
					{
						DOM_Element elemHost;
						if(getDOMChildByName(elemVisAddress,(UINT8*)"Host",elemHost)==E_SUCCESS)
							{
								UINT8 tmp[255];
								UINT32 len=255;
								if(getDOMElementValue(elemHost,tmp,&len)==E_SUCCESS)
									{//append the new address to the list of addresses
										UINT8** tmpAr=new UINT8*[m_cnVisibleAddresses+1];
										if(m_arStrVisibleAddresses!=NULL)
											{
												memcpy(tmpAr,m_arStrVisibleAddresses,m_cnVisibleAddresses*sizeof(UINT8*));
												delete[] m_arStrVisibleAddresses;
											}
										tmpAr[m_cnVisibleAddresses]=new UINT8[len+1];
										memcpy(tmpAr[m_cnVisibleAddresses],tmp,len+1);
										m_cnVisibleAddresses++;
										m_arStrVisibleAddresses=tmpAr;	
									}
							}
					}
				elemVisAddress=elemVisAddress.getNextSibling();	
			}
		return E_SUCCESS;
	}

SINT32 CACmdLnOptions::getVisibleAddress(UINT8* strAddressBuff, UINT32 len,UINT32 nr)
	{
		if(strAddressBuff==NULL||nr==0||nr>m_cnVisibleAddresses)
			{
				return E_UNKNOWN;
			}
		if(strlen((char*)m_arStrVisibleAddresses[nr-1]	)>=len)
			{
				return E_SPACE;
			}
		strcpy((char*)strAddressBuff,(char*)m_arStrVisibleAddresses[nr-1]);
		return E_SUCCESS;	
	}
#endif //ONLY_LOCAL_PROXY

/** Deletes and resssource allocated by objects of this class EXPECT the locks necessary to controll access to the properties of this class*/
void CACmdLnOptions::clean()
  {
		if(m_strConfigFile!=NULL)
			{
				delete[] m_strConfigFile;
				m_strConfigFile=NULL;
			}
		if(m_strTargetHost!=NULL)
			{
				delete[] m_strTargetHost;
	    }
		m_strTargetHost=NULL;
		if(m_strSOCKSHost!=NULL)
			{
				delete[] m_strSOCKSHost;
	    }
		m_strSOCKSHost=NULL;
#ifndef ONLY_LOCAL_PROXY
		if (m_addrInfoServices != NULL)
			{
	    	for (UINT32 i = 0; i < m_addrInfoServicesSize; i++)
	    		{
	    			delete m_addrInfoServices[i];
	    		}
	    	delete[] m_addrInfoServices;
				m_addrInfoServices=NULL;
	    	m_addrInfoServicesSize = 0;
	    }
#endif //ONLY_LOCAL_PROXY

		if(m_strCascadeName!=NULL)
			delete[] m_strCascadeName;
		m_strCascadeName=NULL;
		if(m_strLogDir!=NULL)
			delete[] m_strLogDir;
		m_strLogDir=NULL;
		if(m_strPidFile!=NULL)
			delete[] m_strPidFile;
		m_strPidFile=NULL;
		if(m_strEncryptedLogDir!=NULL)
			delete[] m_strEncryptedLogDir;
		m_strEncryptedLogDir=NULL;
		if(m_strUser!=NULL)
			delete[] m_strUser;
		m_strUser=NULL;
		if(m_strMixID!=NULL)
			delete[] m_strMixID;
		m_strMixID=NULL;
		clearTargetInterfaces();
		clearListenerInterfaces();
#ifndef ONLY_LOCAL_PROXY
		if(m_docMixInfo!=NULL)
			m_docMixInfo=NULL;
		clearVisibleAddresses();
		if(m_pSignKey!=NULL)
			delete m_pSignKey;
		m_pSignKey=NULL;
		if(m_pOwnCertificate!=NULL)
			delete m_pOwnCertificate;
		m_pOwnCertificate=NULL;
		// deleting whole array and array elements
		if (m_OpCerts != NULL)
		{
			if (m_OpCertsLength > 0)
			{
				for (UINT32 i = 0; i < m_OpCertsLength; i++)
				{
					delete m_OpCerts[i];
				}
			}
			delete[] m_OpCerts;
		}
		m_OpCerts=NULL;
		if(m_pNextMixCertificate!=NULL)
			delete m_pNextMixCertificate;
		m_pNextMixCertificate=NULL;
		if(m_pPrevMixCertificate!=NULL)
			delete m_pPrevMixCertificate;
		m_pPrevMixCertificate=NULL;
		if(m_pLogEncryptionCertificate!=NULL)
			delete m_pLogEncryptionCertificate;
		m_pLogEncryptionCertificate=NULL;
#endif //ONLY_LOCAL_PROXY
}

SINT32 CACmdLnOptions::parse(int argc,const char** argv)
    {
	int iDaemon=0;
  char* target=NULL;
	int iLocalProxy=0;
	int SOCKSport=-1;
	char* socks=NULL;
	char* logdir=NULL;
	int iCompressedLogs=0;
	char* serverPort=NULL;
	int iVersion=0;
	char* configfile=NULL;
	int iAutoReconnect=0;
	char* strPidFile=NULL;
	char* strCreateConf=0;
	//DOM_Document docMixXml;
	poptOption theOptions[]=
	 {
		{"localproxy",'j',POPT_ARG_NONE,&iLocalProxy,0,"act as local proxy",NULL},
		{"daemon",'d',POPT_ARG_NONE,&iDaemon,0,"start as daemon [only for local proxy]",NULL},
		{"next",'n',POPT_ARG_STRING,&target,0,"first mix of cascade [only for local proxy]","<ip:port>"},
		{"autoreconnect",'a',POPT_ARG_NONE,&iAutoReconnect,0,"auto reconnects if connection to first mix was lost [only for local proxy]",NULL},
		{"port",'p',POPT_ARG_STRING,&serverPort,0,"listening on [host:]port|path [only for local proxy]","<[host:]port|path>"},
		{"socksport",'s',POPT_ARG_INT,&SOCKSport,0,"listening port for socks","<portnumber>"},
		{"logdir",'l',POPT_ARG_STRING,&logdir,0,"directory where log files go to [only for local proxy]","<dir>"},
#ifdef COMPRESSED_LOGS
		{"gzip",'z',POPT_ARG_NONE,&iCompressedLogs,0,"create gziped logs",NULL},
#endif
		{"config",'c',POPT_ARG_STRING,&configfile,0,"config file to use [for a real Mix in a cascade]","<file>"},
		{"version",'v',POPT_ARG_NONE,&iVersion,0,"show version",NULL},
		{"pidfile",'r',POPT_ARG_STRING,&strPidFile,0,"file where the PID will be stored","<file>"},
		{"createConf",0,POPT_ARG_STRING,&strCreateConf,0,"creates a generic configuration for MixOnCD","[<file>]"},
		POPT_AUTOHELP
		{NULL,0,0,
		NULL,0,NULL,NULL}
	};
	poptContext ctx=poptGetContext(NULL,argc,argv,theOptions,0);
	SINT32 ret=poptGetNextOpt(ctx);
	while(ret==POPT_ERROR_BADOPT)
		ret=poptGetNextOpt(ctx);
	poptFreeContext(ctx);
	if(iVersion!=0)
		{
			printf(MIX_VERSION_INFO);
			for(UINT32 t=0;t<10000;t++)
				{
					CASocket* pSocket=new CASocket;
					if(pSocket->create(false)!=E_SUCCESS)
						{
							printf("Max open sockets: %u\n",t);
							exit(0);
						}
				}
			printf("Max open sockets: >10000\n");
			exit(0);
		}
#ifndef ONLY_LOCAL_PROXY
	if(strCreateConf!=NULL)
		{
			createMixOnCDConfiguration((UINT8*)strCreateConf);
			exit(0);
		}
#endif
	if(iLocalProxy!=0)
		m_bLocalProxy=true;
	if(m_bLocalProxy&&iAutoReconnect!=0)
		m_bAutoReconnect=true;

    /* LERNGRUPPE: Also try to use default config file for Mix Category 1 */
    if(configfile == NULL)
    {
        configfile = (char*) malloc(sizeof(char) * (strlen(DEFAULT_CONFIG_FILE)+1));
        strncpy(configfile, DEFAULT_CONFIG_FILE, (strlen(DEFAULT_CONFIG_FILE)+1));

#if defined (_WIN32) &&!defined(__CYGWIN__)
// R_OK is not defined in Windows POSIX implementation
#define R_OK 4
#endif

        int err = access(configfile, R_OK);
        if( err )
        {
            if(configfile != NULL)
            {
                free(configfile);
                configfile = NULL;
            }
        }
    }
    /* END LERNGRUPPE */


	if(configfile!=NULL)
		{
#ifndef ONLY_LOCAL_PROXY
			ret=readXmlConfiguration(m_docMixXml,(UINT8*)configfile);
			if(ret==E_FILE_OPEN)
				CAMsg::printMsg(LOG_CRIT,"Could not open config file: %s\n",configfile);
			else if(ret==E_FILE_READ)
				CAMsg::printMsg(LOG_CRIT,"Could not read config file: %s\n",configfile);
			else if(ret==E_XML_PARSE)
				CAMsg::printMsg(LOG_CRIT,"Could not parse config file: %s\n",configfile);
			else
				{
					m_strConfigFile=new UINT8[strlen(configfile)+1];
					memcpy(m_strConfigFile,configfile,strlen(configfile)+1);
				}
#endif
			free(configfile);
		}
	if(iDaemon==0)
	    m_bDaemon=false;
	else
	    m_bDaemon=true;
  if(target!=NULL)
	    {
				if(target[0]=='/') //Unix Domain Sockaet
				 {
					m_strTargetHost=new char[strlen(target)+1];
					strcpy(m_strTargetHost,target);
				 }
				else
					{
						char tmpHostname[255];
						SINT32 tmpPort;
						char* tmpStr1=strchr(target,':');
						if(tmpStr1!=NULL)
							{
								memcpy(tmpHostname,target,tmpStr1-target);
								tmpHostname[tmpStr1-target]=0;
								tmpPort=(SINT32)atol(tmpStr1+1);
							}
						else
							{//TODO what if not in right form ?
								//try if it is a number --> use it as port
								//and use 'localhost' as traget-host
								tmpPort=(SINT32)atol(target);
								if(tmpPort!=0) //we get it
									{
										strcpy(tmpHostname,"localhost");
									}
								else //we try to use it as host and use the default port
									{
/* LERNGRUPPE moved the define to CACmdLnOption.hpp because we need it elsewhere too */
//#define DEFAULT_TARGET_PORT 6544
										tmpPort=DEFAULT_TARGET_PORT;
										strcpy(tmpHostname,target);
									}
							}
						m_strTargetHost=new char[strlen(tmpHostname)+1];
						strcpy(m_strTargetHost,tmpHostname);
						m_iTargetPort=(UINT16)tmpPort;
					}
				free(target);
	    }
	if(socks!=NULL)
			{
				char* tmpStr;
				if((tmpStr=strchr(socks,':'))!=NULL)
					{
						m_strSOCKSHost=new char[tmpStr-socks+1];
						(*tmpStr)=0;
						strcpy(m_strSOCKSHost,socks);
						m_iSOCKSPort=(UINT16)atol(tmpStr+1);
					}
				free(socks);
	    }
	if(logdir!=NULL)
	    {
					m_strLogDir=new char[strlen(logdir)+1];
					strcpy(m_strLogDir,logdir);
					free(logdir);
	    }
	if(strPidFile!=NULL)
		{
			m_strPidFile=new char[strlen(strPidFile)+1];
			strcpy(m_strPidFile,strPidFile);
			free(strPidFile);
		}
	if(iCompressedLogs==0)
		m_bCompressedLogs=false;
	else
		m_bCompressedLogs=true;
	if(serverPort!=NULL&&m_bLocalProxy)
		{
			m_arListenerInterfaces=new CAListenerInterface*[1];
			m_arListenerInterfaces[0]=NULL;
			m_cnListenerInterfaces=0;
			char* tmpStr;
			if(serverPort[0]=='/') //Unix Domain Socket
				{
					m_arListenerInterfaces[0]=CAListenerInterface::getInstance(RAW_UNIX,(UINT8*)serverPort);
				}
			else //Internet Socket
				{
					char* strServerHost=NULL;
					SINT32 iServerPort;
					if((tmpStr=strchr(serverPort,':'))!=NULL) //host:port
						{
							strServerHost=new char[tmpStr-serverPort+1];
							(*tmpStr)=0;
							strcpy(strServerHost,serverPort);
							iServerPort=(SINT32)atol(tmpStr+1);
						}
					else //port only ?
						{
							iServerPort=(SINT32)atol(serverPort);
						}
						m_arListenerInterfaces[0]=CAListenerInterface::getInstance(RAW_TCP,(UINT8*)strServerHost,(UINT16)iServerPort);
					delete [] strServerHost;
				}
			free(serverPort);
			if(m_arListenerInterfaces[0]!=0)
				m_cnListenerInterfaces=1;
		}

	m_iSOCKSServerPort=(UINT16)SOCKSport;
#ifndef ONLY_LOCAL_PROXY
	if(!m_bLocalProxy)
		{
			ret=processXmlConfiguration(m_docMixXml);
#ifndef DYNAMIC_MIX
			if(ret!=E_SUCCESS)
				return ret;
		}
#else
    /* LERNGRUPPE: Let's try to recover and build a default configuration */
    if(ret!=E_SUCCESS)
    {
        createDefaultConfiguration();
        ret=processXmlConfiguration(m_docMixXml);
        if(ret!=E_SUCCESS) 
            return ret;
    }
                }
#endif

		/* Try to read InfoService configuration from  external file infoservices.xml */
		DOM_Document infoservices;
		if( readXmlConfiguration(infoservices,(UINT8*)"infoservices.xml") == E_SUCCESS )
		{
			CAMsg::printMsg(LOG_DEBUG, "Will now get InfoServices from infoservices.xml (this overrides the InfoServices from the default config!)\n");
			DOM_Element elemIs=infoservices.getDocumentElement();
			parseInfoServices(elemIs);
		}

#ifdef DYNAMIC_MIX
    /*  Ok, at this point we should make sure that we have a minimal configuration.
    If not we try to fill up the missing parameters with default values*/
    if( checkCertificates() != E_SUCCESS )
    {
        CAMsg::printMsg(LOG_CRIT, "I was not able to get a working certificate, please check the configuration! Exiting now\n");
        exit(0);
    }
    UINT32 running = 0;
    CAMsg::printMsg( LOG_INFO, "I will now test if I have enough information about InfoServices...\n");
    if( checkInfoServices(&running) != E_SUCCESS )
    {
        /** @todo what shall the mix-operator do? ask AN.ON team? */
        CAMsg::printMsg(LOG_CRIT, "Problems with InfoServices\nI need at least %i running InfoServices, but i only know about %i at the moment.\n", MIN_INFOSERVICES, running);
        exit(0);
    }
    CAMsg::printMsg( LOG_INFO, "InfoService information ok\n");
    if( checkListenerInterfaces() != E_SUCCESS )
    {
        CAMsg::printMsg(LOG_CRIT, "I don't have any usefull ListenerInterfaces and I canot determine one. please check the configuration! Hints should have been given\n");
        exit(0);
    }
    if( checkMixId() != E_SUCCESS)
    {
        CAMsg::printMsg(LOG_CRIT, "ARGS, I don't have an unique ID, cannot create one! Exiting now\n");
        exit(0);
    }
#endif // DYNAMIC_MIX
#endif //ONLY_LOCAL_PROXY
	return E_SUCCESS;
 }

#ifndef ONLY_LOCAL_PROXY
struct t_CMNDLN_REREAD_PARAMS
	{
		CACmdLnOptions* pCmdLnOptions;
		CAMix* pMix;
	};
#endif //ONLY_LOCAL_PROXY

/** Copies options from \c newOptions. Only those options which are specified
	* in \c newOptions are copied. The others are left untouched!
	*
	* @param newOptions \c CACmdLnOptions object from which the new values are copied
	* @retval E_UNKNOWN if an error occurs
	* @retval E_SUCCESS otherwise
	*/
SINT32 CACmdLnOptions::setNewValues(CACmdLnOptions& newOptions)
	{
		//Copy Targets
		if(newOptions.getTargetInterfaceCount()>0)
			{
				clearTargetInterfaces();
				m_cnTargets=newOptions.getTargetInterfaceCount();
				m_arTargetInterfaces=new TargetInterface[m_cnTargets];
				for(UINT32 i=0;i<m_cnTargets;i++)
					newOptions.getTargetInterface(m_arTargetInterfaces[i],i+1);
			}
#if defined( DELAY_CHANNELS)||defined(DELAY_USERS)			
		//Copy ressources limitation
		m_u32DelayChannelUnlimitTraffic=newOptions.getDelayChannelUnlimitTraffic();
		m_u32DelayChannelBucketGrow=newOptions.getDelayChannelBucketGrow();
		m_u32DelayChannelBucketGrowIntervall=newOptions.getDelayChannelBucketGrowIntervall();
#endif			
#if defined( DELAY_CHANNELS_LATENCY)			
		//Copy ressources limitation
		m_u32DelayChannelLatency=newOptions.getDelayChannelLatency();
#endif
		return E_SUCCESS;
}

#ifndef ONLY_LOCAL_PROXY
/** Modifies the next mix settings (target interface and certificate) according to
* the specified options object. Target interfaces are only copied if they denote a
* next mix. HTTP and SOCKS proxy settings are ignored.
* @param doc a DOM document containing XML data with the new options
*/
#ifndef DYNAMIC_MIX
SINT32 CACmdLnOptions::setNextMix(DOM_Document& doc)
	{
		CAMsg::printMsg(LOG_DEBUG,"setNextMix() - start\n");
    DOM_Element elemRoot = doc.getDocumentElement();

    //getCertificates if given...
    DOM_Element elemSig;
    getDOMChildByName(elemRoot,(UINT8*)"Signature",elemSig,false);
    //Own Certiticate first
    //nextMixCertificate if given
    DOM_Element elemCert;
    getDOMChildByName(elemSig,(UINT8*)"X509Data",elemCert,true);
    if(elemSig!=NULL)
        m_pNextMixCertificate = CACertificate::decode(elemCert.getFirstChild(),CERT_X509CERTIFICATE);

    DOM_Element elemOptionsRoot = m_docMixXml.getDocumentElement();
    DOM_Element elemOptionsCerts;
    getDOMChildByName(elemOptionsRoot, (UINT8*) "Certificates", elemOptionsCerts, false);
    DOM_Element elemOptionsNextMixCert;

    if(getDOMChildByName(elemOptionsRoot, (UINT8*) "NextMixCertificate", elemOptionsNextMixCert, false) != E_SUCCESS)
    {
        elemOptionsNextMixCert = m_docMixXml.createElement("NextMixCertificate");
        elemOptionsCerts.appendChild(elemOptionsNextMixCert);
        elemOptionsNextMixCert.appendChild(m_docMixXml.importNode(elemCert.getFirstChild(),true));
    }
    else
    {
        if(elemOptionsNextMixCert.hasChildNodes())
        {
            elemOptionsNextMixCert.replaceChild(                                         m_docMixXml.importNode(elemCert.getFirstChild(),true),
                    elemOptionsNextMixCert.getFirstChild());
        }
        else
        {
            elemOptionsNextMixCert.appendChild(m_docMixXml.importNode(elemCert.getFirstChild(),true));
        }
    }
		CAMsg::printMsg(LOG_DEBUG,"setNextMix() - certificates done\n");
    DOM_Element elemNextMix;
    getDOMChildByName(elemRoot,(UINT8*)"ListenerInterface",elemNextMix,true);

    DOM_Element elemOptionsNetwork;
    DOM_Element elemOptionsNextMixInterface;

    if(getDOMChildByName(elemOptionsRoot, (UINT8*) "Network", elemOptionsNetwork, false) != E_SUCCESS)
    {
        elemOptionsNetwork = m_docMixXml.createElement("Network");
        elemOptionsRoot.appendChild(elemOptionsNetwork);
    }

    if(getDOMChildByName(elemOptionsNetwork, (UINT8*) "NextMix", elemOptionsNextMixInterface, false) != E_SUCCESS)
    {
        elemOptionsNextMixInterface = m_docMixXml.createElement("NextMix");
        elemOptionsNetwork.appendChild(elemOptionsNextMixInterface);
    }
    else
    {
        while(elemOptionsNextMixInterface.hasChildNodes())
        {
            elemOptionsNextMixInterface.removeChild(elemOptionsNextMixInterface.getFirstChild());
        }
    }

    DOM_Node interfaceData = elemNextMix.getFirstChild();
    while(interfaceData != NULL)
    {
        elemOptionsNextMixInterface.appendChild(m_docMixXml.importNode(interfaceData,true));
        interfaceData = interfaceData.getNextSibling();
    }

		CAMsg::printMsg(LOG_DEBUG,"setNextMix() - end\n");
    return processXmlConfiguration(m_docMixXml);
}

#else //DYNAMIC_MIX

SINT32 CACmdLnOptions::setNextMix(DOM_Document& doc)
{
	resetNextMix();
	/** First set the next mix's certificate */
	DOM_Element elemRoot = doc.getDocumentElement();
	//getCertificates if given...
	DOM_Element elemSig;
	getDOMChildByName(elemRoot,(UINT8*)"Signature",elemSig,false);
	DOM_Element elemCert;
	getDOMChildByName(elemSig,(UINT8*)"X509Data",elemCert,true);
	if(elemCert!=NULL)
		m_pNextMixCertificate = CACertificate::decode(elemCert.getFirstChild(),CERT_X509CERTIFICATE);
	/** Now import the next mix's network stuff */
	DOM_Node elemNextMix;
	DOM_Element elemListeners;
	// Search through the ListenerInterfaces an use a non-hidden one!
	getDOMChildByName(elemRoot,(UINT8*)"ListenerInterfaces",elemListeners,true);
	DOM_NodeList nlListenerInterfaces = elemListeners.getElementsByTagName(CAListenerInterface::XML_ELEMENT_NAME);
	UINT32 len = nlListenerInterfaces.getLength();
	bool foundNonHiddenInterface = false;
	for(UINT32 i=0;i<len;i++)
	{
		elemNextMix=nlListenerInterfaces.item(i);
		SINT32 ret = E_SUCCESS;
		ret = getDOMElementAttribute(elemNextMix,"hidden",foundNonHiddenInterface);
		// Interface was not hidden or "hidden"-Attribute was not specified
		if(foundNonHiddenInterface || ret == E_UNSPECIFIED)
		{
			foundNonHiddenInterface = true;
			break;
		}
	}
	if(!foundNonHiddenInterface)
	{
		CAMsg::printMsg(LOG_ERR, "NEXT MIX HAS NO REAL LISTENERINTERFACES!\n");
		exit(0);
		return E_UNKNOWN;
  }
	/** @todo LERNGRUPPE: Here is much copied code! Refactor! */
	clearTargetInterfaces();
	//get TargetInterfaces
	m_cnTargets=0;
	TargetInterface* targetInterfaceNextMix=NULL;
	if(elemNextMix!=NULL)
{
	NetworkType type;
	CASocketAddr* addr=NULL;
	DOM_Element elemType;
	getDOMChildByName(elemNextMix,(UINT8*)"NetworkProtocol",elemType,false);
	UINT8 tmpBuff[255];
	UINT32 tmpLen=255;
	if(getDOMElementValue(elemType,tmpBuff,&tmpLen)!=E_SUCCESS)
		goto SKIP_NEXT_MIX;
	strtrim(tmpBuff);
	if(strcmp((char*)tmpBuff,"RAW/TCP")==0)
		type=RAW_TCP;
	else if(strcmp((char*)tmpBuff,"RAW/UNIX")==0)
		type=RAW_UNIX;
	else if(strcmp((char*)tmpBuff,"SSL/TCP")==0)
		type=SSL_TCP;
	else if(strcmp((char*)tmpBuff,"SSL/UNIX")==0)
		type=SSL_UNIX;
	else
		goto SKIP_NEXT_MIX;
	if(type==SSL_TCP||type==RAW_TCP)
	{
		DOM_Element elemPort;
		DOM_Element elemHost;
		DOM_Element elemIP;
		UINT8 buffHost[255];
		UINT32 buffHostLen=255;
		UINT16 port;
		getDOMChildByName(elemNextMix,(UINT8*)"Port",elemPort,false);
		if(getDOMElementValue(elemPort,&port)!=E_SUCCESS)
			goto SKIP_NEXT_MIX;
		addr=new CASocketAddrINet;
		bool bAddrIsSet=false;
		getDOMChildByName(elemNextMix,(UINT8*)"Host",elemHost,false);
			/* The rules for <Host> and <IP> are as follows:
		* 1. if <Host> is given and not empty take the <Host> value for the address of the next mix; if not go to 2
		* 2. if <IP> if given and not empty take <IP> value for the address of the next mix; if not goto 3.
			* 3. this entry for the next mix is invalid!*/
		if(elemHost!=NULL)
		{
			if(getDOMElementValue(elemHost,buffHost,&buffHostLen)==E_SUCCESS&&((CASocketAddrINet*)addr)->setAddr(buffHost,port)==E_SUCCESS)
			{
				bAddrIsSet=true;
			}
		}
		if(!bAddrIsSet)//now try <IP>
		{
			getDOMChildByName(elemNextMix,(UINT8*)"IP",elemIP,false);
			if(elemIP == NULL || getDOMElementValue(elemIP,buffHost,&buffHostLen)!=E_SUCCESS)
				goto SKIP_NEXT_MIX;
			if(((CASocketAddrINet*)addr)->setAddr(buffHost,port)!=E_SUCCESS)
				goto SKIP_NEXT_MIX;
		}
		CAMsg::printMsg(LOG_INFO, "Setting target interface: %s:%d\n", buffHost, port);
	}
	else
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL

	{
		DOM_Element elemFile;
		getDOMChildByName(elemNextMix,(UINT8*)"File",elemFile,false);
		tmpLen=255;
		if(getDOMElementValue(elemFile,tmpBuff,&tmpLen)!=E_SUCCESS)
			goto SKIP_NEXT_MIX;
		tmpBuff[tmpLen]=0;
		strtrim(tmpBuff);
		addr=new CASocketAddrUnix;
		if(((CASocketAddrUnix*)addr)->setPath((char*)tmpBuff)!=E_SUCCESS)
			goto SKIP_NEXT_MIX;
	}
#else
			goto SKIP_NEXT_MIX;
#endif
		targetInterfaceNextMix=new TargetInterface;
		targetInterfaceNextMix->target_type=TARGET_MIX;
		targetInterfaceNextMix->net_type=type;
		targetInterfaceNextMix->addr=addr->clone();
		m_cnTargets=1;
		if(targetInterfaceNextMix!=NULL)
		{
			if(m_arTargetInterfaces==NULL)
			{
				m_cnTargets=0;
				m_arTargetInterfaces=new TargetInterface[1];
			}
			m_arTargetInterfaces[m_cnTargets].net_type=targetInterfaceNextMix->net_type;
			m_arTargetInterfaces[m_cnTargets].target_type=targetInterfaceNextMix->target_type;
			m_arTargetInterfaces[m_cnTargets++].addr=targetInterfaceNextMix->addr;
			delete targetInterfaceNextMix;
		}

	SKIP_NEXT_MIX:
			delete addr;
}

	CAMsg::printMsg(LOG_DEBUG,"setNextMix() - end\n");
	return E_SUCCESS;
}
#endif //DYNAMIC_MIX

#endif //ONLY_LOCAL_PROXY

#ifndef ONLY_LOCAL_PROXY
/** Modifies the next mix settings (target interface and certificate) according to
* the specified options object. Target interfaces are only copied if they denote a
* next mix. HTTP and SOCKS proxy settings are ignored.
* @param doc  a DOM document containing XML data with the new options
*/
#ifndef DYNAMIC_MIX
SINT32 CACmdLnOptions::setPrevMix(DOM_Document& doc)
{
		CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - start\n");
		DOM_Element elemRoot = doc.getDocumentElement();
    
    //getCertificates if given...
    DOM_Element elemSig;
    getDOMChildByName(elemRoot,(UINT8*)"Signature",elemSig,false);
    //Own Certiticate first
    //nextMixCertificate if given
    DOM_Element elemCert;
    getDOMChildByName(elemSig,(UINT8*)"X509Data",elemCert,true);
    if(elemCert!=NULL)
			{
				CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - elem cert found in data from infoservice\n");
        DOM_Element elemOptionsRoot = m_docMixXml.getDocumentElement();
				CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - got  current options root element\n");
        DOM_Element elemOptionsCerts;
        getDOMChildByName(elemOptionsRoot, (UINT8*) "Certificates", elemOptionsCerts, false);
        DOM_Element elemOptionsPrevMixCert;

        if(getDOMChildByName(elemOptionsRoot, (UINT8*) "PrevMixCertificate", elemOptionsPrevMixCert, false) != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - no prev cert set at the moment\n");
            elemOptionsPrevMixCert = m_docMixXml.createElement("PrevMixCertificate");
            elemOptionsCerts.appendChild(elemOptionsPrevMixCert);
  					CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - try to import the one we got from infoservice\n");
						getDOMChildByName(elemCert,(UINT8*)"X509Certificate",elemCert,false);
						
						CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - Cert to be imported:\n");
						UINT8 buff[8192];
						UINT32 len=8192;
						DOM_Output::dumpToMem(elemCert,buff,&len);
						CAMsg::printMsg(LOG_DEBUG,(char*)buff);
						
						elemOptionsPrevMixCert.appendChild(m_docMixXml.importNode(elemCert,true));
						CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - MixConf now:\n");
						len=8192;
						DOM_Output::dumpToMem(m_docMixXml,buff,&len);
						buff[len]=0;
						CAMsg::printMsg(LOG_DEBUG,(char*)buff);
					}
        else
        {
            if(elemOptionsPrevMixCert.hasChildNodes())
            {
                elemOptionsPrevMixCert.replaceChild(                                         m_docMixXml.importNode(elemCert.getFirstChild(),true),
                        elemOptionsPrevMixCert.getFirstChild());
            }
            else
            {
                elemOptionsPrevMixCert.appendChild(m_docMixXml.importNode(elemCert.getFirstChild(),true));
            }
				}
			CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - end\n");
			return processXmlConfiguration(m_docMixXml);
    }
		CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - end with error\n");
    return E_UNKNOWN;
}

#else //DYNAMIC_MIX

SINT32 CACmdLnOptions::setPrevMix(DOM_Document& doc)
{
	resetPrevMix();
	CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - start\n");
	DOM_Element elemRoot = doc.getDocumentElement();
	DOM_Element elemSig;
	getDOMChildByName(elemRoot,(UINT8*)"Signature",elemSig,false);
	DOM_Element elemCert;
	getDOMChildByName(elemSig,(UINT8*)"X509Data",elemCert,true);

	if(elemCert!=NULL)
	{
		m_pPrevMixCertificate=CACertificate::decode(elemCert.getFirstChild(),CERT_X509CERTIFICATE);
		return E_SUCCESS; //processXmlConfiguration(m_docMixXml);
	}
	CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - end with error\n");
	return E_UNKNOWN;
}
#endif //DYNAMIC_MIX
#endif //ONLY_LOCAL_PROXY

#ifndef ONLY_LOCAL_PROXY
#ifdef DYNAMIC_MIX
SINT32 CACmdLnOptions::resetNextMix()
{
	if(m_pNextMixCertificate != NULL)
	{
		delete m_pNextMixCertificate;
		m_pNextMixCertificate = NULL;
	}
	DOM_Element elemOptionsRoot = m_docMixXml.getDocumentElement();
	DOM_Element elemOptionsCerts;
	getDOMChildByName(elemOptionsRoot, (UINT8*) "Certificates", elemOptionsCerts, false);
	DOM_Element elemTmp;
	// Remove existing certificates
	if(getDOMChildByName(elemOptionsCerts, (UINT8*) "PrevMixCertificate", elemTmp, false) == E_SUCCESS)
	{

		elemOptionsCerts.removeChild(elemTmp);
	}
	if(getDOMChildByName(elemOptionsCerts, (UINT8*) "PrevOperatorCertificate", elemTmp, false) == E_SUCCESS)
	{
		
		elemOptionsCerts.removeChild(elemTmp);
	}
	clearTargetInterfaces();
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::resetPrevMix()
{
	if(m_pPrevMixCertificate != NULL)
	{
		delete m_pPrevMixCertificate;
		m_pPrevMixCertificate = NULL;
	}
	DOM_Element elemOptionsRoot = m_docMixXml.getDocumentElement();
	DOM_Element elemOptionsCerts;
	getDOMChildByName(elemOptionsRoot, (UINT8*) "Certificates", elemOptionsCerts, false);
	DOM_Element elemTmp;
	// Remove existing certificates
	if(getDOMChildByName(elemOptionsCerts, (UINT8*) "NextMixCertificate", elemTmp, false) == E_SUCCESS)
	{
		
		elemOptionsCerts.removeChild(elemTmp);
	}
	if(getDOMChildByName(elemOptionsCerts, (UINT8*) "NextOperatorCertificate", elemTmp, false) == E_SUCCESS)
	{
		
		elemOptionsCerts.removeChild(elemTmp);
	}
	return E_SUCCESS;

}
#endif //DYNAMIC_MIX
#endif //ONLY_LOCAL_PROXY

#ifndef ONLY_LOCAL_PROXY
/** Rereads the configuration file (if one was given on startup) and reconfigures
	* the mix according to the new values. This is done asyncronous. A new thread is
	* started, which does the actual work.
	* Note: We have to avoid an blocking on any mutex, as this function typically is called from a signal handler - and who knows which mutexes are blocked if this happend...
	* @retval E_SUCCESS if successful
	* @retval E_UNKNOWN if an error occurs
	*/
SINT32 CACmdLnOptions::reread(CAMix* pMix)
	{
		if(m_bIsRunReConfigure)
			return E_UNKNOWN;
		m_bIsRunReConfigure=true;
		m_threadReConfigure.setMainLoop(threadReConfigure);
		t_CMNDLN_REREAD_PARAMS* param=new t_CMNDLN_REREAD_PARAMS;
		param->pCmdLnOptions=this;
		param->pMix=pMix;
		m_threadReConfigure.start(param,true,false);
		return E_SUCCESS;
	}

/** Thread that does the actual reconfigure work. Only one is running at the same time.
	* @param param pointer to a \c t_CMNDLN_REREAD_PARAMS stuct containing a
	* CACmdLnOptions object pointer and a CMix object pointer.
	*/
THREAD_RETURN threadReConfigure(void *param)
	{
		CACmdLnOptions* pOptions=((t_CMNDLN_REREAD_PARAMS*)param)->pCmdLnOptions;
		CAMix* pMix=((t_CMNDLN_REREAD_PARAMS*)param)->pMix;
		//pOptions->m_pcsReConfigure->lock();
		CAMsg::printMsg(LOG_DEBUG,"ReConfiguration of the Mix is under way....\n");
		CACmdLnOptions otmpOptions;
		DOM_Document docConfig;
		if(otmpOptions.readXmlConfiguration(docConfig,pOptions->m_strConfigFile)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_DEBUG,"Could not re-read the config file!\n");
				goto REREAD_FINISH;
			}
		CAMsg::printMsg(LOG_DEBUG,"Re-readed config file -- start processing config file!\n");
		if(otmpOptions.processXmlConfiguration(docConfig)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_DEBUG,"Re-readed config file -- could not process configuration!\n");
				goto REREAD_FINISH;
			}
		pOptions->setNewValues(otmpOptions);
		if(pMix!=NULL)
			pMix->reconfigure();

REREAD_FINISH:
		CAMsg::printMsg(LOG_DEBUG,"ReConfiguration of the Mix finished!\n");
		//pOptions->m_pcsReConfigure->unlock();
		pOptions->m_bIsRunReConfigure=false;
		THREAD_RETURN_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY

bool CACmdLnOptions::getDaemon()
	{
		return m_bDaemon;
  }

SINT32 CACmdLnOptions::getMixId(UINT8* id,UINT32 len)
	{
		if(len<24||m_strMixID==NULL) //we need 24 chars (including final \0)
			return E_UNKNOWN;
		strcpy((char*)id,m_strMixID);
		return E_SUCCESS;
	}


UINT16 CACmdLnOptions::getSOCKSServerPort()
  {
		return m_iSOCKSServerPort;
  }

UINT16 CACmdLnOptions::getMixPort()
	{
		return m_iTargetPort;
  }


SINT32 CACmdLnOptions::getMixHost(UINT8* host,UINT32 len)
  {
		if(m_strTargetHost==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(m_strTargetHost))
				{
					return E_UNKNOWN;
				}
		strcpy((char*)host,m_strTargetHost);
		return E_SUCCESS;
  }

#ifndef ONLY_LOCAL_PROXY
UINT16 CACmdLnOptions::getSOCKSPort()
  {
		return m_iSOCKSPort;
  }

SINT32 CACmdLnOptions::getSOCKSHost(UINT8* host,UINT32 len)
  {
		if(m_strSOCKSHost==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(m_strSOCKSHost))
				{
					return E_UNKNOWN;
				}
		strcpy((char*)host,m_strSOCKSHost);
		return (SINT32)strlen(m_strSOCKSHost);
  }
#endif //ONLY_LOCAL_PROXY

#ifdef PAYMENT
/**Returns an CAXMLBI object, which describes the BI this AI uses. This is not a copy of the 
  * CAXMLBI object. The caller should not delete it!
	* @retval NULL if BI was not set in the configuration file
	* @return information stored inthe configuration file about the BI
	*/
CAXMLBI* CACmdLnOptions::getBI()
	{
		return m_pBI;
	}

SINT32 CACmdLnOptions::getDatabaseHost(UINT8 * host, UINT32 len)
	{
		if(m_strDatabaseHost==NULL)
			return E_UNKNOWN;
		if(len<=(UINT32)strlen((char *)m_strDatabaseHost))
			{
				return E_UNKNOWN;
			}
		strcpy((char*)host,(char *)m_strDatabaseHost);
		return E_SUCCESS;	
}

UINT16 CACmdLnOptions::getDatabasePort()
	{
		return m_iDatabasePort;
	}

SINT32 CACmdLnOptions::getDatabaseName(UINT8 * name, UINT32 len)
	{
		if(m_strDatabaseName==NULL)
			return E_UNKNOWN;
		if(len<=(UINT32)strlen((char *)m_strDatabaseName))
			{
				return E_UNKNOWN;
			}
		strcpy((char*)name,(char *)m_strDatabaseName);
		return E_SUCCESS;	
	}

SINT32 CACmdLnOptions::getDatabaseUsername(UINT8 * user, UINT32 len)
	{
		if(m_strDatabaseUser==NULL)
			return E_UNKNOWN;
		if(len<=(UINT32)strlen((char *)m_strDatabaseUser))
			{
				return E_UNKNOWN;
			}
		strcpy((char*)user,(char *)m_strDatabaseUser);
		return E_SUCCESS;	
	}

SINT32 CACmdLnOptions::getDatabasePassword(UINT8 * pass, UINT32 len)
	{
		if(m_strDatabasePassword==NULL)
			return E_UNKNOWN;
		if(len<=(UINT32)strlen((char *)m_strDatabasePassword))
			{
				return E_UNKNOWN;
			}
		strcpy((char*)pass,(char *)m_strDatabasePassword);
		return E_SUCCESS;	
	}

SINT32 CACmdLnOptions::getAiID(UINT8 * id, UINT32 len)
	{
		if(m_strAiID==NULL)
			return E_UNKNOWN;
		if(len<=(UINT32)strlen((char *)m_strAiID))
			{
				return E_UNKNOWN;
			}
		strcpy((char*)id,(char *)m_strAiID);
		return E_SUCCESS;
	}

SINT32 CACmdLnOptions::getPaymentHardLimit(UINT32 *pHardLimit)
	{
		*pHardLimit = m_iPaymentHardLimit;
		return E_SUCCESS;
	}

SINT32 CACmdLnOptions::getPrepaidInterval(UINT32 *pPrepaidInterval)
	{
		*pPrepaidInterval = m_iPrepaidInterval;
		return E_SUCCESS;
	}

SINT32 CACmdLnOptions::getPaymentSoftLimit(UINT32 *pSoftLimit)
	{
		*pSoftLimit = m_iPaymentSoftLimit;
		return E_SUCCESS;
	}

SINT32 CACmdLnOptions::getPaymentSettleInterval(UINT32 *pInterval)
	{
		*pInterval = m_iPaymentSettleInterval;
		return E_SUCCESS;
	}

#endif /* ifdef PAYMENT */


#ifndef ONLY_LOCAL_PROXY
CAListenerInterface** CACmdLnOptions::getInfoServices(UINT32& r_size)
 {
 		r_size = m_addrInfoServicesSize;
 		return m_addrInfoServices;
  }
SINT32 CACmdLnOptions::getCascadeName(UINT8* name,UINT32 len)
  {
		if(m_strCascadeName==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(m_strCascadeName))
				{
					return E_UNKNOWN;
				}
		strcpy((char*)name,m_strCascadeName);
		return E_SUCCESS;
  }


SINT32 CACmdLnOptions::getEncryptedLogDir(UINT8* name,UINT32 len)
  {
		if(m_strEncryptedLogDir==NULL||name==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(m_strEncryptedLogDir))
			return E_UNKNOWN;
		strcpy((char*)name,m_strEncryptedLogDir);
		return E_SUCCESS;
  }
#endif //ONLY_LOCAL_PROXY

SINT32 CACmdLnOptions::getLogDir(UINT8* name,UINT32 len)
  {
		if(m_strLogDir==NULL||name==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(m_strLogDir))
				{
					return E_SPACE;
				}
		strcpy((char*)name,m_strLogDir);
		return E_SUCCESS;
  }

SINT32 CACmdLnOptions::setLogDir(const UINT8* name,UINT32 len)
  {
		if(m_strLogDir!=NULL)
			{
				delete[] m_strLogDir;
			}
		m_strLogDir=new char[len+1];
		memcpy(m_strLogDir,name,len);
		m_strLogDir[len]=0;
		return E_SUCCESS;	
	}

SINT32 CACmdLnOptions::getPidFile(UINT8* pidfile,UINT32 len)
  {
		if(m_strPidFile==NULL||pidfile==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(m_strPidFile))
				{
					return E_SPACE;
				}
		strcpy((char*)pidfile,m_strPidFile);
		return E_SUCCESS;
  }


SINT32 CACmdLnOptions::getUser(UINT8* user,UINT32 len)
  {
		if(m_strUser==NULL||user==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(m_strUser))
				{
					return E_UNKNOWN;
				}
		strcpy((char*)user,m_strUser);
		return E_SUCCESS;
  }

bool CACmdLnOptions::isFirstMix()
  {
		return m_bFirstMix;
  }

bool CACmdLnOptions::isMiddleMix()
    {
			return m_bMiddleMix;
    }

bool CACmdLnOptions::isLastMix()
    {
			return m_bLastMix;
    }

bool CACmdLnOptions::isLocalProxy()
    {
			return m_bLocalProxy;
    }

#ifndef ONLY_LOCAL_PROXY
/** Returns the XML tree describing the Mix . This is NOT a copy!
	* @param docMixInfo destination for the XML tree
	*	@retval E_SUCCESS if it was successful
	* @retval E_UNKNOWN in case of an error
*/
SINT32 CACmdLnOptions::getMixXml(DOM_Document& docMixInfo)
	{
		docMixInfo=m_docMixInfo;
	//insert (or update) the Timestamp
	DOM_Element elemTimeStamp;
	DOM_Element elemRoot=docMixInfo.getDocumentElement();
	if(getDOMChildByName(elemRoot,(UINT8*)"LastUpdate",elemTimeStamp,false)!=E_SUCCESS)
	{
		elemTimeStamp=docMixInfo.createElement("LastUpdate");
		elemRoot.appendChild(elemTimeStamp);
	}
	UINT64 currentMillis;
	getcurrentTimeMillis(currentMillis);
	UINT8 tmpStrCurrentMillis[50];
	print64(tmpStrCurrentMillis,currentMillis);
	setDOMElementValue(elemTimeStamp,tmpStrCurrentMillis);		
	
		return E_SUCCESS;
}

/** Tries to read the XML configuration file \c configFile and parses (but not process) it.
	* Returns the parsed document as \c DOM_Document.
	* @param docConfig on return contains the parsed XMl document
	* @param configFile file name of the XML config file
	* @retval E_SUCCESS if successful
	* @retval E_FILE_OPEN if error in opening the file
	* @retval E_FILE_READ if not the whole file could be read
	* @retval E_XML_PARSE if the file could not be parsed
	*/
SINT32 CACmdLnOptions::readXmlConfiguration(DOM_Document& docConfig,const UINT8* const configFile)
	{
		int handle;
		handle=open((char*)configFile,O_BINARY|O_RDONLY);
		if(handle==-1)
			return E_FILE_OPEN;
		SINT32 len=filelength(handle);
		UINT8* tmpChar=new UINT8[len];
		int ret=read(handle,tmpChar,len);
		close(handle);
		if(ret!=len)
			return E_FILE_READ;
    SINT32 retVal = readXmlConfiguration(docConfig, tmpChar, len);
    delete[] tmpChar;
    return retVal;
}

/** Tries to read the XML configuration from byte array \c buf. The parsed XML document
	* is parsed only, not processed.
	* Returns the parsed document as a \c DOM_Document.
	* @param docConfig on return contains the parsed XMl document
	* @param buf a byte array containing the XML data
	* @param len the length of the byte array
	* @retval E_SUCCESS if successful
	* @retval E_XML_PARSE if the data could not be parsed
	*/
SINT32 CACmdLnOptions::readXmlConfiguration(DOM_Document& docConfig,const UINT8* const buf, UINT32 len)
{
		DOMParser parser;
    MemBufInputSource in(buf,len,"tmpConfigBuff");
		parser.parse(in);
		if(parser.getErrorCount()>0)
			return E_XML_PARSE;
		docConfig=parser.getDocument();
		return E_SUCCESS;
}

/** Processes a XML configuration document. This sets the values of the
	* options to the values found in the XML document.
	* Note that only the values are changed, which are given in the XML document!
	* @param docConfig the configuration as XML document
	* @retval E_UNKNOWN if an error occurs
	* @retval E_SUCCESS otherwise
	*/
SINT32 CACmdLnOptions::processXmlConfiguration(DOM_Document& docConfig)
	{
		if(docConfig==NULL)
			return E_UNKNOWN;
		DOM_Element elemRoot=docConfig.getDocumentElement();
		DOM_Element elemGeneral;
		if (getDOMChildByName(elemRoot,(UINT8*)"General",elemGeneral,false) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,"No \"General\" node found in configuration file!\n");
			return E_UNKNOWN;
		}

		UINT8 tmpBuff[255];
		UINT32 tmpLen=255;
		
		
		//getMixType
		DOM_Element elem;
		if (getDOMChildByName(elemGeneral,(UINT8*)"MixType",elem,false) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,"No \"MixType\" node found in configuration file!\n");
			return E_UNKNOWN;
		}
		tmpLen=255;
		if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS)
		{
			if(memcmp(tmpBuff,"FirstMix",8)==0)
				m_bFirstMix=true;
			else if (memcmp(tmpBuff,"MiddleMix",9)==0)
				m_bMiddleMix=true;
			else if (memcmp(tmpBuff,"LastMix",7)==0)
				m_bLastMix=true;
		}
		else
		{
			CAMsg::printMsg(LOG_CRIT,"Node \"MixType\" is empty!\n");
			return E_UNKNOWN;
		}

		// LERNGRUPPE
		// get Dynamic flag
		m_bDynamic = false;
		getDOMChildByName(elemGeneral,(UINT8*)"Dynamic",elem,false);
		if(elem != NULL)
			{
    			if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS)
    			{
        			m_bDynamic = (strcmp("True",(char*)tmpBuff) == 0);
    			}

			}
		if(m_bDynamic)
			CAMsg::printMsg( LOG_DEBUG, "I am a dynamic mix\n");

		//getCascadeName
		getDOMChildByName(elemGeneral,(UINT8*)"CascadeName",elem,false);
		tmpLen=255;
#ifdef DYNAMIC_MIX
		bool bNeedCascadeNameFromMixID=false;
#endif
		if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS)
			{
				m_strCascadeName=new char[tmpLen+1];
				memcpy(m_strCascadeName,tmpBuff,tmpLen);
				m_strCascadeName[tmpLen]=0;
			}
#ifdef DYNAMIC_MIX
			/* LERNGRUPPE: Dynamic Mixes must have a cascade name, as MiddleMixes may be reconfigured to be FirstMixes */
		else
			{
				bNeedCascadeNameFromMixID=true;
				m_strCascadeName = new char[strlen(m_strMixID) + 1];
				strncpy(m_strCascadeName, m_strMixID, strlen(m_strMixID)+1);
			}
#endif
		//get Username to run as...
		getDOMChildByName(elemGeneral,(UINT8*)"UserID",elem,false);
		tmpLen=255;
		if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS)
		{
			m_strUser=new char[tmpLen+1];
			memcpy(m_strUser,tmpBuff,tmpLen);
			m_strUser[tmpLen]=0;
		}
		//get Number of File Descriptors to use
		getDOMChildByName(elemGeneral,(UINT8*)"NrOfFileDescriptors",elem,false);
		UINT32 tmp;
		if(getDOMElementValue(elem,&tmp)==E_SUCCESS)
			m_nrOfOpenFiles=tmp;
		//get Run as Daemon
		getDOMChildByName(elemGeneral,(UINT8*)"Daemon",elem,false);
		tmpLen=255;
		if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS&&memcmp(tmpBuff,"True",4)==0)
			m_bDaemon=true;
			
		// get max users
		DOM_Element elemMaxUsers;
		getDOMChildByName(elemGeneral,(UINT8*)"MaxUsers",elemMaxUsers,false);
		if(elemMaxUsers!=NULL)
		{
			if(getDOMElementValue(elemMaxUsers, &tmp)==E_SUCCESS)
			{
				m_maxNrOfUsers = tmp;
			}
		}
			
		//get Logging
		DOM_Element elemLogging;
		getDOMChildByName(elemGeneral,(UINT8*)"Logging",elemLogging,false);
		if(elemLogging!=NULL)
			{
				tmpLen=255;
				getDOMChildByName(elemLogging,(UINT8*)"File",elem,false);
				if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS)
					{
						strtrim(tmpBuff);
						m_strLogDir=new char[strlen((char*)tmpBuff)+1];
						strcpy(m_strLogDir,(char*)tmpBuff);
					}
				getDOMChildByName(elemLogging,(UINT8*)"Syslog",elem,false);
				tmpLen=255;
				if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS&&memcmp(tmpBuff,"True",4)==0)
				{
					m_bSyslog=true;
				}
					
				DOM_Element elemEncLog;
				//get Encrypted Log Info
				if(getDOMChildByName(elemLogging,(UINT8*)"EncryptedLog",elemEncLog,false)==E_SUCCESS)
					{
						m_bIsEncryptedLogEnabled=true;
						getDOMChildByName(elemEncLog,(UINT8*)"File",elem,false);
						tmpLen=255;
						if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS)
							{
								strtrim(tmpBuff);
								m_strEncryptedLogDir=new char[strlen((char*)tmpBuff)+1];
								strcpy(m_strEncryptedLogDir,(char*)tmpBuff);
							}
						DOM_Element elemKeyInfo;
						DOM_Element elemX509Data;
						if(getDOMChildByName(elemEncLog,(UINT8*)"KeyInfo",elemKeyInfo,false)==E_SUCCESS&&
							getDOMChildByName(elemKeyInfo,(UINT8*)"X509Data",elemX509Data,false)==E_SUCCESS)
							{
								m_pLogEncryptionCertificate=CACertificate::decode(elemX509Data.getFirstChild(),CERT_X509CERTIFICATE);
							}
					}
				else
					m_bIsEncryptedLogEnabled=false;
			}
		//getCertificates if given...
		DOM_Element elemCertificates;
		getDOMChildByName(elemRoot,(UINT8*)"Certificates",elemCertificates,false);
		//Own Certiticate first
		DOM_Element elemOwnCert;
		getDOMChildByName(elemCertificates,(UINT8*)"OwnCertificate",elemOwnCert,false);
		if (elemOwnCert == NULL)
			{
				return E_UNKNOWN;
			}
		
		m_pSignKey=new CASignature();
		UINT8 passwd[500];
		passwd[0]=0;
		if(m_pSignKey->setSignKey(elemOwnCert.getFirstChild(),SIGKEY_PKCS12)!=E_SUCCESS)
		{//Maybe not an empty passwd
			printf("I need a passwd for the SignKey: ");
			fflush(stdout);
			readPasswd(passwd,500);
			if(m_pSignKey->setSignKey(elemOwnCert.getFirstChild(),SIGKEY_PKCS12,(char*)passwd)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not read own signature key!\n");
				delete m_pSignKey;
				m_pSignKey=NULL;
			}
		}		
		m_pOwnCertificate=CACertificate::decode(elemOwnCert.getFirstChild(),CERT_PKCS12,(char*)passwd);
		if (m_pOwnCertificate == NULL)
		{
			CAMsg::printMsg(LOG_CRIT,"Could not decode mix certificate!\n");
			return E_UNKNOWN;
		}
		
		//get MixID
		tmpLen=255;
		if (m_pOwnCertificate->getSubjectKeyIdentifier(tmpBuff, &tmpLen) != E_SUCCESS)
		{
			DOM_Element elemMixID;
			getDOMChildByName(elemGeneral,(UINT8*)"MixID",elemMixID,false);
			if(elemMixID==NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"Node \"MixID\" not found!\n");
				return E_UNKNOWN;
			}
			if(getDOMElementValue(elemMixID,tmpBuff,&tmpLen)!=E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Node \"MixID\" is empty!\n");
				return E_UNKNOWN;
			}
		}
		strtrim(tmpBuff);
		m_strMixID=new char[strlen((char*)tmpBuff)+1];
		strcpy(m_strMixID,(char*) tmpBuff);		

#ifdef DYNAMIC_MIX
			/* LERNGRUPPE: Dynamic Mixes must have a cascade name, as MiddleMixes may be reconfigured to be FirstMixes */
		if(bNeedCascadeNameFromMixID)
			{
				m_strCascadeName = new char[strlen(m_strMixID) + 1];
				strncpy(m_strCascadeName, m_strMixID, strlen(m_strMixID)+1);
			}
#endif

		//then Operator Certificate
		DOM_Element elemOpCert;
		if (getDOMChildByName(elemCertificates,(UINT8*)"OperatorOwnCertificate",elemOpCert,false) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,"Node \"OperatorOwnCertificate\" not found!\n");
			return E_UNKNOWN;
		}
		//CAMsg::printMsg(LOG_DEBUG,"Node: %s\n",elemOpCert.getNodeName().transcode());
		
		m_OpCertsLength = 0;
		if (elemOpCert != NULL)
		{
			DOM_NodeList opCertList = elemOpCert.getElementsByTagName("X509Certificate");		
			m_OpCerts = new CACertificate*[opCertList.getLength()];	
			for (UINT32 i = 0; i < opCertList.getLength(); i++)
			{
				m_OpCerts[m_OpCertsLength] = CACertificate::decode(opCertList.item(i),CERT_X509CERTIFICATE);
				if (m_OpCerts[m_OpCertsLength] != NULL)
				{
					m_OpCertsLength++;
				}
			}
			if (m_OpCertsLength == 0)
			{
				CAMsg::printMsg(LOG_CRIT,"Node \"X509Certificate\" of operator certificate not found!\n");
				return E_UNKNOWN;
			}
		}		
		
		
		//nextMixCertificate if given
		DOM_Element elemNextCert;
		getDOMChildByName(elemCertificates,(UINT8*)"NextMixCertificate",elemNextCert,false);
		if(elemNextCert!=NULL)
			m_pNextMixCertificate=CACertificate::decode(elemNextCert.getFirstChild(),CERT_X509CERTIFICATE);
		//prevMixCertificate if given
		DOM_Element elemPrevCert;
		getDOMChildByName(elemCertificates,(UINT8*)"PrevMixCertificate",elemPrevCert,false);
		if(elemPrevCert!=NULL)
			m_pPrevMixCertificate=CACertificate::decode(elemPrevCert.getFirstChild(),CERT_X509CERTIFICATE);

#ifdef PAYMENT
// Added by Bastian Voigt: 
// Read PaymentInstance data (JPI Hostname, Port, Publickey) from configfile

		DOM_Element elemAccounting;
		if (getDOMChildByName(elemRoot,(UINT8*)"Accounting",elemAccounting,false) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,"Node \"Accounting\" not found!\n");
			return E_UNKNOWN;
		}
		if(elemAccounting != NULL) 
		{

			//get price certificate
			DOM_Element pcElem;
			//function in CAUtil, last param is "deep", needs to be set to include child elems				
			getDOMChildByName(elemAccounting, (UINT8*)"PriceCertificate",pcElem, false);
			if (pcElem == NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"Node \"PriceCertificate\" not found!\n");
				return E_UNKNOWN;
			} else 
			{
				m_pPriceCertificate = CAXMLPriceCert::getInstance(pcElem); 
				if (m_pPriceCertificate == NULL) {
					CAMsg::printMsg(LOG_DEBUG, "PRICECERT PROCESSED, BUT STILL NULL");
					return E_UNKNOWN;
				}
			}				

			//if (m_bFirstMix)
			//{
				CAMsg::printMsg(LOG_DEBUG, "Parsing JPI values.\n");
	
				DOM_Element elemJPI;
				getDOMChildByName(elemAccounting, CAXMLBI::getXMLElementName(), elemJPI, false);
				m_pBI = CAXMLBI::getInstance(elemJPI);
				if (m_pBI == NULL)
				{
					CAMsg::printMsg(LOG_CRIT,"Could not instantiate payment instance interface!\n");
					return E_UNKNOWN;
				}
				if (getDOMChildByName(elemAccounting, (UINT8*)"SoftLimit", elem, false) != E_SUCCESS)
				{
					CAMsg::printMsg(LOG_CRIT,"Node \"SoftLimit\" not found!\n");
					return E_UNKNOWN;
				}
				if(getDOMElementValue(elem, &tmp)==E_SUCCESS)
				{
					m_iPaymentSoftLimit = tmp;
				}
				if (getDOMChildByName(elemAccounting, (UINT8*)"HardLimit", elem, false) != E_SUCCESS)
				{
					CAMsg::printMsg(LOG_CRIT,"Node \"HardLimit\" not found!\n");
					return E_UNKNOWN;
				}
				if(getDOMElementValue(elem, &tmp)==E_SUCCESS)
				{
					m_iPaymentHardLimit = tmp;
				}
				if (getDOMChildByName(elemAccounting, (UINT8*)"PrepaidInterval", elem, false) != E_SUCCESS)
				{
					CAMsg::printMsg(LOG_CRIT,"Node \"PrepaidInterval\" not found!\n");
					
					if (getDOMChildByName(elemAccounting, (UINT8*)"PrepaidIntervalKbytes", elem, false) != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"Node \"PrepaidIntervalKbytes\" not found!\n");
					}
					else
					{
						if(getDOMElementValue(elem, &tmp)==E_SUCCESS)
						{
							m_iPrepaidInterval = tmp * 1000;
						}
					}
				}
				else if(getDOMElementValue(elem, &tmp) == E_SUCCESS)	
				{
					m_iPrepaidInterval = tmp;
				}
				else 
				{
					CAMsg::printMsg(LOG_CRIT,"Node \"PrepaidInterval\" is empty! Setting default...\n");
					m_iPrepaidInterval = 3000000; //3 MB as safe default if not explicitly set in config file	
				}
				if (m_iPrepaidInterval > 3000000)
				{
					CAMsg::printMsg(LOG_CRIT,"Prepaid interval is higher than 3000000! No JAP will pay more in advance!\n");
				}
				else if (m_iPrepaidInterval < 5000)
				{
					CAMsg::printMsg(LOG_CRIT,"Prepaid interval of %u is far too low! Performance will be critical and clients will lose connection!\n", m_iPrepaidInterval);
				}
				if (getDOMChildByName(elemAccounting, (UINT8*)"SettleInterval", elem, false) != E_SUCCESS)
				{
					CAMsg::printMsg(LOG_CRIT,"Node \"SettleInterval\" not found!\n");
					return E_UNKNOWN;
				}
				if(getDOMElementValue(elem, &tmp)==E_SUCCESS)
				{
					m_iPaymentSettleInterval = tmp;
				}
				else
				{
					CAMsg::printMsg(LOG_CRIT,"Node \"SettleInterval\" is empty!\n");
					return E_UNKNOWN;
				}
			//}
			
			CAMsg::printMsg(LOG_DEBUG, "Parsing AI values.\n");
				
			// get AiID (NOT a separate element /Accounting/AiID any more, rather the subjectkeyidentifier given in the price certificate
			m_strAiID = m_pPriceCertificate->getSubjectKeyIdentifier();
				
			if (m_bFirstMix)
			{
				DOM_Element elemDatabase;
				if (getDOMChildByName(elemAccounting, (UINT8*)"Database", elemDatabase, false) != E_SUCCESS)
				{
					CAMsg::printMsg(LOG_CRIT,"Node \"Database\" not found!\n");
					return E_UNKNOWN;
				}								
				else //if(elemDatabase != NULL) 
				{
					// get DB Hostname
					if (getDOMChildByName(elemDatabase, (UINT8*)"Host", elem, false) != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"Node \"Host\" not found!\n");
						return E_UNKNOWN;
					}
					tmpLen = 255;
					if(getDOMElementValue(elem, tmpBuff, &tmpLen)==E_SUCCESS) 
					{
						strtrim(tmpBuff);
						m_strDatabaseHost = new UINT8[strlen((char*)tmpBuff)+1];
						strcpy((char *)m_strDatabaseHost, (char *) tmpBuff);
					}
					else
					{
						CAMsg::printMsg(LOG_CRIT,"Node \"Host\" is empty!\n");
						return E_UNKNOWN;
					}
					// get Database Port
					if (getDOMChildByName(elemDatabase, (UINT8*)"Port", elem, false) != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"Node \"Port\" not found!\n");
						return E_UNKNOWN;
					}
					if(getDOMElementValue(elem, &tmp)==E_SUCCESS) 
					{
						m_iDatabasePort = tmp;
					}
					else
					{
						CAMsg::printMsg(LOG_CRIT,"Node \"Port\" is empty!\n");
						return E_UNKNOWN;
					}
					// get DB Name
					if (getDOMChildByName(elemDatabase, (UINT8*)"DBName", elem, false) != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"Node \"DBName\" not found!\n");
						return E_UNKNOWN;
					}
					tmpLen = 255;
					if(getDOMElementValue(elem, tmpBuff, &tmpLen)==E_SUCCESS) 
					{
						strtrim(tmpBuff);
						m_strDatabaseName = new UINT8[strlen((char*)tmpBuff)+1];
						strcpy((char *)m_strDatabaseName, (char *) tmpBuff);
					}
					else
					{
						CAMsg::printMsg(LOG_CRIT,"Node \"DBName\" is empty!\n");
						return E_UNKNOWN;
					}
					// get DB Username
					if (getDOMChildByName(elemDatabase, (UINT8*)"Username", elem, false) != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_CRIT,"Node \"Username\" not found!\n");
						return E_UNKNOWN;
					}
					tmpLen = 255;
					if(getDOMElementValue(elem, tmpBuff, &tmpLen)==E_SUCCESS) 
					{
						strtrim(tmpBuff);
						m_strDatabaseUser = new UINT8[strlen((char*)tmpBuff)+1];
						strcpy((char *)m_strDatabaseUser, (char *) tmpBuff);
					}
					else
					{
						CAMsg::printMsg(LOG_CRIT,"Node \"Username\" is empty!\n");
						return E_UNKNOWN;
					}
							
					//get DB password from xml 	
					getDOMChildByName(elemDatabase, (UINT8*)"Password", elem, false);
					tmpLen = 255;
					//read password from xml if given
					if(getDOMElementValue(elem, tmpBuff, &tmpLen)==E_SUCCESS) {
						strtrim(tmpBuff);
						m_strDatabasePassword = new UINT8[strlen((char*)tmpBuff)+1];
						strcpy((char *)m_strDatabasePassword, (char *) tmpBuff);
					}
					else
					{      
				        //read password from stdin:
						UINT8 dbpass[500];
						dbpass[0]=0;
						printf("Please enter password for postgresql user %s at %s: ",m_strDatabaseUser, m_strDatabaseHost);
						scanf("%400[^\n]%*1[\n]",(char*)dbpass); 
						int len = strlen((char *)dbpass);
						if(len>0) 
						{
							m_strDatabasePassword = new UINT8[len+1];
							strcpy((char *)m_strDatabasePassword, (char *)dbpass);
						}
						else
						{
							m_strDatabasePassword = new UINT8[1];
							m_strDatabasePassword[0] = '\0';
						}	
					}
							
				} //of elem database
			}
			
			CAMsg::printMsg(LOG_DEBUG, "Accounting values parsed OK.\n");
		} //of elem accounting
		else 
		{
			CAMsg::printMsg(LOG_CRIT, "No accounting instance info found in configfile. Payment will not work!\n");
			return E_UNKNOWN;
		}

#endif /* ifdef PAYMENT */


		//get InfoService data
		DOM_Element elemNetwork;
		if (getDOMChildByName(elemRoot,(UINT8*)"Network",elemNetwork,false) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,"Node \"Network\" not found!\n");				
			return E_UNKNOWN;
		}
		DOM_Element elemInfoServiceContainer;
		getDOMChildByName(elemNetwork,(UINT8*)"InfoServices",elemInfoServiceContainer,false);
		if (elemInfoServiceContainer ==	NULL)
		{
			// old configuration version <= 0.61
			DOM_Element elemInfoService;
			DOM_Element elemAllowReconfig;
			if (getDOMChildByName(elemNetwork,(UINT8*)"InfoService",elemInfoService,false) != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Node \"InfoService\" not found!\n");				
			}
			/* LERNGRUPPE: There might not be any InfoService configuration in the file, but in infoservices.xml, so check this */
			if(elemInfoService != NULL)
			{ 
				getDOMChildByName(elemInfoService,(UINT8*)"AllowAutoConfiguration",elemAllowReconfig,false);
				CAListenerInterface* isListenerInterface = CAListenerInterface::getInstance(elemInfoService);
				if (!isListenerInterface)
				{
					CAMsg::printMsg(LOG_CRIT,"Node \"InfoService\" does not contain valid data!\n");				
				}
				else
				{
					m_addrInfoServicesSize = 1;
					m_addrInfoServices = new CAListenerInterface*[m_addrInfoServicesSize];
					m_addrInfoServices[0] = isListenerInterface;
					if(getDOMElementValue(elemAllowReconfig,tmpBuff,&tmpLen)==E_SUCCESS)
					{
						m_bAcceptReconfiguration = (strcmp("True",(char*)tmpBuff) == 0);
					}
				}
			}
		}
		else
	    {
				// Refactored
				parseInfoServices(elemInfoServiceContainer);
	    }
		 
		//get ListenerInterfaces
		DOM_Element elemListenerInterfaces;
		getDOMChildByName(elemNetwork,(UINT8*)
		CAListenerInterface::XML_ELEMENT_CONTAINER_NAME,elemListenerInterfaces,false);
		m_arListenerInterfaces = CAListenerInterface::getInstance(
			elemListenerInterfaces, m_cnListenerInterfaces);

#ifndef DYNAMIC_MIX
    /* LERNGRUPPE: ListenerInterfaces may be configured dynamically */
		if (m_cnListenerInterfaces == 0)
		{
			CAMsg::printMsg(LOG_CRIT, "No listener interfaces found!\n");
			return E_UNKNOWN;
		}
#endif


		//get TargetInterfaces
		m_cnTargets=0;
		TargetInterface* targetInterfaceNextMix=NULL;
		//NextMix --> only one!!
		DOM_Element elemNextMix;
		getDOMChildByName(elemNetwork,(UINT8*)"NextMix",elemNextMix,false);
		if(elemNextMix!=NULL)
			{
				NetworkType type;
				CASocketAddr* addr=NULL;
				DOM_Element elemType;
				getDOMChildByName(elemNextMix,(UINT8*)"NetworkProtocol",elemType,false);
				tmpLen=255;
				if(getDOMElementValue(elemType,tmpBuff,&tmpLen)!=E_SUCCESS)
					goto SKIP_NEXT_MIX;
				strtrim(tmpBuff);
				if(strcmp((char*)tmpBuff,"RAW/TCP")==0)
					type=RAW_TCP;
				else if(strcmp((char*)tmpBuff,"RAW/UNIX")==0)
					type=RAW_UNIX;
				else if(strcmp((char*)tmpBuff,"SSL/TCP")==0)
					type=SSL_TCP;
				else if(strcmp((char*)tmpBuff,"SSL/UNIX")==0)
					type=SSL_UNIX;
				else
					goto SKIP_NEXT_MIX;
				if(type==SSL_TCP||type==RAW_TCP)
					{
						DOM_Element elemPort;
						DOM_Element elemHost;
            DOM_Element elemIP;
            UINT8 buffHost[255];
            UINT32 buffHostLen=255;
						UINT16 port;
						getDOMChildByName(elemNextMix,(UINT8*)"Port",elemPort,false);
						if(getDOMElementValue(elemPort,&port)!=E_SUCCESS)
							goto SKIP_NEXT_MIX;

						addr=new CASocketAddrINet;
						bool bAddrIsSet=false;
						getDOMChildByName(elemNextMix,(UINT8*)"Host",elemHost,false);
						/* The rules for <Host> and <IP> are as follows:
							* 1. if <Host> is given and not empty take the <Host> value for the address of the next mix; if not go to 2
							* 2. if <IP> if given and not empty take <IP> value for the address of the next mix; if not goto 3.
							* 3. this entry for the next mix is invalid!*/
						if(elemHost!=NULL)
							{
								if(getDOMElementValue(elemHost,buffHost,&buffHostLen)==E_SUCCESS&&
                  ((CASocketAddrINet*)addr)->setAddr(buffHost,port)==E_SUCCESS)
								{
									bAddrIsSet=true;
								}
							}
						if(!bAddrIsSet)//now try <IP>
							{
                getDOMChildByName(elemNextMix,(UINT8*)"IP",elemIP,false);
                if(elemIP == NULL || getDOMElementValue(elemIP,buffHost,&buffHostLen)!=E_SUCCESS)
                    goto SKIP_NEXT_MIX;
                if(((CASocketAddrINet*)addr)->setAddr(buffHost,port)!=E_SUCCESS)
									goto SKIP_NEXT_MIX;
							}
            CAMsg::printMsg(LOG_INFO, "Setting target interface: %s:%d\n", buffHost, port);
					}
				else
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
					{
						DOM_Element elemFile;
						getDOMChildByName(elemNextMix,(UINT8*)"File",elemFile,false);
						tmpLen=255;
						if(getDOMElementValue(elemFile,tmpBuff,&tmpLen)!=E_SUCCESS)
							goto SKIP_NEXT_MIX;
						tmpBuff[tmpLen]=0;
						strtrim(tmpBuff);
						addr=new CASocketAddrUnix;
						if(((CASocketAddrUnix*)addr)->setPath((char*)tmpBuff)!=E_SUCCESS)
							goto SKIP_NEXT_MIX;
					}
#else
					goto SKIP_NEXT_MIX;
#endif
				targetInterfaceNextMix=new TargetInterface;
				targetInterfaceNextMix->target_type=TARGET_MIX;
				targetInterfaceNextMix->net_type=type;
				targetInterfaceNextMix->addr=addr->clone();
				m_cnTargets=1;
SKIP_NEXT_MIX:
				delete addr;
			}

		//Next Proxies and visible adresses
		clearVisibleAddresses();
		DOM_Element elemProxies;
		getDOMChildByName(elemNetwork,(UINT8*)"Proxies",elemProxies,false);
		if(elemProxies!=NULL)
			{
				DOM_NodeList nlTargetInterfaces;
				nlTargetInterfaces=elemProxies.getElementsByTagName("Proxy");
				m_cnTargets+=nlTargetInterfaces.getLength();
				if(nlTargetInterfaces.getLength()>0)
					{
						m_arTargetInterfaces=new TargetInterface[m_cnTargets];
						UINT32 aktInterface=0;
						NetworkType type=UNKNOWN_NETWORKTYPE;
						UINT32 proxy_type=0;
						CASocketAddr* addr=NULL;
						UINT16 port;
						for(UINT32 i=0;i<nlTargetInterfaces.getLength();i++)
							{
								if(addr!=NULL)
									delete addr;
								addr=NULL;
								DOM_Node elemTargetInterface;
								elemTargetInterface=nlTargetInterfaces.item(i);
								DOM_Element elemType;
								getDOMChildByName(elemTargetInterface,(UINT8*)"NetworkProtocol",elemType,false);
								tmpLen=255;
								if(getDOMElementValue(elemType,tmpBuff,&tmpLen)!=E_SUCCESS)
									continue;
								strtrim(tmpBuff);
								if(strcmp((char*)tmpBuff,"RAW/TCP")==0)
									type=RAW_TCP;
								else if(strcmp((char*)tmpBuff,"RAW/UNIX")==0)
									type=RAW_UNIX;
								else if(strcmp((char*)tmpBuff,"SSL/TCP")==0)
									type=SSL_TCP;
								else if(strcmp((char*)tmpBuff,"SSL/UNIX")==0)
									type=SSL_UNIX;
								else
									continue;
								//ProxyType
								elemType=NULL;
								getDOMChildByName(elemTargetInterface,(UINT8*)"ProxyType",elemType,false);
								tmpLen=255;
								if(getDOMElementValue(elemType,tmpBuff,&tmpLen)!=E_SUCCESS)
									continue;
								strtrim(tmpBuff);
								if(strcmp((char*)tmpBuff,"SOCKS")==0)
								{
									m_bSocksSupport = true;
									proxy_type=TARGET_SOCKS_PROXY;
								}
								else if(strcmp((char*)tmpBuff,"HTTP")==0)
								{
									proxy_type=TARGET_HTTP_PROXY;
								}
								else
								{
									continue;
								}

								if(type==SSL_TCP||type==RAW_TCP)
									{
										DOM_Element elemPort;
										DOM_Element elemHost;
										getDOMChildByName(elemTargetInterface,(UINT8*)"Port",elemPort,false);
										if(getDOMElementValue(elemPort,&port)!=E_SUCCESS)
											continue;

										addr=new CASocketAddrINet;
										getDOMChildByName(elemTargetInterface,(UINT8*)"Host",elemHost,false);
										if(elemHost!=NULL)
											{
												UINT8 buffHost[255];
												UINT32 buffHostLen=255;
												if(getDOMElementValue(elemHost,buffHost,&buffHostLen)!=E_SUCCESS)
													continue;
												if(((CASocketAddrINet*)addr)->setAddr(buffHost,port)!=E_SUCCESS)
													continue;
											}
										else
											continue;
									}
								else
		#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
									{
										DOM_Element elemFile;
										getDOMChildByName(elemTargetInterface,(UINT8*)"File",elemFile,false);
										tmpLen=255;
										if(getDOMElementValue(elemFile,tmpBuff,&tmpLen)!=E_SUCCESS)
											continue;
										tmpBuff[tmpLen]=0;
										strtrim(tmpBuff);
										addr=new CASocketAddrUnix;
										if(((CASocketAddrUnix*)addr)->setPath((char*)tmpBuff)!=E_SUCCESS)
											continue;
									}
		#else
									continue;
		#endif
								addVisibleAddresses(elemTargetInterface);
								m_arTargetInterfaces[aktInterface].net_type=type;
								m_arTargetInterfaces[aktInterface].target_type=proxy_type;
								m_arTargetInterfaces[aktInterface].addr=addr->clone();
								delete addr;
								addr=NULL;
								aktInterface++;
							}
						m_cnTargets=aktInterface;
					}
			}//end if elemProxies!=null
		//add NextMixInterface to the End of the List...
		if(targetInterfaceNextMix!=NULL)
			{
				if(m_arTargetInterfaces==NULL)
					{
						m_cnTargets=0;
						m_arTargetInterfaces=new TargetInterface[1];
					}
				m_arTargetInterfaces[m_cnTargets].net_type=targetInterfaceNextMix->net_type;
				m_arTargetInterfaces[m_cnTargets].target_type=targetInterfaceNextMix->target_type;
				m_arTargetInterfaces[m_cnTargets++].addr=targetInterfaceNextMix->addr;
				delete targetInterfaceNextMix;
			}
		
		//-----------------------------------------------------------------------------	
		//construct a XML-String, which describes the Mix (send via Infoservice.Helo())
		m_docMixInfo=DOM_Document::createDocument();
		DOM_Element elemMix=m_docMixInfo.createElement("Mix");
		elemMix.setAttribute("id",DOMString(m_strMixID));
		m_docMixInfo.appendChild(elemMix);

		//Inserting the Name if given...
		getDOMChildByName(elemGeneral,(UINT8*)"MixName",elem,false);
		tmpLen=255;
		if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS)
			{
				DOM_Element elemName=m_docMixInfo.createElement("Name");
				setDOMElementValue(elemName,tmpBuff);
				elemMix.appendChild(elemName);
			}

    //Inserting the min. cascade length if given...
    getDOMChildByName(elemGeneral,(UINT8*)"MinCascadeLength",elem,false);
    if(elem != NULL)
    {
    	elemMix.appendChild(m_docMixInfo.importNode(elem, true));
    }

    //Inserting the min. cascade length if given...
    getDOMChildByName(elemGeneral,(UINT8*)"MixType",elem,false);
    if(elem != NULL)
    {
    	elemMix.appendChild(m_docMixInfo.importNode(elem, true));
    }    
        
    // LERNGRUPPE: insert dynamic flag
    getDOMChildByName(elemGeneral,(UINT8*)"Dynamic",elem,false);
    if(elem != NULL)
    {
    	elemMix.appendChild(m_docMixInfo.importNode(elem, true));
    }    
    
		//Import the Description if given
		DOM_Element elemMixDescription;
		getDOMChildByName(elemRoot,(UINT8*)"Description",elemMixDescription,false);
		if(elemMixDescription!=NULL)
			{
				DOM_Node tmpChild=elemMixDescription.getFirstChild();
				while(tmpChild!=NULL)
					{
						elemMix.appendChild(m_docMixInfo.importNode(tmpChild,true));
						tmpChild=tmpChild.getNextSibling();
					}
			}
    /* LERNGRUPPE: This only works if ListenerInterfaces were given in the config-file, otherwise we crash here -> not good, better test for NULL */
    if(elemListenerInterfaces != NULL)
    {
    // import listener interfaces element; this is needed for cascade auto configuration
    // -- inserted by ronin <ronin2@web.de> 2004-08-16
    elemMix.appendChild(m_docMixInfo.importNode(elemListenerInterfaces,true));
    }
    /* END LERNGRUPPE */

		//Set Proxy Visible Addresses if Last Mix and given
		if(isLastMix())
		{
			DOM_Element elemProxies=m_docMixInfo.createElement("Proxies");
			if (m_bSocksSupport)
			{
				setDOMElementAttribute(elemProxies, "socks5Support", (UINT8*)"true");
			}
			DOM_Element elemProxy=m_docMixInfo.createElement("Proxy");
			DOM_Element elemVisAddresses=m_docMixInfo.createElement("VisibleAddresses");
			elemMix.appendChild(elemProxies);
			elemProxies.appendChild(elemProxy);
			elemProxy.appendChild(elemVisAddresses);
			for(UINT32 i=1;i<=getVisibleAddressesCount();i++)
			{
				UINT8 tmp[255];
				UINT32 tmplen=255;
				if(getVisibleAddress(tmp,tmplen,i)==E_SUCCESS)
				{
					DOM_Element elemVisAddress=m_docMixInfo.createElement("VisibleAddress");
					DOM_Element elemHost=m_docMixInfo.createElement("Host");
					elemVisAddress.appendChild(elemHost);
					setDOMElementValue(elemHost,tmp);
					elemVisAddresses.appendChild(elemVisAddress);
				}
			}
		}
		
		//Set Software-Version...
		DOM_Element elemSoftware=m_docMixInfo.createElement("Software");
		DOM_Element elemVersion=m_docMixInfo.createElement("Version");
		setDOMElementValue(elemVersion,(UINT8*)MIX_VERSION);
		elemSoftware.appendChild(elemVersion);
		elemMix.appendChild(elemSoftware);

#ifdef PAYMENT
		
		//insert price certificate
		if (getPriceCertificate() == NULL)
		{
			CAMsg::printMsg(LOG_CRIT, "can't insert price certificate because it's Null\n");
			return E_UNKNOWN;
		} else {
			DOM_Element pcElem;		
			getPriceCertificate()->toXmlElement(m_docMixInfo,pcElem);	
			elemMix.appendChild(pcElem);
		}
		//insert prepaid interval
		UINT32 prepaidInterval;
		getPrepaidInterval(&prepaidInterval);
		DOM_Element elemInterval = m_docMixInfo.createElement("PrepaidIntervalKbytes");
		setDOMElementValue(elemInterval,prepaidInterval / 1000); 
		elemMix.appendChild(elemInterval);	
			
			
		
#endif /*payment*/


#ifdef LOG_CRIME
		m_arCrimeRegExpsURL=NULL;
		m_nCrimeRegExpsURL=0;
		m_arCrimeRegExpsPayload=NULL;
		m_nCrimeRegExpsPayload=0;
		CAMsg::printMsg(LOG_INFO,"Loading Crime Detection Data....\n");
		DOM_Element elemCrimeDetection;
		getDOMChildByName(elemRoot,(UINT8*)"CrimeDetection",elemCrimeDetection,false);
		if(elemCrimeDetection!=NULL)
		{
			DOM_NodeList nlRegExp;
			nlRegExp=elemCrimeDetection.getElementsByTagName("RegExpURL");
			m_arCrimeRegExpsURL=new regex_t[nlRegExp.getLength()];
			for(UINT32 i=0;i<nlRegExp.getLength();i++)
			{
				DOM_Node tmpChild=nlRegExp.item(i);
				UINT32 lenRegExp=4096;
				UINT8 buffRegExp[4096];
				if(getDOMElementValue(tmpChild,buffRegExp,&lenRegExp)==E_SUCCESS)
				{
					if(regcomp(&m_arCrimeRegExpsURL[m_nCrimeRegExpsURL],(char*)buffRegExp,REG_EXTENDED|REG_ICASE|REG_NOSUB)!=0)
					{
						CAMsg::printMsg(LOG_CRIT,"Could not compile URL regexp: %s\n",buffRegExp);
						exit(-1);
					}
					CAMsg::printMsg(LOG_DEBUG,"Looking for crime URL RegExp: %s\n",buffRegExp);

					m_nCrimeRegExpsURL++;
				}
			}
			
			nlRegExp=elemCrimeDetection.getElementsByTagName("RegExpPayload");
			m_arCrimeRegExpsPayload=new regex_t[nlRegExp.getLength()];
			for(UINT32 i=0;i<nlRegExp.getLength();i++)
			{
				DOM_Node tmpChild=nlRegExp.item(i);
				UINT32 lenRegExp=4096;
				UINT8 buffRegExp[4096];
				if(getDOMElementValue(tmpChild,buffRegExp,&lenRegExp)==E_SUCCESS)
				{
					if(regcomp(&m_arCrimeRegExpsPayload[m_nCrimeRegExpsPayload],(char*)buffRegExp,REG_EXTENDED|REG_ICASE|REG_NOSUB)!=0)
					{
						CAMsg::printMsg(LOG_CRIT,"Could not compile payload regexp: %s\n",buffRegExp);
						exit(-1);
					}
					CAMsg::printMsg(LOG_DEBUG,"Looking for crime Payload RegExp: %s\n",buffRegExp);

					m_nCrimeRegExpsPayload++;
				}
			}				
		}
		CAMsg::printMsg(LOG_DEBUG,"Loading Crime Detection Data finished\n");

#endif
#if defined (DELAY_CHANNELS) ||defined(DELAY_USERS)||defined(DELAY_CHANNELS_LATENCY)
		///reads the parameters for the ressource limitation for last mix/first mix
		//this is at the moment:
		//<Ressources>
		//<UnlimitTraffic></UnlimitTraffic>    #Number of bytes/packets without resource limitation
		//<BytesPerIntervall></BytesPerIntervall>   #upper limit of number of bytes/packets which are processed per channel/per user per time intervall
		//<Intervall></Intervall>  #duration of one intervall in ms
		//<Latency></Latency> #minimum Latency per channel in ms
		//</Ressources>
		CAMsg::printMsg(LOG_INFO,"Loading Parameters for Resources limitation....\n");
#if defined(DELAY_CHANNELS)&&defined(DELAY_USERS)
		if(isFirstMix())
			{
				m_u32DelayChannelUnlimitTraffic=DELAY_USERS_TRAFFIC;	
				m_u32DelayChannelBucketGrow=DELAY_USERS_BUCKET_GROW;	
				m_u32DelayChannelBucketGrowIntervall=DELAY_USERS_BUCKET_GROW_INTERVALL;	
			}	
		else
			{
				m_u32DelayChannelUnlimitTraffic=DELAY_CHANNEL_TRAFFIC;	
				m_u32DelayChannelBucketGrow=DELAY_BUCKET_GROW;	
				m_u32DelayChannelBucketGrowIntervall=DELAY_BUCKET_GROW_INTERVALL;	
			}	
#elif defined(DELAY_CHANNELS)
		m_u32DelayChannelUnlimitTraffic=DELAY_CHANNEL_TRAFFIC;	
		m_u32DelayChannelBucketGrow=DELAY_BUCKET_GROW;	
		m_u32DelayChannelBucketGrowIntervall=DELAY_BUCKET_GROW_INTERVALL;	
#elif defined (DELAY_USERS)
		m_u32DelayChannelUnlimitTraffic=DELAY_USERS_TRAFFIC;	
		m_u32DelayChannelBucketGrow=DELAY_USERS_BUCKET_GROW;	
		m_u32DelayChannelBucketGrowIntervall=DELAY_USERS_BUCKET_GROW_INTERVALL;	
#endif
#if defined(DELAY_CHANNELS_LATENCY)
		m_u32DelayChannelLatency=DELAY_CHANNEL_LATENCY;	
#endif
		DOM_Element elemRessources;
		getDOMChildByName(elemRoot,(UINT8*)"Ressources",elemRessources,false);
		if(elemRessources!=NULL)
			{
				UINT32 u32;
#if defined (DELAY_CHANNELS) ||defined(DELAY_USERS)
				if(	getDOMChildByName(elemRessources,(UINT8*)"UnlimitTraffic",elem,false)==E_SUCCESS&&
						getDOMElementValue(elem,&u32)==E_SUCCESS)
					m_u32DelayChannelUnlimitTraffic=u32;
				if(	getDOMChildByName(elemRessources,(UINT8*)"BytesPerIntervall",elem,false)==E_SUCCESS&&
						getDOMElementValue(elem,&u32)==E_SUCCESS)
					m_u32DelayChannelBucketGrow=u32;
				if(	getDOMChildByName(elemRessources,(UINT8*)"Intervall",elem,false)==E_SUCCESS&&
						getDOMElementValue(elem,&u32)==E_SUCCESS)
					m_u32DelayChannelBucketGrowIntervall=u32;
#endif
#if defined (DELAY_CHANNELS_LATENCY)
				if(	getDOMChildByName(elemRessources,(UINT8*)"Latency",elem,false)==E_SUCCESS&&
						getDOMElementValue(elem,&u32)==E_SUCCESS)
					m_u32DelayChannelLatency=u32;
#endif
			}
#endif
///---- Red Configuartion of KeepAliveTraffic
		DOM_Element elemKeepAlive;
		DOM_Element elemKeepAliveSendInterval;
		DOM_Element elemKeepAliveRecvInterval;
		getDOMChildByName(elemNetwork,(UINT8*)"KeepAlive",elemKeepAlive,false);
		getDOMChildByName(elemKeepAlive,(UINT8*)"SendInterval",elemKeepAliveSendInterval,false);
		getDOMChildByName(elemKeepAlive,(UINT8*)"ReceiveInterval",elemKeepAliveRecvInterval,false);
		getDOMElementValue(elemKeepAliveSendInterval,m_u32KeepAliveSendInterval,KEEP_ALIVE_TRAFFIC_SEND_WAIT_TIME);
		getDOMElementValue(elemKeepAliveRecvInterval,m_u32KeepAliveRecvInterval,KEEP_ALIVE_TRAFFIC_RECV_WAIT_TIME);
		
    //getMixType
    DOM_Element elemMixType;
    getDOMChildByName(elemGeneral,(UINT8*)"MixType",elemMixType,false);
    tmpLen=255;
    if(getDOMElementValue(elemMixType,tmpBuff,&tmpLen)==E_SUCCESS)
    {
        if(memcmp(tmpBuff,"FirstMix",8)==0)
            m_bFirstMix=true;
        else if (memcmp(tmpBuff,"MiddleMix",9)==0)
            m_bMiddleMix=true;
        else if (memcmp(tmpBuff,"LastMix",7)==0)
            m_bLastMix=true;
    }

    //getCascadeName
    getDOMChildByName(elemGeneral,(UINT8*)"CascadeName",elem,false);
    tmpLen=255;
    if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS)
    {
        m_strCascadeName=new char[tmpLen+1];
        memcpy(m_strCascadeName,tmpBuff,tmpLen);
        m_strCascadeName[tmpLen]=0;
	}

    DOM_Element elemCascade;
    SINT32 haveCascade = getDOMChildByName(elemRoot,(UINT8*)"MixCascade",elemCascade,false);

#ifndef DYNAMIC_MIX
    /* LERNGRUPPE: This is no error in the fully dynamic model */
    if(isLastMix() && haveCascade != E_SUCCESS && getPrevMixTestCertificate() == NULL)
    {
        CAMsg::printMsg(LOG_CRIT,"Error in configuration: You must either specify cascade info or the previous mix's certificate.\n");
        return E_UNKNOWN;
    }
#endif
    if(isLastMix() && haveCascade == E_SUCCESS)
    {
        getDOMChildByName(elemRoot,(UINT8*)"MixCascade",m_oCascadeXML,false);

        DOM_NodeList nl = m_oCascadeXML.getElementsByTagName("Mix");
        UINT16 len = (UINT16)nl.getLength();
        if(len == 0)
        {
            CAMsg::printMsg(LOG_CRIT,"Error in configuration: Empty cascade specified.\n");
            return E_UNKNOWN;
        }
    }

    return E_SUCCESS;
}
#endif //ONLY_LOCAL_PROXY

#ifndef ONLY_LOCAL_PROXY

/**
  * LERNGRUPPE
  * Parses the \c InfoServices Node in a) a mix configuration or b) out of \c infoservices.xml
  * (Code refactored from CACmdLnOptions::processXmlConfiguration
  * @param a_infoServiceNode The \c InfoServices Element
  * @retval E_SUCCESS
  */
SINT32 CACmdLnOptions::parseInfoServices(DOM_Element a_infoServiceNode)
{
	DOM_Element elemAllowReconfig;
	getDOMChildByName(a_infoServiceNode,(UINT8*)"AllowAutoConfiguration",elemAllowReconfig,false);
	DOM_NodeList isList = a_infoServiceNode.getElementsByTagName("InfoService");
	/* If there are no InfoServices in the file, keep the (hopefully) previously configured InfoServices */
	if(isList.getLength() == 0)
	{
		return E_SUCCESS;
	}
	/* If there are already InfoServices, delete them */
	/** @todo merge could be better... */
	if(m_addrInfoServices!=NULL)
	{
		for(UINT32 i=0;i<m_addrInfoServicesSize;i++)
		{
			delete m_addrInfoServices[i];
		}
		delete[] m_addrInfoServices;
	}
	m_addrInfoServicesSize=0;
	m_addrInfoServices=NULL;

	UINT32 nrListenerInterfaces;
	m_addrInfoServicesSize = 0;
	m_addrInfoServices = new CAListenerInterface*[isList.getLength()];
	CAListenerInterface** isListenerInterfaces;
	for (UINT32 i = 0; i < isList.getLength(); i++)
	{
		//get ListenerInterfaces
		DOM_Element elemListenerInterfaces;
		getDOMChildByName(isList.item(i),(UINT8*)CAListenerInterface::XML_ELEMENT_CONTAINER_NAME,elemListenerInterfaces,false);
		isListenerInterfaces = CAListenerInterface::getInstance(elemListenerInterfaces, nrListenerInterfaces);
		if (nrListenerInterfaces > 0)
		{
			/** @todo Take more than one listener interface for a given IS... */
			m_addrInfoServices[m_addrInfoServicesSize] = isListenerInterfaces[0];
			m_addrInfoServicesSize++;
			for (UINT32 j = 1; j < nrListenerInterfaces; j++)
			{
				// the other interfaces are not needed...
				delete isListenerInterfaces[j];
			}
		}
	}
	UINT8 tmpBuff[255];
	UINT32 tmpLen=255;
	if(getDOMElementValue(elemAllowReconfig,tmpBuff,&tmpLen)==E_SUCCESS)
	{	
		m_bAcceptReconfiguration = (strcmp("True",(char*)tmpBuff) == 0);
	}

	return E_SUCCESS;
}


/** Builds a default Configuration
	* @param strFileName filename of the file in which the default configuration is stored, if NULL stdout is used
	*/
SINT32 CACmdLnOptions::createMixOnCDConfiguration(const UINT8* strFileName)
{
	DOM_Document doc = DOM_Document::createDocument();
	//Neasty but cool...
	bool bForLast=false;
	if(strFileName!=NULL&&strncmp((char*)strFileName,"last",4)==0)
			bForLast=true;
	buildDefaultConfig(doc,bForLast);
	saveToFile(doc, strFileName);
	return E_SUCCESS;
}

/**
  * Creates a default mix configuration.
  * @return r_doc The XML Document containing the default mix configuration
  * @retval E_SUCCESS
  */
SINT32 CACmdLnOptions::buildDefaultConfig(DOM_Document doc,bool bForLastMix=false)
{
    CASignature* pSignature=new CASignature();
    pSignature->generateSignKey(1024);
    DOM_Element elemRoot=doc.createElement("MixConfiguration");
    doc.appendChild(elemRoot);
    setDOMElementAttribute(elemRoot,"version",(UINT8*)"0.5");
    DOM_Element elemGeneral=doc.createElement("General");
    elemRoot.appendChild(elemGeneral);

    /** @todo MixType can be chosen randomly between FirstMix and MiddleMix but not LastMix!
		*sk13: ok this is a hack - but this way it can also create configurations for LastMixes which makes testing of the dynamic szenario much easier...
		*/
    DOM_Element elemTmp=doc.createElement("MixType");
		if(bForLastMix)
			setDOMElementValue(elemTmp,(UINT8*)"LastMix");
		else
			setDOMElementValue(elemTmp,(UINT8*)"FirstMix");
    elemGeneral.appendChild(elemTmp);

		/** MixID must be the SubjectKeyIdentifier of the mix' certificate */
    elemTmp=doc.createElement("MixID");
    CACertificate* pCert;
    pSignature->getVerifyKey(&pCert);
    UINT8 buf[255];
    UINT32 len = 255;
    pCert->getSubjectKeyIdentifier( buf, &len);
    setDOMElementValue(elemTmp,buf);
    elemGeneral.appendChild(elemTmp);
    elemTmp=doc.createElement("Dynamic");
    setDOMElementValue(elemTmp,(UINT8*)"True");
    elemGeneral.appendChild(elemTmp);

		elemTmp=doc.createElement("Daemon");
    setDOMElementValue(elemTmp,(UINT8*)"True");
    elemGeneral.appendChild(elemTmp);

    elemTmp=doc.createElement("CascadeName");
    setDOMElementValue(elemTmp,(UINT8*)"Dynamic Cascade");
    elemGeneral.appendChild(elemTmp);
    elemTmp=doc.createElement("MixName");
    setDOMElementValue(elemTmp,(UINT8*)"Dynamic Mix");
    elemGeneral.appendChild(elemTmp);
    elemTmp=doc.createElement("UserID");
    setDOMElementValue(elemTmp,(UINT8*)"mix");
    elemGeneral.appendChild(elemTmp);
    DOM_Element elemLogging=doc.createElement("Logging");
    elemGeneral.appendChild(elemLogging);
    elemTmp=doc.createElement("SysLog");
    setDOMElementValue(elemTmp,(UINT8*)"True");
    elemLogging.appendChild(elemTmp);
    DOM_Element elemNet=doc.createElement("Network");
    elemRoot.appendChild(elemNet);
    
    /** @todo Add a list of default InfoServices to the default configuration */
    DOM_Element elemISs=doc.createElement("InfoServices");
		elemNet.appendChild(elemISs);
		elemTmp=doc.createElement("AllowAutoConfiguration");
		setDOMElementValue(elemTmp,(UINT8*)"True");
		elemISs.appendChild(elemTmp);

		DOM_Element elemIS=doc.createElement("InfoService");
    elemISs.appendChild(elemIS);
		DOM_Element elemISListeners=doc.createElement("ListenerInterfaces");
		elemIS.appendChild(elemISListeners);
		DOM_Element elemISLi=doc.createElement("ListenerInterface");
		elemISListeners.appendChild(elemISLi);
		elemTmp=doc.createElement("Host");
    setDOMElementValue(elemTmp,(UINT8*)DEFAULT_INFOSERVICE);
    elemISLi.appendChild(elemTmp);
    elemTmp=doc.createElement("Port");
    setDOMElementValue(elemTmp,6543U);
    elemISLi.appendChild(elemTmp);
    elemTmp=doc.createElement("AllowAutoConfiguration");
    setDOMElementValue(elemTmp,(UINT8*)"True");
    elemISs.appendChild(elemTmp);

    /** We add this for compatability reasons. ListenerInterfaces can be determined dynamically now */
   /* DOM_Element elemListeners=doc.createElement("ListenerInterfaces");
    elemNet.appendChild(elemListeners);
		DOM_Element elemListener=doc.createElement("ListenerInterface");
    elemListeners.appendChild(elemListener);
    elemTmp=doc.createElement("Port");
    setDOMElementValue(elemTmp,6544U);
    elemListener.appendChild(elemTmp);
    elemTmp=doc.createElement("NetworkProtocol");
    setDOMElementValue(elemTmp,(UINT8*)"RAW/TCP");
    elemListener.appendChild(elemTmp);
		*/
		if(bForLastMix)
			{
				DOM_Element elemProxies=doc.createElement("Proxies");
				DOM_Element elemProxy=doc.createElement("Proxy");
				elemProxies.appendChild(elemProxy);
				elemTmp=doc.createElement("ProxyType");
				setDOMElementValue(elemTmp,(UINT8*)"HTTP");
				elemProxy.appendChild(elemTmp);
				elemTmp=doc.createElement("Host");
				setDOMElementValue(elemTmp,(UINT8*)"127.0.0.1");
				elemProxy.appendChild(elemTmp);
				elemTmp=doc.createElement("Port");
				setDOMElementValue(elemTmp,3128U);
				elemProxy.appendChild(elemTmp);
				elemTmp=doc.createElement("NetworkProtocol");
				setDOMElementValue(elemTmp,(UINT8*)"RAW/TCP");
				elemProxy.appendChild(elemTmp);
				elemNet.appendChild(elemProxies);
			}
    DOM_Element elemCerts=doc.createElement("Certificates");
    elemRoot.appendChild(elemCerts);
    DOM_Element elemOwnCert=doc.createElement("OwnCertificate");
    elemCerts.appendChild(elemOwnCert);
    DOM_DocumentFragment docFrag;
    pSignature->getSignKey(docFrag,doc);
    elemOwnCert.appendChild(docFrag);

    pCert->encode(docFrag,doc);
    elemOwnCert.appendChild(docFrag);

    /** @todo Add Description section because InfoService doesn't accept MixInfos without Location or Operator */
    delete pCert;
    delete pSignature;
    return E_SUCCESS;
}

/**
  * Saves the given XML Document to a file
  * @param p_doc The XML Document to be saved
  * @param p_strFileName The name of the file to be saved to
  * @retval E_SUCCESS
  */
SINT32 CACmdLnOptions::saveToFile(DOM_Document p_doc, const UINT8* p_strFileName)
{
    /** @todo Check for errors */
    UINT32 len;
    UINT8* buff = DOM_Output::dumpToMem(p_doc,&len);
    if(p_strFileName!=NULL)
    {
        FILE *handle;
        handle=fopen((const char*)p_strFileName, "w");
        fwrite(buff,len,1,handle);
        fflush(handle);
        fclose(handle);
    }
    else
    {
        fwrite(buff,len,1,stdout);
        fflush(stdout);
    }
    delete[] buff;
    return E_SUCCESS;
}


#ifdef DYNAMIC_MIX
/**
  * LERNGRUPPE
  * Creates a default configuration for this mix. The configuration is then used to start up
  * the mix! This default-config will be written to DEFAULT_CONFIG_FILE for use in the next
  * startups of this mix
  * @retval E_UNKNOWN if an error occurs
  * @retval E_SUCCESS otherwise
  */
SINT32 CACmdLnOptions::createDefaultConfiguration()
{
    m_docMixXml = DOM_Document::createDocument();
    buildDefaultConfig(m_docMixXml);
    saveToFile(m_docMixXml, (const UINT8*)DEFAULT_CONFIG_FILE);

    char *configfile = (char*) malloc(sizeof(char) * (strlen(DEFAULT_CONFIG_FILE)));
    strcpy(configfile, DEFAULT_CONFIG_FILE);

    // Set default config file for possible reread attempt
    m_strConfigFile=new UINT8[ strlen(DEFAULT_CONFIG_FILE)+1 ];
    memcpy(m_strConfigFile,DEFAULT_CONFIG_FILE, strlen(DEFAULT_CONFIG_FILE)+1);
    return E_SUCCESS;
}

/**
  * Adds a ListenerInterface to the configuration. Information is parsed out of a_elem
  * @param a_elem A XML Element containing information about the ListenerInterface to be added
  * @retval E_SUCCESS upon successful addition
  * @retval E_UNKNOWN otherwise
  */
SINT32 CACmdLnOptions::addListenerInterface(DOM_Element a_elem)
{
    CAListenerInterface *pListener = CAListenerInterface::getInstance(a_elem);
    if(pListener == NULL)
        return E_UNKNOWN;

    if(m_arListenerInterfaces != NULL && m_cnListenerInterfaces > 0)
    {
        CAListenerInterface **tmp = new CAListenerInterface*[m_cnListenerInterfaces + 1];
        for(unsigned int i = 0; i < m_cnListenerInterfaces; i++)
        {
            tmp[i] = m_arListenerInterfaces[i];
        }
        delete[] m_arListenerInterfaces;
        m_arListenerInterfaces=NULL;
        m_arListenerInterfaces = tmp;
        m_cnListenerInterfaces++;
    }
    else
    {
        m_arListenerInterfaces = new CAListenerInterface*[1];
        m_cnListenerInterfaces = 1;
    }

    m_arListenerInterfaces[m_cnListenerInterfaces-1] = pListener;
    return E_SUCCESS;

}

/**
  * Resets the network configuration of this mix. All known ListenerInterfaces in m_arListenerInterfaces will
  * be deleted, the information purged from m_docMixInfo and m_cnListenerInterfaces reset to 0
  * @retval E_SUCCESS
  */
SINT32 CACmdLnOptions::resetNetworkConfiguration()
{
    DOM_Element elemRoot = m_docMixInfo.getDocumentElement();
    if(elemRoot != NULL)
    {
        DOM_Element elemListeners;
        getDOMChildByName(elemRoot,(UINT8*)"ListenerInterfaces",elemListeners,false);
        if(elemListeners != NULL)
        {
            elemRoot.removeChild( elemListeners );
        }
    }
	clearListenerInterfaces();
    return E_SUCCESS;
}

/**
  * Tests if we have at least one usable ListenerInterface. If we don't, we try to determine
  * the needed information and create a ListenerInterface. This test includes a connectivity test.
  * @retval E_SUCCESS if we have at least one usable ListenerInterface and it is reachable
  * @retval E_UNKNOWN otherwise
  */
SINT32 CACmdLnOptions::checkListenerInterfaces()
{
    SINT32 result = E_UNKNOWN;

    UINT32 interfaces = getListenerInterfaceCount();
    CAListenerInterface *pListener = NULL;
    CADynaNetworking *dyn = new CADynaNetworking();

    for( UINT32 i = 1; i <= interfaces; i++ )
    {
        pListener = getListenerInterface(i);
        if(!pListener->isVirtual())
        {
            result = E_SUCCESS;
            break;
        }
        delete pListener;
        pListener=NULL;
    }
    if( pListener == NULL )
    {
        /** @todo Maybe we could use another port here? From the config? From the commandline? */
        result = dyn->updateNetworkConfiguration(DEFAULT_TARGET_PORT);
        if( result != E_SUCCESS )
            goto error;
    }
    if( dyn->verifyConnectivity() != E_SUCCESS )
    {
        CAMsg::printMsg( LOG_CRIT, "Your mix is not reachable from the internet.\n Please make sure that your open port %i in your firewall and forward this port to this machine.\n", DEFAULT_TARGET_PORT);
				result = E_UNKNOWN;
    }
error:
    delete dyn;
    return result;
}

/**
  * LERNGRUPPE
  * Perform a test if we have at least MIN_INFOSERVICES working 
  * InfoServices. We can not work correctly without them
  * @return r_runningInfoServices The actual number of runnung InfoServices
  * @retval E_SUCCESS if we have the InfoServices
  * @retval E_UNKOWN otherwise
*/
SINT32 CACmdLnOptions::checkInfoServices(UINT32 *r_runningInfoServices)
{
    UINT32 i;
		*r_runningInfoServices = 0;
    if(m_addrInfoServicesSize == 0 ) // WTH?
        return E_UNKNOWN;

    /** @todo Better test if these InfoServices are reachable */
    for(i = 0; i < m_addrInfoServicesSize; i++)
    {
        CASocket socket;
        socket.setSendTimeOut(1000);
        if(socket.connect( *m_addrInfoServices[i]->getAddr() )== E_SUCCESS )
        {
            (*r_runningInfoServices)++;
        }
        socket.close();
    }
    if((*r_runningInfoServices) < MIN_INFOSERVICES)
        return E_UNKNOWN;
    return E_SUCCESS;
}

/**
  * Checks if all certificate information is ok. 
  * @retval E_SUCCESS if test succeeds
  * @retval E_UNKNOWN otherwise
*/
SINT32 CACmdLnOptions::checkCertificates()
{
    /** @todo implement test. if signkey and cert are NULL here, then we parsed a user-defined config
        file without certificates in it. So we need to create a signkey and add it to the mix config. Possibly
        save it to the config file the user gave us so that in subsequent startups the certs don't change */
    return E_SUCCESS;
}

/**
  * Checks if the ID of this mix is ok. A MixID is ok if it equals( 
  * the SubjectKeyIdentifier of the mix' certificate
  * @retval E_SUCCESS if the test is successfull
  * @retval E_UNKNOWN otherwise
  */
SINT32 CACmdLnOptions::checkMixId()
{
    UINT8 ski[255];
    UINT32 len = 255;
    if( m_pOwnCertificate->getSubjectKeyIdentifier( ski, &len ) != E_SUCCESS )
        return E_UNKNOWN;

    CAMsg::printMsg( LOG_DEBUG, "\nID : (%s)\n SKI: (%s)\n", m_strMixID, ski);
    if( strcmp( (const char*)m_strMixID, (const char*)ski ) != 0 )
        return E_UNKNOWN;
    return E_SUCCESS;
}

/**
  * Returns a random InfoService's address from the list of the known InfoServices
  * The returned InfoService is tested to be online (i.e. reachable through a socket connction) 
  * @return r_address The address of the random InfoService
  * @retval E_SUCCESS if successfull
  * @retval E_UNKNOWN otherwise
  */
SINT32 CACmdLnOptions::getRandomInfoService(CASocketAddrINet *&r_address)
{
    UINT32 nrAddresses;
    CAListenerInterface** socketAddresses = pglobalOptions->getInfoServices(nrAddresses);
    if( socketAddresses == NULL )
    {
        CAMsg::printMsg( LOG_ERR, "Unable to get a list of InfoServices from the options, check your configuration!\n");
        return E_UNKNOWN;
    }
    UINT32 index = getRandom(nrAddresses);
    // Search for a runnung infoservice from the random index on overlapping at nrAddresses
	UINT32 i = (index+1) % nrAddresses;
	while(true)
	{
		CASocket socket;
		socket.setSendTimeOut(1000);
		r_address = (CASocketAddrINet*)m_addrInfoServices[i]->getAddr();
		if(socket.connect( *r_address )== E_SUCCESS )
    {
#ifdef DEBUG
    UINT8 buf[2048];
    UINT32 len=2047;
    r_address->getHostName( buf, len );
		CAMsg::printMsg( LOG_DEBUG, "getRandomInfoService: Chose InfoService server  %s:%i\n", buf, r_address->getPort());
#endif
			socket.close();
    return E_SUCCESS;
		}
		else
		{
			socket.close();
			delete r_address;
			r_address = NULL;
		}
		if(i == index) break;
		i = (i+1) % nrAddresses;
	}
	return E_UNKNOWN;
}

/**
  * Returns a random number between 0 and a_max
  * @param a_max Max value of the returned random
  * @retval The random number
  */
UINT32 CACmdLnOptions::getRandom(UINT32 a_max)
{
    UINT32 result = (UINT32) (a_max * (rand() / (RAND_MAX + 1.0)));
    return result;
}

/**
 * LERNGRUPPE
 * Changes the information about the type of this mix. This is needed if a 
 * FirstMix should be reconfigured as MiddleMix and vice versa. 
 * @param a_newMixType The new type of this mix
 * @retval E_SUCCESS if everything went well
 * @retval E_UNKNOWN otherwise
 */
SINT32 CACmdLnOptions::changeMixType(CAMix::tMixType a_newMixType)
{
	if( a_newMixType == CAMix::LAST_MIX )
	{
		CAMsg::printMsg( LOG_ERR,"Trying to reconfigure a dynamic mix to LastMix, that is evil!\n");
		return E_UNKNOWN;
	}

	if( a_newMixType == CAMix::MIDDLE_MIX && isFirstMix())
	{
		CAMsg::printMsg( LOG_DEBUG,"Reconfiguring a FirstMix to MiddleMix.\n");
		m_bFirstMix = false;
		m_bMiddleMix = true;
		DOM_Element elemRoot = m_docMixInfo.getDocumentElement();
		if(elemRoot != NULL)
		{
			DOM_Element elemMixType;
			getDOMChildByName(elemRoot,(UINT8*)"MixType",elemMixType,false);
			if(elemMixType != NULL)
			{
				setDOMElementValue(elemMixType,(UINT8*)"MiddleMix");
			}
		}
	}
	else if( a_newMixType == CAMix::FIRST_MIX && isMiddleMix())
	{
		CAMsg::printMsg( LOG_DEBUG,"Reconfiguring a MiddleMix to FirstMix.\n");
		m_bFirstMix = true;
		m_bMiddleMix = false;
		DOM_Element elemRoot = m_docMixInfo.getDocumentElement();
		if(elemRoot != NULL)
		{
			DOM_Element elemMixType;
			getDOMChildByName(elemRoot,(UINT8*)"MixType",elemMixType,false);
			if(elemMixType != NULL)
			{
				setDOMElementValue(elemMixType,(UINT8*)"FirstMix");
			}
		}
	}
	else
	{
		CAMsg::printMsg( LOG_ERR, "Error reconfiguring the mix, some strange combination of existing and new type happened\n");
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}

#endif //DYNAMIC_MIX
#endif //ONLY_LOCAL_PROXY
