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
#include "CAMsg.hpp"
#include "CASocketAddrINet.hpp"

CACmdLnOptions::CACmdLnOptions()
  {
		bDaemon=m_bHttps=false;
		bLocalProxy=bFirstMix=bLastMix=bMiddleMix=false;
		iTargetPort=iSOCKSPort=iServerPort=iSOCKSServerPort=iInfoServerPort=0xFFFF;
		iTargetRTTPort=iServerRTTPort=-1;
		strServerHost=strTargetHost=strSOCKSHost=strInfoServerHost=NULL;
		m_strMixXml=m_strUser=strCascadeName=strLogDir=NULL;
		pTargets=NULL;
		cntTargets=0;
		m_strMixID=NULL;
		m_pSignKey=NULL;
		m_pOwnCertificate=NULL;
		m_pPrevMixCertificate=NULL;
		m_pNextMixCertificate=NULL;
  }
CACmdLnOptions::~CACmdLnOptions()
	{
		clean();
	}

void CACmdLnOptions::clean()
  {
		if(strTargetHost!=NULL)
			{
				delete[] strTargetHost;
	    }
		if(strServerHost!=NULL)
			{
				delete[] strServerHost;
	    }
		if(strSOCKSHost!=NULL)
			{
				delete[] strSOCKSHost;
	    }
		if(strInfoServerHost!=NULL)
			{
				delete[] strInfoServerHost;
	    }
		if(strCascadeName!=NULL)
			delete[] strCascadeName;
		if(strLogDir!=NULL)
			delete[] strLogDir;
		if(m_strUser!=NULL)
			delete[] m_strUser;
		if(m_strMixXml!=NULL)
			delete[] m_strMixXml;
		if(m_strMixID!=NULL)
			delete[] m_strMixID;
		if(pTargets!=NULL)
			{
				delete[] pTargets;
			}
		if(m_pSignKey!=NULL)
			delete m_pSignKey;
		if(m_pOwnCertificate!=NULL)
			delete m_pOwnCertificate;
		if(m_pNextMixCertificate!=NULL)
			delete m_pNextMixCertificate;
		if(m_pPrevMixCertificate!=NULL)
			delete m_pPrevMixCertificate;
  }
    
