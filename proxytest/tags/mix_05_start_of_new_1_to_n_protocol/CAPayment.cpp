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
