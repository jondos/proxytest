#include "StdAfx.h"
#include "CACmdLnOptions.hpp"
CACmdLnOptions::CACmdLnOptions()
  {
		bDaemon=false;
		bLocalProxy=bFirstMix=bLastMix=bMiddleMix=false;
		iTargetPort=iSOCKSPort=iServerPort=iSOCKSServerPort=-1;
		strTargetHost=strSOCKSHost=NULL;
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
  }
    
int CACmdLnOptions::parse(int argc,const char** argv)
    {
	int ret;
	poptOption options[7];
	memset(options,0,sizeof(options));
	
	options[0].shortName='d';
	options[0].argInfo=POPT_ARG_NONE;
	int iDaemon=0;
	options[0].arg=&iDaemon;
	
	options[1].shortName='n';
	options[1].argInfo=POPT_ARG_STRING;
	char* target=NULL;
//	memset(target,0,sizeof(target));
	options[1].arg=&target;

	options[2].shortName='p';
	options[2].argInfo=POPT_ARG_INT;
	int port=-1;
//	memset(target,0,sizeof(target));
	options[2].arg=&port;

	options[3].shortName='m';
	options[3].argInfo=POPT_ARG_INT;
	int mix=-1;
//	memset(target,0,sizeof(target));
	options[3].arg=&mix;

	options[4].shortName='s';
	options[4].argInfo=POPT_ARG_INT;
	int SOCKSport=-1;
//	memset(target,0,sizeof(target));
	options[4].arg=&SOCKSport;

	options[5].shortName='o';
	options[5].argInfo=POPT_ARG_STRING;
	char* socks=NULL;
//	memset(target,0,sizeof(target));
	options[5].arg=&socks;

	poptContext ctx=poptGetContext(NULL,argc,argv,options,0);
	ret=poptGetNextOpt(ctx);
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
					iTargetPort=atol(tmpStr+1);
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
					iSOCKSPort=atol(tmpStr+1);
						}
				free(socks);	
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
					return strlen(strTargetHost)+1;		
				}
		strcpy(host,strTargetHost);
		return strlen(strTargetHost);
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
					return strlen(strSOCKSHost)+1;		
				}
		strcpy(host,strSOCKSHost);
		return strlen(strSOCKSHost);
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
