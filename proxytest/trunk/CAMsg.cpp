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
#include "CAMsg.hpp"
#include "CACmdLnOptions.hpp"
#define FILENAME_ERRORLOG "/errors"
#define FILENAME_INFOLOG "/messages"

extern CACmdLnOptions options;

CAMsg CAMsg::oMsg;

CAMsg::CAMsg()
    {
			m_uLogType=MSG_STDOUT;
			m_hFileErr=m_hFileInfo=-1;
			m_strMsgBuff[0]='[';
			InitializeCriticalSection(&csPrint);
		}

CAMsg::~CAMsg()
    {
			closeLog();
			DeleteCriticalSection(&csPrint);
    }
  
char* CAMsg::m_strMsgTypes[4]={", error   ] ",", critical] ",", info    ] ",", debug   ] "}; //all same size!
#define STRMSGTYPES_SIZE 12

  
SINT32 CAMsg::setOptions(UINT32 opt)
    {
			if(oMsg.m_uLogType==opt)
					return E_SUCCESS;
			if(oMsg.openLog(opt)==E_SUCCESS)
				{
					oMsg.closeLog();
					oMsg.m_uLogType=opt;
					return E_SUCCESS;
				}
			return E_UNKNOWN;
    }

SINT32 CAMsg::printMsg(UINT32 type,char* format,...)
	{
		EnterCriticalSection(&oMsg.csPrint);
		va_list ap;
		va_start(ap,format);
		SINT32 ret=E_SUCCESS;

		//Date is: yyyy/mm/dd-hh:mm:ss   -- the size is: 19 
		printf("h1\n");
		time_t currtime=time(NULL);
		printf("h2\n");
		strftime(oMsg.m_strMsgBuff+1,255,"%Y/%m/%d-%H:%M:%S",localtime(&currtime));
		printf("h3\n");
		switch(type)
			{
				case LOG_DEBUG:
					strcat(oMsg.m_strMsgBuff,oMsg.m_strMsgTypes[3]);
				break;
				case LOG_INFO:
					strcat(oMsg.m_strMsgBuff,oMsg.m_strMsgTypes[2]);
				break;
				case LOG_CRIT:
					strcat(oMsg.m_strMsgBuff,oMsg.m_strMsgTypes[1]);
				break;
				case LOG_ERR:
					strcat(oMsg.m_strMsgBuff,oMsg.m_strMsgTypes[0]);
				break;
				default:
					va_end(ap);
					LeaveCriticalSection(&oMsg.csPrint);
					return E_UNKNOWN;
			}
#ifdef HAVE_VSNPRINTF
		vsnprintf(oMsg.m_strMsgBuff+20+STRMSGTYPES_SIZE,1024,format,ap);
#else
	  vsprintf(oMsg.m_strMsgBuff+20+STRMSGTYPES_SIZE,format,ap);
#endif
		va_end(ap);

		switch(oMsg.m_uLogType)
	    {
				case MSG_LOG:
#ifndef _WIN32
					syslog(type,oMsg.m_strMsgBuff);
#endif
				break;
				case MSG_FILE:
				/*	if(type==LOG_ERR||type==LOG_CRIT)
						{
							if(oMsg.m_hFileErr!=-1)
								{
									if(write(oMsg.m_hFileErr,oMsg.m_strMsgBuff,strlen(oMsg.m_strMsgBuff))==-1)
									 ret=E_UNKNOWN;
								}
						}
					else
						{
					*/		if(oMsg.m_hFileInfo!=-1)
								{
									if(write(oMsg.m_hFileInfo,oMsg.m_strMsgBuff,strlen(oMsg.m_strMsgBuff))==-1)
									 ret=E_UNKNOWN;
								}
					//	}
				break;
				case MSG_STDOUT:
					printf(oMsg.m_strMsgBuff);
				break;
				default:
				 ret=E_UNKNOWN;
			}
		LeaveCriticalSection(&oMsg.csPrint);
		return ret;
  }

SINT32 CAMsg::closeLog()
	{
		switch(m_uLogType)
			{
				case MSG_LOG:
					#ifndef _WIN32
						::closelog();
					#endif
				break;
				case MSG_FILE:
					if(m_hFileErr!=-1)
						close(m_hFileErr);
					m_hFileErr=-1;
					if(m_hFileInfo!=-1)
						close(m_hFileInfo);
					m_hFileInfo=-1;
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
					m_hFileErr=open(buff,O_APPEND|O_CREAT|O_WRONLY,S_IREAD|S_IWRITE);
					strcpy(buff,logdir);
					strcat(buff,FILENAME_INFOLOG);
					m_hFileInfo=open(buff,O_APPEND|O_CREAT|O_WRONLY,S_IREAD|S_IWRITE);										
				break;
			}
		return E_SUCCESS;
	}