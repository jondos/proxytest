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
    
SINT32 CAMsg::setOptions(UINT32 options)
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
			return E_SUCCESS;
    }

SINT32 CAMsg::printMsg(UINT32 type,char* format,...)
	{
		va_list ap;
		va_start(ap,format);
		if(oMsg.isLog)
	    {
				#ifndef _WIN32
				#ifdef HAVE_VSYSLOG
					vsyslog(type,format,ap);
				#else
					char buff[1024];
					vsnprintf(buff,1024,format,ap);
					syslog(type,buff);
				#endif	
				#endif
	    }
		else
	    {
				vprintf(format,ap);
	    }
		va_end(ap);
		return E_SUCCESS;
  }
