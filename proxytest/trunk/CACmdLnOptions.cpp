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
#include "CAXMLBI.hpp"
#include "xml/DOM_Output.hpp"
#ifdef LOG_CRIME
	#include "tre/regex.h"
#endif

CACmdLnOptions::CACmdLnOptions()
  {
		m_bDaemon=m_bIsRunReConfigure=false;
		m_bLocalProxy=m_bFirstMix=m_bLastMix=m_bMiddleMix=false;
		m_iTargetPort=m_iSOCKSPort=m_iSOCKSServerPort=m_iInfoServerPort=0xFFFF;
		m_strTargetHost=m_strSOCKSHost=m_strInfoServerHost=NULL;
		m_strUser=m_strCascadeName=m_strLogDir=m_strEncryptedLogDir=NULL;
		m_arTargetInterfaces=NULL;
		m_cnTargets=0;
		m_arListenerInterfaces=NULL;
		m_cnListenerInterfaces=0;
		m_nrOfOpenFiles=-1;
		m_strMixID=NULL;
		m_pSignKey=NULL;
		m_pOwnCertificate=NULL;
		m_pPrevMixCertificate=NULL;
		m_pNextMixCertificate=NULL;
		m_bCompressedLogs=false;
		m_bAutoReconnect=false;
		m_strConfigFile=NULL;
		m_docMixInfo=NULL;
		m_pLogEncryptionCertificate=NULL;
		m_bIsEncryptedLogEnabled=false;
		m_pcsReConfigure=new CAMutex();
		m_strPidFile=NULL;
 }

CACmdLnOptions::~CACmdLnOptions()
	{
		clean();
		delete m_pcsReConfigure;
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
		if(m_strInfoServerHost!=NULL)
			{
				delete[] m_strInfoServerHost;
	    }
		m_strInfoServerHost=NULL;
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
		if(m_docMixInfo!=NULL)
			m_docMixInfo=NULL;
		if(m_strMixID!=NULL)
			delete[] m_strMixID;
		m_strMixID=NULL;
		clearTargetInterfaces();
		clearListenerInterfaces();
		if(m_pSignKey!=NULL)
			delete m_pSignKey;
		m_pSignKey=NULL;
		if(m_pOwnCertificate!=NULL)
			delete m_pOwnCertificate;
		m_pOwnCertificate=NULL;
		if(m_pNextMixCertificate!=NULL)
			delete m_pNextMixCertificate;
		m_pNextMixCertificate=NULL;
		if(m_pPrevMixCertificate!=NULL)
			delete m_pPrevMixCertificate;
		m_pPrevMixCertificate=NULL;
		if(m_pLogEncryptionCertificate!=NULL)
			delete m_pLogEncryptionCertificate;
		m_pLogEncryptionCertificate=NULL;
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
	//DOM_Document docMixXml;
	poptOption options[]=
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
		POPT_AUTOHELP
		{NULL,0,0,
		NULL,0,NULL,NULL}
	};
	poptContext ctx=poptGetContext(NULL,argc,argv,options,0);
	int ret=poptGetNextOpt(ctx);
	while(ret==POPT_ERROR_BADOPT)
		ret=poptGetNextOpt(ctx);
	poptFreeContext(ctx);
	if(iVersion!=0)
		{
			printf(MIX_VERSION_INFO);
			exit(0);
		}

	if(iLocalProxy!=0)
		m_bLocalProxy=true;
	if(m_bLocalProxy&&iAutoReconnect!=0)
		m_bAutoReconnect=true;
	if(configfile!=NULL)
		{
			SINT32 ret=readXmlConfiguration(m_docMixXml,(UINT8*)configfile);
			if(ret==E_FILE_OPEN)
				CAMsg::printMsg(LOG_CRIT,"Couldt not open config file: %s\n",configfile);
			else if(ret==E_FILE_READ)
				CAMsg::printMsg(LOG_CRIT,"Couldt not read config file: %s\n",configfile);
			else if(ret==E_XML_PARSE)
				CAMsg::printMsg(LOG_CRIT,"Couldt not parse config file: %s\n",configfile);
			else
				{
					m_strConfigFile=new UINT8[strlen(configfile)+1];
					memcpy(m_strConfigFile,configfile,strlen(configfile)+1);
				}
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
						int tmpPort;
						char* tmpStr1=strchr(target,':');
						if(tmpStr1!=NULL)
							{
								memcpy(tmpHostname,target,tmpStr1-target);
								tmpHostname[tmpStr1-target]=0;
								tmpPort=(int)atol(tmpStr1+1);
							}
						else
							{//TODO what if not in right form ?
								//try if it is a number --> use it as port
								//and use 'localhost' as traget-host
								tmpPort=(int)atol(target);
								if(tmpPort!=0) //we get it
									{
										strcpy(tmpHostname,"localhost");
									}
								else //we try to use it as host and use the default port
									{
#define DEFAULT_TARGET_PORT 6544
										tmpPort=DEFAULT_TARGET_PORT;
										strcpy(tmpHostname,target);
									}
							}
						m_strTargetHost=new char[strlen(tmpHostname)+1];
						strcpy(m_strTargetHost,tmpHostname);
						m_iTargetPort=tmpPort;
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
					m_iSOCKSPort=(int)atol(tmpStr+1);
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
					int iServerPort;
					if((tmpStr=strchr(serverPort,':'))!=NULL) //host:port
						{
							strServerHost=new char[tmpStr-serverPort+1];
							(*tmpStr)=0;
							strcpy(strServerHost,serverPort);
							iServerPort=(int)atol(tmpStr+1);
						}
					else //port only ?
						{
							iServerPort=(int)atol(serverPort);
						}
						m_arListenerInterfaces[0]=CAListenerInterface::getInstance(RAW_TCP,(UINT8*)strServerHost,iServerPort);
					delete [] strServerHost;
				}
			free(serverPort);
			if(m_arListenerInterfaces[0]!=0)
				m_cnListenerInterfaces=1;
		}

	m_iSOCKSServerPort=SOCKSport;
	if(!m_bLocalProxy)
		{
			ret=processXmlConfiguration(m_docMixXml);
			if(ret!=E_SUCCESS)
				return ret;
		}
	return E_SUCCESS;
 }

