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
//#include "CAIPAddrWithNetmask.hpp"
#include "CASocket.hpp"
#include "CAXMLBI.hpp"
#include "xml/DOM_Output.hpp"
#include "CABase64.hpp"
#include "CADynaNetworking.hpp"
#ifdef PAYMENT
#include "CAAccountingDBInterface.hpp"
#endif
//#ifdef LOG_CRIME
	#include "tre/tre.h"
//#endif


CACmdLnOptions::CACmdLnOptions()
	{
		m_bDaemon=false;
		m_bSyslog=false;
		m_bLogConsole = false;
		m_bSocksSupport = false;
		m_bVPNSupport = false;
		m_bLocalProxy=m_bFirstMix=m_bLastMix=m_bMiddleMix=false;

#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX
		m_docMixInfo= createDOMDocument();
		m_docMixXml=NULL;
		m_addrInfoServices = NULL;
		m_addrInfoServicesSize=0;
		m_OpCert=NULL;
		m_pMultiSignature=NULL;
		m_pPrevMixCertificate=NULL;
		m_pNextMixCertificate=NULL;
		m_pTrustedRootCertificates=NULL;
		m_bVerifyMixCerts=false;
		m_bAcceptReconfiguration=false;
#endif

#ifndef ONLY_LOCAL_PROXY
		m_bIsRunReConfigure=false;
		m_pcsReConfigure=new CAMutex();
		//m_pSignKey=NULL;
		//m_pOwnCertificate=NULL;
		m_bCompressedLogs=false;
		m_pLogEncryptionCertificate=NULL;
		m_bIsEncryptedLogEnabled=false;
		m_pCascadeXML=NULL;
		m_docOpTnCs=NULL; //Operator Terms and Conditions (if any)
		m_maxNrOfUsers = 0;
		m_strAccessControlCredential = NULL;
#ifdef PAYMENT		
		m_PaymentReminderProbability= -1;
#else
		m_PaymentReminderProbability= -1;
#endif
		
#ifdef COUNTRY_STATS
		m_dbCountryStatsHost=m_dbCountryStatsPasswd=m_dbCountryStatsUser=NULL;
#endif
		m_termsAndConditionsTemplates = NULL;
		m_nrOfTermsAndConditionsTemplates = 0;
#endif //ONLY_LOCAL_PROXY
		m_iTargetPort=m_iSOCKSPort=m_iSOCKSServerPort=0xFFFF;
		m_strTargetHost=m_strSOCKSHost=NULL;
		m_strUser=NULL;
		m_strCascadeName=NULL;
		m_strLogDir=NULL;
		m_strLogLevel=NULL;
		setZero64(m_maxLogFileSize);
		m_strEncryptedLogDir=NULL;
		m_arTargetInterfaces=NULL;
		m_cnTargets=0;
		m_arListenerInterfaces=NULL;
		m_cnListenerInterfaces=0;
		m_arStrVisibleAddresses=NULL;
		m_cnVisibleAddresses=0;
		m_nrOfOpenFiles=-1;
		m_strMixID=NULL;
		m_strMixName=NULL;
		m_bAutoReconnect=false;
		m_strConfigFile=NULL;
		m_strPidFile=NULL;
		m_strCredential = NULL;
#ifdef PAYMENT
		m_pBI=NULL;
		m_strDatabaseHost=NULL;
		m_strDatabaseName=NULL;
		m_strDatabaseUser=NULL;
		m_strDatabasePassword=NULL;
		m_strAiID=NULL;
#endif
#ifdef SERVER_MONITORING
		m_strMonitoringListenerHost = NULL;
		m_iMonitoringListenerPort = 0xFFFF;
#endif

#ifdef LOG_CRIME
		m_logPayload = false;
		m_arCrimeRegExpsURL=NULL;
		m_nCrimeRegExpsURL=0;
		m_arCrimeRegExpsPayload=NULL;
		m_nCrimeRegExpsPayload=0;
		m_nrOfSurveillanceIPs = 0;
		m_surveillanceIPs = NULL;
		m_nrOfSurveillanceAccounts = 0;
		m_surveillanceAccounts = NULL;
#endif

#ifdef DATA_RETENTION_LOG
		m_strDataRetentionLogDir=NULL;
#endif

#ifdef EXPORT_ASYM_PRIVATE_KEY
		m_strImportKeyFile=NULL;
		m_strExportKeyFile=NULL;
#endif

#ifdef DYNAMIC_MIX
		m_strLastCascadeProposal = NULL;
#endif

#if defined(DELAY_CHANNELS) && defined(DELAY_USERS)
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
		m_u32DelayChannelLatency = DELAY_CHANNEL_LATENCY;
#endif

#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX
		// initialize pointer to option setter functions 
		initMainOptionSetters();
		initGeneralOptionSetters();
		initCertificateOptionSetters();
#ifdef PAYMENT
		initAccountingOptionSetters();
#endif
		initNetworkOptionSetters();
#ifdef PAYMENT
		initTermsAndConditionsOptionSetters();
#endif
#ifdef LOG_CRIME
		initCrimeDetectionOptionSetters();
#endif
#endif //ONLY_LOCAL_PROXY

 }

 
CACmdLnOptions::~CACmdLnOptions()
	{
		cleanup();
	}

#ifndef ONLY_LOCAL_PROXY


void CACmdLnOptions::initTermsAndConditionsOptionSetters()
{

	m_arpTermsAndConditionsOptionSetters = new optionSetter_pt [TERMS_AND_CONDITIONS_OPTIONS_NR];
	int count = -1;

	m_arpTermsAndConditionsOptionSetters[++count]=& CACmdLnOptions::setTermsAndConditionsTemplates;
	m_arpTermsAndConditionsOptionSetters[++count]=	&CACmdLnOptions::setTermsAndConditionsList;
}

#ifdef LOG_CRIME
void CACmdLnOptions::initCrimeDetectionOptionSetters()
{
	crimeDetectionOptionSetters = new optionSetter_pt[CRIME_DETECTION_OPTIONS_NR];
	int count = -1;

	crimeDetectionOptionSetters[++count]=
		&CACmdLnOptions::setCrimeURLRegExp;
	crimeDetectionOptionSetters[++count]=
		&CACmdLnOptions::setCrimePayloadRegExp;
	crimeDetectionOptionSetters[++count]=
		&CACmdLnOptions::setCrimeSurveillanceAccounts;
	crimeDetectionOptionSetters[++count]=
		&CACmdLnOptions::setCrimeSurveillanceIP;
}
#endif

#endif //ONLY_LOCAL_PROXY

// This is the final cleanup, which deletes every resource (including any locks necessary to synchronise read/write to properties).
//
SINT32 CACmdLnOptions::cleanup()
	{
		clean();
#ifndef ONLY_LOCAL_PROXY
		delete m_pcsReConfigure;
		m_pcsReConfigure=NULL;
#endif
		return E_SUCCESS;
	}

// Deletes all information about the target interfaces.
	//
SINT32 CACmdLnOptions::clearTargetInterfaces()
	{
		if(m_arTargetInterfaces!=NULL)
			{
				for(UINT32 i=0;i<m_cnTargets;i++)
				{
					m_arTargetInterfaces[i].cleanAddr();
				}
				delete[] m_arTargetInterfaces;
				m_arTargetInterfaces = NULL;
			}
		m_cnTargets=0;
		m_arTargetInterfaces=NULL;
		return E_SUCCESS;
	}

// Deletes all information about the listener interfaces.
	//
SINT32 CACmdLnOptions::clearListenerInterfaces()
	{
		if(m_arListenerInterfaces!=NULL)
			{
				for(UINT32 i=0;i<m_cnListenerInterfaces;i++)
				{
					delete m_arListenerInterfaces[i];
					m_arListenerInterfaces[i] = NULL;
				}
				delete[] m_arListenerInterfaces;
			}
		m_cnListenerInterfaces=0;
		m_arListenerInterfaces=NULL;
		return E_SUCCESS;
	}

#ifndef ONLY_LOCAL_PROXY
#endif //ONLY_LOCAL_PROXY



/** Deletes all resssource allocated by objects of this class EXPECT the locks necessary to controll access to the properties of this class*/
void CACmdLnOptions::clean()
	{
		delete[] m_strConfigFile;
		m_strConfigFile=NULL;

		delete[] m_strTargetHost;
		m_strTargetHost=NULL;

		delete[] m_strSOCKSHost;
		m_strSOCKSHost=NULL;

#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX
		if (m_addrInfoServices != NULL)
			{
				for (UINT32 i = 0; i < m_addrInfoServicesSize; i++)
				{
					delete m_addrInfoServices[i];
					m_addrInfoServices[i] = NULL;
				}
				delete[] m_addrInfoServices;
				m_addrInfoServices=NULL;
				m_addrInfoServicesSize = 0;
			}
#endif //ONLY_LOCAL_PROXY


		delete[] m_strCascadeName;
		m_strCascadeName=NULL;

		delete[] m_strLogDir;
		m_strLogDir=NULL;

		delete[] m_strCredential;
		m_strCredential=NULL;

		delete[] m_strLogLevel;
		m_strLogLevel=NULL;

		delete[] m_strPidFile;
		m_strPidFile=NULL;

		delete[] m_strEncryptedLogDir;
		m_strEncryptedLogDir=NULL;

		delete[] m_strUser;
		m_strUser=NULL;

		delete[] m_strMixID;
		m_strMixID=NULL;

		delete[] m_strMixName;
		m_strMixName=NULL;

		clearTargetInterfaces();
		clearListenerInterfaces();
#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX
		if(m_docMixInfo!=NULL)
		{
			m_docMixInfo->release();
			m_docMixInfo=NULL;
		}

		if(m_docMixXml!=NULL)
		{
			m_docMixXml->release();
			m_docMixXml=NULL;
		}
#endif
#ifndef ONLY_LOCAL_PROXY
		clearVisibleAddresses();

		//delete m_pSignKey;
		//m_pSignKey=NULL;

		//delete m_pOwnCertificate;
		//m_pOwnCertificate=NULL;

		delete m_pMultiSignature;
		m_pMultiSignature = NULL;

		delete m_OpCert;
		m_OpCert=NULL;

		delete m_pNextMixCertificate;
		m_pNextMixCertificate=NULL;

		delete m_pPrevMixCertificate;
		m_pPrevMixCertificate=NULL;

		delete m_pLogEncryptionCertificate;
		m_pLogEncryptionCertificate=NULL;

		if(m_docOpTnCs!=NULL)
		{
			m_docOpTnCs->release();
			m_docOpTnCs=NULL;
		}

		if (m_strAccessControlCredential != NULL)
			{
			delete[]m_strAccessControlCredential;
			m_strAccessControlCredential = NULL;
			}

#ifdef COUNTRY_STATS
		delete[] m_dbCountryStatsHost;
		m_dbCountryStatsHost = NULL;
		delete[] m_dbCountryStatsUser;
		m_dbCountryStatsUser = NULL;
		delete[] m_dbCountryStatsPasswd;
		m_dbCountryStatsPasswd = NULL;
#endif

#ifdef DATA_RETENTION_LOG
		delete[] m_strDataRetentionLogDir;
		m_strDataRetentionLogDir=NULL;
#endif

#ifdef EXPORT_ASYM_PRIVATE_KEY
		if(m_strImportKeyFile!=NULL)
			delete[] m_strImportKeyFile;
		m_strImportKeyFile=NULL;
		if(m_strExportKeyFile!=NULL)
			delete[] m_strExportKeyFile;
		m_strExportKeyFile=NULL;
#endif


#endif //ONLY_LOCAL_PROXY
#ifdef SERVER_MONITORING
		if(m_strMonitoringListenerHost != NULL)
		{
			delete[] m_strMonitoringListenerHost;
			m_strMonitoringListenerHost = NULL;
		}
#endif
		delete [] mainOptionSetters;
		mainOptionSetters = NULL;
		delete [] generalOptionSetters;
		generalOptionSetters = NULL;
		delete certificateOptionSetters;
		certificateOptionSetters = NULL;
#ifdef PAYMENT
		delete [] accountingOptionSetters;
		accountingOptionSetters = NULL;
#endif
		delete [] networkOptionSetters;
		networkOptionSetters = NULL;
		delete [] m_arpTermsAndConditionsOptionSetters;
		m_arpTermsAndConditionsOptionSetters=NULL;
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
	char* strCreateConf=NULL;
	char* strCredential = NULL;
#ifdef EXPORT_ASYM_PRIVATE_KEY
	char* strImportKey=NULL;
	char* strExportKey=NULL;
#endif
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
		{"credential",0,POPT_ARG_STRING,&strCredential,0,"credential for connetion to cascade [only for local proxy]","<credential>"},
#ifdef EXPORT_ASYM_PRIVATE_KEY
		{"exportKey",0,POPT_ARG_STRING,&strExportKey,0,"export private encryption key to file","<file>"},
		{"importKey",0,POPT_ARG_STRING,&strImportKey,0,"import private encryption key from file","<file>"},
#endif
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
		
#ifdef MIX_VERSION_TESTING
		CAMsg::printMsg(LOG_WARNING, MIX_VERSION_TESTING_TEXT);
#endif

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
#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX 
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
	if(iDaemon!=0)
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
						char tmpHostname[TMP_BUFF_SIZE];
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

	if(strCredential!=NULL)
			{
					m_strCredential=new char[strlen(strCredential)+1];
					strcpy(m_strCredential,strCredential);
					free(strCredential);
			}

#ifdef EXPORT_ASYM_PRIVATE_KEY
	if(strExportKey!=NULL)
		{
			m_strExportKeyFile=new UINT8[strlen(strExportKey)+1];
			strcpy((char*)m_strExportKeyFile,strExportKey);
			free(strExportKey);
		}
	if(strImportKey!=NULL)
		{
			m_strImportKeyFile=new UINT8[strlen(strImportKey)+1];
			strcpy((char*)m_strImportKeyFile,strImportKey);
			free(strImportKey);
		}
#endif
	if(iCompressedLogs!=0)
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
					strServerHost = NULL;
				}
			free(serverPort);
			if(m_arListenerInterfaces[0]!=0)
				m_cnListenerInterfaces=1;
		}

	m_iSOCKSServerPort=(UINT16)SOCKSport;
#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX
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
#endif
#if !defined ONLY_LOCAL_PROXY 

		/* Try to read InfoService configuration from  external file infoservices.xml */
		XERCES_CPP_NAMESPACE::DOMDocument* infoservices;
		if( readXmlConfiguration(infoservices,(UINT8*)"infoservices.xml") == E_SUCCESS )
		{
			CAMsg::printMsg(LOG_DEBUG, "Will now get InfoServices from infoservices.xml (this overrides the InfoServices from the default config!)\n");
			DOMElement* elemIs=infoservices->getDocumentElement();
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
				m_arTargetInterfaces=new CATargetInterface[m_cnTargets];
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
		if(newOptions.getMaxNrOfUsers()>0)
			m_maxNrOfUsers=newOptions.getMaxNrOfUsers();
		return E_SUCCESS;
}
#endif //ONLY_LOCAL_PROXY