int CACmdLnOptions::parse(int argc,const char** argv)
    {
	//int ret;
	
	int iDaemon=0;
	int iTemplate=0;
  int bHttps=0;
  char* target=NULL;
	int serverrttport=-1;
	int mix=-1;
	int SOCKSport=-1;
	char* socks=NULL;
	char* infoserver=NULL;
	char* certsdir=NULL;
	char* cascadename=NULL;
	char* logdir=NULL;
	char* serverPort=NULL;
	char* user=NULL;
	char* configfile=NULL;
  int bXmlKey=0;

	poptOption options[]=
	 {
		{"daemon",'d',POPT_ARG_NONE,&iDaemon,0,"start as daemon",NULL},
		{"next",'n',POPT_ARG_STRING,&target,0,"next mix/http-proxy(s)","<path|{ip:port[,rttport][;ip:port]*}>"},
		{"port",'p',POPT_ARG_STRING,&serverPort,0,"listening [host:]port|path","<[host:]port|path>"},
		{"https",'h',POPT_ARG_NONE,&bHttps,0,"support proxy requests",NULL},
		{"rttport",'r',POPT_ARG_INT,&serverrttport,0,"round trip time port","<portnumber>"},
		{"mix",'m',POPT_ARG_INT,&mix,0,"local|first|middle|last mix","<0|1|2|3>"},
		{"socksport",'s',POPT_ARG_INT,&SOCKSport,0,"listening port for socks","<portnumber>"},
		{"socksproxy",'o',POPT_ARG_STRING,&socks,0,"socks proxy","<ip:port>"},
		{"infoserver",'i',POPT_ARG_STRING,&infoserver,0,"info server","<ip:port>"},
		{"certs",'e',POPT_ARG_STRING,&certsdir,0,"certs and key directory which the files: own.pfx (or privkey.xml), next.cer, prev.cer","<dir>"},
		{"xmlkey",'x',POPT_ARG_NONE,&bXmlKey,0,"sign key is in XML-Format",NULL},
		{"name",'a',POPT_ARG_STRING,&cascadename,0,"name of the cascade","<string>"},
		{"logdir",'l',POPT_ARG_STRING,&logdir,0,"directory where log files go to","<dir>"},
		{"user",'u',POPT_ARG_STRING,&user,0,"effective user","<user>"},
		{"template",'t',POPT_ARG_NONE,&iTemplate,0,"generate conf template and exit",NULL},
		{"config",'c',POPT_ARG_STRING,&configfile,0,"config file to use","<file>"},
		POPT_AUTOHELP
		{NULL,0,0,
		NULL,0,NULL,NULL}
	};
	poptContext ctx=poptGetContext(NULL,argc,argv,options,0);
	int ret=poptGetNextOpt(ctx);
	while(ret==POPT_ERROR_BADOPT)
		ret=poptGetNextOpt(ctx);
	poptFreeContext(ctx);
	if(iTemplate!=0)
		{
			generateTemplate();
			exit(0);
		}
	if(configfile!=NULL)
		{
			int handle;
			handle=open(configfile,O_BINARY|O_RDONLY);
			if(handle==-1)
				CAMsg::printMsg(LOG_CRIT,"Couldt not read config file: %s\n",configfile);
			else
				{
					SINT32 len=filelength(handle);
					m_strMixXml=new char[len+50]; //some space for the id, which is the only thing, which is changed....
					read(handle,m_strMixXml,len);
					m_strMixXml[len]=0;
					close(handle);
				}
			free(configfile);
		}
	if(iDaemon==0)
	    bDaemon=false;
	else
	    bDaemon=true;
  if(bHttps==0)
  	m_bHttps=false;
  else
  	m_bHttps=true;
  if(target!=NULL)
	    {
				char* tmpStr;
				if(target[0]=='/') //Unix Domain Sockaet
				 {
					strTargetHost=new char[strlen(target)+1];
					strcpy(strTargetHost,target);
				 }
				else
					{
						cntTargets=1;
						UINT32 i;
						for(i=0;i<strlen(target);i++)
							{
								if(target[i]==';')
									cntTargets++;
							}
						pTargets=new CASocketAddrINet[cntTargets];
						tmpStr=strtok(target,";");
						char tmpHostname[255];
						int tmpPort;
						i=0;
						while(tmpStr!=NULL)
							{
								char* tmpStr1=strchr(tmpStr,':');
								if(tmpStr1!=NULL)
									{
										memcpy(tmpHostname,tmpStr,tmpStr1-tmpStr);
										tmpHostname[tmpStr1-tmpStr]=0;
										tmpPort=(int)atol(tmpStr1+1);
									}
								else
									{//TODO what if not in right form ?
										//try if it is a number --> use it as port
										//and use 'localhost' as traget-host 
										tmpPort=(int)atol(tmpStr);
										if(tmpPort!=0) //we get it
											{
												strcpy(tmpHostname,"localhost");
											}
										else //we try to use it as host and use the default port
											{
#define DEFAULT_TARGET_PORT 6544
												tmpPort=DEFAULT_TARGET_PORT;
												strcpy(tmpHostname,tmpStr);
											}
									}
								pTargets[i].setAddr((UINT8*)tmpHostname,tmpPort);
								if(i==0)
									{
										strTargetHost=new char[strlen(tmpHostname)+1];
										strcpy(strTargetHost,tmpHostname);
										iTargetPort=tmpPort;
									}
								i++;
								tmpStr=strtok(NULL,";");
							}
					}
				free(target);
	    }
	if(socks!=NULL)
	    {
				char* tmpStr;
				if((tmpStr=strchr(socks,':'))!=NULL)
						{
					strSOCKSHost=new char[tmpStr-socks+1];
					(*tmpStr)=0;
					strcpy(strSOCKSHost,socks);
					iSOCKSPort=(int)atol(tmpStr+1);
						}
				free(socks);	
	    }
	if(infoserver!=NULL)
	    {
				char* tmpStr;
				if((tmpStr=strchr(infoserver,':'))!=NULL)
						{
					strInfoServerHost=new char[tmpStr-infoserver+1];
					(*tmpStr)=0;
					strcpy(strInfoServerHost,infoserver);
					iInfoServerPort=(int)atol(tmpStr+1);
						}
				free(infoserver);	
	    }
	if(cascadename!=NULL)
	    {
					strCascadeName=new char[strlen(cascadename)+1];
					strcpy(strCascadeName,cascadename);
					free(cascadename);	
	    }
	if(logdir!=NULL)
	    {
					strLogDir=new char[strlen(logdir)+1];
					strcpy(strLogDir,logdir);
					free(logdir);	
	    }
	if(user!=NULL)
	    {
					m_strUser=new char[strlen(user)+1];
					strcpy(m_strUser,user);
					free(user);	
	    }
	if(serverPort!=NULL)
		{
			char* tmpStr;
			if(serverPort[0]=='/') //Unix Domain Socket
			 {
				strServerHost=new char[strlen(serverPort)+1];
				strcpy(strServerHost,serverPort);
			 }
			else if((tmpStr=strchr(serverPort,':'))!=NULL) //host:port
				{
					strServerHost=new char[tmpStr-serverPort+1];
					(*tmpStr)=0;
					strcpy(strServerHost,serverPort);
					iServerPort=(int)atol(tmpStr+1);
				}
			else //port only ?
				{
					if(strServerHost!=NULL)
						delete strServerHost;
					strServerHost=NULL;
					iServerPort=(int)atol(serverPort);
				}
			free(serverPort);
		}
	if(serverrttport!=-1)
		iServerRTTPort=serverrttport;
	iSOCKSServerPort=SOCKSport;
	if(mix==0)
		bLocalProxy=true;
	else if(mix==1)
		bFirstMix=true;
	else if(mix==2)
		bMiddleMix=true;
	else 
		bLastMix=true;
	if(m_strMixXml!=NULL) //we should insert the right Id and other stuff...
		{
			UINT8 id[50];
			getMixId(id,50);
			char* pos=strstr(m_strMixXml,"id=\"\"");
			if(pos!=NULL)
				{
					pos+=4;
					pos=strins(m_strMixXml,pos,(char*)id);
					delete m_strMixXml;
					m_strMixXml=pos;
					/*UINT32 left=strlen(m_strMixXml)+m_strMixXml-pos+1;
					memmove(pos+strlen((char*)id),pos,left);
					memcpy(pos,id,strlen((char*)id));*/
				}
			else
				{
					CAMsg::printMsg(LOG_CRIT,"Placeholder for Mix-Id not found in Mix conf file\n");
					delete m_strMixXml;
					m_strMixXml=NULL;
				}
			//Insert Version
			pos=strstr(m_strMixXml,"</Version>");
			if(pos!=NULL)
				{
					pos=strins(m_strMixXml,pos,MIX_VERSION);					
					delete m_strMixXml;
					m_strMixXml=pos;
				}
			else
				CAMsg::printMsg(LOG_CRIT,"Software <Version> not set\n");
		}
	if(m_strMixXml==NULL) //Ok the get an error in proccesing infos about the mix, we should at leas t give the id!
		{
			m_strMixXml=new char[255];
			strcpy((char*)m_strMixXml,"<?xml version=\"1.0\"?><Mix id=\"");
			UINT8 id[50];
			getMixId(id,50);
			strcat((char*)m_strMixXml,(char*)id);
			strcat((char*)m_strMixXml,"\"></Mix>");
		}

	UINT8 tmpCertDir[2048];
	UINT8 tmpFileName[2048];
	UINT8* buff=NULL;
	UINT32 size;
	if(certsdir!=NULL)
		{
			strcpy((char*)tmpCertDir,certsdir);
			free(certsdir);				
		}
	else
		tmpCertDir[0]=0;
	//Try to load SignKey
	if(bXmlKey)
		{
			strcpy((char*)tmpFileName,(char*)tmpCertDir);
			strcat((char*)tmpFileName,"/privkey.xml");
			buff=readFile(tmpFileName,&size);
			if(buff!=NULL)
				{
					m_pSignKey=new CASignature();
					m_pSignKey->setSignKey(buff,size,SIGKEY_XML);
				}
		}
	else
		{
			strcpy((char*)tmpFileName,(char*)tmpCertDir);
			strcat((char*)tmpFileName,"/own.pfx");
			UINT8* buff=readFile(tmpFileName,&size);
			if(buff!=NULL)
				{
					m_pSignKey=new CASignature();
					UINT8 passwd[500];
					passwd[0]=0;
					if(m_pSignKey->setSignKey(buff,size,SIGKEY_PKCS12)!=E_SUCCESS)
						{//Maybe not an empty passwd
							printf("I need a passwd for the SignKey: ");
							scanf("%s",(char*)passwd); //This is a typicall Buffer Overflow :-)
							if(m_pSignKey->setSignKey(buff,size,SIGKEY_PKCS12,(char*)passwd)!=E_SUCCESS)
								{
									delete m_pSignKey;
									m_pSignKey=NULL;
								}
						}
					m_pOwnCertificate=CACertificate::decode(buff,size,CERT_PKCS12,(char*)passwd);
				}
		}
	delete buff;
	//Try to load Certificates
	strcpy((char*)tmpFileName,(char*)tmpCertDir);
	strcat((char*)tmpFileName,"/next.cer");
	buff=readFile(tmpFileName,&size);
	m_pNextMixCertificate=CACertificate::decode(buff,size,CERT_DER);
	delete buff;
	strcpy((char*)tmpFileName,(char*)tmpCertDir);
	strcat((char*)tmpFileName,"/prev.cer");
	buff=readFile(tmpFileName,&size);
	m_pPrevMixCertificate=CACertificate::decode(buff,size,CERT_DER);
	delete buff;

	return E_SUCCESS;
	
 }

