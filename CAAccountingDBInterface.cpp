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

	// connect to DB
	//SINT32 dbStatus = initDBConnection(host, tcp_port, dbName, userName, password);
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
SINT32 CAAccountingDBInterface::initDBConnection()
{
	if(m_bConnected) return E_UNKNOWN;
	
  // Get database connection info from configfile and/or commandline
	UINT8 host[255];
	if(options.getDatabaseHost(host, 255) == E_UNKNOWN) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Host!");
		return E_UNKNOWN;
	}
	UINT32 tcp_port = options.getDatabasePort();
	UINT8 dbName[255];
	if(options.getDatabaseName(dbName, 255) == E_UNKNOWN) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Name!");
		return E_UNKNOWN;
	}
	UINT8 userName[255];
	if(options.getDatabaseUsername(userName, 255) == E_UNKNOWN) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Username!");
		return E_UNKNOWN;
	}
	UINT8 password[255];
	if(options.getDatabasePassword(password, 255) == E_UNKNOWN) {
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Password!");
		return E_UNKNOWN;
	}	
	
	char port[20];
	sprintf(port, "%i", tcp_port);
	m_dbConn = PQsetdbLogin(
			(char*)host, port, "", "",
			(char*)dbName, (char*)userName, (char*)password
		);
  if(PQstatus(m_dbConn) == CONNECTION_BAD) 
	{
		CAMsg::printMsg(
				LOG_ERR, "CAAccountingDBInteface: Could not connect to Database. Reason: %s\n",
				PQerrorMessage(m_dbConn)
			);
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
 * gets the latest cost confirmation stored for the given user account
 *
 * @return E_SUCCESS, if everything is OK
 * @return E_NOT_CONNECTED, if the DB query could not be executed
 * @return E_NOT_FOUND, if there was no XMLCC found
 * @return E_SPACE, if the buffer is too small. In this case len contains the
 * minimum number of bytes needed
 */
SINT32 CAAccountingDBInterface::getCostConfirmation(UINT64 accountNumber, CAXMLCostConfirmation *pCC)
{
	if(!m_bConnected) return E_NOT_CONNECTED;
	UINT8 queryF[] = "SELECT XMLCC FROM COSTCONFIRMATIONS WHERE ACCOUNTNUMBER=%lld";
	UINT8 query[128];
	UINT8 * xmlCC;
	PGresult * result;
//	UINT32 reslen;
	
	sprintf( (char *)query, (char *)queryF, accountNumber);
	#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: executing query %s\n", query);
	#endif
	result = PQexec(m_dbConn, (char *)query);
	if(PQresultStatus(result)!=PGRES_TUPLES_OK) 
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Could not read XMLCC. Reason: %s\n", 
				PQresultErrorMessage(result));
		return E_UNKNOWN;
	}
	if(PQntuples(result)!=1) 
	{
		#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: XMLCC not found.\n");
		#endif
		return E_NOT_FOUND;
	}
	
	xmlCC = (UINT8*) PQgetvalue(result, 0, 0);
	pCC = new CAXMLCostConfirmation(xmlCC);
	return E_SUCCESS;
}





/**
 * stores a cost confirmation in the DB
 * @todo optimize - maybe do check and insert/update in one step??
 */
