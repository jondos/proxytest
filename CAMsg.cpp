#include "StdAfx.h"
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#define FILENAME_ERRORLOG "/errors"
#define FILENAME_INFOLOG "/messages"

extern CACmdLnOptions options;

CAMsg CAMsg::oMsg;

CAMsg::CAMsg()
    {
			uLogType=MSG_STDOUT;
			hFileErr=hFileInfo=-1;
    }

CAMsg::~CAMsg()
    {
			closeLog();
    }
    
SINT32 CAMsg::setOptions(UINT32 opt)
    {
			if(oMsg.uLogType==opt)
					return E_SUCCESS;
			if(oMsg.openLog(opt)==E_SUCCESS)
				{
					oMsg.closeLog();
					oMsg.uLogType=opt;
					return E_SUCCESS;
				}
			return E_UNKNOWN;
    }

SINT32 CAMsg::printMsg(UINT32 type,char* format,...)
	{
		va_list ap;
		va_start(ap,format);
		SINT32 ret=E_SUCCESS;
		switch(oMsg.uLogType)
	    {
				case MSG_LOG:
					#ifndef _WIN32
						#ifdef HAVE_VSYSLOG
						 vsyslog(type,format,ap);
						#else
							char buff[1024];
							vsnprintf(buff,1024,format,ap);
							syslog(type,buff);
						#endif	
					#endif
				break;
				case MSG_FILE:
					if(type==LOG_ERR||type==LOG_CRIT)
						{
							if(oMsg.hFileErr!=-1)
								{
									char buff[1024];
									#ifndef _WIN32
										vsnprintf(buff,1024,format,ap);
									#else
										_vsnprintf(buff,1024,format,ap);
									#endif
									if(write(oMsg.hFileErr,buff,strlen(buff))==1)
									 ret=E_UNKNOWN;
								}
						}
					else
						{
							if(oMsg.hFileErr!=-1)
								{
									char buff[1024];
									#ifndef _WIN32
										vsnprintf(buff,1024,format,ap);
									#else
										_vsnprintf(buff,1024,format,ap);
									#endif
									if(write(oMsg.hFileInfo,buff,strlen(buff))==-1)
									 ret=E_UNKNOWN;
								}
						}
				break;
				case MSG_STDOUT:
					vprintf(format,ap);
				break;
				default:
				 ret=E_UNKNOWN;
			}
		va_end(ap);
		return ret;
  }

SINT32 CAMsg::closeLog()
	{
		switch(uLogType)
			{
				case MSG_LOG:
					#ifndef _WIN32
						::closelog();
					#endif
				break;
				case MSG_FILE:
					if(hFileErr!=-1)
						close(hFileErr);
					hFileErr=-1;
					if(hFileInfo!=-1)
						close(hFileInfo);
					hFileInfo=-1;
				break;
			}
		return E_SUCCESS;
	}

SINT32 CAMsg::openLog(UINT32 type)
	{
		switch(type)
			{
				case MSG_LOG:
					#ifndef _WIN32
						openlog("AnonMix",0,LOG_USER);
					#endif
				break;
				case MSG_FILE:
					char logdir[255];
					char buff[1024];
					if(options.getLogDir((UINT8*)logdir,255)!=E_SUCCESS)
						return E_UNKNOWN;
					strcpy(buff,logdir);
					strcat(buff,FILENAME_ERRORLOG);
					hFileErr=open(buff,O_APPEND|O_CREAT|O_WRONLY,S_IREAD|S_IWRITE);
					strcpy(buff,logdir);
					strcat(buff,FILENAME_INFOLOG);
					hFileInfo=open(buff,O_APPEND|O_CREAT|O_WRONLY,S_IREAD|S_IWRITE);										
				break;
			}
		return E_SUCCESS;
	}