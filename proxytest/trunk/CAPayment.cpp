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
#ifdef PAYMENT_SUPPORT
#include "CAPayment.hpp"

#define PAYMENT_DB_NAME "PaymentDB"

SINT32 CAPayment::init(UINT8* host,SINT32 port,UINT8* user,UINT8* passwd)
	{
		if(m_mysqlCon!=NULL)
			return E_UNKNOWN;
		m_mysqlCon=new MYSQL;
		mysql_init(m_mysqlCon);
		MYSQL* tmp=NULL;
		if(port<0)
			{
				tmp=mysql_real_connect(m_mysqlCon,NULL,(char*)user,(char*)passwd,PAYMENT_DB_NAME,0,(char*)host,0);
			}
		else
			{
				tmp=mysql_real_connect(m_mysqlCon,(char*)host,(char*)user,(char*)passwd,PAYMENT_DB_NAME,0,NULL,0);
			}
		if(tmp==NULL)
			{
				my_thread_end();
				mysql_close(m_mysqlCon);
				delete m_mysqlCon;
				m_mysqlCon=NULL;
				return E_UNKNOWN;
			}
		return E_SUCCESS;
	}

SINT32 CAPayment::checkAccess(UINT8* code,UINT8 codeLen,UINT32* accessUntil)
	{
		char query[1024];
		sprintf(query,"SELECT accessUntil FROM PaymentTable WHERE code=\"%s\" LIMIT 1",(char*)code);
		int ret=mysql_query(m_mysqlCon,query);
		if(ret!=0)
			return E_UNKNOWN;
		MYSQL_RES* result=mysql_store_result(m_mysqlCon);
		if(result==NULL)
			return E_UNKNOWN;
		MYSQL_ROW row=mysql_fetch_row(result);
		if(row==NULL)
			{
				mysql_free_result(result);
				return E_UNKNOWN;
			}
		*accessUntil=atol(row[0]);
		mysql_free_result(result);
		return E_SUCCESS;
	}
#endif
