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
#ifdef PAYMENT
#include "CAAccountingDBInterface.hpp"
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"

extern CACmdLnOptions options;

/**
 * Constructor
 */
CAAccountingDBInterface::CAAccountingDBInterface()
{
	m_bConnected = false;

  // Get database connection info from configfile and/or commandline
	UINT8 host[255];
	if(options.getDatabaseHost(host, 255) == E_UNKNOWN) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Host!");
		return;
	}
	UINT32 tcp_port = options.getDatabasePort();
	UINT8 dbName[255];
	if(options.getDatabaseName(dbName, 255) == E_UNKNOWN) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Name!");
		return;
	}
	UINT8 userName[255];
	if(options.getDatabaseUsername(userName, 255) == E_UNKNOWN) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Username!");
		return;
	}
	UINT8 password[255];
	if(options.getDatabasePassword(password, 255) == E_UNKNOWN) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Password!");
		return;
	}

	// connect to DB
	SINT32 dbStatus = initDBConnection(host, tcp_port, dbName, userName, password);
}


/**
 * Destructor
 */
CAAccountingDBInterface::~CAAccountingDBInterface()
{
	terminateDBConnection();	
}


/**
 * Initiates the database connection. 
 * This function is called inside the aiThread
 *
 * @return E_NOT_CONNECTED if the connection could not be established
 * @return E_UNKNOWN if we are already connected
 * @return E_SUCCESS if all is OK
 */
SINT32 CAAccountingDBInterface::initDBConnection(const UINT8 * host, UINT32 tcp_port,
																								 const UINT8 * dbName,
																								 const UINT8 * userName,
																								 const UINT8 * password)
{
	if(m_bConnected) return E_UNKNOWN;
	
	char port[20];
	sprintf(port, "%i", tcp_port);
	m_dbConn = PQsetdbLogin((char*)host, port, "", "",
													(char*)dbName,	(char*)userName,	(char*)password);
  if(PQstatus(m_dbConn) == CONNECTION_BAD) {
		
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInteface: Could not connect to Database. Reason: %s\n",
										PQerrorMessage(m_dbConn));
		PQfinish(m_dbConn);
		m_dbConn = 0;
		m_bConnected = false;
		return E_NOT_CONNECTED;
	}
	m_bConnected = true;
	return E_SUCCESS;
}


/**
 * Terminates the database connection
 * @return E_SUCCESS
 */
SINT32 CAAccountingDBInterface::terminateDBConnection() 
{
	if(m_bConnected) {
	  PQfinish(m_dbConn);
	}
	m_bConnected = false;
	return E_SUCCESS;
}


/**
 * Creates the tables we need in the DB
 *
 * @return E_SUCCESS if all is OK
 * @return E_UNKNOWN if the query could not be executed
 * @return E_NOT_CONNECTED if we are not connected to the DB
 */
SINT32 CAAccountingDBInterface::createTables()
{
	if(!m_bConnected) return E_NOT_CONNECTED;
	
	char createTable[] = 
		"CREATE TABLE COSTCONFIRMATIONS ("
			"ACCOUNTNUMBER BIGSERIAL PRIMARY KEY,"
			"BYTES BIGINT,"
			"XMLCC VARCHAR(2000),"
			"SETTLED INTEGER"
		");";
	
	PGresult * result;
	ExecStatusType resStatus;
	result = PQexec(m_dbConn, createTable);
	resStatus = PQresultStatus(result);
	if(resStatus!=PGRES_COMMAND_OK) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Could not create tables. Reason: %s\n", PQresultErrorMessage(result));
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}


/**
 * Drops (=deletes) the tables from the DB
 *
 * @return E_SUCCESS if all is OK
 * @return E_NOT_CONNECTED if we are not connected to the DB
 * @return E_UNKNOWN if the query could not be executed
 */
SINT32 CAAccountingDBInterface::dropTables()
{
	if(!m_bConnected) return E_NOT_CONNECTED;
	char dropTable[] = 
		"DROP TABLE COSTCONFIRMATIONS;";

	PGresult * result;
	result = PQexec(m_dbConn, dropTable);
	if(PQresultStatus(result)!=PGRES_COMMAND_OK) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Could not drop tables. Reason: %s\n",
											PQresultErrorMessage(result));
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}


/**
 * gets the latest cost confirmation stored for the given user account
 *
 * @return E_SUCCESS, if everything is OK
 * @return E_NOT_CONNECTED, if the DB query could not be executed
 * @return E_NOT_FOUND, if there was no XMLCC found
 * @return E_SPACE, if the buffer is too small. In this case len contains the
 * minimum number of bytes needed
 */
SINT32 CAAccountingDBInterface::getCostConfirmation(UINT64 accountNumber, UINT8 *buf, UINT32 *len)
{
	if(!m_bConnected) return E_NOT_CONNECTED;
	UINT8 queryF[] = "SELECT XMLCC FROM COSTCONFIRMATIONS WHERE ACCOUNTNUMBER=%i";
	UINT8 query[100+25];
	UINT8 * xmlCC;
	sprintf( (char *)query, (char *)queryF, accountNumber);

	PGresult * result;
	result = PQexec(m_dbConn, (char *)query);
	if(PQresultStatus(result)!=PGRES_TUPLES_OK) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Could not read XMLCC. Reason: %s\n", 
				PQresultErrorMessage(result));
		return E_NOT_CONNECTED;
	}
	if(PQntuples(result)!=0) {
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: XMLCC not found.\n");
		return E_NOT_FOUND;
	}
	xmlCC = (UINT8*) PQgetvalue(result, 0, 0);
	int reslen = strlen((char *)xmlCC);
	if(reslen>= *len) {
		*len = reslen;
		delete [] xmlCC;
		return E_SPACE;
	}
	strncpy((char*)buf, (char *)xmlCC, *len);
	delete [] xmlCC;
	return E_SUCCESS;
}


/**
 * stores a cost confirmation in the DB
 */
SINT32 CAAccountingDBInterface::storeCostConfirmation(
		UINT64 accountNumber, UINT64 bytes, 
		UINT8 * xmlCC, UINT32 isSettled) 
{
	if(!m_bConnected) return E_NOT_CONNECTED;
	
	// hack to see if there is already an entry
	if(getCostConfirmation(accountNumber, 0, 0)==E_NOT_FOUND) {
		char queryF[] = "INSERT INTO COSTCONFIRMATIONS VALUES %i, %i, %s, %d;";
		char query[100+2100];
		sprintf(query, queryF, accountNumber, bytes, xmlCC, isSettled);

		PGresult * result;
		result = PQexec(m_dbConn, query);
		if(PQresultStatus(result)!=PGRES_COMMAND_OK) {
			CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Could not write XMLCC(1). Reason: %s\n", 
						PQresultErrorMessage(result));
			return E_NOT_CONNECTED;
		}
	}
	
	else {
		char queryF[] = "UPDATE COSTCONFIRMATIONS SET BYTES=%i,XMLCC='%s',SETTLED=%d WHERE ACCOUNTNUMBER=%i;";
		char query[200+2100];
		sprintf(query, queryF, bytes, xmlCC, isSettled);

		PGresult * result;
		result = PQexec(m_dbConn, query);
		if(PQresultStatus(result) != PGRES_COMMAND_OK) {
			CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Could not write XMLCC(2). Reason: %s\n", 
						PQresultErrorMessage(result));
			return E_NOT_CONNECTED;
		}
	}
	return E_SUCCESS;
}

#endif