SINT32 CAAccountingDBInterface::storeCostConfirmation( CAXMLCostConfirmation &cc )
{
	#ifndef HAVE_NATIVE_UINT64
		#warning Native UINT64 type not available - CostConfirmation Database might be non-functional
	#endif
	UINT8 query1F[] = "SELECT COUNT(*) FROM COSTCONFIRMATIONS WHERE ACCOUNTNUMBER=%lld";
	UINT8 query2F[] = "INSERT INTO COSTCONFIRMATIONS VALUES (%lld, %lld, '%s', %d)";
	UINT8 query3F[] = 
		"UPDATE COSTCONFIRMATIONS SET BYTES=%lld, XMLCC='%s', SETTLED=%d "
		"WHERE ACCOUNTNUMBER=%lld";
	UINT8 * query;
	UINT8 * pStrCC;
	UINT32 size;
	PGresult * pResult;
		
	if(!m_bConnected) return E_NOT_CONNECTED;
	pStrCC = cc.toXmlString(size);
	
	// Test: is there already an entry with this accountno.?
	query = new UINT8[ strlen((char*)query1F) + strlen((char*)pStrCC) + 128 ];
	sprintf( (char*)query, (char*)query1F, cc.getAccountNumber());
	
	// to receive result in binary format...
	pResult = PQexec(m_dbConn, (char*)query);
	if(PQresultStatus(pResult) != PGRES_TUPLES_OK)
	{
		CAMsg::printMsg(
				LOG_ERR, 
				"Database Error '%s' while processing query '%s'\n", 
				PQresultErrorMessage(pResult), query
			);
		delete[] pStrCC;
		delete[] query;
		return E_UNKNOWN;
	}
	if ( (PQntuples(pResult) != 1) ||
				(PQgetisnull(pResult, 0, 0)) )
	{
		CAMsg::printMsg(LOG_ERR, "DBInterface: Wrong number of tuples or null value\n");
		delete[] pStrCC;
		delete[] query;
		return E_UNKNOWN;
	}
	
	// put query together (either insert or update)
	UINT8* pVal = (UINT8*)PQgetvalue(pResult, 0, 0);
	CAMsg::printMsg(LOG_DEBUG, "DB store -> pVal ist %s, atoi gibt %d\n", pVal, atoi((char*)pVal));
	if(atoi( (char*)pVal ) == 0)
		{
			sprintf( // do insert
					(char*)query, (char*)query2F, 
					cc.getAccountNumber(), cc.getTransferredBytes(), 
					pStrCC, 0);
		}
	else
		{
			sprintf( // do update
					(char*)query, (char*) query3F,
					cc.getTransferredBytes(), pStrCC, 0, cc.getAccountNumber()
				);
		}
	
	// issue query..
	pResult = PQexec(m_dbConn, (char*)query);
	delete[] pStrCC;
	if(PQresultStatus(pResult) != PGRES_COMMAND_OK)
	{
		CAMsg::printMsg(
				LOG_ERR, 
				"Database Error '%s' while processing query '%s'\n", 
				PQresultErrorMessage(pResult), query
			);
		delete[] query;	
		return E_UNKNOWN;
	}
	delete[] query;	
	return E_SUCCESS;
}


/**
	* Fills the CAQueue with all non-settled cost confirmations
	*
	*/
SINT32 CAAccountingDBInterface::getUnsettledCostConfirmations(CAQueue &q)
{
	UINT8 query[] = "SELECT XMLCC FROM COSTCONFIRMATIONS WHERE SETTLED=0";
	PGresult * result;
	SINT32 numTuples, i;
	UINT8 * pTmpStr;
	CAXMLCostConfirmation * pCC;
	
	if(!m_bConnected) return E_NOT_CONNECTED;
	
	result = PQexec(m_dbConn, (char *)query);
	if(PQresultStatus(result) != PGRES_TUPLES_OK)
	{
		return E_UNKNOWN;
	}
	numTuples = PQntuples(result);
	for(i=0; i<numTuples; i++)
	{
		pTmpStr = (UINT8*)PQgetvalue(result, i, 0);
		pCC = new CAXMLCostConfirmation( pTmpStr );
		q.add(&pCC, sizeof(CAXMLCostConfirmation *));
	}
	
	return E_SUCCESS;
}

/**
	* Marks this account as settled.
	* @todo what to do if there was a new CC stored while we were busy settling the old one?
	*/
SINT32 CAAccountingDBInterface::markAsSettled(UINT64 accountNumber)
{
	UINT8 queryF[] = "UPDATE COSTCONFIRMATIONS SET SETTLED=1 WHERE ACCOUNTNUMBER=%lld";
	UINT8 * query;
	PGresult * result;
	
	if(!m_bConnected) return E_NOT_CONNECTED;
	query = new UINT8[strlen((char*)queryF)+32];
	sprintf((char *)query, (char*)queryF, accountNumber);
	result = PQexec(m_dbConn, (char *)query);
	delete[] query;
	if(PQresultStatus(result) != PGRES_TUPLES_OK)
	{
		return E_UNKNOWN;
	}
	
	return E_SUCCESS;
}


#endif
