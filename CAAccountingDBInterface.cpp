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
		m_dbConn=NULL;
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
		if(m_bConnected) 
			return E_UNKNOWN;
		
		// Get database connection info from configfile and/or commandline
		UINT8 host[255];
		if(options.getDatabaseHost(host, 255) != E_SUCCESS) 
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Host!\n");
				return E_UNKNOWN;
			}
		UINT32 tcp_port = options.getDatabasePort();
		UINT8 dbName[255];
		if(options.getDatabaseName(dbName, 255) != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Name!\n");
				return E_UNKNOWN;
			}
		UINT8 userName[255];
		if(options.getDatabaseUsername(userName, 255) != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Username!\n");
				return E_UNKNOWN;
			}
		UINT8 password[255];
		if(options.getDatabasePassword(password, 255) != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Password!\n");
				return E_UNKNOWN;
			}

		
		char port[20];
		sprintf(port, "%i", tcp_port);
		m_dbConn = PQsetdbLogin(
				(char*)host, port, "", "",
				(char*)dbName, (char*)userName, (char*)password
			);
		if(m_dbConn==NULL||PQstatus(m_dbConn) == CONNECTION_BAD) 
			{
				CAMsg::printMsg(
					LOG_ERR, "CAAccountingDBInteface: Could not connect to Database. Reason: %s\n",
					PQerrorMessage(m_dbConn)
				);
				PQfinish(m_dbConn);
				m_dbConn = NULL;
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
		if(m_bConnected) 
			{
				PQfinish(m_dbConn);
			}
		m_dbConn=NULL;
		m_bConnected = false;
		return E_SUCCESS;
	}

/**
 * Gets the latest cost confirmation stored for the given user account.
 *
 * @param accountNumber the account for which the cost confirmation is requested
 * @param pCC on return contains a pointer to the Cost confirmation (the caller is responsible for deleting this object),
 * NULL in case of an error 
 *
 * @retval E_SUCCESS, if everything is OK
 * @retval E_NOT_CONNECTED, if the DB query could not be executed
 * @retval E_NOT_FOUND, if there was no XMLCC found
 * @retval E_UNKOWN in case of a general error
 */
SINT32 CAAccountingDBInterface::getCostConfirmation(UINT64 accountNumber, CAXMLCostConfirmation **pCC)
	{
		if(!m_bConnected) 
			return E_NOT_CONNECTED;
		
		const char* queryF = "SELECT XMLCC FROM COSTCONFIRMATIONS WHERE ACCOUNTNUMBER=%s";
		UINT8 query[128];
		UINT8* xmlCC;
		PGresult* result;
	
		UINT8 tmp[32];
		print64(tmp,accountNumber);
		sprintf( (char *)query, queryF, tmp);
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: executing query %s\n", query);
		#endif
		result = PQexec(m_dbConn, (char *)query);
		if(PQresultStatus(result)!=PGRES_TUPLES_OK) 
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Could not read XMLCC. Reason: %s\n", 
				PQresultErrorMessage(result));
				PQclear(result);
				return E_UNKNOWN;
			}
		
		if(PQntuples(result)!=1) 
			{
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: XMLCC not found.\n");
				#endif
				PQclear(result);
				return E_NOT_FOUND;
			}
	
		xmlCC = (UINT8*) PQgetvalue(result, 0, 0);
		*pCC = CAXMLCostConfirmation::getInstance(xmlCC,strlen((char*)xmlCC));
		PQclear(result);
		if(*pCC==NULL)
			return E_UNKNOWN;
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
		const char* query1F = "SELECT COUNT(*) FROM COSTCONFIRMATIONS WHERE ACCOUNTNUMBER=%s";
		const char* query2F = "INSERT INTO COSTCONFIRMATIONS VALUES (%s, %s, '%s', %d)";
		const char* query3F = "UPDATE COSTCONFIRMATIONS SET BYTES=%s, XMLCC='%s', SETTLED=%d WHERE ACCOUNTNUMBER=%s";
	
		UINT8 * query;
		UINT8 * pStrCC;
		UINT32 size;
		PGresult * pResult;
		
		if(!m_bConnected) 
			return E_NOT_CONNECTED;
		
		pStrCC = new UINT8[8192];
		size=8192;
		if(cc.toXMLString(pStrCC,&size)!=E_SUCCESS)
			{
				delete[] pStrCC;
				return E_UNKNOWN;
			}

		// Test: is there already an entry with this accountno.?
		query = new UINT8[ strlen(query1F) + size + 128 ];
		UINT8 strAccountNumber[32];
		print64(strAccountNumber,cc.getAccountNumber());
		sprintf( (char*)query, query1F, strAccountNumber);
	
		// to receive result in binary format...
		pResult = PQexec(m_dbConn, (char*)query);
		if(PQresultStatus(pResult) != PGRES_TUPLES_OK)
			{
				CAMsg::printMsg(LOG_ERR, 
												"Database Error '%s' while processing query '%s'\n", 
												PQresultErrorMessage(pResult), query
												);
				delete[] pStrCC;
				delete[] query;
				PQclear(pResult);
				return E_UNKNOWN;
			}
		
		if ( (PQntuples(pResult) != 1) ||
					(PQgetisnull(pResult, 0, 0)) )
			{
				CAMsg::printMsg(LOG_ERR, "DBInterface: Wrong number of tuples or null value\n");
				delete[] pStrCC;
				delete[] query;
				PQclear(pResult);
				return E_UNKNOWN;
			}
	
		// put query together (either insert or update)
		UINT8* pVal = (UINT8*)PQgetvalue(pResult, 0, 0);
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "DB store -> pVal ist %s, atoi gibt %i\n", pVal, atoi((char*)pVal));
		#endif
		if(atoi( (char*)pVal ) == 0)
			{
				UINT8 tmp2[32];
				print64(tmp2,cc.getTransferredBytes());
				sprintf( // do insert
					(char*)query, query2F, 
					strAccountNumber, tmp2, 
					pStrCC, 0);
			}
		else
			{
				UINT8 tmp2[32];
				print64(tmp2,cc.getTransferredBytes());
				sprintf( // do update
					(char*)query, (char*) query3F,
					tmp2, pStrCC, 0, strAccountNumber
				);
			}
		PQclear(pResult);
	
		// issue query..
		pResult = PQexec(m_dbConn, (char*)query);
		delete[] pStrCC;
		if(PQresultStatus(pResult) != PGRES_COMMAND_OK)
			{
				CAMsg::printMsg(LOG_ERR, 
												"Database Error '%s' while processing query '%s'\n", 
												PQresultErrorMessage(pResult), query
												);
				delete[] query;	
				PQclear(pResult);
				return E_UNKNOWN;
			}
		delete[] query;	
		PQclear(pResult);
		return E_SUCCESS;
	}


