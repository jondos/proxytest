#include "StdAfx.h"
#include "CACmdLnOptions.hpp"
#include "./popt/popt.h"
CACmdLnOptions::CACmdLnOptions()
  {
		bDaemon=false;
		bLocalProxy=bFirstMix=bLastMix=bMiddleMix=false;
		iTargetPort=iSOCKSPort=iServerPort=iSOCKSServerPort=iInfoServerPort-1;
		strTargetHost=strSOCKSHost=strInfoServerHost=NULL;
		strKeyFileName=strCascadeName=NULL;
  }

CACmdLnOptions::~CACmdLnOptions()
  {
		if(strTargetHost!=NULL)
			{
				delete strTargetHost;
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
  }
    
int CACmdLnOptions::parse(int argc,const char** argv)
    {
	//int ret;
	
	int iDaemon=0;
	char* target=NULL;
	int port=-1;
	int mix=-1;
	int SOCKSport=-1;
	char* socks=NULL;
	char* infoserver=NULL;
	char* keyfile=NULL;
	char* cascadename=NULL;
	poptOption options[]=
	 {
		{"daemon",'d',POPT_ARG_NONE,&iDaemon,0,"start as daemon",NULL},
		{"next",'n',POPT_ARG_STRING,&target,0,"next mix/http-proxy","<ip:port>"},
		{"port",'p',POPT_ARG_INT,&port,0,"listening port","<portnumber>"},
		{"mix",'m',POPT_ARG_INT,&mix,0,"local|first|middle|last mix","<0|1|2|3>"},
		{"socksport",'s',POPT_ARG_INT,&SOCKSport,0,"listening port for socks","<portnumber>"},
		{"socksproxy",'o',POPT_ARG_STRING,&socks,0,"socks proxy","<ip:port>"},
		{"infoserver",'i',POPT_ARG_STRING,&infoserver,0,"info server","<ip:port>"},
		{"key",'k',POPT_ARG_STRING,&keyfile,0,"sign key file","<file>"},
		{"name",'a',POPT_ARG_STRING,&cascadename,0,"name of the cascade","<string>"},
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
	iServerPort=port;
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
    
int CACmdLnOptions::getServerPort()
    {
	return iServerPort;
    }
    
int CACmdLnOptions::getSOCKSServerPort()
  {
		return iSOCKSServerPort;
  }

int CACmdLnOptions::getTargetPort()
    {
	return iTargetPort;
    }
    
int CACmdLnOptions::getTargetHost(char* host,int len)
  {
		if(strTargetHost==NULL)
				return -1;
		if(len<=(int)strlen(strTargetHost))
				{
					return -1;		
				}
		strcpy(host,strTargetHost);
		return (int)strlen(strTargetHost);
  }

int CACmdLnOptions::getSOCKSPort()
  {
		return iSOCKSPort;
  }
    
int CACmdLnOptions::getSOCKSHost(char* host,int len)
  {
		if(strSOCKSHost==NULL)
				return -1;
		if(len<=(int)strlen(strSOCKSHost))
				{
					return -1;		
				}
		strcpy(host,strSOCKSHost);
		return strlen(strSOCKSHost);
  }

int CACmdLnOptions::getInfoServerPort()
  {
		return iInfoServerPort;
  }
    
int CACmdLnOptions::getInfoServerHost(char* host,int len)
  {
		if(strInfoServerHost==NULL)
				return -1;
		if(len<=(int)strlen(strInfoServerHost))
				{
					return -1;		
				}
		strcpy(host,strInfoServerHost);
		return (int)strlen(strInfoServerHost);
  }

int CACmdLnOptions::getKeyFileName(char* filename,int len)
  {
		if(strKeyFileName==NULL)
				return -1;
		if(len<=(int)strlen(strKeyFileName))
				{
					return -1;		
				}
		strcpy(filename,strKeyFileName);
		return (int)strlen(strKeyFileName);
  }

int CACmdLnOptions::getCascadeName(char* name,int len)
  {
		if(strCascadeName==NULL)
				return -1;
		if(len<=(int)strlen(strCascadeName))
				{
					return -1;		
				}
		strcpy(name,strCascadeName);
		return (int)strlen(strCascadeName);
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