struct t_CMNDLN_REREAD_PARAMS
	{
		CACmdLnOptions* pCmdLnOptions;
		CAMix* pMix;
	};

/** Copies options from \c newOptions. Only those options which are specified
	* in \c newOptions are copied. The others are left uuntouched!
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
#ifdef DELAY_CHANNELS			
		//Copy ressources limitation
		m_u32DelayChannelUnlimitTraffic=newOptions.getDelayChannelUnlimitTraffic();
		m_u32DelayChannelBucketGrow=newOptions.getDelayChannelBucketGrow();
		m_u32DelayChannelBucketGrowIntervall=newOptions.getDelayChannelBucketGrowIntervall();
#endif			
		return E_SUCCESS;
}

/** Modifies the next mix settings (target interface and certificate) according to
* the specified options object. Target interfaces are only copied if they denote a
* next mix. HTTP and SOCKS proxy settings are ignored.
* @param xmlData a string containing XML data with the new options
* @param len the length of the string
* 
*/
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

/** Modifies the next mix settings (target interface and certificate) according to
* the specified options object. Target interfaces are only copied if they denote a
* next mix. HTTP and SOCKS proxy settings are ignored.
* @param xmlData a string containing XML data with the new options
* @param len the length of the string
* 
*/
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




/** Rereads the configuration file (if one was given on startup) and reconfigures
	* the mix according to the new values. This is done asyncronous. A new thread is
	* started, which does the actual work.
	* @retval E_SUCCESS if successful
	* @retval E_UNKNOWN if an error occurs
	*/
SINT32 CACmdLnOptions::reread(CAMix* pMix)
	{
		if(m_bIsRunReConfigure)
			return E_UNKNOWN;
		CAMsg::printMsg(LOG_DEBUG,"Re-readed before lock\n");
		m_pcsReConfigure->lock();
		CAMsg::printMsg(LOG_DEBUG,"Re-readed after lock\n");
		m_bIsRunReConfigure=true;
		m_threadReConfigure.setMainLoop(threadReConfigure);
		CAMsg::printMsg(LOG_DEBUG,"Re-read After set thread loop\n");
		t_CMNDLN_REREAD_PARAMS* param=new t_CMNDLN_REREAD_PARAMS;
		param->pCmdLnOptions=this;
		param->pMix=pMix;
		m_threadReConfigure.start(param);
		m_pcsReConfigure->unlock();
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
		pOptions->m_pcsReConfigure->lock();
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
		pOptions->m_bIsRunReConfigure=false;
		CAMsg::printMsg(LOG_DEBUG,"ReConfiguration of the Mix finished!\n");
		pOptions->m_pcsReConfigure->unlock();
		THREAD_RETURN_SUCCESS;
	}

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

