#include "StdAfx.h"
#include "CACmdLnOptions.hpp"
CACmdLnOptions::CACmdLnOptions()
    {
	bDaemon=false;
	bFirstMix=bLastMix=bMiddleMix=false;
	iTargetPort=iServerPort=-1;
	strTargetHost=NULL;
    }
CACmdLnOptions::~CACmdLnOptions()
    {
	if(strTargetHost!=NULL)
	    {
		delete strTargetHost;
	    }
    }
    
int CACmdLnOptions::parse(int argc,const char** argv)
    {
	int ret;
	poptOption options[5];
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
	iServerPort=port;
	if(mix==0)
		bFirstMix=true;
	else if(mix==1)
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