bool CACmdLnOptions::getDaemon()
	{
		return bDaemon;
  }

bool CACmdLnOptions::getProxySupport()
	{
  	return m_bHttps;
  }
UINT16 CACmdLnOptions::getServerPort()
  {
		return iServerPort;
  }
    
SINT32 CACmdLnOptions::getServerHost(UINT8* path,UINT32 len)
	{
		if(path==NULL||strServerHost==NULL||len<=strlen(strServerHost))
			return E_UNKNOWN;
		strcpy((char*)path,strServerHost);
		return E_SUCCESS;
	}

SINT32 CACmdLnOptions::getMixId(UINT8* id,UINT32 len)
	{
		if(len<24) //we need 24 chars (including final \0)
			return E_UNKNOWN;
		if(m_strMixID==NULL)
			{
				UINT8 buff[4];
				UINT16 thePort=iServerPort;
				if(strServerHost==NULL||strServerHost[0]=='/')
					{
						CASocketAddrINet::getLocalHostIP(buff);
						if(thePort==0xFFFF)
							getRandom((UINT8*)&thePort,2);
					}
				else
					{
						CASocketAddrINet oAddr;
						oAddr.setAddr((UINT8*)strServerHost,0);
						oAddr.getIP(buff);
					}
				m_strMixID=new char[24];
				sprintf(m_strMixID,"%u.%u.%u.%u%%3A%u",buff[0],buff[1],buff[2],buff[3],thePort);
			}
		strcpy((char*)id,m_strMixID);		
		return E_SUCCESS;
	}

