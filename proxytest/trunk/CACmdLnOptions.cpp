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
CACmdLnOptions::CACmdLnOptions()
  {
		bDaemon=m_bHttps=false;
		bLocalProxy=bFirstMix=bLastMix=bMiddleMix=false;
		iTargetPort=iSOCKSPort=iServerPort=iSOCKSServerPort=iInfoServerPort=0xFFFF;
		iTargetRTTPort=iServerRTTPort=-1;
		strServerPath=strTargetHost=strSOCKSHost=strInfoServerHost=NULL;
		strKeyFileName=strCascadeName=strLogDir=NULL;
  }

CACmdLnOptions::~CACmdLnOptions()
  {
		if(strTargetHost!=NULL)
			{
				delete strTargetHost;
	    }
		if(strServerPath!=NULL)
			{
				delete strServerPath;
	    }
		if(strSOCKSHost!=NULL)
			{
				delete strSOCKSHost;
	    }
		if(strInfoServerHost!=NULL)
			{
				delete strInfoServerHost;
	    }
		if(strKeyFileName!=NULL)
			delete strKeyFileName;
		if(strCascadeName!=NULL)
			delete strCascadeName;
		if(strLogDir!=NULL)
			delete strLogDir;
  }
    
int CACmdLnOptions::parse(int argc,const char** argv)
    {
	//int ret;
	
	int iDaemon=0;
  int bHttps=0;
  char* target=NULL;
	int port=-1;
	int serverrttport=-1;
	int mix=-1;
	int SOCKSport=-1;
	char* socks=NULL;
	char* infoserver=NULL;
	char* keyfile=NULL;
	char* cascadename=NULL;
	char* logdir=NULL;
	char* serverPort=NULL;
	poptOption options[]=
	 {
		{"daemon",'d',POPT_ARG_NONE,&iDaemon,0,"start as daemon",NULL},
		{"next",'n',POPT_ARG_STRING,&target,0,"next mix/http-proxy","<ip:port[,rttport]>"},
		{"port",'p',POPT_ARG_STRING,&serverPort,0,"listening port|path","<portnumber|path>"},
		{"https",'h',POPT_ARG_NONE,&bHttps,0,"support proxy requests",NULL},
		{"rttport",'r',POPT_ARG_INT,&serverrttport,0,"round trip time port","<portnumber>"},
		{"mix",'m',POPT_ARG_INT,&mix,0,"local|first|middle|last mix","<0|1|2|3>"},
		{"socksport",'s',POPT_ARG_INT,&SOCKSport,0,"listening port for socks","<portnumber>"},
		{"socksproxy",'o',POPT_ARG_STRING,&socks,0,"socks proxy","<ip:port>"},
		{"infoserver",'i',POPT_ARG_STRING,&infoserver,0,"info server","<ip:port>"},
		{"key",'k',POPT_ARG_STRING,&keyfile,0,"sign key file","<file>"},
		{"name",'a',POPT_ARG_STRING,&cascadename,0,"name of the cascade","<string>"},
		{"logdir",'l',POPT_ARG_STRING,&logdir,0,"directory where log files go to","<dir>"},
		POPT_AUTOHELP
		{NULL,0,0,
		NULL,0,NULL,NULL}
	};
	poptContext ctx=poptGetContext(NULL,argc,argv,options,0);
	poptGetNextOpt(ctx);
	poptFreeContext(ctx);
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
				if((tmpStr=strchr(target,':'))!=NULL)
					{
						strTargetHost=new char[tmpStr-target+1];
						(*tmpStr)=0;
						strcpy(strTargetHost,target);
						iTargetPort=(int)atol(tmpStr+1);
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
	if(keyfile!=NULL)
	    {
					strKeyFileName=new char[strlen(keyfile)+1];
					strcpy(strKeyFileName,keyfile);
					free(keyfile);	
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
	if(serverPort!=NULL)
		{
			if(serverPort[0]!='/')
				{
					iServerPort=atol(serverPort);
				}
			else
				{
					strServerPath=new char[strlen(serverPort)];
					strcpy(strServerPath,serverPort);
					free(serverPort);
				}
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
	return 0;
	
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
    
SINT32 CACmdLnOptions::getServerPath(UINT8* path,UINT32 len)
	{
		if(path==NULL||strServerPath==NULL||len<=strlen(strServerPath))
			return E_UNKNOWN;
		strcpy((char*)path,strServerPath);
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

SINT32 CACmdLnOptions::getKeyFileName(UINT8* filename,UINT32 len)
  {
		if(strKeyFileName==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(strKeyFileName))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)filename,strKeyFileName);
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
		if(strLogDir==NULL)
				return E_UNKNOWN;
		if(len<=(UINT32)strlen(strLogDir))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)name,strLogDir);
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
