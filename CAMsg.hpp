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
// TODO: Sylog for NT!!

#ifndef __CAMSG__
#define __CAMSG__

#define MSG_STDOUT	0x00
#define MSG_LOG			0x01
#define MSG_FILE		0x02

#ifdef _WIN32
	#define LOG_ERR		0
	#define LOG_CRIT	1
	#define LOG_INFO	2 
	#define LOG_DEBUG 3
#endif
#include "CAMutex.hpp"
class CAMsg
	{
		protected:
			CAMsg(); //Singleton!
			static CAMsg oMsg;
		public:
			~CAMsg();
			static SINT32 setOptions(UINT32 options);
			static SINT32 printMsg(UINT32 typ,char* format,...);
		protected:
			SINT32 openLog(UINT32 type);
			SINT32 closeLog();
			UINT32 m_uLogType;
			int m_hFileErr;
			int m_hFileInfo;
			char m_strMsgBuff[1050];
			static const char* const m_strMsgTypes[4];
			CAMutex m_csPrint;
   };
#endif