SINT32 CACmdLnOptions::getServerRTTPort()
  {
		if(iServerRTTPort!=-1)
			return iServerRTTPort;
		else
			return E_UNSPECIFIED;
  }

UINT16 CACmdLnOptions::getSOCKSServerPort()
  {
		return iSOCKSServerPort;
  }

UINT16 CACmdLnOptions::getTargetPort()
	{
		return iTargetPort;
  }
    
SINT32 CACmdLnOptions::getTargetRTTPort()
  {
		if(iTargetRTTPort!=-1)
			return iTargetRTTPort;
		else
			return E_UNSPECIFIED;
  }

SINT32 CACmdLnOptions::getTargetHost(UINT8* host,UINT32 len)
  {
		if(strTargetHost==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(strTargetHost))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)host,strTargetHost);
		return E_SUCCESS;
  }

UINT16 CACmdLnOptions::getSOCKSPort()
  {
		return iSOCKSPort;
  }
    
SINT32 CACmdLnOptions::getSOCKSHost(UINT8* host,UINT32 len)
  {
		if(strSOCKSHost==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(strSOCKSHost))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)host,strSOCKSHost);
		return (SINT32)strlen(strSOCKSHost);
  }

UINT16 CACmdLnOptions::getInfoServerPort()
  {
		return iInfoServerPort;
  }
    
