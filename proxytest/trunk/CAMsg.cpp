#include "StdAfx.h"
#include "CAMsg.hpp"

CAMsg CAMsg::oMsg;

CAMsg::CAMsg()
    {
	isLog=false;
    }

CAMsg::~CAMsg()
    {
	if(isLog)
	    {
		closelog();
	    }	
    }
    
int CAMsg::setOptions(int options)
    {
	if(options&MSG_LOG)
	    {
		if(oMsg.isLog)
		    return 0;
		openlog("AnonMix",0,LOG_USER);
		oMsg.isLog=true;
	    }
	return 0;
    }

int CAMsg::printMsg(int type,char* format,...)
    {
	va_list ap;
	va_start(ap,format);
	if(oMsg.isLog)
	    {
		syslog(type,format,ap);
	    }
	else
	    {
		vprintf(format,ap);
	    }
	va_end(ap);
	return 0;
    }