#ifndef ONLY_LOCAL_PROXY
/** Modifies the next mix settings (target interface and certificate) according to
* the specified options object. Target interfaces are only copied if they denote a
* next mix. HTTP and SOCKS proxy settings are ignored.
* @param doc a DOM document containing XML data with the new options
*/
#ifndef DYNAMIC_MIX
SINT32 CACmdLnOptions::setNextMix(XERCES_CPP_NAMESPACE::DOMDocument* doc)
	{
		CAMsg::printMsg(LOG_DEBUG,"setNextMix() - start\n");
		DOMElement* elemRoot = doc->getDocumentElement();

		//getCertificates if given...
		DOMElement* elemSig;
		getDOMChildByName(elemRoot, OPTIONS_NODE_SIGNATURE, elemSig, false);
		//Own Certiticate first
		//nextMixCertificate if given
		DOMElement* elemCert;
		getDOMChildByName(elemSig, OPTIONS_NODE_X509DATA, elemCert,true);
		if(elemSig!=NULL)
				m_pNextMixCertificate = CACertificate::decode(elemCert->getFirstChild(),CERT_X509CERTIFICATE);

		DOMElement* elemOptionsRoot = m_docMixXml->getDocumentElement();
		DOMElement* elemOptionsCerts;
		getDOMChildByName(elemOptionsRoot, OPTIONS_NODE_CERTIFICATE_LIST, elemOptionsCerts, false);
		DOMElement* elemOptionsNextMixCert;

		if(getDOMChildByName(elemOptionsRoot, OPTIONS_NODE_NEXT_MIX_CERTIFICATE, elemOptionsNextMixCert, false) != E_SUCCESS)
		{
				elemOptionsNextMixCert = createDOMElement(m_docMixXml, OPTIONS_NODE_NEXT_MIX_CERTIFICATE);
				elemOptionsCerts->appendChild(elemOptionsNextMixCert);
				elemOptionsNextMixCert->appendChild(m_docMixXml->importNode(elemCert->getFirstChild(),true));
		}
		else
		{
				if(elemOptionsNextMixCert->hasChildNodes())
				{
						elemOptionsNextMixCert->replaceChild(m_docMixXml->importNode(elemCert->getFirstChild(),true),
										elemOptionsNextMixCert->getFirstChild());
				}
				else
				{
						elemOptionsNextMixCert->appendChild(m_docMixXml->importNode(elemCert->getFirstChild(),true));
				}
		}
		CAMsg::printMsg(LOG_DEBUG,"setNextMix() - certificates done\n");
		DOMElement* elemNextMix;
		getDOMChildByName(elemRoot, OPTIONS_NODE_LISTENER_INTERFACE, elemNextMix,true);

		DOMElement* elemOptionsNetwork;
		DOMElement* elemOptionsNextMixInterface;

		if(getDOMChildByName(elemOptionsRoot, OPTIONS_NODE_NETWORK, elemOptionsNetwork, false) != E_SUCCESS)
		{
				elemOptionsNetwork = createDOMElement(m_docMixXml, OPTIONS_NODE_NETWORK);
				elemOptionsRoot->appendChild(elemOptionsNetwork);
		}

		if(getDOMChildByName(elemOptionsNetwork, OPTIONS_NODE_NEXT_MIX, elemOptionsNextMixInterface, false) != E_SUCCESS)
		{
				elemOptionsNextMixInterface = createDOMElement(m_docMixXml, OPTIONS_NODE_NEXT_MIX);
				elemOptionsNetwork->appendChild(elemOptionsNextMixInterface);
		}
		else
		{
				while(elemOptionsNextMixInterface->hasChildNodes())
				{
						elemOptionsNextMixInterface->removeChild(elemOptionsNextMixInterface->getFirstChild());
				}
		}

		DOMNode* interfaceData = elemNextMix->getFirstChild();
		while(interfaceData != NULL)
		{
				elemOptionsNextMixInterface->appendChild(m_docMixXml->importNode(interfaceData,true));
				interfaceData = interfaceData->getNextSibling();
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
	getDOMChildByName(elemRoot,(UINT8*) OPTIONS_NODE_SIGNATURE, elemSig, false);
	DOM_Element elemCert;
	getDOMChildByName(elemSig,(UINT8*) OPTIONS_NODE_X509DATA, elemCert, true);
	if(elemCert!=NULL)
		m_pNextMixCertificate = CACertificate::decode(elemCert.getFirstChild(),CERT_X509CERTIFICATE);
	/** Now import the next mix's network stuff */
	DOM_Node elemNextMix;
	DOM_Element elemListeners;
	// Search through the ListenerInterfaces an use a non-hidden one!
	getDOMChildByName(elemRoot,(UINT8*) OPTIONS_NODE_LISTENER_INTERFACE_LIST, elemListeners, true);
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
	getDOMChildByName(elemNextMix,(UINT8*) OPTIONS_NODE_NETWORK, elemType, false);
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;
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
		getDOMChildByName(elemNextMix,(UINT8*) OPTIONS_NODE_PORT, elemPort, false);
		if(getDOMElementValue(elemPort,&port)!=E_SUCCESS)
			goto SKIP_NEXT_MIX;
		addr=new CASocketAddrINet;
		bool bAddrIsSet=false;
		getDOMChildByName(elemNextMix,(UINT8*) OPTIONS_NODE_HOST, elemHost, false);
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
			getDOMChildByName(elemNextMix,(UINT8*) OPTIONS_NODE_IP, elemIP, false);
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
		getDOMChildByName(elemNextMix,(UINT8*) OPTIONS_NODE_FILE, elemFile, false);
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
			targetInterfaceNextMix = NULL;
		}

	SKIP_NEXT_MIX:
			delete addr;
			addr = NULL;
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
SINT32 CACmdLnOptions::setPrevMix(XERCES_CPP_NAMESPACE::DOMDocument* doc)
{
		CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - start\n");
		DOMElement* elemRoot = doc->getDocumentElement();

		//getCertificates if given...
		DOMElement* elemSig;
		getDOMChildByName(elemRoot, OPTIONS_NODE_SIGNATURE, elemSig, false);
		//Own Certiticate first
		//nextMixCertificate if given
		DOMElement* elemCert;
		getDOMChildByName(elemSig, OPTIONS_NODE_X509DATA, elemCert, true);
		if(elemCert!=NULL)
			{
				CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - elem cert found in data from infoservice\n");
				DOMElement* elemOptionsRoot = m_docMixXml->getDocumentElement();
				CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - got  current options root element\n");
				DOMElement* elemOptionsCerts;
				getDOMChildByName
					(elemOptionsRoot, OPTIONS_NODE_CERTIFICATE_LIST, elemOptionsCerts, false);
				DOMElement* elemOptionsPrevMixCert;

				if(getDOMChildByName
						(elemOptionsRoot,OPTIONS_NODE_PREV_MIX_CERTIFICATE, elemOptionsPrevMixCert, false) != E_SUCCESS)
					{
						CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - no prev cert set at the moment\n");
						elemOptionsPrevMixCert =createDOMElement( m_docMixXml,"PrevMixCertificate");
						elemOptionsCerts->appendChild(elemOptionsPrevMixCert);
						CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - try to import the one we got from infoservice\n");
						getDOMChildByName
							(elemCert, OPTIONS_NODE_X509_CERTIFICATE, elemCert, false);

						CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - Cert to be imported:\n");
						UINT8 buff[8192];
						UINT32 len=8192;
						DOM_Output::dumpToMem(elemCert,buff,&len);
						CAMsg::printMsg(LOG_DEBUG,(char*)buff);

						elemOptionsPrevMixCert->appendChild(m_docMixXml->importNode(elemCert,true));
						CAMsg::printMsg(LOG_DEBUG,"setPrevMix() - MixConf now:\n");
						len=8192;
						DOM_Output::dumpToMem(m_docMixXml,buff,&len);
						buff[len]=0;
						CAMsg::printMsg(LOG_DEBUG,(char*)buff);
					}
				else
				{
						if(elemOptionsPrevMixCert->hasChildNodes())
						{
								elemOptionsPrevMixCert->replaceChild(m_docMixXml->importNode(elemCert->getFirstChild(),true),
												elemOptionsPrevMixCert->getFirstChild());
						}
						else
						{
								elemOptionsPrevMixCert->appendChild(m_docMixXml->importNode(elemCert->getFirstChild(),true));
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
	getDOMChildByName(elemRoot,(UINT8*) OPTIONS_NODE_SIGNATURE, elemSig, false);
	DOM_Element elemCert;
	getDOMChildByName(elemSig,(UINT8*) OPTIONS_NODE_X509DATA, elemCert,true);

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
	getDOMChildByName(elemOptionsRoot, (UINT8*) OPTIONS_NODE_CERTIFICATE_LIST, elemOptionsCerts, false);
	DOM_Element elemTmp;
	// Remove existing certificates
	if(getDOMChildByName(elemOptionsCerts, (UINT8*) OPTIONS_NODE_PREV_MIX_CERTIFICATE, elemTmp, false) == E_SUCCESS)
	{

		elemOptionsCerts.removeChild(elemTmp);
	}
	if(getDOMChildByName(elemOptionsCerts, (UINT8*) OPTIONS_NODE_PREV_OPERATOR_CERTIFICATE, elemTmp, false) == E_SUCCESS)
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
	getDOMChildByName(elemOptionsRoot, (UINT8*) OPTIONS_NODE_CERTIFICATE_LIST, elemOptionsCerts, false);
	DOM_Element elemTmp;
	// Remove existing certificates
	if(getDOMChildByName(elemOptionsCerts, (UINT8*) OPTIONS_NODE_NEXT_MIX_CERTIFICATE, elemTmp, false) == E_SUCCESS)
	{

		elemOptionsCerts.removeChild(elemTmp);
	}
	if(getDOMChildByName(elemOptionsCerts, (UINT8*) OPTIONS_NODE_NEXT_OPERATOR_CERTIFICATE, elemTmp, false) == E_SUCCESS)
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
		XERCES_CPP_NAMESPACE::DOMDocument* docConfig=NULL;
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

UINT32 CACmdLnOptions::getPaymentHardLimit()
{
	return m_iPaymentHardLimit;
}

UINT32 CACmdLnOptions::getPrepaidInterval()
{
	return m_iPrepaidInterval;
}

UINT32 CACmdLnOptions::getPaymentSoftLimit()
{
	return m_iPaymentSoftLimit;
}

UINT32 CACmdLnOptions::getPaymentSettleInterval()
{
	return m_iPaymentSettleInterval;
}

#endif /* ifdef PAYMENT */

#ifndef ONLY_LOCAL_PROXY
SINT32 CACmdLnOptions::getOperatorSubjectKeyIdentifier(UINT8 *buffer, UINT32 *length)
{
	if(m_OpCert == NULL)
	{
		(*length) = 0;
		return E_UNKNOWN;
	}
	return m_OpCert->getSubjectKeyIdentifier(buffer, length);

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
			m_strLogDir = NULL;
		}
		m_strLogDir=new char[len+1];
		memcpy(m_strLogDir,name,len);
		m_strLogDir[len]=0;
		return E_SUCCESS;
	}

SINT32 CACmdLnOptions::getCredential(UINT8* credential,UINT32 len)
	{
		if(m_strCredential==NULL||credential==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(m_strCredential))
				{
					return E_SPACE;
				}
		strcpy((char*)credential,m_strCredential);
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
	{
		return E_UNKNOWN;
	}
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


#ifdef SERVER_MONITORING
char *CACmdLnOptions::getMonitoringListenerHost()
{
	return m_strMonitoringListenerHost;
}

UINT16 CACmdLnOptions::getMonitoringListenerPort()
{
	return m_iMonitoringListenerPort;
}
#endif /* SERVER_MONITORING */

SINT32 CACmdLnOptions::initLogging()
	{
	SINT32 ret = E_SUCCESS;
	UINT8 buff[2000];
	UINT32 iLogOptions = 0;

	CAMsg::init();


#ifndef ONLY_LOCAL_PROXY
	if(isSyslogEnabled())
		{
		iLogOptions |= MSG_LOG; 
		}
#endif
	if(getLogDir((UINT8*)buff,2000)==E_SUCCESS)
		{
		if(getCompressLogs())
			iLogOptions |= MSG_COMPRESSED_FILE;
		else
			iLogOptions |= MSG_FILE;
		}
#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX

	if (m_bLogConsole || iLogOptions == 0)
		{
		iLogOptions |= MSG_STDOUT;
		}
	ret = CAMsg::setLogOptions(iLogOptions);

	if(m_strLogLevel!=NULL)
		{
		if (strcmp(m_strLogLevel,"info") == 0)
			{
			CAMsg::setLogLevel(LOG_INFO);
			}
		else if (strcmp(m_strLogLevel,"warning") == 0)
			{
			CAMsg::setLogLevel(LOG_WARNING);
			}
		else if (strcmp(m_strLogLevel,"error") == 0)
			{
			CAMsg::setLogLevel(LOG_ERR);
			}
		else if (strcmp(m_strLogLevel,"critical") == 0)
			{
			CAMsg::setLogLevel(LOG_CRIT);
			}
		}	
#endif
#if!defined ONLY_LOCAL_PROXY
	if(isEncryptedLogEnabled())
		{
		SINT32 retEncr;
		if ((retEncr = CAMsg::openEncryptedLog()) != E_SUCCESS)
			{
			CAMsg::printMsg(LOG_ERR,"Could not open encrypted log - exiting!\n");
			return retEncr;
			}
		}
#endif

	if(getDaemon() && ret != E_SUCCESS) 
		{
		CAMsg::printMsg(LOG_CRIT, "We need a log file in daemon mode in order to get any messages! Exiting...\n");
		return ret;
		}

	return E_SUCCESS;
	}


#ifndef ONLY_LOCAL_PROXY



UINT32 CACmdLnOptions::getNumberOfTermsAndConditionsTemplates()
{
	return m_nrOfTermsAndConditionsTemplates;
}
XERCES_CPP_NAMESPACE::DOMDocument **CACmdLnOptions::getAllTermsAndConditionsTemplates()
{
	return m_termsAndConditionsTemplates;
}

/* a reference to the Terms and conditions document stored by this class.
 * this method does not return a copy of the doc so don't release it.
 */
XERCES_CPP_NAMESPACE::DOMElement* CACmdLnOptions::getTermsAndConditions()
{
	//DOMElement *docElement = NULL;
	if(m_docOpTnCs == NULL)
	{
		return NULL;
	}
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;
	memset(tmpBuff, 0, tmpLen);
	getOperatorSubjectKeyIdentifier(tmpBuff, &tmpLen);
	setDOMElementAttribute(m_docOpTnCs->getDocumentElement(), OPTIONS_ATTRIBUTE_TNC_ID, tmpBuff);
	return m_docOpTnCs->getDocumentElement();
	//docElement = m_docOpTnCs->getDocumentElement();
	//return (docElement == NULL) ? NULL : getElementsByTagName(docElement, OPTION_NODE_TNCS);
}




SINT32 CACmdLnOptions::setMaxUsers(DOMElement* elemGeneral)
{
	DOMElement* elemMaxUsers=NULL;
	UINT32 tmp = 0;

	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_MAX_USERS);

	// get max users
	getDOMChildByName(elemGeneral, OPTIONS_NODE_MAX_USERS, elemMaxUsers, false);
	if(elemMaxUsers!=NULL)
	{
		if(getDOMElementValue(elemMaxUsers, &tmp)==E_SUCCESS)
		{
			m_maxNrOfUsers = tmp;
		}
	}
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setPaymentReminder(DOMElement* elemGeneral)
{
	DOMElement* elemPaymentReminder=NULL;
	m_PaymentReminderProbability = -1;
		
	if(elemGeneral == NULL) 
		return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT (elemGeneral->getNodeName(), OPTIONS_NODE_PAYMENT_REMINDER);

	// get payment reminder probabilty
	getDOMChildByName(elemGeneral, OPTIONS_NODE_PAYMENT_REMINDER, elemPaymentReminder, false);
	bool bEnabled=false;
	getDOMElementAttribute(elemPaymentReminder, "enable", bEnabled);
	if (!bEnabled) 
		{
			m_PaymentReminderProbability = -1;
		}
	else
		{
			getDOMElementValue(elemPaymentReminder, &m_PaymentReminderProbability);
		}
	return E_SUCCESS;
}


SINT32 CACmdLnOptions::setAccessControlCredential(DOMElement* elemGeneral)
	{
		if (m_strAccessControlCredential != NULL)
			{
				delete[]m_strAccessControlCredential;
				m_strAccessControlCredential = NULL;
			}

		DOMElement* elemCredential = NULL;
		UINT8 tmpBuff[TMP_BUFF_SIZE];
		UINT32 tmpLen = TMP_BUFF_SIZE;

		if (elemGeneral == NULL)
			return E_UNKNOWN;
		ASSERT_GENERAL_OPTIONS_PARENT(elemGeneral->getNodeName(), OPTIONS_NODE_CREDENTIAL);

		//get Accesscontrol credentila
		getDOMChildByName(elemGeneral, OPTIONS_NODE_CREDENTIAL, elemCredential, false);

		if (getDOMElementValue(elemCredential, tmpBuff, &tmpLen) == E_SUCCESS)
			{	
				m_strAccessControlCredential = new UINT8[tmpLen + 1]; 
				memcpy(m_strAccessControlCredential,tmpBuff,tmpLen);
				m_strAccessControlCredential[tmpLen] = 0;
				DOMElement* elemMixType=NULL;
				getDOMChildByName(m_docMixInfo, "MixType", elemMixType, true);
				setDOMElementAttribute(elemMixType, "accessControlled", true);
			}
		return E_SUCCESS;
	}

SINT32 CACmdLnOptions::getAccessControlCredential(UINT8* outbuff, UINT32* inoutsize)
	{
	
	if (outbuff== NULL || inoutsize == NULL || m_strAccessControlCredential==NULL || *inoutsize<=strlen((char*)m_strAccessControlCredential))
		return E_UNKNOWN;
	strcpy((char*)outbuff, (char*)m_strAccessControlCredential);
	*inoutsize = strlen((char*)m_strAccessControlCredential);
	return E_SUCCESS;
	}




// **************************************
// accounting option setter functions *
// *************************************
#ifdef PAYMENT
SINT32 CACmdLnOptions::setAccountingOptions(DOMElement *elemRoot)
{
	// the accoutning options are added by Bastian Voigt
	DOMElement* elemAccounting=NULL;
	if (getDOMChildByName
			(elemRoot, OPTIONS_NODE_ACCOUNTING, elemAccounting, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_ACCOUNTING);
		return E_UNKNOWN;
	}

	return invokeOptionSetters
		(accountingOptionSetters, elemAccounting, ACCOUNTING_OPTIONS_NR);
	return E_SUCCESS;
}


SINT32 CACmdLnOptions::setPriceCertificate(DOMElement *elemAccounting)
{

	DOMElement* elemPriceCert = NULL;

	if(elemAccounting == NULL) return E_UNKNOWN;
	 ASSERT_ACCOUNTING_OPTIONS_PARENT
		(elemAccounting->getNodeName(), OPTIONS_NODE_PRICE_CERTIFICATE);

	//function in CAUtil, last param is "deep", needs to be set to include child elems
	getDOMChildByName
		(elemAccounting, OPTIONS_NODE_PRICE_CERTIFICATE, elemPriceCert, false);
	if (elemPriceCert == NULL)
	{
		CAMsg::printMsg(LOG_CRIT, "Did you really want to compile the mix with payment support?\n");
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_PRICE_CERTIFICATE);
		return E_UNKNOWN;
	}
	else
	{
//		UINT8 digest[SHA_DIGEST_LENGTH];
//		UINT8* out=new UINT8[5000];
//		UINT32 outlen=5000;
//
//		DOM_Output::makeCanonical(elemPriceCert,out,&outlen);
//		out[outlen] = 0;
////#ifdef DEBUG
//		CAMsg::printMsg(LOG_DEBUG, "price cert (%u bytes) to be hashed: %s\n",outlen, out);
////#endif
//		SHA1(out,outlen,digest);
//		delete[] out;
//		out = NULL;
//
//		UINT32 len2 = 1024;
//		UINT8* tmpBuff2 = new UINT8[len2+1];
//		memset(tmpBuff2, 0, len2+1);
//		CABase64::encode(digest,SHA_DIGEST_LENGTH, tmpBuff2, &len2);
//		CAMsg::printMsg(LOG_CRIT,"hash: %s\n", tmpBuff2);
//		exit(0);
		m_pPriceCertificate = CAXMLPriceCert::getInstance(elemPriceCert);
		if (m_pPriceCertificate == NULL) 
		{
			CAMsg::printMsg(LOG_CRIT, "Could not parse price certificate!");
			return E_UNKNOWN;
		}
		m_strAiID = m_pPriceCertificate->getSubjectKeyIdentifier();
		
		if (m_pMultiSignature != NULL && m_pMultiSignature->findSKI(m_strAiID) != E_SUCCESS)
		{
				CAMsg::printMsg(LOG_CRIT,"Your price certificate does not fit to your mix certificate(s). Please import the proper price certificate or mix certificate.\n");
				return E_UNKNOWN;
		}

		if (m_pBI == NULL)
		{
			CAMsg::printMsg(LOG_CRIT,"Could not verify price certificate, as no payment instance was found!\n");
			return E_UNKNOWN;
		}


		if (CAMultiSignature::verifyXML(elemPriceCert, m_pBI->getCertificate()) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT,"Signature of price certificate is invalid! It may be damaged, or maybe you are using the wrong payment instance certificate?\n");
			return E_UNKNOWN;
		}

	}

	//insert price certificate
	return appendMixInfo_internal(elemPriceCert, WITH_SUBTREE);

	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setPaymentInstance(DOMElement *elemAccounting)
{

	DOMElement* elemJPI = NULL;

	if(elemAccounting == NULL) return E_UNKNOWN;
	 ASSERT_ACCOUNTING_OPTIONS_PARENT
		(elemAccounting->getNodeName(), OPTIONS_NODE_PAYMENT_INSTANCE);

	CAMsg::printMsg(LOG_DEBUG, "Parsing JPI values.\n");

	getDOMChildByName(elemAccounting, OPTIONS_NODE_PAYMENT_INSTANCE, elemJPI, false);
	m_pBI = CAXMLBI::getInstance(elemJPI);
	if (m_pBI == NULL)
	{
		CAMsg::printMsg(LOG_CRIT,"Could not instantiate payment instance interface. Did you really want to compile the mix with payment support?\n");
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}


SINT32 CACmdLnOptions::setAccountingSoftLimit(DOMElement *elemAccounting)
{

	DOMElement* elemAISoftLimit = NULL;
	UINT32 tmp = 0;

	if(elemAccounting == NULL) return E_UNKNOWN;
	ASSERT_ACCOUNTING_OPTIONS_PARENT
		(elemAccounting->getNodeName(), OPTIONS_NODE_AI_SOFT_LIMIT);

	if (getDOMChildByName
			(elemAccounting, OPTIONS_NODE_AI_SOFT_LIMIT, elemAISoftLimit, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_AI_SOFT_LIMIT);
		return E_UNKNOWN;
	}
	if(getDOMElementValue(elemAISoftLimit, &tmp)==E_SUCCESS)
	{
		m_iPaymentSoftLimit = tmp;
	}
	else
	{
		//or better set default values?
		LOG_NODE_EMPTY_OR_INVALID(OPTIONS_NODE_AI_SOFT_LIMIT);
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}


SINT32 CACmdLnOptions::setAccountingHardLimit(DOMElement *elemAccounting)
{

	DOMElement* elemAIHardLimit = NULL;
	UINT32 tmp = 0;

	if(elemAccounting == NULL) return E_UNKNOWN;
	ASSERT_ACCOUNTING_OPTIONS_PARENT
		(elemAccounting->getNodeName(), OPTIONS_NODE_AI_HARD_LIMIT);

	if (getDOMChildByName
			(elemAccounting, OPTIONS_NODE_AI_HARD_LIMIT, elemAIHardLimit, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_AI_HARD_LIMIT);
		return E_UNKNOWN;
	}
	if(getDOMElementValue(elemAIHardLimit, &tmp)==E_SUCCESS)
	{
		m_iPaymentHardLimit = tmp;
	}
	else
	{
		//or better set default values?
		LOG_NODE_EMPTY_OR_INVALID(OPTIONS_NODE_AI_HARD_LIMIT);
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}


SINT32 CACmdLnOptions::setPrepaidInterval(DOMElement *elemAccounting)
{

	DOMElement* elemPrepaidIval = NULL;
	UINT32 tmp = 0;

	if(elemAccounting == NULL) return E_UNKNOWN;
	ASSERT_ACCOUNTING_OPTIONS_PARENT
		(elemAccounting->getNodeName(), OPTIONS_NODE_PREPAID_IVAL);

	if (getDOMChildByName
			(elemAccounting, OPTIONS_NODE_PREPAID_IVAL, elemPrepaidIval, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_PREPAID_IVAL);

		if (getDOMChildByName
				(elemAccounting, OPTIONS_NODE_PREPAID_IVAL_KB, elemPrepaidIval, false) != E_SUCCESS)
		{
			LOG_NODE_NOT_FOUND(OPTIONS_NODE_PREPAID_IVAL_KB);
		}
		else
		{
			if(getDOMElementValue(elemPrepaidIval, &tmp)==E_SUCCESS)
			{
				m_iPrepaidInterval = tmp * 1000;
			}
		}
	}
	else if(getDOMElementValue(elemPrepaidIval, &tmp) == E_SUCCESS)
	{
		m_iPrepaidInterval = tmp;
	}
	else
	{
		CAMsg::printMsg(LOG_INFO,"Node \"%s\" is empty! Setting default...\n",
				OPTIONS_NODE_PREPAID_IVAL);
		m_iPrepaidInterval = OPTIONS_DEFAULT_PREPAID_IVAL;
	}
	if (m_iPrepaidInterval > OPTIONS_DEFAULT_PREPAID_IVAL )
	{
		CAMsg::printMsg(LOG_WARNING,"Prepaid interval is higher than %u! "
				"No JAP will pay more in advance!\n", OPTIONS_DEFAULT_PREPAID_IVAL);
	}
	else if (m_iPrepaidInterval < 5000)
	{
		CAMsg::printMsg(LOG_WARNING,"Prepaid interval of %u is far too low! "
				"Performance will be critical and clients will lose connection!\n", m_iPrepaidInterval);
	}

	//insert prepaid interval
	DOMElement* elemInterval = createDOMElement(m_docMixInfo, OPTIONS_NODE_PREPAID_IVAL_KB);
	setDOMElementValue(elemInterval, (m_iPrepaidInterval / 1000) );
	//TODO: handle exceptional cases 
	m_docMixInfo->getDocumentElement()->appendChild(elemInterval);
	return E_SUCCESS;
}


SINT32 CACmdLnOptions::setSettleInterval(DOMElement *elemAccounting)
{

	DOMElement* elemSettleIval = NULL;
	UINT32 tmp = 0;

	if(elemAccounting == NULL) return E_UNKNOWN;
	ASSERT_ACCOUNTING_OPTIONS_PARENT
		(elemAccounting->getNodeName(), OPTIONS_NODE_SETTLE_IVAL);

	if (getDOMChildByName
			(elemAccounting, OPTIONS_NODE_SETTLE_IVAL, elemSettleIval, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_SETTLE_IVAL);
		return E_UNKNOWN;
	}
	if(getDOMElementValue(elemSettleIval, &tmp)==E_SUCCESS)
	{
		m_iPaymentSettleInterval = tmp;
	}
	else
	{
		LOG_NODE_EMPTY_OR_INVALID(OPTIONS_NODE_SETTLE_IVAL);
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}


SINT32 CACmdLnOptions::setAccountingDatabase(DOMElement *elemAccounting)
{

	DOMElement* elem = NULL;
	DOMElement* elemDatabase = NULL;
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE, tmp = 0;

	// DDB is only configured for first payment mix 
	if (!m_bFirstMix)
	{
		return E_SUCCESS;
	}

	if(elemAccounting == NULL) return E_UNKNOWN;
	ASSERT_ACCOUNTING_OPTIONS_PARENT
		(elemAccounting->getNodeName(), OPTIONS_NODE_AI_DB);

	CAMsg::printMsg(LOG_DEBUG, "Parsing AI values.\n");

	if (getDOMChildByName(elemAccounting, OPTIONS_NODE_AI_DB, elemDatabase, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_AI_DB);
		return E_UNKNOWN;
	}

	// get DB Hostname
	if (getDOMChildByName(elemDatabase, OPTIONS_NODE_AI_DB_HOST, elem, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_AI_DB_HOST);
		return E_UNKNOWN;
	}

	if(getDOMElementValue(elem, tmpBuff, &tmpLen) != E_SUCCESS)
	{
		LOG_NODE_EMPTY_OR_INVALID(OPTIONS_NODE_AI_DB_HOST);
		return E_UNKNOWN;
	}
	strtrim(tmpBuff);
	m_strDatabaseHost = new UINT8[strlen((char*)tmpBuff)+1];
	strcpy((char *)m_strDatabaseHost, (char *) tmpBuff);

	// get Database Port
	if (getDOMChildByName
			(elemDatabase, OPTIONS_NODE_AI_DB_PORT, elem, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_AI_DB_PORT);
		return E_UNKNOWN;
	}
	if(getDOMElementValue(elem, &tmp) != E_SUCCESS)
	{
		LOG_NODE_EMPTY_OR_INVALID(OPTIONS_NODE_AI_DB_PORT);
		return E_UNKNOWN;
	}
	m_iDatabasePort = tmp;

	// get DB Name
	if (getDOMChildByName
			(elemDatabase, OPTIONS_NODE_AI_DB_NAME, elem, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_AI_DB_NAME);
		return E_UNKNOWN;
	}

	tmpLen = TMP_BUFF_SIZE;
	memset(tmpBuff, 0, tmpLen);

	if(getDOMElementValue
			(elem, tmpBuff, &tmpLen) != E_SUCCESS)
	{
		LOG_NODE_EMPTY_OR_INVALID(OPTIONS_NODE_AI_DB_NAME);
		return E_UNKNOWN;
	}
	strtrim(tmpBuff);
	m_strDatabaseName = new UINT8[strlen((char*)tmpBuff)+1];
	strcpy((char *)m_strDatabaseName, (char *) tmpBuff);

	// get DB Username
	if (getDOMChildByName
			(elemDatabase, OPTIONS_NODE_AI_DB_USER, elem, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_AI_DB_USER);
		return E_UNKNOWN;
	}

	tmpLen = TMP_BUFF_SIZE;
	memset(tmpBuff, 0, tmpLen);

	if(getDOMElementValue
			(elem, tmpBuff, &tmpLen) != E_SUCCESS)
	{
		LOG_NODE_EMPTY_OR_INVALID(OPTIONS_NODE_AI_DB_USER);
		return E_UNKNOWN;
	}
	strtrim(tmpBuff);
	m_strDatabaseUser = new UINT8[strlen((char*)tmpBuff)+1];
	strcpy((char *)m_strDatabaseUser, (char *) tmpBuff);

	//get DB password from xml
	getDOMChildByName(elemDatabase, OPTIONS_NODE_AI_DB_PASSW, elem, false);

	tmpLen = TMP_BUFF_SIZE;
	memset(tmpBuff, 0, tmpLen);

	//read password from xml if given
	if(getDOMElementValue(elem, tmpBuff, &tmpLen) != E_SUCCESS)
	{
		//read password from stdin:
		UINT8 dbpass[500];
		dbpass[0] = 0;
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
	else
	{
		strtrim(tmpBuff);
		m_strDatabasePassword = new UINT8[strlen((char*)tmpBuff)+1];
		strcpy((char *)m_strDatabasePassword, (char *) tmpBuff);
	}
	CAMsg::printMsg(LOG_DEBUG, "Accounting database information parsed successfully.\n");
	
	// just for testing the connection to the database
	if(CAAccountingDBInterface::init() != E_SUCCESS)
	{
		exit(EXIT_FAILURE);
	}
	CAAccountingDBInterface::cleanup();
	
	return E_SUCCESS;
}
#endif //PAYMENT









// *************************************************
// * terms and condition option setter function(s) *
// *************************************************
SINT32 CACmdLnOptions::setTermsAndConditions(DOMElement *elemRoot)
{
	DOMElement *elemTnCs = NULL;

	if(elemRoot == NULL)
	{
		return E_UNKNOWN;
	}

	getDOMChildByName(elemRoot, OPTIONS_NODE_TNCS_OPTS, elemTnCs, true);
	if(elemTnCs != NULL)
	{
		return invokeOptionSetters
			(m_arpTermsAndConditionsOptionSetters, elemTnCs, TERMS_AND_CONDITIONS_OPTIONS_NR);
	}
	else
	{
		CAMsg::printMsg(LOG_WARNING,"No Terms & Conditions for Operator specified!\n");
		return E_SUCCESS;
	}
}

SINT32 CACmdLnOptions::setTermsAndConditionsTemplates(DOMElement *elemTnCs)
{
	if(elemTnCs == NULL)
	{
		CAMsg::printMsg(LOG_CRIT,"Terms And Conditions root element is null!\n");
		return E_UNKNOWN;
	}
	DOMElement *elemTnCsTemplates = NULL;
	DOMNodeList *templateList = NULL;
	bool nothingFound = true;
	getDOMChildByName(elemTnCs, OPTIONS_NODE_TNCS_TEMPLATES, elemTnCsTemplates);

	UINT8** loadedTemplateRefIds = NULL;
	bool templateError = false;

	if(elemTnCsTemplates != NULL)
	{
		templateList = getElementsByTagName(elemTnCsTemplates, OPTIONS_NODE_TNCS_TEMPLATE);
		if(templateList->getLength() > 0)
		{
			nothingFound = false;
			m_nrOfTermsAndConditionsTemplates = templateList->getLength();
			m_termsAndConditionsTemplates = new XERCES_CPP_NAMESPACE::DOMDocument*[m_nrOfTermsAndConditionsTemplates];
			loadedTemplateRefIds = new UINT8*[m_nrOfTermsAndConditionsTemplates];
			memset(loadedTemplateRefIds, 0, (sizeof(UINT8*)*m_nrOfTermsAndConditionsTemplates) );

			UINT8 currentTemplateURL[TMP_BUFF_SIZE];
			UINT32 len = TMP_BUFF_SIZE;
			memset(currentTemplateURL, 0, len);

			for (XMLSize_t i = 0; i < templateList->getLength(); i++)
			{
				getDOMElementValue(templateList->item(i), currentTemplateURL, &len);
				m_termsAndConditionsTemplates[i] = parseDOMDocument(currentTemplateURL);
				if(m_termsAndConditionsTemplates[i] == NULL)
				{
					CAMsg::printMsg(LOG_WARNING, "Cannot load Terms And Conditions template '%s'.\n",
							currentTemplateURL);
					return E_UNKNOWN;
				}
				UINT8* refId = getTermsAndConditionsTemplateRefId(m_termsAndConditionsTemplates[i]->getDocumentElement());
				if(refId != NULL)
				{
					loadedTemplateRefIds[i] = refId;
					for(XMLSize_t j = 0; j < i; j++)
					{
						if(strncmp((char *)refId, (char *) loadedTemplateRefIds[j], TEMPLATE_REFID_MAXLEN) == 0 )
						{
							templateError = true;
							CAMsg::printMsg(LOG_ERR, "duplicate Terms And Conditions template '%s'.\n",refId);
							break;
						}
					}
				}
				else
				{
					templateError = true;
					CAMsg::printMsg(LOG_ERR, "Terms And Conditions template with invalid refid found.\n");
					break;
				}

				if(!templateError)
				{
					CAMsg::printMsg(LOG_INFO, "loaded Terms And Conditions template '%s'.\n",refId);
				}
				else
				{
					break;
				}
				len = TMP_BUFF_SIZE;
			}
		}
		if(loadedTemplateRefIds != NULL)
		{
			for(XMLSize_t j = 0; j < m_nrOfTermsAndConditionsTemplates; j++)
			{
				delete [] loadedTemplateRefIds[j];
				loadedTemplateRefIds[j] = NULL;
			}
			delete [] loadedTemplateRefIds;
			loadedTemplateRefIds = NULL;
		}
		if(templateError)
		{
			return E_UNKNOWN;
		}
	}

	if(nothingFound)
	{
		CAMsg::printMsg(LOG_INFO,"No Terms And Conditions templates found.\n");
	}

	return E_SUCCESS;
}
SINT32 CACmdLnOptions::setTermsAndConditionsList(DOMElement *elemTnCs)
{
	if(elemTnCs == NULL)
	{
		CAMsg::printMsg(LOG_CRIT,"Terms And Conditions root element is null!\n");
		return E_UNKNOWN;
	}
	DOMElement *elemTnCsList = NULL;
	getDOMChildByName(elemTnCs, OPTIONS_NODE_TNCS, elemTnCsList);

	if(elemTnCsList == NULL)
	{
		CAMsg::printMsg(LOG_CRIT,"No definitions for Terms And Conditions found!\n");
		return E_UNKNOWN;
	}

	UINT32 attrCheckLen = TMP_BUFF_SIZE;
	UINT8 attrCheck[TMP_BUFF_SIZE];
	memset(attrCheck, 0, attrCheckLen);

	UINT32 localeLen = TMP_LOCALE_SIZE;
	UINT8 locale[TMP_LOCALE_SIZE];
	memset(locale, 0, localeLen);

	UINT32 dateLen = TMP_DATE_SIZE;
	UINT8 date[TMP_DATE_SIZE];
	memset(date, 0, dateLen);

	if( (getDOMElementAttribute(elemTnCsList, OPTIONS_ATTRIBUTE_TNC_DATE, date, &dateLen) != E_SUCCESS) ||
		(strlen((char *)date) != ((TMP_DATE_SIZE) - 1) ) )
	{
		CAMsg::printMsg(LOG_CRIT,"Attribute '%s' is not properly set for the global definition of Terms And Conditions!\n",
				OPTIONS_ATTRIBUTE_TNC_DATE);
		return E_UNKNOWN;
	}

	m_docOpTnCs = createDOMDocument();

	DOMElement *currentTnCEntry = NULL;
	DOMNodeList *tncDefEntryList = getElementsByTagName(elemTnCsList, OPTIONS_NODE_TNCS_TRANSLATION);

	if(tncDefEntryList->getLength() < 1)
	{
		CAMsg::printMsg(LOG_CRIT,"No Terms And Conditions entries found!\n");
		return E_UNKNOWN;
	}

	DOMElement *tncTranslationImports = NULL;
	DOMElement *tncOperatorNode = NULL;
	getDOMChildByName(elemTnCsList, OPTIONS_NODE_TNCS_TRANSLATION_IMPORTS, tncTranslationImports, false);
	if(tncTranslationImports != NULL)
	{
		getDOMChildByName(tncTranslationImports, OPTIONS_NODE_TNCS_OPERATOR, tncOperatorNode, false);
	}
	bool defaultLangValue = false;
	bool defaultLangFound = false;
	bool operatorImportNodeFound = (tncOperatorNode != NULL);

	// validity check for every definition: are all necessary attributes set (referenceId, locale), length ok
	// * and is there EXACTLY ONE default language specified?
	//
	for (XMLSize_t j = 0; j < tncDefEntryList->getLength(); j++)
	{
		attrCheckLen = TMP_BUFF_SIZE;
		localeLen = TMP_LOCALE_SIZE;
		defaultLangValue = false;
		currentTnCEntry = (DOMElement *) tncDefEntryList->item(j);

		if( (getDOMElementAttribute(currentTnCEntry, OPTIONS_ATTRIBUTE_TNC_TEMPLATE_REFID, attrCheck, &attrCheckLen) != E_SUCCESS) ||
				(strlen((char *)attrCheck) < 1)  )
		{
			CAMsg::printMsg(LOG_CRIT,"Attribute '%s' is not proper set for definition %u of Terms And Conditions!\n",
					OPTIONS_ATTRIBUTE_TNC_TEMPLATE_REFID, (j+1));
			return E_UNKNOWN;
		}
		else if( (getDOMElementAttribute(currentTnCEntry, OPTIONS_ATTRIBUTE_TNC_LOCALE, locale, &localeLen) != E_SUCCESS) ||
				(strlen((char *)locale) != ((TMP_LOCALE_SIZE) - 1) ) )
		{
			CAMsg::printMsg(LOG_CRIT,"Attribute '%s' is not proper set for definition %u of Terms And Conditions!\n",
					OPTIONS_ATTRIBUTE_TNC_LOCALE, (j+1));
			return E_UNKNOWN;
		}

		if(!operatorImportNodeFound)
		{
			tncOperatorNode = NULL;
			getDOMChildByName(currentTnCEntry, OPTIONS_NODE_TNCS_OPERATOR, tncOperatorNode, false);
			if(tncOperatorNode == NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"No Node '%s' defined for the translation [%s]. Either define it in '%s' or"
						" in this %s.\n", OPTIONS_NODE_TNCS_OPERATOR, locale, OPTIONS_NODE_TNCS_TRANSLATION_IMPORTS,
						OPTIONS_NODE_TNCS_TRANSLATION);
				return E_UNKNOWN;
			}
		}

		//setDOMElementAttribute(currentTnCEntry, OPTIONS_ATTRIBUTE_TNC_DATE, date);
		getDOMElementAttribute(currentTnCEntry,
				OPTIONS_ATTRIBUTE_TNC_DEFAULT_LANG_DEFINED, defaultLangValue);

		if(defaultLangValue && defaultLangFound)
		{
			CAMsg::printMsg(LOG_CRIT,"exactly ONE default language must be specified for the Terms And Conditions!\n");
			return E_UNKNOWN;
		}

		//import nodes global for all translations
		if(tncTranslationImports != NULL)
		{
			if(integrateDOMNode(tncTranslationImports, currentTnCEntry, true, false) != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Integrating imports failed!\n");
				return E_UNKNOWN;
			}
		}
		defaultLangFound = (defaultLangFound || defaultLangValue);
	}

	if(!defaultLangFound)
	{
		CAMsg::printMsg(LOG_CRIT,"There is no default language specified for the Terms And Conditions!\n");
		return E_UNKNOWN;
	}
	if(tncTranslationImports != NULL)
	{
		elemTnCsList->removeChild(tncTranslationImports);
	}
	m_docOpTnCs->appendChild(m_docOpTnCs->importNode(elemTnCsList, WITH_SUBTREE));

	return E_SUCCESS;
}

// *******************************************
// * crime detection option setter functions *
// *******************************************
#ifdef LOG_CRIME
SINT32 CACmdLnOptions::setCrimeDetectionOptions(DOMElement *elemRoot)
{
	DOMElement* elemCrimeDetection = NULL;
	if (getDOMChildByName
			(elemRoot, OPTIONS_NODE_CRIME_DETECTION, elemCrimeDetection, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_CRIME_DETECTION);
		return E_SUCCESS;
	}

	CAMsg::printMsg(LOG_INFO,"Loading Crime Detection Data....\n");

	if(elemCrimeDetection != NULL)
	{
		if( getDOMElementAttribute(elemCrimeDetection,
				OPTIONS_ATTRIBUTE_LOG_PAYLOAD, m_logPayload) != E_SUCCESS)
		{
			m_logPayload = false;
		}
		return invokeOptionSetters
				(crimeDetectionOptionSetters, elemCrimeDetection, CRIME_DETECTION_OPTIONS_NR);
	}
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setCrimeURLRegExp(DOMElement *elemCrimeDetection)
{

	if(elemCrimeDetection == NULL) return E_UNKNOWN;
	ASSERT_CRIME_DETECTION_OPTIONS_PARENT
		(elemCrimeDetection->getNodeName(), OPTIONS_NODE_CRIME_REGEXP_URL);

	return setRegExpressions(elemCrimeDetection, OPTIONS_NODE_CRIME_REGEXP_URL,
				&m_arCrimeRegExpsURL, &m_nCrimeRegExpsURL);

	///DOM_NodeList nlRegExp = elemCrimeDetection.getElementsByTagName("RegExpURL");
	//m_arCrimeRegExpsURL=new regex_t[nlRegExp.getLength()];
	//for(UINT32 i=0;i<nlRegExp.getLength();i++)
	//{
	//	DOM_Node tmpChild=nlRegExp.item(i);
	//	UINT32 lenRegExp=4096;
	//	UINT8 buffRegExp[4096];
	//	if(getDOMElementValue(tmpChild,buffRegExp,&lenRegExp)==E_SUCCESS)
	//	{
	//		if(regcomp(&m_arCrimeRegExpsURL[m_nCrimeRegExpsURL],(char*)buffRegExp,REG_EXTENDED|REG_ICASE|REG_NOSUB)!=0)
	//		{
	//			CAMsg::printMsg(LOG_CRIT,"Could not compile URL regexp: %s\n",buffRegExp);
	//			exit(-1);
	//		}
	//		CAMsg::printMsg(LOG_DEBUG,"Looking for crime URL RegExp: %s\n",buffRegExp);

	//		m_nCrimeRegExpsURL++;
	//	}
	//}

	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setCrimePayloadRegExp(DOMElement *elemCrimeDetection)
{

	if(elemCrimeDetection == NULL) return E_UNKNOWN;
	ASSERT_CRIME_DETECTION_OPTIONS_PARENT
		(elemCrimeDetection->getNodeName(), OPTIONS_NODE_CRIME_REGEXP_PAYLOAD);

	return setRegExpressions(elemCrimeDetection, OPTIONS_NODE_CRIME_REGEXP_PAYLOAD,
			&m_arCrimeRegExpsPayload, &m_nCrimeRegExpsPayload);

	///DOMNodeList *nlRegExp =
	//	getElementsByTagName(elemCrimeDetection, OPTIONS_NODE_CRIME_REGEXP_PAYLOAD);

	//if(nlRegExp != NULL)
	//{
	//	m_arCrimeRegExpsPayload = new regex_t[nlRegExp->getLength()];
	//	for(UINT32 i = 0; i < nlRegExp->getLength(); i++)
	//	{
	//		DOMNode *tmpChild = nlRegExp->item(i);

	//		UINT32 lenRegExp = REGEXP_BUFF_SIZE;
	//		UINT8 buffRegExp[REGEXP_BUFF_SIZE];

	//		if(getDOMElementValue(tmpChild, buffRegExp, &lenRegExp)==E_SUCCESS)
	//		{
	//			if(regcomp(&m_arCrimeRegExpsPayload[m_nCrimeRegExpsPayload],(char*)buffRegExp,REG_EXTENDED|REG_ICASE|REG_NOSUB)!=0)
	//			{
	//				CAMsg::printMsg(LOG_CRIT,"Could not compile payload regexp: %s\n",buffRegExp);
	//				exit(-1);
	//			}
	//			CAMsg::printMsg(LOG_DEBUG,"Looking for crime Payload RegExp: %s\n",buffRegExp);

	//			m_nCrimeRegExpsPayload++;
	//		}
	//	}
	//}
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setCrimeSurveillanceIP(DOMElement *elemCrimeDetection)
{
	if(elemCrimeDetection == NULL) return E_UNKNOWN;
	ASSERT_CRIME_DETECTION_OPTIONS_PARENT	(elemCrimeDetection->getNodeName(), OPTIONS_NODE_CRIME_SURVEILLANCE_IP);

	UINT8 ipBuff[TMP_BUFF_SIZE];
	
	DOMNodeList *surveillanceIPNodes =getElementsByTagName(elemCrimeDetection, OPTIONS_NODE_CRIME_SURVEILLANCE_IP);
	m_nrOfSurveillanceIPs = (UINT32) surveillanceIPNodes->getLength();

	if (m_nrOfSurveillanceIPs == 0)
	{
		CAMsg::printMsg(LOG_INFO,"No surveillance IP specified.\n");
		return E_SUCCESS;
	}

	m_surveillanceIPs = new CAIPAddrWithNetmask[m_nrOfSurveillanceIPs];
	for (UINT32 i = 0; i < m_nrOfSurveillanceIPs; i++)
	{
		UINT32 ipBuffSize = TMP_BUFF_SIZE;
		DOMNode* pelemCurrentIP=surveillanceIPNodes->item(i);
		if(getDOMElementValue(pelemCurrentIP, ipBuff,&ipBuffSize) == E_SUCCESS )
		{
			m_surveillanceIPs[i].setAddr(ipBuff);
			ipBuffSize = TMP_BUFF_SIZE;
			if(getDOMElementAttribute(pelemCurrentIP,OPTIONS_NODE_CRIME_SURVEILLANCE_IP_NETMASK,ipBuff,&ipBuffSize)==E_SUCCESS)
				{
					m_surveillanceIPs[i].setNetmask(ipBuff);
				}
			ipBuffSize = TMP_BUFF_SIZE;
			m_surveillanceIPs[i].toString(ipBuff,&ipBuffSize);
			CAMsg::printMsg(LOG_INFO,"Found Surveillance IP %s\n", ipBuff);
		}
		else
		{
			CAMsg::printMsg(LOG_INFO,"Could not read surveillance IP!\n");
			delete[] m_surveillanceIPs;
			m_surveillanceIPs = NULL;
			m_nrOfSurveillanceIPs = 0;
			return E_UNKNOWN;
		}
	}

	
	return E_SUCCESS;
}


SINT32 CACmdLnOptions::setCrimeSurveillanceAccounts(DOMElement *elemCrimeDetection)
{

	if(elemCrimeDetection == NULL) return E_UNKNOWN;
	
	ASSERT_CRIME_DETECTION_OPTIONS_PARENT
		(elemCrimeDetection->getNodeName(), OPTIONS_NODE_CRIME_SURVEILLANCE_ACCOUNT);

		
	UINT64 accountNumber;

	DOMNodeList *surveillanceIPNodes =getElementsByTagName(elemCrimeDetection, OPTIONS_NODE_CRIME_SURVEILLANCE_ACCOUNT);
	m_nrOfSurveillanceAccounts = (UINT32) surveillanceIPNodes->getLength();
	
	if (m_nrOfSurveillanceAccounts == 0)
	{
		CAMsg::printMsg(LOG_INFO,"No surveillance accounts specified.\n");
		return E_SUCCESS;
	}

	DOMNode* node=NULL;
	m_surveillanceAccounts = new UINT64[m_nrOfSurveillanceAccounts];
	for (UINT32 i = 0; i < m_nrOfSurveillanceAccounts; i++)
	{
		node = surveillanceIPNodes->item(i);
		if(getDOMElementValue(node, accountNumber) == E_SUCCESS)
		{
			m_surveillanceAccounts[i] = accountNumber;
			CAMsg::printMsg(LOG_INFO,"Found surveillance account %llu.\n", accountNumber);
		}
		else
		{
			CAMsg::printMsg(LOG_INFO,"Could not read surveillance account number!\n");
			delete[] m_surveillanceAccounts;
			m_surveillanceAccounts = NULL;
			m_nrOfSurveillanceAccounts = 0;
			return E_UNKNOWN;
		}
	}
	
	
	
	return E_SUCCESS;
}

SINT32 setRegExpressions(DOMElement *rootElement, const char* const childElementName,
		tre_regex_t **regExContainer, UINT32* regExNr)
{
	if( (rootElement == NULL) || (childElementName == NULL) ||
		(regExNr == NULL) || (regExContainer == NULL) )
	{
		return E_UNKNOWN;
	}

	(*regExNr) = 0;

	DOMNodeList *nlRegExp =
			getElementsByTagName(rootElement, childElementName);

	if(nlRegExp != NULL)
	{
		(*regExContainer) = new tre_regex_t[nlRegExp->getLength()];

		for(UINT32 i = 0; i < nlRegExp->getLength(); i++)
		{
			DOMNode *tmpChild = nlRegExp->item(i);

			UINT32 lenRegExp = REGEXP_BUFF_SIZE;
			UINT8 buffRegExp[REGEXP_BUFF_SIZE];

			if(getDOMElementValue(tmpChild, buffRegExp, &lenRegExp)==E_SUCCESS)
			{
				if(tre_regcomp( &((*regExContainer)[(*regExNr)]),
							 ((char*) buffRegExp),
							 REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0 )
				{
					CAMsg::printMsg(LOG_CRIT,"Could not compile regexp: %s\n",buffRegExp);
					return E_UNKNOWN;
				}
				CAMsg::printMsg(LOG_DEBUG,"Looking for RegExp: %s\n",buffRegExp);
				(*regExNr)++;
			}
		}
	}
	return E_SUCCESS;
}
#endif //LOG_CRIME




#endif //ONLY_LOCAL_PROXY

#ifndef ONLY_LOCAL_PROXY





/** Builds a default Configuration
	* @param strFileName filename of the file in which the default configuration is stored, if NULL stdout is used
	*/
SINT32 CACmdLnOptions::createMixOnCDConfiguration(const UINT8* strFileName)
{
	XERCES_CPP_NAMESPACE::DOMDocument* doc = createDOMDocument();
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
SINT32 CACmdLnOptions::buildDefaultConfig(XERCES_CPP_NAMESPACE::DOMDocument* doc,bool bForLastMix=false)
{
		CASignature* pSignature=new CASignature();
		pSignature->generateSignKey(1024);
		DOMElement* elemRoot=createDOMElement(doc,"MixConfiguration");
		doc->appendChild(elemRoot);
		setDOMElementAttribute(elemRoot,"version",(UINT8*)"0.5");
		DOMElement* elemGeneral=createDOMElement(doc,"General");
		elemRoot->appendChild(elemGeneral);

		/** @todo MixType can be chosen randomly between FirstMix and MiddleMix but not LastMix!
		*sk13: ok this is a hack - but this way it can also create configurations for LastMixes which makes testing of the dynamic szenario much easier...
		*/
		DOMElement* elemTmp=createDOMElement(doc,"MixType");
		if(bForLastMix)
			setDOMElementValue(elemTmp,(UINT8*)"LastMix");
		else
			setDOMElementValue(elemTmp,(UINT8*)"FirstMix");
		elemGeneral->appendChild(elemTmp);

		/** MixID must be the SubjectKeyIdentifier of the mix' certificate */
		elemTmp=createDOMElement(doc,"MixID");
		CACertificate* pCert;
		pSignature->getVerifyKey(&pCert);
		UINT8 buf[255];
		UINT32 len = 255;
		pCert->getSubjectKeyIdentifier( buf, &len);
		setDOMElementValue(elemTmp,buf);
		elemGeneral->appendChild(elemTmp);
		elemTmp=createDOMElement(doc,"Dynamic");
		setDOMElementValue(elemTmp,(UINT8*)"True");
		elemGeneral->appendChild(elemTmp);

		elemTmp=createDOMElement(doc,"Daemon");
		setDOMElementValue(elemTmp,(UINT8*)"True");
		elemGeneral->appendChild(elemTmp);

		elemTmp=createDOMElement(doc,"CascadeName");
		setDOMElementValue(elemTmp,(UINT8*)"Dynamic Cascade");
		elemGeneral->appendChild(elemTmp);
		elemTmp=createDOMElement(doc,"MixName");
		setDOMElementValue(elemTmp,(UINT8*)"Dynamic Mix");
		elemGeneral->appendChild(elemTmp);
		elemTmp=createDOMElement(doc,"UserID");
		setDOMElementValue(elemTmp,(UINT8*)"mix");
		elemGeneral->appendChild(elemTmp);
		DOMElement* elemLogging=createDOMElement(doc,"Logging");
		elemGeneral->appendChild(elemLogging);
		elemTmp=createDOMElement(doc,"SysLog");
		setDOMElementValue(elemTmp,(UINT8*)"True");
		elemLogging->appendChild(elemTmp);
		DOMElement* elemNet=createDOMElement(doc,"Network");
		elemRoot->appendChild(elemNet);

		/** @todo Add a list of default InfoServices to the default configuration */
		DOMElement*elemISs=createDOMElement(doc,"InfoServices");
		elemNet->appendChild(elemISs);
		elemTmp=createDOMElement(doc,"AllowAutoConfiguration");
		setDOMElementValue(elemTmp,(UINT8*)"True");
		elemISs->appendChild(elemTmp);

		DOMElement* elemIS=createDOMElement(doc,"InfoService");
		elemISs->appendChild(elemIS);
		DOMElement* elemISListeners=createDOMElement(doc,"ListenerInterfaces");
		elemIS->appendChild(elemISListeners);
		DOMElement* elemISLi=createDOMElement(doc,"ListenerInterface");
		elemISListeners->appendChild(elemISLi);
		elemTmp=createDOMElement(doc,"Host");
		setDOMElementValue(elemTmp,(UINT8*)DEFAULT_INFOSERVICE);
		elemISLi->appendChild(elemTmp);
		elemTmp=createDOMElement(doc,"Port");
		setDOMElementValue(elemTmp,6543U);
		elemISLi->appendChild(elemTmp);
		elemTmp=createDOMElement(doc,"AllowAutoConfiguration");
		setDOMElementValue(elemTmp,(UINT8*)"True");
		elemISs->appendChild(elemTmp);

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
				DOMElement* elemProxies=createDOMElement(doc,"Proxies");
				DOMElement* elemProxy=createDOMElement(doc,"Proxy");
				elemProxies->appendChild(elemProxy);
				elemTmp=createDOMElement(doc,"ProxyType");
				setDOMElementValue(elemTmp,(UINT8*)"HTTP");
				elemProxy->appendChild(elemTmp);
				elemTmp=createDOMElement(doc,"Host");
				setDOMElementValue(elemTmp,(UINT8*)"127.0.0.1");
				elemProxy->appendChild(elemTmp);
				elemTmp=createDOMElement(doc,"Port");
				setDOMElementValue(elemTmp,3128U);
				elemProxy->appendChild(elemTmp);
				elemTmp=createDOMElement(doc,"NetworkProtocol");
				setDOMElementValue(elemTmp,(UINT8*)"RAW/TCP");
				elemProxy->appendChild(elemTmp);
				elemNet->appendChild(elemProxies);
			}
		DOMElement* elemCerts=createDOMElement(doc,"Certificates");
		elemRoot->appendChild(elemCerts);
		DOMElement* elemOwnCert=createDOMElement(doc,"OwnCertificate");
		elemCerts->appendChild(elemOwnCert);
		DOMElement* tmpElemSigKey=NULL;
		pSignature->getSignKey(tmpElemSigKey,doc);
		elemOwnCert->appendChild(tmpElemSigKey);

		DOMElement* elemTmpCert=NULL;
		pCert->encode(elemTmpCert,doc);
		elemOwnCert->appendChild(elemTmpCert);

		/** @todo Add Description section because InfoService doesn't accept MixInfos without Location or Operator */
		delete pCert;
		pCert = NULL;
		delete pSignature;
		pSignature = NULL;
		return E_SUCCESS;
}

/**
	* Saves the given XML Document to a file
	* @param p_doc The XML Document to be saved
	* @param p_strFileName The name of the file to be saved to
	* @retval E_SUCCESS
	*/
SINT32 CACmdLnOptions::saveToFile(XERCES_CPP_NAMESPACE::DOMDocument* p_doc, const UINT8* p_strFileName)
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
		buff = NULL;
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
		dyn = NULL;
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

#ifdef COUNTRY_STATS
SINT32 CACmdLnOptions::getCountryStatsDBConnectionLoginData(char** db_host,char**db_user,char**db_passwd)
	{
		*db_host=*db_user=*db_passwd=NULL;
		if(m_dbCountryStatsHost!=NULL)
			{
				*db_host=new char[strlen(m_dbCountryStatsHost)+1];
				strcpy(*db_host,m_dbCountryStatsHost);
			}
		if(m_dbCountryStatsUser!=NULL)
			{
				*db_user=new char[strlen(m_dbCountryStatsUser)+1];
				strcpy(*db_user,m_dbCountryStatsUser);
			}
		if(m_dbCountryStatsPasswd!=NULL)
			{
				*db_passwd=new char[strlen(m_dbCountryStatsPasswd)+1];
				strcpy(*db_passwd,m_dbCountryStatsPasswd);
			}
		return E_SUCCESS;
	}
#endif

#ifdef DATA_RETENTION_LOG
SINT32 CACmdLnOptions::getDataRetentionLogDir(UINT8* strLogDir,UINT32 len)
	{
		if(strLogDir==NULL||m_strDataRetentionLogDir==NULL)
			return E_UNKNOWN;
		if(len<=(UINT32)strlen((char*)m_strDataRetentionLogDir))
				{
					return E_UNKNOWN;
				}
		strcpy((char*)strLogDir,(char*)m_strDataRetentionLogDir);
		return E_SUCCESS;
	}
#endif// DATA_RETENTION_LOG

#endif //ONLY_LOCAL_PROXY

#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX
/** Returns the XML tree describing the Mix . This is NOT a copy!
	* @param docMixInfo destination for the XML tree
	*	@retval E_SUCCESS if it was successful
	* @retval E_UNKNOWN in case of an error
*/
SINT32 CACmdLnOptions::getMixXml(XERCES_CPP_NAMESPACE::DOMDocument* & docMixInfo)
{
	if(m_docMixInfo == NULL)
	{
		CAMsg::printMsg(LOG_CRIT,"No mixinfo document initialized!\n");
		return E_UNKNOWN;
	}
	docMixInfo=m_docMixInfo;
	//insert (or update) the Timestamp
	DOMElement* elemTimeStamp=NULL;
	DOMElement* elemRoot=docMixInfo->getDocumentElement();
	if(getDOMChildByName(elemRoot, UNIVERSAL_NODE_LAST_UPDATE, elemTimeStamp, false)!=E_SUCCESS)
	{
		elemTimeStamp=createDOMElement(docMixInfo, UNIVERSAL_NODE_LAST_UPDATE);
		elemRoot->appendChild(elemTimeStamp);
	}
	UINT64 currentMillis;
	getcurrentTimeMillis(currentMillis);
	UINT8 tmpStrCurrentMillis[50];
	print64(tmpStrCurrentMillis,currentMillis);
	setDOMElementValue(elemTimeStamp,tmpStrCurrentMillis);
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::getCascadeName(UINT8* name,UINT32 len) const
	{
		if(m_strCascadeName==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen((char*)m_strCascadeName))
				{
					return E_UNKNOWN;
				}
		strcpy((char*)name,(char*)m_strCascadeName);
		return E_SUCCESS;
	}

CAListenerInterface** CACmdLnOptions::getInfoServices(UINT32& r_size)
 {
		r_size = m_addrInfoServicesSize;
		return m_addrInfoServices;
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
SINT32 CACmdLnOptions::readXmlConfiguration(XERCES_CPP_NAMESPACE::DOMDocument* & docConfig,const UINT8* const configFile)
	{
		int handle;
		handle=open((char*)configFile,O_BINARY|O_RDONLY);
		if(handle==-1)
			return E_FILE_OPEN;
		SINT32 len=filesize32(handle);
		UINT8* tmpChar=new UINT8[len];
		int ret=read(handle,tmpChar,len);
		close(handle);
		if(ret!=len)
			return E_FILE_READ;
		SINT32 retVal = readXmlConfiguration(docConfig, tmpChar, len);
		delete[] tmpChar;
		tmpChar = NULL;
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
SINT32 CACmdLnOptions::readXmlConfiguration(XERCES_CPP_NAMESPACE::DOMDocument* & docConfig,const UINT8* const buf, UINT32 len)
{
		docConfig=parseDOMDocument(buf,len);
		if(docConfig==NULL)
		{
			CAMsg::printMsg(LOG_CRIT, "Your configuration is not a valid XML document and therefore could not be parsed. Please repair the configuration structure or create a new configuration.\n");
			return E_UNKNOWN;
		}
		return E_SUCCESS;
}

// ** Processes a XML configuration document. This sets the values of the
//	* options to the values found in the XML document.
//	* Note that only the values are changed, which are given in the XML document!
//	* @param docConfig the configuration as XML document
//	* @retval E_UNKNOWN if an error occurs
//	* @retval E_SUCCESS otherwise
//	
SINT32 CACmdLnOptions::processXmlConfiguration(XERCES_CPP_NAMESPACE::DOMDocument* docConfig)
{
	SINT32 ret = E_SUCCESS;
	if(docConfig==NULL)
	{
		return E_UNKNOWN;
	}
	DOMElement* elemRoot=docConfig->getDocumentElement();

	// * Initialize Mixinfo DOM structure so that neccessary
	// * option can be appended to it.
	// 
	DOMElement* elemMix=createDOMElement(m_docMixInfo, MIXINFO_NODE_PARENT);
	m_docMixInfo->appendChild(elemMix);

	// * invoke all main option setters
	// * which then invoke their own specific
	// * option setters
	// 
	ret = invokeOptionSetters(mainOptionSetters, elemRoot, MAIN_OPTION_SETTERS_NR);
	if(ret != E_SUCCESS)
	{
		return E_UNKNOWN;
	}

	//Set Software-Version...
	DOMElement* elemSoftware=createDOMElement(m_docMixInfo, MIXINFO_NODE_SOFTWARE);
	DOMElement* elemVersion=createDOMElement(m_docMixInfo, MIXINFO_NODE_VERSION);
	setDOMElementValue(elemVersion,(UINT8*)MIX_VERSION);
	elemSoftware->appendChild(elemVersion);
	elemMix->appendChild(elemSoftware);

#ifdef PAYMENT
	// Add the payment reminder 
	DOMElement* elemPaymentReminder=createDOMElement(m_docMixInfo, MIXINFO_NODE_PAYMENTREMINDER);
	setDOMElementValue(elemPaymentReminder, m_PaymentReminderProbability);
	elemMix->appendChild(elemPaymentReminder);
#endif
#ifdef COUNTRY_STATS
		DOMElement* elemCountryStats=NULL;
		getDOMChildByName(elemRoot,"CountryStatsDB",elemCountryStats,false);
		UINT8 db_tmp_buff[4096];
		UINT32 db_tmp_buff_len=4096;
		if(getDOMElementAttribute(elemCountryStats,"host",db_tmp_buff,&db_tmp_buff_len)==E_SUCCESS)
			{
				m_dbCountryStatsHost=new char[db_tmp_buff_len+1];
				memcpy(m_dbCountryStatsHost,db_tmp_buff,db_tmp_buff_len);
				m_dbCountryStatsHost[db_tmp_buff_len]=0;
			}
		db_tmp_buff_len=4096;
		if(getDOMElementAttribute(elemCountryStats,"user",db_tmp_buff,&db_tmp_buff_len)==E_SUCCESS)
			{
				m_dbCountryStatsUser=new char[db_tmp_buff_len+1];
				memcpy(m_dbCountryStatsUser,db_tmp_buff,db_tmp_buff_len);
				m_dbCountryStatsUser[db_tmp_buff_len]=0;
			}
		db_tmp_buff_len=4096;
		if(getDOMElementAttribute(elemCountryStats,"passwd",db_tmp_buff,&db_tmp_buff_len)==E_SUCCESS)
			{
				m_dbCountryStatsPasswd=new char[db_tmp_buff_len+1];
				memcpy(m_dbCountryStatsPasswd,db_tmp_buff,db_tmp_buff_len);
				m_dbCountryStatsPasswd[db_tmp_buff_len]=0;
			}
#endif

		DOMElement* elemCascade;
		SINT32 haveCascade = getDOMChildByName(elemRoot,"MixCascade",elemCascade,false);

#ifndef DYNAMIC_MIX
		// LERNGRUPPE: This is no error in the fully dynamic model 
		if(isLastMix() && haveCascade != E_SUCCESS && !hasPrevMixTestCertificate() && !verifyMixCertificates())
		{
				CAMsg::printMsg(LOG_CRIT,"Error in configuration: You must either specify cascade info or the previous mix's certificate.\n");
				return E_UNKNOWN;
		}
#endif
#ifndef ONLY_LOCAL_PROXY
		if(isLastMix() && haveCascade == E_SUCCESS)
		{
				getDOMChildByName(elemRoot,"MixCascade",m_pCascadeXML,false);

				DOMNodeList* nl = getElementsByTagName(m_pCascadeXML,"Mix");
				UINT16 len = (UINT16)nl->getLength();
				if(len == 0)
				{
						CAMsg::printMsg(LOG_CRIT,"Error in configuration: Empty cascade specified.\n");
						return E_UNKNOWN;
				}
		}
#endif
#ifdef DATA_RETENTION_LOG
		DOMElement* elemDataRetention=NULL;
		getDOMChildByName(elemRoot,"DataRetention",elemDataRetention,false);
		DOMElement* elemDataRetentionLogDir=NULL;
		getDOMChildByName(elemDataRetention,"LogDir",elemDataRetentionLogDir,false);
		UINT8 log_dir[4096];
		UINT32 log_dir_len=4096;
		if(getDOMElementValue(elemDataRetentionLogDir,log_dir,&log_dir_len)==E_SUCCESS)
			{
				m_strDataRetentionLogDir=new UINT8[log_dir_len+1];
				memcpy(m_strDataRetentionLogDir,log_dir,log_dir_len);
				m_strDataRetentionLogDir[log_dir_len]=0;
			}
		CAMsg::printMsg(LOG_CRIT,"Data retention log dir in config file: %s\n",log_dir);

		this->m_pDataRetentionPublicEncryptionKey=new CAASymCipher();
		DOMElement* elemDataRetentionPublicKey=NULL;
		getDOMChildByName(elemDataRetention,"PublicEncryptionKey",elemDataRetentionPublicKey,false);
		DOMElement* elemDataRetentionPublicRSAKey=NULL;
		getDOMChildByName(elemDataRetentionPublicKey,"RSAKeyValue",elemDataRetentionPublicRSAKey,false);
		m_pDataRetentionPublicEncryptionKey->setPublicKeyAsDOMNode(elemDataRetentionPublicRSAKey);

		//Add info to MixInfo structure...
		elemDataRetention=createDOMElement(m_docMixInfo, "DataRetention");
		elemMix->appendChild(elemDataRetention);
		DOMElement* elemLoggedElements=createDOMElement(m_docMixInfo,"LoggedElements");
		elemDataRetention->appendChild(elemLoggedElements);
		DOMElement* elemTemp=createDOMElement(m_docMixInfo,"InputTime");
		elemLoggedElements->appendChild(elemTemp);
		setDOMElementValue(elemTemp,true);
		elemTemp=createDOMElement(m_docMixInfo,"OutputTime");
		elemLoggedElements->appendChild(elemTemp);
		setDOMElementValue(elemTemp,true);
		elemTemp=createDOMElement(m_docMixInfo,"InputChannelID");
		elemLoggedElements->appendChild(elemTemp);
		setDOMElementValue(elemTemp,true);
		elemTemp=createDOMElement(m_docMixInfo,"OutputChannelID");
		elemLoggedElements->appendChild(elemTemp);
		setDOMElementValue(elemTemp,true);
		elemTemp=createDOMElement(m_docMixInfo,"InputSourceIPAddress");
		elemLoggedElements->appendChild(elemTemp);
		setDOMElementValue(elemTemp,true);
		elemTemp=createDOMElement(m_docMixInfo,"OutputSourceIPAddress");
		elemLoggedElements->appendChild(elemTemp);
		setDOMElementValue(elemTemp,true);
		elemTemp=createDOMElement(m_docMixInfo,"InputSourceIPPort");
		elemLoggedElements->appendChild(elemTemp);
		setDOMElementValue(elemTemp,true);
		elemTemp=createDOMElement(m_docMixInfo,"OutputSourceIPPort");
		elemLoggedElements->appendChild(elemTemp);
		setDOMElementValue(elemTemp,true);
		elemTemp=createDOMElement(m_docMixInfo,"RetentionPeriod");
		elemDataRetention->appendChild(elemTemp);
		setDOMElementValue(elemTemp,(UINT8*)"P6M");
#endif //DATA_RETENTION_LOG

		return E_SUCCESS;
}

/**
 * framework-function for calling predefined option setter functions.
 * Used by functions that handle a certain type of options, i.e.
 * general settings, account setting, etc.
 */
SINT32 CACmdLnOptions::invokeOptionSetters (const optionSetter_pt *optionsSetters, DOMElement* optionsSource, SINT32 optionsSettersLength)
{
	SINT32 i = 0;
	SINT32 ret = E_SUCCESS;

	if( optionsSetters == NULL )
	{
		CAMsg::printMsg(LOG_CRIT,"Error parsing config file: OptionSetters not initialized!\n");
		return E_UNKNOWN;
	}

	if( optionsSettersLength < 0)
	{
		CAMsg::printMsg(LOG_CRIT,"Error parsing config file: Negative number of option setters specified!\n");
		return E_UNKNOWN;
	}

	/* Only warn when we have a null DOM Element */
	if( optionsSource == NULL )
	{
		CAMsg::printMsg(LOG_INFO, "Found NULL DOM element. "
				"NULL element handling is delegated to the specified setter method!\n");
	}

	for(i=0; i < optionsSettersLength; i++ )
		{
			if(optionsSetters[i]!=NULL)
				{
					ret = (this->*(optionsSetters[i]))(optionsSource);
					if(ret != E_SUCCESS)
						{
							return ret;
						}
				}
		}
	return E_SUCCESS;
}

void CACmdLnOptions::initMainOptionSetters()
{
	mainOptionSetters = new optionSetter_pt[MAIN_OPTION_SETTERS_NR];
	memset(mainOptionSetters,0,sizeof(optionSetter_pt)*MAIN_OPTION_SETTERS_NR);
	int count = -1;

	mainOptionSetters[++count]=
		&CACmdLnOptions::setGeneralOptions;
	mainOptionSetters[++count]=
		&CACmdLnOptions::setMixDescription;
	mainOptionSetters[++count]=
		&CACmdLnOptions::setCertificateOptions;
#ifdef PAYMENT
	mainOptionSetters[++count]=
		&CACmdLnOptions::setAccountingOptions;
#endif
	mainOptionSetters[++count]=
		&CACmdLnOptions::setNetworkOptions;
	mainOptionSetters[++count]=
		&CACmdLnOptions::setRessourceOptions;
#ifdef PAYMENT
	mainOptionSetters[++count]=
		&CACmdLnOptions::setTermsAndConditions;
#endif
#ifdef LOG_CRIME
	mainOptionSetters[++count]=
		&CACmdLnOptions::setCrimeDetectionOptions;
#endif
}

void CACmdLnOptions::initGeneralOptionSetters()
{
	generalOptionSetters = new optionSetter_pt[GENERAL_OPTIONS_NR];
	memset(generalOptionSetters,0,sizeof(optionSetter_pt)*GENERAL_OPTIONS_NR);
	int count = -1;

	generalOptionSetters[++count]=
		&CACmdLnOptions::setDaemonMode;
	generalOptionSetters[++count]=
		&CACmdLnOptions::setNrOfFileDescriptors;		
	generalOptionSetters[++count]=
		&CACmdLnOptions::setUserID;
	generalOptionSetters[++count]=
		&CACmdLnOptions::setLoggingOptions;
	generalOptionSetters[++count]=
		&CACmdLnOptions::setMixType;
	generalOptionSetters[++count]=
		&CACmdLnOptions::setMixName;
	generalOptionSetters[++count]=
		&CACmdLnOptions::setMixID;
	generalOptionSetters[++count]=
		&CACmdLnOptions::setDynamicMix;
	generalOptionSetters[++count]=
		&CACmdLnOptions::setMinCascadeLength;
	generalOptionSetters[++count]=
		&CACmdLnOptions::setCascadeNameFromOptions;
#if !defined ONLY_LOCAL_PROXY 
	generalOptionSetters[++count]=
		&CACmdLnOptions::setMaxUsers;
	generalOptionSetters[++count]=
		&CACmdLnOptions::setPaymentReminder;
	generalOptionSetters[++count] =
		&CACmdLnOptions::setAccessControlCredential;
#endif
	}

void CACmdLnOptions::initCertificateOptionSetters()
{
	certificateOptionSetters = new optionSetter_pt[MAX_CERTIFICATE_OPTIONS_NR];
	m_nCertificateOptionsSetters = 0;

	certificateOptionSetters[m_nCertificateOptionsSetters++]=&CACmdLnOptions::setOwnOperatorCertificate;
	certificateOptionSetters[m_nCertificateOptionsSetters++]=&CACmdLnOptions::setOwnCertificate;
	certificateOptionSetters[m_nCertificateOptionsSetters++]=&CACmdLnOptions::setNextMixCertificate;
	certificateOptionSetters[m_nCertificateOptionsSetters++]=&CACmdLnOptions::setPrevMixCertificate;
}

#ifdef PAYMENT
void CACmdLnOptions::initAccountingOptionSetters()
{
	accountingOptionSetters = new optionSetter_pt[ACCOUNTING_OPTIONS_NR];
	int count = -1;

	accountingOptionSetters[++count]=
		&CACmdLnOptions::setPaymentInstance;
	accountingOptionSetters[++count]=
			&CACmdLnOptions::setPriceCertificate;
	accountingOptionSetters[++count]=
		&CACmdLnOptions::setAccountingSoftLimit;
	accountingOptionSetters[++count]=
		&CACmdLnOptions::setAccountingHardLimit;
	accountingOptionSetters[++count]=
		&CACmdLnOptions::setPrepaidInterval;
	accountingOptionSetters[++count]=
		&CACmdLnOptions::setSettleInterval;
	accountingOptionSetters[++count]=
		&CACmdLnOptions::setAccountingDatabase;
}
#endif
void CACmdLnOptions::initNetworkOptionSetters()
{
	networkOptionSetters = new optionSetter_pt[NETWORK_OPTIONS_NR];
	int count = -1;

	networkOptionSetters[++count]=
		&CACmdLnOptions::setInfoServices;
	networkOptionSetters[++count]=
		&CACmdLnOptions::setListenerInterfaces;
	networkOptionSetters[++count]=
		&CACmdLnOptions::setTargetInterfaces;
	networkOptionSetters[++count]=
		&CACmdLnOptions::setServerMonitoring;
	networkOptionSetters[++count]=
		&CACmdLnOptions::setKeepAliveTraffic;
}

/** This method sets the proxy or next mix settings.

**/
SINT32 CACmdLnOptions::setTargetInterfaces(DOMElement *elemNetwork)
{
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;
	DOMElement* elemNextMix = NULL;
	DOMElement* elemProxies=NULL;
	CATargetInterface* targetInterfaceNextMix = NULL;
	//get TargetInterfaces
	m_cnTargets=0;

	if(elemNetwork == NULL) return E_UNKNOWN;
	ASSERT_NETWORK_OPTIONS_PARENT(elemNetwork->getNodeName(), OPTIONS_NODE_NEXT_MIX);

	//NextMix --> only one!!
	getDOMChildByName(elemNetwork, OPTIONS_NODE_NEXT_MIX, elemNextMix, false);
	if(elemNextMix != NULL)
	{
		NetworkType type=RAW_TCP;
		CASocketAddr* addr = NULL;
		DOMElement* elemType = NULL;
		getDOMChildByName(elemNextMix, OPTIONS_NODE_NETWORK_PROTOCOL, elemType, false);

		bool bAddrIsSet = false;

		if(getDOMElementValue(elemType, tmpBuff, &tmpLen) == E_SUCCESS)
		{
			strtrim(tmpBuff);
			if(strcmp((char*)tmpBuff, "RAW/TCP") == 0)
			{
				type=RAW_TCP;
			}
			else if(strcmp((char*)tmpBuff, "RAW/UNIX") == 0)
			{
				type=RAW_UNIX;
			}
			else if(strcmp((char*)tmpBuff, "SSL/TCP") == 0)
			{
				type=SSL_TCP;
			}
			else if(strcmp((char*)tmpBuff, "SSL/UNIX") == 0)
			{
				type=SSL_UNIX;
			}

			if( (type == SSL_TCP) || (type == RAW_TCP) )
			{
				DOMElement* elemPort = NULL;
				DOMElement* elemHost = NULL;
					DOMElement* elemIP = NULL;
					UINT8 buffHost[TMP_BUFF_SIZE];
					UINT32 buffHostLen = TMP_BUFF_SIZE;
				UINT16 port;
				getDOMChildByName
					(elemNextMix, OPTIONS_NODE_PORT, elemPort, false);
				if(getDOMElementValue(elemPort,&port) == E_SUCCESS)
				{
					addr = new CASocketAddrINet;
					//bool bAddrIsSet=false;
					getDOMChildByName
						(elemNextMix, OPTIONS_NODE_HOST, elemHost, false);
					// The rules for <Host> and <IP> are as follows:
					//	* 1. if <Host> is given and not empty take the <Host> value for the address of the next mix; if not go to 2
					//	* 2. if <IP> if given and not empty take <IP> value for the address of the next mix; if not goto 3.
					//	* 3. this entry for the next mix is invalid!
					if(elemHost != NULL)
					{
						if(getDOMElementValue(elemHost,buffHost,&buffHostLen)==E_SUCCESS &&
								((CASocketAddrINet*)addr)->setAddr(buffHost,port)==E_SUCCESS)
						{
							bAddrIsSet = true;
						}
					}
					if(!bAddrIsSet)//now try <IP>
					{
						getDOMChildByName(elemNextMix, OPTIONS_NODE_IP, elemIP, false);
						if(elemIP == NULL || getDOMElementValue(elemIP,buffHost,&buffHostLen) == E_SUCCESS)
						{
							((CASocketAddrINet*)addr)->setAddr(buffHost,port);
							bAddrIsSet = true;
						}
					}
						CAMsg::printMsg(LOG_INFO, "Setting target interface: %s:%d\n", buffHost, port);
				}
			}
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
			else if( (type == SSL_UNIX) || (type == RAW_UNIX) )
			{
				DOMElement* elemFile=NULL;
				getDOMChildByName(elemNextMix, OPTIONS_NODE_FILE, elemFile, false);
				tmpLen = TMP_BUFF_SIZE;
				if(getDOMElementValue(elemFile, tmpBuff, &tmpLen) == E_SUCCESS)
				{
					tmpBuff[tmpLen]=0;
					strtrim(tmpBuff);
					addr=new CASocketAddrUnix;
					if(((CASocketAddrUnix*)addr)->setPath((char*)tmpBuff) == E_SUCCESS)
					{
						bAddrIsSet = true;
					}
				}
			}
#endif
		}

		if(bAddrIsSet)
		{
			targetInterfaceNextMix=new CATargetInterface(TARGET_MIX,type,addr->clone());
			m_cnTargets=1;
		}

		delete addr;
		addr = NULL;
	}

	//Next Proxies and visible adresses
	SINT32 ret;
	UINT8 buff[255];
	UINT32 buffLen = 255;
	CASocket* tmpSocket;

	clearVisibleAddresses();
	getDOMChildByName(elemNetwork, OPTIONS_NODE_PROXY_LIST, elemProxies, false);
	if(elemProxies != NULL)
	{
		DOMNodeList* nlTargetInterfaces=NULL;
		nlTargetInterfaces=getElementsByTagName(elemProxies, OPTIONS_NODE_PROXY);
		m_cnTargets+=nlTargetInterfaces->getLength();

		if(nlTargetInterfaces->getLength()>0)
		{
			m_arTargetInterfaces=new CATargetInterface[m_cnTargets];
			UINT32 aktInterface=0;
			NetworkType type=UNKNOWN_NETWORKTYPE;
			TargetType proxy_type=TARGET_UNKNOWN;
			CASocketAddr* addr=NULL;
			UINT16 port;
			bool bHttpProxyFound = false;
			for(UINT32 i=0; i < nlTargetInterfaces->getLength(); i++)
			{
				delete addr;
				addr=NULL;
				DOMNode* elemTargetInterface=NULL;
				elemTargetInterface=nlTargetInterfaces->item(i);
				DOMElement* elemType;
				getDOMChildByName	(elemTargetInterface, OPTIONS_NODE_NETWORK_PROTOCOL, elemType,false);
				tmpLen = TMP_BUFF_SIZE;
				if(getDOMElementValue(elemType,tmpBuff,&tmpLen)!=E_SUCCESS)
					continue;
				strtrim(tmpBuff);
				if(strcmp((char*)tmpBuff,"RAW/TCP") == 0)
				{
					type=RAW_TCP;
				}
				else if(strcmp((char*)tmpBuff,"RAW/UNIX") == 0)
				{
					type=RAW_UNIX;
				}
				else if(strcmp((char*)tmpBuff,"SSL/TCP") == 0)
				{
					type=SSL_TCP;
				}
				else if(strcmp((char*)tmpBuff,"SSL/UNIX") == 0)
				{
					type=SSL_UNIX;
				}
				else
				{
					continue;
				}
				//ProxyType
				elemType=NULL;
				getDOMChildByName	(elemTargetInterface, OPTIONS_NODE_PROXY_TYPE, elemType, false);
				tmpLen = TMP_BUFF_SIZE;
				if(getDOMElementValue(elemType,tmpBuff,&tmpLen)!=E_SUCCESS)
					continue;
				strtrim(tmpBuff);
				if(strcmp((char*)tmpBuff,"SOCKS")==0)
				{
					proxy_type=TARGET_SOCKS_PROXY;
				}
				else if(strcmp((char*)tmpBuff,"HTTP")==0)
				{
					proxy_type=TARGET_HTTP_PROXY;
				}
				else if (strcmp((char*)tmpBuff, "VPN") == 0)
				{
					proxy_type = TARGET_VPN_PROXY;
				}
				else
				{
					continue;
				}

				if( (type==SSL_TCP) || (type == RAW_TCP) )
				{
					DOMElement* elemPort;
					DOMElement* elemHost;
					getDOMChildByName
						(elemTargetInterface, OPTIONS_NODE_PORT, elemPort, false);
					if(getDOMElementValue(elemPort,&port)!=E_SUCCESS)
					{
						continue;
					}
					addr=new CASocketAddrINet;
					getDOMChildByName
						(elemTargetInterface, OPTIONS_NODE_HOST, elemHost, false);
					if(elemHost != NULL)
					{
						UINT8 buffHost[TMP_BUFF_SIZE];
						UINT32 buffHostLen = TMP_BUFF_SIZE;
						if(getDOMElementValue(elemHost, buffHost, &buffHostLen) != E_SUCCESS)
						{
							continue;
						}
						if(((CASocketAddrINet*)addr)->setAddr(buffHost, port) != E_SUCCESS)
						{
							continue;
						}
					}
					else
					{
						continue;
					}
				}
				else
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
				{
					DOMElement* elemFile;
					getDOMChildByName
						(elemTargetInterface, OPTIONS_NODE_FILE, elemFile, false);
					tmpLen = TMP_BUFF_SIZE;
					if(getDOMElementValue(elemFile, tmpBuff, &tmpLen) != E_SUCCESS)
					{
						continue;
					}
					tmpBuff[tmpLen]=0;
					strtrim(tmpBuff);
					addr=new CASocketAddrUnix;
					if(((CASocketAddrUnix*)addr)->setPath((char*)tmpBuff) != E_SUCCESS)
					{
						continue;
					}
				}
#else
					continue;
#endif



				// check connection to proxy
				tmpSocket = new CASocket();
				tmpSocket->setRecvBuff(50000);
				tmpSocket->setSendBuff(5000);
				ret = tmpSocket->connect(*addr,LAST_MIX_TO_PROXY_CONNECT_TIMEOUT);
				if (ret != E_SUCCESS)
				{
					if (addr->toString(buff, buffLen) != E_SUCCESS)
					{
						buff[0] = 0;
					}
					if (ret != E_UNKNOWN)
					{
						CAMsg::printMsg(LOG_WARNING, "Could not connect to proxy %s! Reason: %s (%i) Please check if the proxy is running.\n",
								buff, GET_NET_ERROR_STR(GET_NET_ERROR), GET_NET_ERROR);
					}
					else
					{
						CAMsg::printMsg(LOG_WARNING, "Could not connect to proxy %s! Please check if the proxy is running.\n", buff);
					}
				}

				else //if (ret == E_SUCCESS)
				{
					if (proxy_type == TARGET_HTTP_PROXY)
					{
						// TODO: we should not send an HTTP request to a non-existing address, for example to test:9999; if squid runs, we will get an HTTP error response
						//if(tmpSocket->sendTimeOut(pMixPacket->payload.data,payLen,LAST_MIX_TO_PROXY_SEND_TIMEOUT)==SOCKET_ERROR) ...
						// else ... tmpSocket->receive(pMixPacket->payload.data,PAYLOAD_SIZE);
						// if we get no response or an invalid response, we set ret = E_UNKNOWN

						bHttpProxyFound = true;
					}
					else if (proxy_type == TARGET_SOCKS_PROXY)
					{
						// TODO maybe there is also a possibility to check the response of the SOCKS proxy?
						m_bSocksSupport = true;
					}
					else if (proxy_type == TARGET_VPN_PROXY)
						{
							m_bVPNSupport = true;
						}
				}

				tmpSocket->close();
				delete tmpSocket;


				if (ret == E_SUCCESS)
				{
					addVisibleAddresses(elemTargetInterface);
					m_arTargetInterfaces[aktInterface].set(proxy_type,type,addr->clone());
					aktInterface++;
				}


				delete addr;
				addr=NULL;
			}

			if (!bHttpProxyFound&&!m_bVPNSupport)
			{
				CAMsg::printMsg(LOG_CRIT, "No valid HTTP or VPN proxy was specified! Please install and configure an HTTP or VPN  proxy like Squid or the ANONVPN proxy before starting the mix.\n");
				for (UINT32 i = 0; i < aktInterface; i++)
				{
					m_arTargetInterfaces[aktInterface].cleanAddr();
				}


				return E_UNKNOWN;
			}

			m_cnTargets=aktInterface;
		}
	} //end if elemProxies!=null
	//add NextMixInterface to the End of the List...
	if(targetInterfaceNextMix != NULL)
	{
		if(m_arTargetInterfaces == NULL)
		{
			m_cnTargets=0;
			m_arTargetInterfaces=new CATargetInterface[1];
		}
		m_arTargetInterfaces[m_cnTargets++].set(targetInterfaceNextMix);
		delete targetInterfaceNextMix;
		targetInterfaceNextMix = NULL;
	}
	else if(m_arTargetInterfaces == NULL)
	{
		CAMsg::printMsg(LOG_CRIT, "Neither proxy nor next mix target interfaces are specified!\n");
		return E_UNKNOWN;
	}

	//Set Proxy Visible Addresses if Last Mix and given
	if(isLastMix() && (m_docMixInfo != NULL) )
	{
		DOMElement* elemMix = m_docMixInfo->getDocumentElement();
		if(elemMix != NULL)
		{
			DOMElement* elemProxies=createDOMElement(m_docMixInfo,"Proxies");
			if (m_bSocksSupport)
			{
				setDOMElementAttribute(elemProxies, "socks5Support", (UINT8*)"true");
			}
			if (m_bVPNSupport)
			{
				setDOMElementAttribute(elemProxies, "vpnSupport", (UINT8*)"true");
			}
			DOMElement* elemProxy=createDOMElement(m_docMixInfo,"Proxy");
			DOMElement* elemVisAddresses=createDOMElement(m_docMixInfo,"VisibleAddresses");
			elemMix->appendChild(elemProxies);
			elemProxies->appendChild(elemProxy);
			elemProxy->appendChild(elemVisAddresses);
			for(UINT32 i=1;i<=getVisibleAddressesCount();i++)
			{
				UINT8 tmp[255];
				UINT32 tmplen=255;
				if(getVisibleAddress(tmp,tmplen,i)==E_SUCCESS)
				{
					DOMElement* elemVisAddress=createDOMElement(m_docMixInfo,"VisibleAddress");
					DOMElement* elemHost=createDOMElement(m_docMixInfo,"Host");
					elemVisAddress->appendChild(elemHost);
					setDOMElementValue(elemHost,tmp);
					elemVisAddresses->appendChild(elemVisAddress);
				}
			}
		}
	}

	return E_SUCCESS;
}

// Deletes all information about the visible addresses.
	//
SINT32 CACmdLnOptions::clearVisibleAddresses()
	{
		if(m_arStrVisibleAddresses!=NULL)
		{
			for(UINT32 i=0;i<m_cnVisibleAddresses;i++)
			{
				delete[] m_arStrVisibleAddresses[i];
				m_arStrVisibleAddresses[i] = NULL;
			}
			delete[] m_arStrVisibleAddresses;
		}
		m_cnVisibleAddresses=0;
		m_arStrVisibleAddresses=NULL;
		return E_SUCCESS;
	}

/// ** Add all the visible addresses to the list of visible addresses found in the XML description of the \<Proxy\> element given.
//	* The structur is as follows:
//	*@verbatim
//	* <Proxy>
//	*      <VisibleAddresses> <!-- Describes the visible addresses from the 'outside world' -->
// *       <VisibleAddress>
//	*        <Host> <!-- Host or IP -->
//	 *       </Host>
//	 *     </VisibleAddress>
//	 *   </VisibleAddresses>
// *
//	* </Proxy>
//	@endverbatim
//	
SINT32 CACmdLnOptions::addVisibleAddresses(DOMNode* nodeProxy)
	{
		if(nodeProxy==NULL) return E_UNKNOWN;
		ASSERT_PARENT_NODE_NAME
			(nodeProxy->getNodeName(), OPTIONS_NODE_PROXY, OPTIONS_NODE_VISIBLE_ADDRESS_LIST);
		DOMNode* elemVisAdresses=NULL;
		getDOMChildByName(nodeProxy, OPTIONS_NODE_VISIBLE_ADDRESS_LIST, elemVisAdresses);
		DOMNode* elemVisAddress=NULL;
		getDOMChildByName(elemVisAdresses, OPTIONS_NODE_VISIBLE_ADDRESS ,elemVisAddress);
		while(elemVisAddress!=NULL)
			{
				if(equals(elemVisAddress->getNodeName(), OPTIONS_NODE_VISIBLE_ADDRESS))
					{
						DOMElement* elemHost=NULL;
						if(getDOMChildByName(elemVisAddress, OPTIONS_NODE_HOST ,elemHost)==E_SUCCESS)
							{
								UINT8 tmp[TMP_BUFF_SIZE];
								UINT32 len = TMP_BUFF_SIZE;
								if(getDOMElementValue(elemHost,tmp,&len)==E_SUCCESS)
									{//append the new address to the list of addresses
										UINT8** tmpAr=new UINT8*[m_cnVisibleAddresses+1];
										if(m_arStrVisibleAddresses!=NULL)
											{
												memcpy(tmpAr,m_arStrVisibleAddresses,m_cnVisibleAddresses*sizeof(UINT8*));
												delete[] m_arStrVisibleAddresses;
												m_arStrVisibleAddresses = NULL;
											}
										tmpAr[m_cnVisibleAddresses]=new UINT8[len+1];
										memcpy(tmpAr[m_cnVisibleAddresses],tmp,len+1);
										m_cnVisibleAddresses++;
										m_arStrVisibleAddresses=tmpAr;
									}
							}
					}
				elemVisAddress=elemVisAddress->getNextSibling();
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

/* append the mix description to the mix info DOM structure
 * this is a main option (child of <MixConfiguration>)
 */
SINT32  CACmdLnOptions::setMixDescription(DOMElement* elemRoot)
{
	SINT32 ret = E_SUCCESS;
	DOMElement* elemMixDescription = NULL;
	if(elemRoot == NULL)
	{
		return E_UNKNOWN;
	}
	ret = getDOMChildByName
			(elemRoot, OPTIONS_NODE_DESCRIPTION, elemMixDescription, false);

	if(elemMixDescription != NULL )
	{
		DOMNode* tmpChild = elemMixDescription->getFirstChild();
		while( (tmpChild != NULL) && (ret == E_SUCCESS) )
		{
			ret = appendMixInfo_internal(tmpChild, WITH_SUBTREE);
			tmpChild=tmpChild->getNextSibling();
		}
	}
	return ret;
}


// ***************************************
// * certificate option setter functions *
// ***************************************
SINT32 CACmdLnOptions::setCertificateOptions(DOMElement* elemRoot)
{

	DOMElement* elemCertificates;

	if (getDOMChildByName
			(elemRoot, OPTIONS_NODE_CERTIFICATE_LIST, elemCertificates, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_CERTIFICATE_LIST);
		return E_UNKNOWN;
	}

	return invokeOptionSetters(certificateOptionSetters, elemCertificates, m_nCertificateOptionsSetters);
}

// ***********************************
// network option setter functions *
// ***********************************
SINT32 CACmdLnOptions::setNetworkOptions(DOMElement *elemRoot)
{
	DOMElement* elemNetwork = NULL;
	if (getDOMChildByName
			(elemRoot, OPTIONS_NODE_NETWORK, elemNetwork, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_NETWORK);
		return E_UNKNOWN;
	}

	return invokeOptionSetters
		(networkOptionSetters, elemNetwork, NETWORK_OPTIONS_NR);
}

SINT32 CACmdLnOptions::setOwnCertificate(DOMElement *elemCertificates)
{
	DOMElement* elemOwnCert=NULL;
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;

	UINT8 passwd[500];
	passwd[0] = 0;

	if(elemCertificates == NULL) return E_UNKNOWN;
	ASSERT_CERTIFICATES_OPTIONS_PARENT
		(elemCertificates->getNodeName(), OPTIONS_NODE_OWN_CERTIFICATE);

	//Own Certiticate first
	getDOMChildByName(elemCertificates, OPTIONS_NODE_OWN_CERTIFICATE, elemOwnCert, false);
	if (elemOwnCert == NULL)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_OWN_CERTIFICATE);
		return E_UNKNOWN;
	}

	/// m_pSignKey = new CASignature();

	//if(m_pSignKey->setSignKey
	//		(elemOwnCert->getFirstChild(), SIGKEY_PKCS12) != E_SUCCESS)
	//{
	//	//Maybe not an empty passwd
	//	printf("I need a passwd for the SignKey: ");
	//	fflush(stdout);
	//	readPasswd(passwd, 500);
	//	if(m_pSignKey->setSignKey
	//			(elemOwnCert->getFirstChild(),
	//			 SIGKEY_PKCS12,(char*)passwd) != E_SUCCESS)
	//	{
	//		CAMsg::printMsg(LOG_CRIT,"Could not read own signature key!\n");
	//		delete m_pSignKey;
	//		m_pSignKey=NULL;
	//	}
	//}

	/// m_pOwnCertificate =
	//	CACertificate::decode(elemOwnCert->getFirstChild(), CERT_PKCS12, (char*)passwd);
	//if (m_pOwnCertificate == NULL)
	//{
	//	CAMsg::printMsg(LOG_CRIT, "Could not decode mix certificate!\n");
	//	return E_UNKNOWN;
	//}

	// new
	//m_ownCertsLength = 0;
	//m_opCertsLength = 0;

	//decode OpCerts
	UINT32 opCertsLen = m_opCertList->getLength();
	CACertificate** opCerts=new CACertificate*[opCertsLen];
	for(UINT32 j=0; j<opCertsLen; j++)
	{
		DOMNode* a_opCert = m_opCertList->item(j);
		opCerts[j] = CACertificate::decode(a_opCert,CERT_X509CERTIFICATE);
		if(opCerts[j] == NULL)
		{
			CAMsg::printMsg(LOG_CRIT, "Error while decoding operator certificates!");
			delete[] opCerts; 
			return E_UNKNOWN;
		}
	}

	DOMNodeList* ownCertList = getElementsByTagName(elemOwnCert, OPTIONS_NODE_X509_PKCS12);

	m_pMultiSignature = new CAMultiSignature();
	for (UINT32 i=0; i<ownCertList->getLength(); i++)
	{
		DOMNode* a_cert = ownCertList->item(i);
		CASignature* signature = new CASignature();
		CACertStore* certs = new CACertStore();

		//try to get signature key from ownCert
		if(signature->setSignKey(a_cert, SIGKEY_PKCS12, (char*)passwd) != E_SUCCESS)
		{
			//Read password if necessary
			printf("I need a password for the private Mix certificate nr. %d: ", i+1);
			fflush(stdout);
			readPasswd(passwd,500);
			printf("\n");
			if(signature->setSignKey(a_cert, SIGKEY_PKCS12, (char*)passwd) != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Unable to load private Mix certificate nr. %d! Please check your password.\n", i+1);
				delete signature;
				delete certs;
				delete[] opCerts; 
				signature = NULL;
				return E_UNKNOWN;
			}
		}
		//decode own certifciate
		CACertificate* tmpCert = CACertificate::decode(a_cert, CERT_PKCS12, (char*)passwd);
		if(tmpCert== NULL)
		{
			CAMsg::printMsg(LOG_CRIT, "Error while getting own certificate %d!\n", i+1);
			delete signature;
			delete certs;
			delete[] opCerts; 
			return E_UNKNOWN;
		}

		//get SKI
		UINT32 tmpSKIlen = 255;
		UINT8 tmpSKI[255];
		if(tmpCert->getSubjectKeyIdentifier(tmpSKI, &tmpSKIlen) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_CRIT, "Error while getting SKI of own certificate %d!\n", i+1);
			delete signature;
			delete certs;
			delete tmpCert;
			delete[] opCerts; 
			return E_UNKNOWN;
		}
		//CAMsg::printMsg(LOG_DEBUG, "SKI of own cert %d is: %s\n", i+1, tmpSKI);
		//get AKI
		UINT32 tmpAKIlen = 255;
		UINT8 tmpAKI[255];
		if(tmpCert->getAuthorityKeyIdentifier(tmpAKI, &tmpAKIlen) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_WARNING, "Could not get AKI of own certificate. This is not a critical problem, but you have a very old mix certificate. Create a new one as soon as possible.\n");
		}
		else
		{
			//CAMsg::printMsg(LOG_DEBUG, "AKI of own cert %d is: %s\n", i+1, tmpAKI);
		}
		//try to find right opCert
		for(UINT32 j=0; j<opCertsLen; j++)
		{
			if(tmpCert->verify(opCerts[j]) == E_SUCCESS)
			{
				//found right operator cert -> add it to store
				// CAMsg::printMsg(LOG_DEBUG, "Found operator cert for sign key %d!\n", i+1);
				certs->add(opCerts[j]);
				break;
			}
		}
		for(UINT32 j=0; j<opCertsLen; j++)
		{
			delete opCerts[j];
			}
		delete[]opCerts;
		opCerts=NULL;

		if(certs->getNumber() == 0)
		{
			CAMsg::printMsg(LOG_CRIT, "Could not find operator cert for sign key %d! Please check your configuration. Exiting...\n", i+1);
			exit(EXIT_FAILURE);
		}
		//add own cert to store
		certs->add(tmpCert);
		//get Raw SKI
		UINT32 tmpRawSKIlen = 255;
		UINT8 tmpRawSKI[255];
		if(tmpCert->getRawSubjectKeyIdentifier(tmpRawSKI, &tmpRawSKIlen) != E_SUCCESS)
		{
			delete signature;
			delete certs;
			delete tmpCert;
			return E_UNKNOWN;
		}
		if (certs->getNumber() < 2)
		{
			CAMsg::printMsg(LOG_CRIT, "We have less than two certificates (only %d), but we need at least one mix and one operator certificate. There must be something wrong with the cert store. Exiting...\n", certs->getNumber());
			exit(EXIT_FAILURE);
		}
		delete tmpCert;
		tmpCert=NULL;
		CAMsg::printMsg(LOG_DEBUG, "Adding Sign-Key %d with %d certificate(s).\n", i+1, certs->getNumber());
		m_pMultiSignature->addSignature(signature, certs, tmpRawSKI, tmpRawSKIlen);
	}
	if (m_pMultiSignature->getSignatureCount() == 0)
	{
		CAMsg::printMsg(LOG_CRIT, "Could not set a signature key for MultiCert!\n");
		delete m_pMultiSignature;
		m_pMultiSignature = NULL;
		return E_UNKNOWN;
	}
	//end new
	/// if ( (m_pOwnCertificate->getSubjectKeyIdentifier(tmpBuff, &tmpLen) != E_SUCCESS) &&
	//			(m_strMixID == NULL))
	//{
	//	LOG_NODE_NOT_FOUND(OPTIONS_NODE_MIX_ID);
	//	return E_UNKNOWN;
	//}
	//check Mix-ID
	if(m_pMultiSignature->getXORofSKIs(tmpBuff, tmpLen) != E_SUCCESS)
	{
		return E_UNKNOWN;
	}

	if(m_strMixID != NULL )
	{
		if(strncmp(m_strMixID, (char*)tmpBuff, strlen((char*)tmpBuff) ) != 0)
		{
			CAMsg::printMsg(LOG_CRIT,"The configuration file seems inconsistent: it contains another Mix ID (%s) than calculated from the Mix certificate(s), which is %s. Please re-import you mix certificate in the configuration tool, or set the correct mix ID manually by editing the configuration file.\n", m_strMixID, tmpBuff);
			return E_UNKNOWN;
		}
	}
	else
	{
		m_strMixID=new char[strlen((char*)tmpBuff)+1];
		m_strMixID[strlen((char*)tmpBuff)]= (char) 0;
		strcpy(m_strMixID,(char*) tmpBuff);
		return addMixIdToMixInfo();
	}
	
#ifdef PAYMENT
	if (m_strAiID != NULL && m_pMultiSignature->findSKI(m_strAiID) != E_SUCCESS)
	{
		CAMsg::printMsg(LOG_CRIT, "Your price certificate does not fit to your mix certificate(s). Please import the proper price certificate or mix certificate.\n");
	}
#endif
	
#ifdef DYNAMIC_MIX
		// LERNGRUPPE: Dynamic Mixes must have a cascade name, as MiddleMixes may be reconfigured to be FirstMixes 
	if(bNeedCascadeNameFromMixID)
	{
		m_strCascadeName = new char[strlen(m_strMixID) + 1];
		memset(m_strCascadeName, 0, strlen(m_strMixID) + 1);
		strncpy(m_strCascadeName, m_strMixID, strlen(m_strMixID)+1);
	}
#endif
	return E_SUCCESS;
}

/**
 * determines whether this mix is a first a middle or a last mix
 * appears in <General></General> and must be set.
 * */
SINT32 CACmdLnOptions::setMixType(DOMElement* elemGeneral)
{
	DOMElement* elemMixType=NULL;
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;

	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_MIX_TYPE);

	//getMixType
	if (getDOMChildByName(elemGeneral, OPTIONS_NODE_MIX_TYPE,
							elemMixType,false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_MIX_TYPE);
		return E_UNKNOWN;
	}

	if( getDOMElementValue(elemMixType,tmpBuff,&tmpLen) == E_SUCCESS )
	{
		if(memcmp(tmpBuff,"FirstMix",8) == 0)
		{
			m_bFirstMix = true;
		}
		else if (memcmp(tmpBuff,"MiddleMix",9) == 0)
		{
			m_bMiddleMix = true;
		}
		else if (memcmp(tmpBuff,"LastMix",7) == 0)
		{
			m_bLastMix = true;
		}
		if ( appendMixInfo_internal(elemMixType, WITH_SUBTREE) != E_SUCCESS )
		{
			return E_UNKNOWN;
		}
	}
	else
	{
		LOG_NODE_EMPTY_OR_INVALID(OPTIONS_NODE_MIX_TYPE);
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setMixName(DOMElement* elemGeneral)
{
	DOMElement *elemMixName = NULL, *elemMixInfoName = NULL;
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;
	UINT8 *typeValue = NULL;  //(UINT8 *) OPTIONS_VALUE_NAMETYPE_DEFAULT;
	//uncomment the above line to enable a default name type

	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_MIX_NAME);

	//Inserting the Name if given...
	getDOMChildByName(elemGeneral, OPTIONS_NODE_MIX_NAME, elemMixName, false);
	if(elemMixName != NULL)
	{
		if(getDOMElementValue(elemMixName, tmpBuff, &tmpLen) == E_SUCCESS)
		{
			m_strMixName = new char[tmpLen+1];
			memset(m_strMixName, 0, tmpLen+1);
			memcpy(m_strMixName, tmpBuff, tmpLen);
		}
		tmpLen = TMP_BUFF_SIZE;
		getDOMElementAttribute(elemMixName, OPTIONS_ATTRIBUTE_NAME_FOR_CASCADE, tmpBuff, &tmpLen);
	}
	else
	{
		tmpLen = 0;
		m_strMixName = NULL;
	}

	/* now append the values to the mix info
	 * conditions:
	 * - if name is set, m_strMixname points to it.
	 * - if name type is set then it is in tmpBuff.
	 */
	elemMixInfoName = createDOMElement(m_docMixInfo, MIXINFO_NODE_MIX_NAME);

	/* if name is set */
	if(m_strMixName != NULL)
	{
		setDOMElementValue(elemMixInfoName, (UINT8*) m_strMixName);
	}

	if( tmpLen != 0 ) /* if name type is set */
	{
		if( strncasecmp( ((char *)tmpBuff),
					OPTIONS_VALUE_OPERATOR_NAME,
					strlen(OPTIONS_VALUE_OPERATOR_NAME)) == 0 ) /* type is operator name*/
		{
			typeValue = (UINT8 *) OPTIONS_VALUE_OPERATOR_NAME;
		}
		else if( strncasecmp( ((char *)tmpBuff),
					OPTIONS_VALUE_MIX_NAME,
					strlen(OPTIONS_VALUE_MIX_NAME)) == 0 ) /* type is mix name*/
		{
			typeValue = (UINT8 *) OPTIONS_VALUE_MIX_NAME;
		}
	}
	if(typeValue != NULL)
	{
		setDOMElementAttribute(elemMixInfoName,
			OPTIONS_ATTRIBUTE_NAME_FOR_CASCADE, typeValue);
	}

	if(m_docMixInfo->getDocumentElement() != NULL)
	{
		m_docMixInfo->getDocumentElement()->appendChild(elemMixInfoName);
	}
	else
	{
		//Should never happen
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setMixID(DOMElement* elemGeneral)
{
	DOMElement* elemMixID=NULL;
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;
	size_t mixID_strlen = 0;

	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_MIX_NAME);

	getDOMChildByName(elemGeneral, OPTIONS_NODE_MIX_ID, elemMixID, false);
	if(elemMixID != NULL)
	{
		if(getDOMElementValue(elemMixID,tmpBuff,&tmpLen) == E_SUCCESS)
		{
			strtrim(tmpBuff);
			mixID_strlen = strlen((char*)tmpBuff)+1;
			m_strMixID = new char[strlen((char*)tmpBuff)+1];
			memset(m_strMixID, 0, mixID_strlen);
			memcpy(m_strMixID, tmpBuff, mixID_strlen);

			return addMixIdToMixInfo();
		}
	}
	return E_SUCCESS;

}
SINT32 CACmdLnOptions::setCascadeNameFromOptions(DOMElement* elemGeneral)
{
	DOMElement* elemCascadeName=NULL;
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;

	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_CASCADE_NAME);

	//getCascadeName
	getDOMChildByName(elemGeneral, OPTIONS_NODE_CASCADE_NAME, elemCascadeName, false);

#ifdef DYNAMIC_MIX
	bool bNeedCascadeNameFromMixID=false;
#endif
	if(getDOMElementValue(elemCascadeName,tmpBuff,&tmpLen)==E_SUCCESS)
	{
		setCascadeName(tmpBuff);
	}
#ifdef DYNAMIC_MIX
		/* LERNGRUPPE: Dynamic Mixes must have a cascade name, as MiddleMixes may be reconfigured to be FirstMixes */
	else
	{
		bNeedCascadeNameFromMixID=true;
		setCascadeName(m_strMixID);
	}
#endif
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setUserID(DOMElement* elemGeneral)
{
	DOMElement* elemUID=NULL;
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;

	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_USER_ID);

	//get Username to run as...
	getDOMChildByName(elemGeneral, OPTIONS_NODE_USER_ID, elemUID,false);

	if(getDOMElementValue(elemUID,tmpBuff,&tmpLen)==E_SUCCESS)
	{
		m_strUser=new char[tmpLen+1];
		memcpy(m_strUser,tmpBuff,tmpLen);
		m_strUser[tmpLen]=0;
	}

#ifndef WIN32
		UINT8 buff[255];
		if(getUser(buff,255)==E_SUCCESS) //switching user
			{
				struct passwd* pwd=getpwnam((char*)buff);
				if(pwd==NULL || (setegid(pwd->pw_gid)==-1) || (seteuid(pwd->pw_uid)==-1) )
				{
					if (pwd==NULL)
					{
						CAMsg::printMsg(LOG_ERR,
								"Could not switch to effective user '%s'! Reason: User '%s' does not exist on this system. Create this user first.\n",
														buff, buff);
					}
					else
					{
						CAMsg::printMsg(LOG_ERR,"Could not switch to effective user '%s'! Reason: %s (%i)\n",
								buff, GET_NET_ERROR_STR(GET_NET_ERROR), GET_NET_ERROR);
					}
				}
				else
					CAMsg::printMsg(LOG_INFO,"Switched to effective user '%s'!\n",buff);
			}

		if(geteuid()==0)
			CAMsg::printMsg(LOG_WARNING,"Mix is running as root/superuser!\n");
#endif
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setNrOfFileDescriptors(DOMElement* elemGeneral)
{
	DOMElement* elemNrFd=NULL;
	UINT32 tmp = 0;

	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_FD_NR);

	//get Number of File Descriptors to use
	getDOMChildByName(elemGeneral, OPTIONS_NODE_FD_NR, elemNrFd, false);

	if(getDOMElementValue(elemNrFd,&tmp) == E_SUCCESS)
	{
		m_nrOfOpenFiles=tmp;
	}
	
#ifndef WIN32

		struct rlimit coreLimit;
		coreLimit.rlim_cur = coreLimit.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &coreLimit) != 0)
		{
			CAMsg::printMsg(LOG_CRIT,"Could not set RLIMIT_CORE (max core file size) to unlimited size. -- Core dumps might not be generated!\n",m_nrOfOpenFiles);
		}
		
		if(m_nrOfOpenFiles>0)
			{
				struct rlimit lim;
				// Set the new MAX open files limit
				lim.rlim_cur = lim.rlim_max = m_nrOfOpenFiles;
				if (setrlimit(RLIMIT_NOFILE, &lim) != 0)
				{
					CAMsg::printMsg(LOG_CRIT,"Could not set MAX open files to: %u Reason: %s (%i) \nYou might have insufficient user rights. If so, switch to a privileged user or do not set the number of file descriptors. -- Exiting!\n",
							m_nrOfOpenFiles, GET_NET_ERROR_STR(GET_NET_ERROR), GET_NET_ERROR);
					exit(EXIT_FAILURE);
				}
			}
#endif	
	
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setDaemonMode(DOMElement* elemGeneral)
{
	DOMElement* elemDaemonMode = NULL;
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;

	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_DAEMON);

	//get Run as Daemon
	getDOMChildByName(elemGeneral, OPTIONS_NODE_DAEMON, elemDaemonMode,false);

	if(getDOMElementValue(elemDaemonMode, tmpBuff, &tmpLen) == E_SUCCESS &&
		memcmp(tmpBuff,"True",4)==0)
	{
		m_bDaemon=true;
	}
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setNextMixCertificate(DOMElement *elemCertificates)
{
	DOMElement* elemNextCert = NULL;

	if(!m_bVerifyMixCerts)
	{
		if(elemCertificates == NULL) return E_UNKNOWN;
		ASSERT_CERTIFICATES_OPTIONS_PARENT
			(elemCertificates->getNodeName(), OPTIONS_NODE_NEXT_MIX_CERTIFICATE);

		//nextMixCertificate if given
		getDOMChildByName(elemCertificates, OPTIONS_NODE_NEXT_MIX_CERTIFICATE, elemNextCert,false);
		if(elemNextCert!=NULL)
		{
			m_pNextMixCertificate=
				CACertificate::decode(elemNextCert->getFirstChild(),CERT_X509CERTIFICATE);
			if(m_pNextMixCertificate == NULL)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not decode the certificate of the next mix!\n");
				return E_UNKNOWN;
			}
		}
	}
	return E_SUCCESS;

}

SINT32 CACmdLnOptions::setPrevMixCertificate(DOMElement *elemCertificates)
{
	//prevMixCertificate if given
	DOMElement* elemPrevCert=NULL;

	if(!m_bVerifyMixCerts)
	{
		if(elemCertificates == NULL) return E_UNKNOWN;
		ASSERT_CERTIFICATES_OPTIONS_PARENT
			(elemCertificates->getNodeName(), OPTIONS_NODE_PREV_MIX_CERTIFICATE);

		getDOMChildByName(elemCertificates, OPTIONS_NODE_PREV_MIX_CERTIFICATE, elemPrevCert, false);
		if(elemPrevCert!=NULL)
		{
			m_pPrevMixCertificate=
				CACertificate::decode(elemPrevCert->getFirstChild(),CERT_X509CERTIFICATE);
		}
	}
	return E_SUCCESS;

}

SINT32 CACmdLnOptions::setTrustedRootCertificates(DOMElement *elemCertificates)
{
	DOMElement* elemTrustedCerts=NULL;
	DOMNodeList* trustedCerts=NULL;
	CACertificate* cert;

	if(m_bVerifyMixCerts)
	{
		if(elemCertificates == NULL) return E_UNKNOWN;
			ASSERT_CERTIFICATES_OPTIONS_PARENT
				(elemCertificates->getNodeName(), OPTIONS_NODE_TRUSTED_ROOT_CERTIFICATES);

		getDOMChildByName(elemCertificates, OPTIONS_NODE_TRUSTED_ROOT_CERTIFICATES, elemTrustedCerts, false);
		if(elemTrustedCerts!=NULL)
		{
			trustedCerts = getElementsByTagName(elemTrustedCerts, OPTIONS_NODE_X509_CERTIFICATE);

			for(UINT32 i=0; i<trustedCerts->getLength(); i++)
			{
				cert = CACertificate::decode(trustedCerts->item(i), CERT_X509CERTIFICATE);
				if(cert != NULL)
				{
					m_pTrustedRootCertificates->add(cert);
				}
				else
				{
					CAMsg::printMsg(LOG_WARNING, "Root certificate could not be decoded\n");
				}
			}
		}
		else
		{
			LOG_NODE_NOT_FOUND(OPTIONS_NODE_TRUSTED_ROOT_CERTIFICATES);
			return E_UNKNOWN;
		}
		if(m_pTrustedRootCertificates->getNumber() == 0)
		{
			CAMsg::printMsg(LOG_CRIT, "No trusted root certificates found.\n");
			return E_UNKNOWN;
		}
		CAMsg::printMsg(LOG_INFO, "Loaded %d trusted root certificates.\n", m_pTrustedRootCertificates->getNumber());
	}
	return E_SUCCESS;
}

// Read Configuration of KeepAliveTraffic 
SINT32 CACmdLnOptions::setKeepAliveTraffic(DOMElement *elemNetwork)
{
	DOMElement* elemKeepAlive = NULL;
	DOMElement* elemKeepAliveSendInterval = NULL;
	DOMElement* elemKeepAliveRecvInterval = NULL;

	if(elemNetwork == NULL) return E_UNKNOWN;
	ASSERT_NETWORK_OPTIONS_PARENT
		(elemNetwork->getNodeName(), OPTIONS_NODE_SERVER_MONITORING);

	getDOMChildByName(elemNetwork, OPTIONS_NODE_KEEP_ALIVE, elemKeepAlive, false);
	getDOMChildByName(elemKeepAlive, OPTIONS_NODE_KEEP_ALIVE_SEND_IVAL, elemKeepAliveSendInterval, false);
	getDOMChildByName(elemKeepAlive, OPTIONS_NODE_KEEP_ALIVE_RECV_IVAL, elemKeepAliveRecvInterval, false);
	getDOMElementValue(elemKeepAliveSendInterval, m_u32KeepAliveSendInterval, KEEP_ALIVE_TRAFFIC_SEND_WAIT_TIME);
	getDOMElementValue(elemKeepAliveRecvInterval, m_u32KeepAliveRecvInterval, KEEP_ALIVE_TRAFFIC_RECV_WAIT_TIME);
	return E_SUCCESS;
}

/* this method is only for internal use in order to intialize the
 * mixinfo structure when the options are parsed. Don't get confused
 * with the method addMixInfo of class CAMix which appends an
 * additional timestamp.
 * if NULL is specified as name the name of a_node is used
 */
SINT32 CACmdLnOptions::appendMixInfo_internal(DOMNode* a_node, bool with_subtree)
{
	DOMNode *importedNode = NULL;
	DOMNode *appendedNode = NULL;

	if(a_node == NULL)
	{
		CAMsg::printMsg(LOG_CRIT,"No node specified!\n");
		return E_UNKNOWN;
	}
	if(m_docMixInfo == NULL)
	{
		CAMsg::printMsg(LOG_CRIT,"No mixinfo document initialized!\n");
		return E_UNKNOWN;
	}
	if(m_docMixInfo->getDocumentElement() == NULL)
	{
		CAMsg::printMsg(LOG_CRIT,"No mixinfo dom structure initialized!\n");
		return E_UNKNOWN;
	}

	importedNode = m_docMixInfo->importNode(a_node, with_subtree);

	if(importedNode != NULL)
	{
		/** Here we remove any given e-mail address to reduce the spam problem. */
		if (importedNode->getNodeType() == DOMNode::ELEMENT_NODE)
		{
			DOMNodeList* nodesMail = getElementsByTagName((DOMElement*)importedNode, "EMail");
			for (UINT32 i = 0; i < nodesMail->getLength (); i++)
			{
				nodesMail->item(i)->getParentNode()->removeChild(nodesMail->item(i));
			}
		}
		
		appendedNode = m_docMixInfo->getDocumentElement()->appendChild(importedNode);
		if( appendedNode != NULL )
		{
			return E_SUCCESS;
		}
	}
	CAMsg::printMsg(LOG_CRIT,"Could not append Node \"%s\" to Mixinfo!\n", a_node->getNodeName());
	return E_UNKNOWN;
}

/**
 * Just add the id of the Mix to the MixInfo Document
 */
inline SINT32 CACmdLnOptions::addMixIdToMixInfo()
{
	if( (m_docMixInfo != NULL) && (m_strMixID != NULL) )
	{
		return setDOMElementAttribute
				(m_docMixInfo->getDocumentElement(), MIXINFO_ATTRIBUTE_MIX_ID, (UINT8*) m_strMixID);
	}
	CAMsg::printMsg(LOG_CRIT,"No mixinfo document initialized!\n");
	return E_UNKNOWN;
}
SINT32 CACmdLnOptions::setListenerInterfaces(DOMElement *elemNetwork)
{
	DOMElement* elemListenerInterfaces=NULL;

	if(elemNetwork == NULL) return E_UNKNOWN;
	ASSERT_NETWORK_OPTIONS_PARENT
		(elemNetwork->getNodeName(), OPTIONS_NODE_LISTENER_INTERFACES);

	getDOMChildByName
		(elemNetwork, OPTIONS_NODE_LISTENER_INTERFACES, elemListenerInterfaces, false);
	m_arListenerInterfaces = CAListenerInterface::getInstance(
		elemListenerInterfaces, m_cnListenerInterfaces);

#ifndef DYNAMIC_MIX
		// LERNGRUPPE: ListenerInterfaces may be configured dynamically 
	if (m_cnListenerInterfaces == 0)
	{
		CAMsg::printMsg(LOG_CRIT, "No listener interfaces found!\n");
		return E_UNKNOWN;
	}
#endif
	if(elemListenerInterfaces != NULL)
		{
			// import listener interfaces element; this is needed for cascade auto configuration
			// -- inserted by ronin <ronin2@web.de> 2004-08-16
		appendMixInfo_internal(elemListenerInterfaces, WITH_SUBTREE);
		}

	UINT32 i;
	SINT32 ret;
	CASocket** arrSocketsIn=new CASocket*[getListenerInterfaceCount()];
	for (i = 0; i < getListenerInterfaceCount(); i++)
	{
		arrSocketsIn[i] = NULL;
	}

	ret = createSockets(false, arrSocketsIn, getListenerInterfaceCount());

	for(i=0;i<getListenerInterfaceCount();i++)
	{
		if (arrSocketsIn[i] != NULL)
		{
			arrSocketsIn[i]->close();
			delete arrSocketsIn[i];
			arrSocketsIn[i] = NULL;
		}
	}
	delete[] arrSocketsIn;
	arrSocketsIn=NULL;

	
	if (ret != E_SUCCESS && ret != E_UNSPECIFIED && ret != E_SPACE)
	{
		CAMsg::printMsg(LOG_CRIT, "Could not listen on at least one of the specified interfaces. Please check if another running mix or server process is blocking the listen addresses, and if you have sufficient system rights.\n");
	}

	return ret;
}

SINT32 CACmdLnOptions::setOwnOperatorCertificate(DOMElement *elemCertificates)
{
	DOMElement* elemOpCert = NULL;
	DOMElement *opCertX509 = NULL;

	if(elemCertificates == NULL) return E_UNKNOWN;
	ASSERT_CERTIFICATES_OPTIONS_PARENT
		(elemCertificates->getNodeName(), OPTIONS_NODE_OWN_OPERATOR_CERTIFICATE);

	//then Operator Certificate
	if (getDOMChildByName
		 (elemCertificates, OPTIONS_NODE_OWN_OPERATOR_CERTIFICATE,
			elemOpCert, false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_OWN_OPERATOR_CERTIFICATE);
		return E_UNKNOWN;
	}

	if (elemOpCert != NULL)
	{
		m_opCertList = getElementsByTagName(elemOpCert, "X509Certificate");

		getDOMChildByName(elemOpCert, OPTIONS_NODE_X509_CERTIFICATE, opCertX509, true);
		if( opCertX509 != NULL)
		{
			m_OpCert = CACertificate::decode(opCertX509, CERT_X509CERTIFICATE);
		}
		else
		{
			LOG_NODE_NOT_FOUND(OPTIONS_NODE_X509_CERTIFICATE);
			return E_UNKNOWN;
		}
	}
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setMixCertificateVerification(DOMElement *elemCertificates)
{
	DOMElement *elemMixVerify;
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;

	if(elemCertificates == NULL) return E_UNKNOWN;
		ASSERT_CERTIFICATES_OPTIONS_PARENT
			(elemCertificates->getNodeName(), OPTIONS_NODE_MIX_CERTIFICATE_VERIFICATION);

	getDOMChildByName(elemCertificates, OPTIONS_NODE_MIX_CERTIFICATE_VERIFICATION, elemMixVerify, false);
	if(elemMixVerify != NULL)
	{
		if(getDOMElementValue(elemMixVerify, tmpBuff, &tmpLen) == E_SUCCESS &&
				memcmp(tmpBuff,"True",4)==0)
		{
			m_bVerifyMixCerts = true;
			m_pTrustedRootCertificates = new CACertStore();
			CAMsg::printMsg(LOG_INFO, "Mix certificate verification is enabled.\n");
		}
	}
	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setLoggingOptions(DOMElement* elemGeneral)
	{
	//get Logging
	DOMElement* elemLogging=NULL;
	DOMElement* elemEncLog=NULL;
	DOMElement* elem=NULL;

	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;

	SINT32 maxLogFilesTemp = 0;
	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_LOGGING);

	getDOMChildByName(elemGeneral, OPTIONS_NODE_LOGGING, elemLogging, false);
	if(elemLogging != NULL)
		{
		if (getDOMElementAttribute(elemLogging, "level", tmpBuff, &tmpLen) == E_SUCCESS)
			{
			strtrim(tmpBuff);
			toLower(tmpBuff);
			m_strLogLevel = new char[strlen((char*)tmpBuff)+1];
			strcpy(m_strLogLevel, (char*)tmpBuff);
			}
		else
			{
			m_strLogLevel = new char[strlen("debug")+1];
			strcpy(m_strLogLevel, "debug");
			}



		getDOMChildByName(elemLogging, OPTIONS_NODE_LOGGING_FILE, elem, false);
		tmpLen = TMP_BUFF_SIZE;
		if(getDOMElementValue(elem, tmpBuff, &tmpLen) == E_SUCCESS)
			{
			strtrim(tmpBuff);
			m_strLogDir = new char[strlen((char*)tmpBuff)+1];
			strcpy(m_strLogDir, (char*)tmpBuff);
			getDOMElementAttribute
				(elem, OPTIONS_ATTRIBUTE_LOGGING_MAXFILESIZE, m_maxLogFileSize);
			//Set maximum number of logging files
			//CAMsg::printMsg(LOG_ERR,"!!!!!!!!\n");
			if((getDOMElementAttribute
				(elem, OPTIONS_ATTRIBUTE_LOGGING_MAXFILES, &maxLogFilesTemp) != E_SUCCESS) ||
				(maxLogFilesTemp == 0) )
				{
				m_maxLogFiles = LOGGING_MAXFILES_DEFAULT;
				}
			else
				{
				if(maxLogFilesTemp < 0)
					{
					//CAMsg::printMsg(LOG_ERR,"Negative number of log files specified.\n");
					return E_UNKNOWN;
					}
				m_maxLogFiles = (UINT32) maxLogFilesTemp;
				//CAMsg::printMsg(LOG_ERR,"Max log files are %u\n", m_maxLogFiles);
				}
			}
		getDOMChildByName(elemLogging, OPTIONS_NODE_SYSLOG, elem, false);
		tmpLen = TMP_BUFF_SIZE;
		memset(tmpBuff, 0, tmpLen);
		if( (getDOMElementValue(elem, tmpBuff, &tmpLen) == E_SUCCESS) &&
			 (memcmp(tmpBuff,"True",4) == 0) )
			{
			m_bSyslog = true;
			}

		getDOMChildByName(elemLogging, OPTIONS_NODE_LOGGING_CONSOLE, elem, false);
		tmpLen = TMP_BUFF_SIZE;
		memset(tmpBuff, 0, tmpLen);
		if( (getDOMElementValue(elem, tmpBuff, &tmpLen) == E_SUCCESS) &&
			 (memcmp(tmpBuff,"True",4) == 0) )
			{
			m_bLogConsole = true;
			}

		//get Encrypted Log Info
		if( getDOMChildByName
			 (elemLogging, OPTIONS_NODE_ENCRYPTED_LOG, elemEncLog,false) == E_SUCCESS )
			{
			m_bIsEncryptedLogEnabled = true;
			getDOMChildByName(elemEncLog, OPTIONS_NODE_LOGGING_FILE, elem, false);

			tmpLen = TMP_BUFF_SIZE;
			memset(tmpBuff, 0, tmpLen);
			if( getDOMElementValue(elem, tmpBuff, &tmpLen) == E_SUCCESS )
				{
				strtrim(tmpBuff);
				m_strEncryptedLogDir = new char[strlen((char*)tmpBuff)+1];
				strcpy(m_strEncryptedLogDir, (char*)tmpBuff);
				}
			DOMElement* elemKeyInfo;
			DOMElement* elemX509Data;
			if(getDOMChildByName
				 (elemEncLog, OPTIONS_NODE_LOGGING_KEYINFO, elemKeyInfo, false) == E_SUCCESS &&
				 getDOMChildByName
				 (elemKeyInfo, OPTIONS_NODE_X509DATA, elemX509Data, false) == E_SUCCESS )
				{
				m_pLogEncryptionCertificate =
					CACertificate::decode(elemX509Data->getFirstChild(), CERT_X509CERTIFICATE);
				}
			}
		else
			{
			m_bIsEncryptedLogEnabled=false;
			}

		}

	SINT32 ret = initLogging();
	if (ret == E_SUCCESS)
		{
		CAMsg::printMsg(LOG_INFO,MIX_VERSION_INFO);
#ifdef MIX_VERSION_TESTING
		CAMsg::printMsg(LOG_WARNING, MIX_VERSION_TESTING_TEXT);
#endif
		}

	return ret;
	}

/***********************************
 * general option setter functions *
 ***********************************/
SINT32 CACmdLnOptions::setGeneralOptions(DOMElement* elemRoot)
{
	DOMElement* elemGeneral=NULL;

	if (getDOMChildByName(elemRoot, OPTIONS_NODE_GENERAL,
							elemGeneral,false) != E_SUCCESS)
	{
		LOG_NODE_NOT_FOUND(OPTIONS_NODE_GENERAL);
		return E_UNKNOWN;
	}

	return invokeOptionSetters
		(generalOptionSetters, elemGeneral, GENERAL_OPTIONS_NR);
}

SINT32 CACmdLnOptions::createSockets(bool a_bMessages, CASocket** a_sockets, UINT32 a_socketsLen)
{
		if (a_socketsLen <= 0)
		{
			CAMsg::printMsg(LOG_CRIT,"Could not create any listener sockets as we have no space reserved for them. This seems to be an implementation bug.");
			return E_SPACE;
		}


		UINT32 aktSocket;
		UINT8 buff[255];
		SINT32 ret = E_UNKNOWN;
		UINT32 currentInterface;
		CASocketAddr* pAddr;
		UINT32* arrayVirtualPorts = new UINT32[a_socketsLen];
		UINT32 iVirtualPortsLen = 0;
		UINT32 iHiddenPortsLen = 0;
		UINT32* arrayHiddenPorts = new UINT32[a_socketsLen];
		

		aktSocket = 0;
		for(currentInterface=0;currentInterface < getListenerInterfaceCount(); currentInterface++)
			{
				CAListenerInterface* pListener=NULL;
				pListener=getListenerInterface(currentInterface+1);
				if(pListener==NULL)
					{
						CAMsg::printMsg(LOG_CRIT,"Error: Listener interface %d is invalid.\n", currentInterface+1);
					
						delete[] arrayVirtualPorts;
						delete[] arrayHiddenPorts;
					
						return E_UNKNOWN;
					}
				
				pAddr=pListener->getAddr();
				pAddr->toString(buff,255);
				
				if(pAddr->getType()==AF_INET)
				{
					if (pListener->isVirtual())
					{
						arrayVirtualPorts[iVirtualPortsLen] = ((CASocketAddrINet*)pAddr)->getPort();
						iVirtualPortsLen++;
					}
					else if (pListener->isHidden())
					{
						arrayHiddenPorts[iHiddenPortsLen] = ((CASocketAddrINet*)pAddr)->getPort();
						iHiddenPortsLen++;
					}
				}
				
				if(pListener->isVirtual())
				{
					delete pListener;
					pListener = NULL;
					delete pAddr;
					pAddr = NULL;
					continue;
				}
				
				if (a_socketsLen < aktSocket )
				{
					CAMsg::printMsg(LOG_CRIT, 
						"Found %d listener sockets, but we have only reserved memory for %d sockets. This seems to be an implementation error in the code.\n", 
						(aktSocket + 1), a_socketsLen);

					delete[] arrayVirtualPorts;
					delete[] arrayHiddenPorts;
					delete pAddr;
					
					return E_SPACE;
				}
				
				ret = E_SUCCESS;
				a_sockets[aktSocket] = new CASocket();
				a_sockets[aktSocket]->create(pAddr->getType());
				a_sockets[aktSocket]->setReuseAddr(true);
				
				delete pListener;
				pListener = NULL;
#ifndef _WIN32
				//we have to be a temporary superuser if port <1024...
				int old_uid=geteuid();
				if(pAddr->getType()==AF_INET&&((CASocketAddrINet*)pAddr)->getPort()<1024)
				{
					if(seteuid(0)==-1) //changing to root
					{
						CAMsg::printMsg(LOG_CRIT,"Setuid failed! We might not be able to listen on interface %d (%s) as we cannot change to the root user.\n",
								currentInterface+1, buff);
					}
				}
#endif
				ret=a_sockets[aktSocket]->listen(*pAddr);
				delete pAddr;
				pAddr = NULL;

				if(ret!=E_SUCCESS)
				{
					CAMsg::printMsg(LOG_CRIT,"Socket error while listening on interface %d (%s). Reason: %s (%i)\n",currentInterface+1, buff,
							GET_NET_ERROR_STR(GET_NET_ERROR), GET_NET_ERROR);
				}

#ifndef _WIN32
				seteuid(old_uid);
#endif
				if(ret!=E_SUCCESS)
				{
						delete[] arrayVirtualPorts;
						delete[] arrayHiddenPorts;
						return E_UNKNOWN;
				}

				if (a_bMessages)
				{
						CAMsg::printMsg(LOG_DEBUG,"Listening on Interface: %s\n",buff);
				}
					aktSocket++;
			} //END FOR

		if (ret == E_UNKNOWN)
		{
			CAMsg::printMsg(LOG_CRIT,"Could not find any valid (non-virtual) listener interface!\n");
		}
		else if (ret == E_SUCCESS)
		{
			for (UINT32 iHiddenPort = 0; iHiddenPort < iHiddenPortsLen; iHiddenPort++)
			{
				bool bVirtualFound = false;
				for (UINT32 iVirtualPort = 0; iVirtualPort < iVirtualPortsLen; iVirtualPort++)
				{
					if (arrayHiddenPorts[iHiddenPort] == arrayVirtualPorts[iVirtualPort])
					{
						bVirtualFound = true;
						arrayVirtualPorts[iVirtualPort] = 0;
					}
				}
				if (!bVirtualFound)
				{
					CAMsg::printMsg(LOG_CRIT,"No virtuel listener interface found for the hidden interface %d with port %d. Please remove the hidden interface, add the corresponding virtual interface or remove the 'hidden' attribute from the interface.\n", iHiddenPort, arrayHiddenPorts[iHiddenPort]);
					ret = E_UNSPECIFIED;
					break;
				}
			}
			
			if (ret == E_SUCCESS)
			{
				for (UINT32 iVirtualPort = 0; iVirtualPort < iVirtualPortsLen; iVirtualPort++)
				{
					if (arrayVirtualPorts[iVirtualPort] != 0)
					{
						CAMsg::printMsg(LOG_CRIT,"No hidden listener interface found for the virtual interface %d with port %d. Please remove the virtual interface, add the corresponding hidden interface or remove the 'virtual' attribute from the interface.\n", iVirtualPort, arrayVirtualPorts[iVirtualPort]);
						ret = E_UNSPECIFIED;
						break;
					}
				}
			}
		}
		
		if (ret == E_SUCCESS && a_bMessages)
		{
			CAMsg::printMsg(LOG_DEBUG,"Listening on all interfaces.\n");
		}
		
		delete[] arrayVirtualPorts;
		delete[] arrayHiddenPorts;
		arrayVirtualPorts = NULL;

		return ret;
}


/**
 * determines whether this mix is a dynamic mix or not
 * appears in <General></General> and is optional.
 * */
SINT32 CACmdLnOptions::setDynamicMix(DOMElement* elemGeneral)
{
	// LERNGRUPPE
	// get Dynamic flag
	DOMElement* elemDynamic=NULL;
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;
	m_bDynamic = false;

	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_DYNAMIC_MIX);

	getDOMChildByName(elemGeneral, OPTIONS_NODE_DYNAMIC_MIX, elemDynamic, false);
	if(elemDynamic != NULL)
	{
		if(getDOMElementValue(elemDynamic, tmpBuff, &tmpLen)==E_SUCCESS)
			{
			m_bDynamic = (strcmp("True",(char*)tmpBuff) == 0);
			}
	}
	if(m_bDynamic)
	{
		CAMsg::printMsg( LOG_DEBUG, "I am a dynamic mix\n");
	}
	return E_SUCCESS;
}


SINT32 CACmdLnOptions::setInfoServices(DOMElement *elemNetwork)
{
	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;
	DOMElement* elemInfoServiceContainer=NULL;

	if(elemNetwork == NULL) return E_UNKNOWN;
	ASSERT_NETWORK_OPTIONS_PARENT
		(elemNetwork->getNodeName(), OPTIONS_NODE_INFOSERVICE_LIST);

	getDOMChildByName
		(elemNetwork, OPTIONS_NODE_INFOSERVICE_LIST, elemInfoServiceContainer,false);
	if (elemInfoServiceContainer ==	NULL)
	{
		// old configuration version <= 0.61
		DOMElement* elemInfoService=NULL;
		DOMElement* elemAllowReconfig=NULL;
		if (getDOMChildByName
				(elemNetwork, OPTIONS_NODE_INFOSERVICE, elemInfoService, false) != E_SUCCESS)
		{
			LOG_NODE_NOT_FOUND(OPTIONS_NODE_INFOSERVICE);
		}
		// LERNGRUPPE: There might not be any InfoService configuration in the file, but in infoservices.xml, so check this 
		if(elemInfoService != NULL)
		{
			getDOMChildByName
				(elemInfoService, OPTIONS_NODE_ALLOW_AUTO_CONF, elemAllowReconfig, false);
			CAListenerInterface* isListenerInterface = CAListenerInterface::getInstance(elemInfoService);
			if (!isListenerInterface)
			{
				LOG_NODE_EMPTY_OR_INVALID(OPTIONS_NODE_INFOSERVICE);
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

	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setServerMonitoring(DOMElement *elemNetwork)
{
#ifdef SERVER_MONITORING

	UINT8 tmpBuff[TMP_BUFF_SIZE];
	UINT32 tmpLen = TMP_BUFF_SIZE;

	DOMElement* elemServerMonitoringRoot = NULL;
	DOMElement* elemServerMonitoringHost = NULL;
	DOMElement* elemServerMonitoringPort = NULL;

	if(elemNetwork == NULL) return E_UNKNOWN;
	ASSERT_NETWORK_OPTIONS_PARENT
		(elemNetwork->getNodeName(), OPTIONS_NODE_SERVER_MONITORING);

	m_strMonitoringListenerHost = NULL;
	m_iMonitoringListenerPort = 0xFFFF;

	if (getDOMChildByName
			(elemNetwork, OPTIONS_NODE_SERVER_MONITORING, elemServerMonitoringRoot,false) == E_SUCCESS)
	{
		if(getDOMChildByName
				(elemServerMonitoringRoot, OPTIONS_NODE_HOST, elemServerMonitoringHost, false) == E_SUCCESS)
		{
			if(getDOMElementValue(elemServerMonitoringHost,
									(UINT8 *)tmpBuff,&tmpLen)==E_SUCCESS)
			{
				m_strMonitoringListenerHost = new char[tmpLen+1];
				strncpy(m_strMonitoringListenerHost, (const char*) tmpBuff, tmpLen);
				m_strMonitoringListenerHost[tmpLen] = 0;
			}
		}
		if(getDOMChildByName
				(elemServerMonitoringRoot, OPTIONS_NODE_PORT,
				 elemServerMonitoringPort, false) == E_SUCCESS)
		{
			UINT16 port = 0xFFFF;
			if(getDOMElementValue(elemServerMonitoringPort, &port)==E_SUCCESS)
			{
				m_iMonitoringListenerPort = port;
			}
		}

		// only non-local ListnerInterfaces are showed in Mix status info 
			if( (elemServerMonitoringRoot != NULL) &&
				(m_strMonitoringListenerHost != NULL))
			{
				if( (strncmp("localhost", m_strMonitoringListenerHost, 9) != 0) &&
					(strncmp("127.0.0.1", m_strMonitoringListenerHost, 9) != 0)	)
				{
					appendMixInfo_internal(elemServerMonitoringRoot, WITH_SUBTREE);
				}
			}
	}
	else
	{
		CAMsg::printMsg(LOG_DEBUG, "Server Monitoring Config not found\n");
	}
#endif // SERVER_MONITORING 
	return E_SUCCESS;
}

/// **
//	* LERNGRUPPE
//	* Parses the \c InfoServices Node in a) a mix configuration or b) out of \c infoservices.xml
//	* (Code refactored from CACmdLnOptions::processXmlConfiguration
//	* @param a_infoServiceNode The \c InfoServices Element
//	* @retval E_SUCCESS
//	
SINT32 CACmdLnOptions::parseInfoServices(DOMElement* a_infoServiceNode)
{
	DOMElement* elemAllowReconfig;
	getDOMChildByName(a_infoServiceNode, OPTIONS_NODE_ALLOW_AUTO_CONF, elemAllowReconfig, false);
	DOMNodeList* isList = getElementsByTagName(a_infoServiceNode, OPTIONS_NODE_INFOSERVICE);
	// If there are no InfoServices in the file, keep the (hopefully) previously configured InfoServices 
	if(isList->getLength() == 0)
	{
		return E_SUCCESS;
	}
	// If there are already InfoServices, delete them 
	// ** @todo merge could be better... 
	if(m_addrInfoServices!=NULL)
	{
		for(UINT32 i=0;i<m_addrInfoServicesSize;i++)
		{
			delete m_addrInfoServices[i];
			m_addrInfoServices[i] = NULL;
		}
		delete[] m_addrInfoServices;
	}
	m_addrInfoServicesSize=0;
	m_addrInfoServices=NULL;

	UINT32 nrListenerInterfaces;
	m_addrInfoServices = new CAListenerInterface*[isList->getLength()];
	CAListenerInterface** isListenerInterfaces;
	for (UINT32 i = 0; i < isList->getLength(); i++)
	{
		//get ListenerInterfaces
		DOMElement* elemListenerInterfaces;
		getDOMChildByName(isList->item(i),CAListenerInterface::XML_ELEMENT_CONTAINER_NAME,elemListenerInterfaces,false);
		isListenerInterfaces = CAListenerInterface::getInstance(elemListenerInterfaces, nrListenerInterfaces);
		if (nrListenerInterfaces > 0)
		{
			// @todo Take more than one listener interface for a given IS... 
			m_addrInfoServices[m_addrInfoServicesSize] = isListenerInterfaces[0];
			m_addrInfoServicesSize++;
			for (UINT32 j = 1; j < nrListenerInterfaces; j++)
			{
				// the other interfaces are not needed...
				delete isListenerInterfaces[j];
				isListenerInterfaces[j] = NULL;
			}
		}
		delete isListenerInterfaces;
	}
	UINT8 tmpBuff[255];
	UINT32 tmpLen=255;
	if(getDOMElementValue(elemAllowReconfig,tmpBuff,&tmpLen)==E_SUCCESS)
	{
		m_bAcceptReconfiguration = (strcmp("True",(char*)tmpBuff) == 0);
	}

	return E_SUCCESS;
}

SINT32 CACmdLnOptions::setMinCascadeLength(DOMElement* elemGeneral)
{
	DOMElement* elemMinCascadeLength = NULL;
	if(elemGeneral == NULL) return E_UNKNOWN;
	ASSERT_GENERAL_OPTIONS_PARENT
		(elemGeneral->getNodeName(), OPTIONS_NODE_MIN_CASCADE_LENGTH);
	//Inserting the min. cascade length if given...
	getDOMChildByName
		(elemGeneral, OPTIONS_NODE_MIN_CASCADE_LENGTH, elemMinCascadeLength, false);
	if(elemMinCascadeLength != NULL)
	{
		appendMixInfo_internal(elemMinCascadeLength, WITH_SUBTREE);
	}
	return E_SUCCESS;
}

// ***************************************
// * ressource option setter function(s) *
// *         (delay options)             *
// ***************************************

SINT32 CACmdLnOptions::setRessourceOptions(DOMElement *elemRoot)
{
#if defined (DELAY_CHANNELS) ||defined(DELAY_USERS)||defined(DELAY_CHANNELS_LATENCY)
		///reads the parameters for the ressource limitation for last mix/first mix
		//this is at the moment:
		//<Ressources>
		//<UnlimitTraffic></UnlimitTraffic>    #Number of bytes/packets without resource limitation
		//<BytesPerIntervall></BytesPerIntervall>   #upper limit of number of bytes/packets which are processed per channel/per user per time intervall
		//<Intervall></Intervall>  #duration of one intervall in ms
		//<Latency></Latency> #minimum Latency per channel in ms
		//</Ressources>
		CAMsg::printMsg(LOG_INFO,"Loading Parameters for traffic shaping / resource limitation....\n");
		UINT32 u32 = 0;
		DOMElement *elemRessources=NULL;
		DOMElement *elem = NULL;

		if(elemRoot == NULL)
		{
			return E_UNKNOWN;
		}

		getDOMChildByName(elemRoot, OPTIONS_NODE_RESSOURCES, elemRessources,false);
		if(elemRessources!=NULL)
		{
#if defined (DELAY_CHANNELS) || defined(DELAY_USERS)
			if(	getDOMChildByName
					(elemRessources, OPTIONS_NODE_UNLIMIT_TRAFFIC, elem, false) == E_SUCCESS &&
				getDOMElementValue(elem, &u32) == E_SUCCESS )
			{
				m_u32DelayChannelUnlimitTraffic = u32;
			}
			if(	getDOMChildByName
					(elemRessources, OPTIONS_NODE_BYTES_PER_IVAL, elem, false) == E_SUCCESS &&
				getDOMElementValue(elem, &u32) == E_SUCCESS)
			{
				m_u32DelayChannelBucketGrow = u32;
			}
			if(	getDOMChildByName
					(elemRessources, OPTIONS_NODE_DELAY_IVAL, elem, false) == E_SUCCESS &&
				getDOMElementValue(elem, &u32) == E_SUCCESS)
			{
				m_u32DelayChannelBucketGrowIntervall = u32;
			}
#endif
#if defined (DELAY_CHANNELS_LATENCY)
			if(	getDOMChildByName
					(elemRessources, OPTIONS_NODE_LATENCY, elem, false) == E_SUCCESS &&
				getDOMElementValue(elem, &u32) == E_SUCCESS)
			{
				m_u32DelayChannelLatency = u32;
			}
#endif
		}
#endif
	return E_SUCCESS;
}


#endif