SINT32 CACmdLnOptions::getInfoServerHost(UINT8* host,UINT32 len)
  {
		if(strInfoServerHost==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(strInfoServerHost))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)host,strInfoServerHost);
		return E_SUCCESS;
  }

SINT32 CACmdLnOptions::getCascadeName(UINT8* name,UINT32 len)
  {
		if(strCascadeName==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(strCascadeName))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)name,strCascadeName);
		return E_SUCCESS;
  }

SINT32 CACmdLnOptions::getLogDir(UINT8* name,UINT32 len)
  {
		if(strLogDir==NULL||name==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(strLogDir))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)name,strLogDir);
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
		return bFirstMix;
  }

bool CACmdLnOptions::isMiddleMix()
    {
			return bMiddleMix;
    }

bool CACmdLnOptions::isLastMix()
    {
			return bLastMix;
    }

bool CACmdLnOptions::isLocalProxy()
    {
			return bLocalProxy;
    }

SINT32 CACmdLnOptions::getMixXml(UINT8* strxml,UINT32* len)
	{
		UINT32 strMixXmlLen=strlen(m_strMixXml);
		if(strxml==NULL||m_strMixXml==NULL||len==NULL||*len<strMixXmlLen)
			return E_UNKNOWN;
		memcpy(strxml,m_strMixXml,strMixXmlLen);
		*len=strMixXmlLen;
		return E_SUCCESS;
	}

SINT32 CACmdLnOptions::generateTemplate()
	{
#define XML_CONFIG_TEMPLATE "<?xml version=\"1.0\" ?>\
<Mix id=\"\">\
\t<Name><!-- Insert a human readable name here--></Name>\
\t<Location> <!-- Fill out the following Elements to give infomation about the location of the Mix-->\
\t\t<State><!-- Insert the State of the Mix-Location here--></State>\
\t\t<City><!-- Insert the City of the Mix-Location here--></City>\
\t\t<Position> <!-- Insert the Position of the Mix here-->\
\t\t\t<Geo> <!-- Insert the Geographical Position of the Mix here-->\
\t\t\t\t<Longitude> <!-- Insert the Longitude of the Geographical Position of the Mix here--> </Longitude>\
\t\t\t\t<Latitude> <!-- Insert the Latitude of the Geographical Position of the Mix here--> </Latitude>\
\t\t\t\t<Altitude> <!-- Insert the Altitude of the Geographical Position of the Mix here--> </Altitude>\
\t\t\t</Geo>\
\t\t</Position>\
\t</Location>\
\t<Software> <!-- This gives information about the Mix-Software used-->\
\t\t<Version> <!-- The Version of the Software used - automatically set! --> </Version>\
\t</Software>\
</Mix>"

		int handle;
		handle=open("mix_template.conf",O_BINARY|O_RDWR|O_CREAT,S_IREAD | S_IWRITE);
		if(handle==-1)
			return E_UNKNOWN;
		write(handle,XML_CONFIG_TEMPLATE,strlen(XML_CONFIG_TEMPLATE));
		close(handle);
		return E_SUCCESS;
	}