/**
	* Fills the CAQueue with pointer to all non-settled cost confirmations. The caller is responsible for deleating this cost confirmations.
	* @retval E_NOT_CONNECTED if a connection to the DB could not be established
	* @retval E_UNKNOWN in case of a general error
	* @retval E_SUCCESS if the information from the databse could be retrieved successful. Not that this does not 
	* mean that the number of returned unsettled cost confirmations is greater than zero.
	*
	*/
SINT32 CAAccountingDBInterface::getUnsettledCostConfirmations(CAQueue& q)
	{
		const char* query= "SELECT XMLCC FROM COSTCONFIRMATIONS WHERE SETTLED=0";
		PGresult* result;
		SINT32 numTuples, i;
		UINT8* pTmpStr;
		CAXMLCostConfirmation* pCC;
		
		if(!m_bConnected) 
			return E_NOT_CONNECTED;
		
		result = PQexec(m_dbConn, (char *)query);
		if(PQresultStatus(result) != PGRES_TUPLES_OK)
			{
				PQclear(result);
				return E_UNKNOWN;
			}
		numTuples = PQntuples(result);
		for(i=0; i<numTuples; i++)
			{
				pTmpStr = (UINT8*)PQgetvalue(result, i, 0);
				if( (pTmpStr!=NULL)&&
						( (pCC = CAXMLCostConfirmation::getInstance(pTmpStr,strlen((char*)pTmpStr)) )!=NULL))
					{
						q.add(&pCC, sizeof(CAXMLCostConfirmation *));
					}
			}		
		PQclear(result);
		return E_SUCCESS;
	}

/**
	* Marks this account as settled.
	* @todo what to do if there was a new CC stored while we were busy settling the old one?
	* @retval E_NOT_CONNECTED if a connection to the DB could not be established
	* @retval E_UNKNOWN in case of a general error
	*/
SINT32 CAAccountingDBInterface::markAsSettled(UINT64 accountNumber)
	{
		const char* queryF = "UPDATE COSTCONFIRMATIONS SET SETTLED=1 WHERE ACCOUNTNUMBER=%s";
		UINT8 * query;
		PGresult * result;
		
		if(!m_bConnected) 
			return E_NOT_CONNECTED;
		UINT8 tmp[32];
		print64(tmp,accountNumber);
		query = new UINT8[strlen(queryF)+32];
		sprintf((char *)query, queryF, tmp);
		result = PQexec(m_dbConn, (char *)query);
		delete[] query;
		if(PQresultStatus(result) != PGRES_COMMAND_OK)
			{
				PQclear(result);
				return E_UNKNOWN;
			}
		PQclear(result);		
		return E_SUCCESS;
	}
#endif