#ifdef PAYMENT
CAXMLBI * CACmdLnOptions::getBI()
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
	return (SINT32)strlen((char *)m_strDatabaseHost);	
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
	return (SINT32)strlen((char *)m_strDatabaseName);	
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
	return (SINT32)strlen((char *)m_strDatabaseUser);	
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
	return (SINT32)strlen((char *)m_strDatabasePassword);	
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
	return (SINT32)strlen((char *)m_strAiID);
}

SINT32 CACmdLnOptions::getPaymentHardLimit(UINT32 *pHardLimit)
	{
		*pHardLimit = m_iPaymentHardLimit;
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

UINT16 CACmdLnOptions::getInfoServerPort()
  {
		return m_iInfoServerPort;
  }

SINT32 CACmdLnOptions::getInfoServerHost(UINT8* host,UINT32 len)
  {
		if((m_strInfoServerHost==NULL)||(len<=(UINT32)strlen(m_strInfoServerHost)))
			return E_UNKNOWN;
		strcpy((char*)host,m_strInfoServerHost);
		return E_SUCCESS;
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

SINT32 CACmdLnOptions::getEncryptedLogDir(UINT8* name,UINT32 len)
  {
		if(m_strEncryptedLogDir==NULL||name==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(m_strEncryptedLogDir))
			return E_UNKNOWN;
		strcpy((char*)name,m_strEncryptedLogDir);
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

/** Returns the XML tree describing the Mix . This is NOT a copy!
	* @param docMixInfo destination for the XML tree
	*	@retval E_SUCCESS if it was successful
	* @retval E_UNKNOWN in case of an error
*/
SINT32 CACmdLnOptions::getMixXml(DOM_Document& docMixInfo)
	{
		docMixInfo=m_docMixInfo;
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
		getDOMChildByName(elemRoot,(UINT8*)"General",elemGeneral,false);

		//get MixID
		UINT8 tmpBuff[255];
		UINT32 tmpLen=255;
		DOM_Element elemMixID;
		getDOMChildByName(elemGeneral,(UINT8*)"MixID",elemMixID,false);
		if(elemMixID==NULL)
			return E_UNKNOWN;
		if(getDOMElementValue(elemMixID,tmpBuff,&tmpLen)!=E_SUCCESS)
			return E_UNKNOWN;
		strtrim(tmpBuff);
		m_strMixID=new char[strlen((char*)tmpBuff)+1];
		strcpy(m_strMixID,(char*) tmpBuff);
		//getMixType
		DOM_Element elem;
		getDOMChildByName(elemGeneral,(UINT8*)"MixType",elem,false);
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

		//getCascadeName
		getDOMChildByName(elemGeneral,(UINT8*)"CascadeName",elem,false);
		tmpLen=255;
		if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS)
			{
				m_strCascadeName=new char[tmpLen+1];
				memcpy(m_strCascadeName,tmpBuff,tmpLen);
				m_strCascadeName[tmpLen]=0;
			}

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
		if(elemOwnCert!=NULL)
			{
				m_pSignKey=new CASignature();
				UINT8 passwd[500];
				passwd[0]=0;
				if(m_pSignKey->setSignKey(elemOwnCert.getFirstChild(),SIGKEY_PKCS12)!=E_SUCCESS)
					{//Maybe not an empty passwd
						printf("I need a passwd for the SignKey: ");
						scanf("%400[^\n]%*1[\n]",(char*)passwd);
						if(m_pSignKey->setSignKey(elemOwnCert.getFirstChild(),SIGKEY_PKCS12,(char*)passwd)!=E_SUCCESS)
							{
								CAMsg::printMsg(LOG_CRIT,"Could not read own signature key!\n");
								delete m_pSignKey;
								m_pSignKey=NULL;
							}
					}
				m_pOwnCertificate=CACertificate::decode(elemOwnCert.getFirstChild(),CERT_PKCS12,(char*)passwd);
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
		getDOMChildByName(elemRoot,(UINT8*)"Accounting",elemAccounting,false);
		if(elemAccounting != NULL) {
			DOM_Element elemJPI;
			CAXMLBI * pBI=0;
			getDOMChildByName(elemAccounting, CAXMLBI::getXMLElementName(), elemJPI, false);
			if(elemJPI!=NULL)
				pBI = new CAXMLBI(elemJPI);
			if(pBI)
				m_pBI = pBI;
			else
				m_pBI = 0;
			
		
			getDOMChildByName(elemAccounting, (UINT8*)"SoftLimit", elem, false);
			if(getDOMElementValue(elem, &tmp)==E_SUCCESS)
			{
				m_iPaymentSoftLimit = tmp;
				}
				
			getDOMChildByName(elemAccounting, (UINT8*)"HardLimit", elem, false);
			if(getDOMElementValue(elem, &tmp)==E_SUCCESS)
			{
				m_iPaymentHardLimit = tmp;
			}
			getDOMChildByName(elemAccounting, (UINT8*)"SettleInterval", elem, false);
			if(getDOMElementValue(elem, &tmp)==E_SUCCESS)
			{
				m_iPaymentSettleInterval = tmp;
				}
			
			// get DB Username
			getDOMChildByName(elemAccounting, (UINT8*)"AiID", elem, false);
			tmpLen = 255;
			if(getDOMElementValue(elem, tmpBuff, &tmpLen)==E_SUCCESS) {
				strtrim(tmpBuff);
				m_strAiID = new UINT8[strlen((char*)tmpBuff)+1];
				strcpy((char *)m_strAiID, (char *) tmpBuff);
			}

			
			DOM_Element elemDatabase;
			getDOMChildByName(elemAccounting, (UINT8*)"Database", elemDatabase, false);
			if(elemDatabase != NULL) {
				// get DB Hostname
				getDOMChildByName(elemDatabase, (UINT8*)"Host", elem, false);
				tmpLen = 255;
				if(getDOMElementValue(elem, tmpBuff, &tmpLen)==E_SUCCESS) {
					strtrim(tmpBuff);
					m_strDatabaseHost = new UINT8[strlen((char*)tmpBuff)+1];
					strcpy((char *)m_strDatabaseHost, (char *) tmpBuff);
				}
				// get Database Port
				getDOMChildByName(elemDatabase, (UINT8*)"Port", elem, false);
				if(getDOMElementValue(elem, &tmp)==E_SUCCESS) {
					m_iDatabasePort = tmp;
				}
				// get DB Name
				getDOMChildByName(elemDatabase, (UINT8*)"DBName", elem, false);
				tmpLen = 255;
				if(getDOMElementValue(elem, tmpBuff, &tmpLen)==E_SUCCESS) {
					strtrim(tmpBuff);
					m_strDatabaseName = new UINT8[strlen((char*)tmpBuff)+1];
					strcpy((char *)m_strDatabaseName, (char *) tmpBuff);
				}
				// get DB Username
				getDOMChildByName(elemDatabase, (UINT8*)"Username", elem, false);
				tmpLen = 255;
				if(getDOMElementValue(elem, tmpBuff, &tmpLen)==E_SUCCESS) {
					strtrim(tmpBuff);
					m_strDatabaseUser = new UINT8[strlen((char*)tmpBuff)+1];
					strcpy((char *)m_strDatabaseUser, (char *) tmpBuff);
				}
/*				getDOMChildByName(elemDatabase, (UINT8*)"Password", elem, false);
				tmpLen = 255;
				if(getDOMElementValue(elem, tmpBuff, &tmpLen)==E_SUCCESS) {
					strtrim(tmpBuff);
					m_strDatabasePassword = new char[strlen((char*)tmpBuff)+1];
					strcpy(m_strDatabasePassword, (char *) tmpBuff);
				}*/
				// don't read password from XML but from stdin:
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
		}
		else {
			CAMsg::printMsg( 17, "No accounting instance info found in configfile. Payment will not work!");
		}

#endif /* ifdef PAYMENT */


		//get InfoService data
		DOM_Element elemNetwork;
		getDOMChildByName(elemRoot,(UINT8*)"Network",elemNetwork,false);
		DOM_Element elemInfoService;
		getDOMChildByName(elemNetwork,(UINT8*)"InfoService",elemInfoService,false);
    DOM_Element elemAllowReconfig;
    getDOMChildByName(elemInfoService,(UINT8*)"AllowAutoConfiguration",elemAllowReconfig,false);
    if(getDOMElementValue(elemAllowReconfig,tmpBuff,&tmpLen)==E_SUCCESS)
    {
        m_bAcceptReconfiguration = (strcmp("True",(char*)tmpBuff) == 0);
    }
		getDOMChildByName(elemInfoService,(UINT8*)"Host",elem,false);
		tmpLen=255;
		if(getDOMElementValue(elem,tmpBuff,&tmpLen)==E_SUCCESS)
			{
				strtrim(tmpBuff);
				m_strInfoServerHost=new char[strlen((char*)tmpBuff)+1];
				strcpy(m_strInfoServerHost,(char*)tmpBuff);
			}
		getDOMChildByName(elemInfoService,(UINT8*)"Port",elem,false);
		if(getDOMElementValue(elem,&tmp)==E_SUCCESS)
			m_iInfoServerPort=tmp;

		//get ListenerInterfaces
		DOM_Element elemListenerInterfaces;
		getDOMChildByName(elemNetwork,(UINT8*)"ListenerInterfaces",elemListenerInterfaces,false);
		if(elemListenerInterfaces!=NULL)
			{
				DOM_NodeList nlListenerInterfaces;
				nlListenerInterfaces=elemListenerInterfaces.getElementsByTagName("ListenerInterface");
				m_cnListenerInterfaces=nlListenerInterfaces.getLength();
				if(m_cnListenerInterfaces>0)
					{
						m_arListenerInterfaces=new CAListenerInterface*[m_cnListenerInterfaces];
						UINT32 aktInterface=0;
//						UINT32 type=0;
//						bool bHidden=false;
//						bool bVirtual=false;
//						CASocketAddr* addr=NULL;
//						UINT8* hostname=NULL;
//						UINT16 port;
						for(UINT32 i=0;i<m_cnListenerInterfaces;i++)
							{
								DOM_Node elemListenerInterface;
								elemListenerInterface=nlListenerInterfaces.item(i);
								CAListenerInterface* pListener=CAListenerInterface::getInstance(elemListenerInterface);
								if(pListener!=NULL)
									m_arListenerInterfaces[aktInterface++]=pListener;
							}
						m_cnListenerInterfaces=aktInterface;
					}
			}

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

		//Next Proxies
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
									proxy_type=TARGET_SOCKS_PROXY;
								else if(strcmp((char*)tmpBuff,"HTTP")==0)
									proxy_type=TARGET_HTTP_PROXY;
								else
									continue;


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

    // import listener interfaces element; this is needed for cascade auto configuration
    // -- inserted by ronin <ronin2@web.de> 2004-08-16
    elemMix.appendChild(m_docMixInfo.importNode(elemListenerInterfaces,true));

		//Set Software-Version...
		DOM_Element elemSoftware=m_docMixInfo.createElement("Software");
		DOM_Element elemVersion=m_docMixInfo.createElement("Version");
		setDOMElementValue(elemVersion,(UINT8*)MIX_VERSION);
		elemSoftware.appendChild(elemVersion);
		elemMix.appendChild(elemSoftware);

#ifdef LOG_CRIME
		m_arCrimeRegExps=NULL;
		m_nCrimeRegExps=0;
		CAMsg::printMsg(LOG_INFO,"Loading Crime Detection Data....\n");
		DOM_Element elemCrimeDetection;
		getDOMChildByName(elemRoot,(UINT8*)"CrimeDetection",elemCrimeDetection,false);
		if(elemCrimeDetection!=NULL)
			{
				DOM_NodeList nlRegExp;
				nlRegExp=elemCrimeDetection.getElementsByTagName("RegExp");
				m_arCrimeRegExps=new regex_t[nlRegExp.getLength()];
				for(UINT32 i=0;i<nlRegExp.getLength();i++)
					{
						DOM_Node tmpChild=nlRegExp.item(i);
						UINT32 lenRegExp=4096;
						UINT8 buffRegExp[4096];
						if(getDOMElementValue(tmpChild,buffRegExp,&lenRegExp)==E_SUCCESS)
							{
								if(regcomp(&m_arCrimeRegExps[m_nCrimeRegExps],(char*)buffRegExp,REG_EXTENDED|REG_ICASE|REG_NOSUB)!=0)
									{
										CAMsg::printMsg(LOG_CRIT,"Could not compile regexp: %s\n",buffRegExp);
										exit(-1);
									}
									CAMsg::printMsg(LOG_DEBUG,"Looking for crime URL RegExp: %s\n",buffRegExp);

								m_nCrimeRegExps++;
							}
					}
			}
		CAMsg::printMsg(LOG_DEBUG,"Loading Crime Detection Data finished\n");

#endif
#ifdef DELAY_CHANNELS
		///reads the parameters for the ressource limitation
		//this is at the moment:
		//<Ressources>
		//<UnlimitTraffic></UnlimitTraffic>    #Number of bytes without resource limitation
		//<BytesPerIntervall></BytesPerIntervall>   #upper limit of number of bytes which are processed per channel per time intervall
		//<Intervall></Intervall>  #duration of one intervall in ms 
		//</Ressources>
		CAMsg::printMsg(LOG_INFO,"Loading Parameters for Resources limitation....\n");
		m_u32DelayChannelUnlimitTraffic=DELAY_CHANNEL_TRAFFIC;	
		m_u32DelayChannelBucketGrow=DELAY_BUCKET_GROW;	
		m_u32DelayChannelBucketGrowIntervall=DELAY_BUCKET_GROW_INTERVALL;	
		DOM_Element elemRessources;
		getDOMChildByName(elemRoot,(UINT8*)"Ressources",elemRessources,false);
		if(elemRessources!=NULL)
			{
				UINT32 u32;
				if(	getDOMChildByName(elemRessources,(UINT8*)"UnlimitTraffic",elem,false)==E_SUCCESS&&
						getDOMElementValue(elem,&u32)==E_SUCCESS)
					m_u32DelayChannelUnlimitTraffic=u32;
				if(	getDOMChildByName(elemRessources,(UINT8*)"BytesPerIntervall",elem,false)==E_SUCCESS&&
						getDOMElementValue(elem,&u32)==E_SUCCESS)
					m_u32DelayChannelBucketGrow=u32;
				if(	getDOMChildByName(elemRessources,(UINT8*)"Intervall",elem,false)==E_SUCCESS&&
						getDOMElementValue(elem,&u32)==E_SUCCESS)
					m_u32DelayChannelBucketGrowIntervall=u32;
			}
#endif

    tmpLen=255;
    getDOMChildByName(elemGeneral,(UINT8*)"MixID",elemMixID,false);
    if(elemMixID==NULL)
        return E_UNKNOWN;
    if(getDOMElementValue(elemMixID,tmpBuff,&tmpLen)!=E_SUCCESS)
        return E_UNKNOWN;
    strtrim(tmpBuff);
    m_strMixID=new char[strlen((char*)tmpBuff)+1];
    strcpy(m_strMixID,(char*) tmpBuff);
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

    if(isLastMix() && haveCascade != E_SUCCESS && getPrevMixTestCertificate() == NULL)
    {
        CAMsg::printMsg(LOG_CRIT,"Error in configuration: You must either specify cascade info or the previous mix's certificate.\n");
        return E_UNKNOWN;
    }

    if(isLastMix() && haveCascade == E_SUCCESS)
    {
        getDOMChildByName(elemRoot,(UINT8*)"MixCascade",m_oCascadeXML,false);

        DOM_NodeList nl = m_oCascadeXML.getElementsByTagName("Mix");
        UINT16 len = nl.getLength();
        if(len == 0)
        {
            CAMsg::printMsg(LOG_CRIT,"Error in configuration: Empty cascade specified.\n");
            return E_UNKNOWN;
        }
    }

    return E_SUCCESS;
}
