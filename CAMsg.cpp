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
					#ifndef _WIN32
						::closelog();
					#endif
				}		
    }
    
int CAMsg::setOptions(int options)
    {
			if(options&MSG_LOG)
					{
						if(oMsg.isLog)
								return 0;
						#ifndef _WIN32
							openlog("AnonMix",0,LOG_USER);
						#endif
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
				#ifndef _WIN32
					syslog(type,format,ap);
				#endif
	    }
	else
	    {
		vprintf(format,ap);
	    }
	va_end(ap);
	return 0;
    }