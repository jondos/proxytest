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
    
SINT32 CACmdLnOptions::getTargetHost(UINT8* host,UINT32 len)
  {
		if(strTargetHost==NULL)
				return E_UNKNOWN;
		if(len<=(int)strlen(strTargetHost))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)host,strTargetHost);
		return (SINT32)strlen(strTargetHost);
  }

UINT16 CACmdLnOptions::getSOCKSPort()
  {
		return iSOCKSPort;
  }
    
SINT32 CACmdLnOptions::getSOCKSHost(UINT8* host,UINT32 len)
  {
		if(strSOCKSHost==NULL)
				return E_UNKNOWN;
		if(len<=(int)strlen(strSOCKSHost))
				{
					return E_UNKOWN;		
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
		if(len<=(int)strlen(strInfoServerHost))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)host,strInfoServerHost);
		return (SINT32)strlen(strInfoServerHost);
  }

SINT32 CACmdLnOptions::getKeyFileName(UINT8* filename,UINT32 len)
  {
		if(strKeyFileName==NULL)
				return E_UNKNOWN;
		if(len<=(int)strlen(strKeyFileName))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)filename,strKeyFileName);
		return (SINT32)strlen(strKeyFileName);
  }

SINT32 CACmdLnOptions::getCascadeName(UINT8* name,UINT32 len)
  {
		if(strCascadeName==NULL)
				return E_UNKNOWN;
		if(len<=(int)strlen(strCascadeName))
				{
					return E_UNKNOWN;		
				}
		strcpy((char*)name,strCascadeName);
		return (SINT32)strlen(strCascadeName);
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
