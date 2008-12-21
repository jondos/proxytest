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
#include "CAXMLErrorMessage.hpp"
#include "CAMsg.hpp"
#include "CAStatusManager.hpp"

extern CACmdLnOptions* pglobalOptions;

CAConditionVariable *CAAccountingDBInterface::ms_pConnectionAvailable = NULL;
volatile UINT64 CAAccountingDBInterface::ms_threadWaitNr = 0;
volatile UINT64 CAAccountingDBInterface::ms_nextThreadNr = 0;
CAAccountingDBInterface *CAAccountingDBInterface::ms_pDBConnectionPool[MAX_DB_CONNECTIONS];

/**
 * Constructor
 */
CAAccountingDBInterface::CAAccountingDBInterface()
	{
		m_bConnected = false;
		m_dbConn=NULL;/* The owner of the connection */
		m_owner = 0;
		/* indicates whether this connection is not owned by a thread.
		 * (There is no reliable value of m_owner to indicate this).
		 */
		m_free = true;
		/* to ensure atomic access to m_owner and m_free */
		m_pConnectionMutex = new CAMutex();
	}


/**
 * Destructor
 */
CAAccountingDBInterface::~CAAccountingDBInterface()
	{
		terminateDBConnection();
		delete m_pConnectionMutex;
		m_pConnectionMutex = NULL;
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
		{
			return E_UNKNOWN;
		}
		
		// Get database connection info from configfile and/or commandline
		UINT8 host[255];
		if(pglobalOptions->getDatabaseHost(host, 255) != E_SUCCESS) 
		{
			CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Host!\n");
			return E_UNKNOWN;
		}
		UINT32 tcp_port = pglobalOptions->getDatabasePort();
		UINT8 dbName[255];
		if(pglobalOptions->getDatabaseName(dbName, 255) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Name!\n");
			return E_UNKNOWN;
		}
		UINT8 userName[255];
		if(pglobalOptions->getDatabaseUsername(userName, 255) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Error, no Database Username!\n");
			return E_UNKNOWN;
		}
		UINT8 password[255];
		if(pglobalOptions->getDatabasePassword(password, 255) != E_SUCCESS)
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
				LOG_ERR, "CAAccountingDBInterface: Could not connect to Database. Reason: %s\n",
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

bool CAAccountingDBInterface::checkConnectionStatus()
{
	if (!m_bConnected)
	{
		return false;
	}
	
	if (PQstatus(m_dbConn) != CONNECTION_OK) 
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Connection to database lost! Reason: %s\n", 
			PQerrorMessage(m_dbConn));	
    	PQreset(m_dbConn);
    	
      	if (PQstatus(m_dbConn) != CONNECTION_OK) 
      	{
      		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Could not reset database connection! Reason: %s\n", 
      			PQerrorMessage(m_dbConn));	
			terminateDBConnection();
      	}
      	else
      	{
      		CAMsg::printMsg(LOG_INFO, "CAAccountingDBInterface: Database connection has been reset successfully!");
      	}
	}
	return m_bConnected;
}

bool CAAccountingDBInterface::isDBConnected()
{
	return m_bConnected;
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


bool CAAccountingDBInterface::checkOwner()
{
	bool isFree = false;
	pthread_t owner = 0;
	m_pConnectionMutex->lock();
	owner = m_owner;
	isFree = m_free;
	m_pConnectionMutex->unlock();
	
	return ( (owner==pthread_self()) && !isFree);
}

/* Thread safe DB Query method */
SINT32 CAAccountingDBInterface::getCostConfirmation(UINT64 accountNumber, UINT8* cascadeId, CAXMLCostConfirmation **pCC, bool& a_bSettled)
{
	if(checkOwner())
	{
		return __getCostConfirmation(accountNumber, cascadeId, pCC, a_bSettled);
	}
	else
	{
		return E_UNKNOWN;
	}
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
SINT32 CAAccountingDBInterface::__getCostConfirmation(UINT64 accountNumber, UINT8* cascadeId, CAXMLCostConfirmation **pCC, bool& a_bSettled)
	{
		if(!checkConnectionStatus()) 
		{
			/* We assume, that the DB Connection was obtained by CAAccountingDBInterface::getConnnection
			 *  which shall return an established DBConnection. Therefore this is an error event 
			 */
			MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
			return E_NOT_CONNECTED;
		}
		
		const char* queryF = "SELECT XMLCC, SETTLED FROM COSTCONFIRMATIONS WHERE ACCOUNTNUMBER=%s AND CASCADE='%s'";
		UINT8* query;
		UINT8* xmlCC;
		PGresult* result;
		UINT8 tmp[32];
		print64(tmp,accountNumber);

		query = new UINT8[strlen(queryF) + 32 + strlen((char*)cascadeId)];
		sprintf( (char *)query, queryF, tmp, cascadeId);
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: executing query %s\n", query);
		#endif
		
		result = monitored_PQexec(m_dbConn, (char *)query);
		
		delete[] query;
		query = NULL;
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
		if (atoi(PQgetvalue(result, 0, 1)) == 0)
		{
			a_bSettled = false;
		}
		else
		{
			a_bSettled = true;
		}
		
		*pCC = CAXMLCostConfirmation::getInstance(xmlCC,strlen((char*)xmlCC));
		PQclear(result);

		if(*pCC==NULL)
		{
			return E_UNKNOWN;
		}
		return E_SUCCESS;
	}

SINT32 CAAccountingDBInterface::checkCountAllQuery(UINT8* a_query, UINT32& r_count)
{
	if(checkOwner())
	{
		return __checkCountAllQuery(a_query, r_count);
	}
	else
	{
		return E_UNKNOWN;
	}
}

SINT32 CAAccountingDBInterface::__checkCountAllQuery(UINT8* a_query, UINT32& r_count)
{
	r_count = 0;
	
	if (!a_query)
	{		
		return E_UNKNOWN;
	}
	/*
	if(!checkConnectionStatus()) 
	{
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
		return E_NOT_CONNECTED;
	}
	MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
	*/
	PGresult * pResult = monitored_PQexec(m_dbConn, (char*)a_query);
	
	if(PQresultStatus(pResult) != PGRES_TUPLES_OK)
	{
		CAMsg::printMsg(LOG_ERR, 
						"CAAccountingDBInterface: Database Error '%s' while processing query '%s'\n", 
						PQresultErrorMessage(pResult), a_query
						);
		PQclear(pResult);
		return E_UNKNOWN;
	}
	
	if ( (PQntuples(pResult) != 1) || (PQgetisnull(pResult, 0, 0)))
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Wrong number of tuples or null value\n");
		PQclear(pResult);
		return E_UNKNOWN;
	}
	
	r_count = atoi(PQgetvalue(pResult, 0, 0));
	PQclear(pResult);
	return E_SUCCESS;
}


/* Thread safe DB Query method */
SINT32 CAAccountingDBInterface::storeCostConfirmation(CAXMLCostConfirmation &cc, UINT8* ccCascade)
{
	if(checkOwner())
	{
		return __storeCostConfirmation(cc, ccCascade);
	}
	else
	{
		return E_UNKNOWN;
	}
}
/**
 * stores a cost confirmation in the DB
 * @todo optimize - maybe do check and insert/update in one step??
 */
SINT32 CAAccountingDBInterface::__storeCostConfirmation( CAXMLCostConfirmation &cc, UINT8* ccCascade )
	{			
		#ifndef HAVE_NATIVE_UINT64
			#warning Native UINT64 type not available - CostConfirmation Database might be non-functional
		#endif
		const char* previousCCQuery = "SELECT COUNT(*) FROM COSTCONFIRMATIONS WHERE ACCOUNTNUMBER='%s' AND CASCADE='%s'";
		const char* query2F =         "INSERT INTO COSTCONFIRMATIONS(BYTES, XMLCC, SETTLED, ACCOUNTNUMBER, CASCADE) VALUES ('%s', '%s', '%d', '%s', '%s')";
	 	const char* query3F =         "UPDATE COSTCONFIRMATIONS SET BYTES='%s', XMLCC='%s', SETTLED='%d' WHERE ACCOUNTNUMBER='%s' AND CASCADE='%s'";
	 	const char* tempQuery;
	
		UINT8 * query;
		UINT8 * pStrCC;
		UINT32 size;
		UINT32 len;
		UINT32 count;
		PGresult * pResult;
		UINT8 strAccountNumber[32];
		UINT8 tmp[32];
				
		if(!checkConnectionStatus()) 
		{
			MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
			return E_NOT_CONNECTED;
		}
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
		
		pStrCC = new UINT8[8192];
		size=8192;
		if(cc.toXMLString(pStrCC, &size)!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstanceDBInterface: Could not transform CC to XML string!\n");
			delete[] pStrCC;
			pStrCC = NULL;
			return E_UNKNOWN;
		}
		
#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "cc to store in  db:%s\n",pStrCC);	  		
#endif  

		// Test: is there already an entry with this accountno. for the same cascade?		
		len = max(strlen(previousCCQuery), strlen(query2F));
		len = max(len, strlen(query3F));
		query = new UINT8[len + 32 + 32 + 1 + size + strlen((char*)ccCascade)];
		print64(strAccountNumber,cc.getAccountNumber());
		sprintf( (char*)query, previousCCQuery, strAccountNumber, ccCascade);
	
		// to receive result in binary format...
		if (__checkCountAllQuery(query, count) != E_SUCCESS)
		{
			delete[] pStrCC;
			pStrCC = NULL;
			delete[] query;
			query = NULL;
			return E_UNKNOWN;
		}
	
		// put query together (either insert or update)
		print64(tmp,cc.getTransferredBytes());		
		if(count == 0)
		{			
			tempQuery = query2F; // do insert
		}
		else
		{
			tempQuery = query3F; // do update
		}
		sprintf((char*)query, tempQuery, tmp, pStrCC, 0, strAccountNumber, ccCascade);
	
		// issue query..
		pResult = monitored_PQexec(m_dbConn, (char*)query);
		
		delete[] pStrCC;
		pStrCC = NULL;
		if(PQresultStatus(pResult) != PGRES_COMMAND_OK) // || PQntuples(pResult) != 1)
		{
			CAMsg::printMsg(LOG_ERR, "Could not store CC!\n");
			//if (PQresultStatus(pResult) != PGRES_COMMAND_OK)
			{
				CAMsg::printMsg(LOG_ERR, 
								"Database message '%s' while processing query '%s'\n", 
								PQresultErrorMessage(pResult), query
								);
			}
			delete[] query;
			query = NULL;	
			PQclear(pResult);
			return E_UNKNOWN;
		}
		delete[] query;
		query = NULL;	
		PQclear(pResult);		

		#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstanceDBInterface: Finished storing CC in DB.\n");
		#endif
		return E_SUCCESS;
	}

SINT32 CAAccountingDBInterface::getUnsettledCostConfirmations(CAQueue &q, UINT8* cascadeId)
{
	if(checkOwner())
	{
		return __getUnsettledCostConfirmations(q, cascadeId);
	}
	else
	{
		return E_UNKNOWN;
	}
}
/**
	* Fills the CAQueue with pointer to all non-settled cost confirmations. The caller is responsible for deleating this cost confirmations.
	* @retval E_NOT_CONNECTED if a connection to the DB could not be established
	* @retval E_UNKNOWN in case of a general error
	* @retval E_SUCCESS : database operation completed successfully (but queue might still be empty if there were no unsettled cost confirmation to find).
	* Param: cascadeId : String identifier of a cascade for which to return the cost confirmations (concatenated hashes of all the cascade's mixes' price certs) 
	* 
	*/
SINT32 CAAccountingDBInterface::__getUnsettledCostConfirmations(CAQueue& q, UINT8* cascadeId)
	{
		const char* query= "SELECT XMLCC FROM COSTCONFIRMATIONS WHERE SETTLED=0 AND CASCADE = '%s' ";
		UINT8* finalQuery;
		PGresult* result;
		SINT32 numTuples, i;
		UINT8* pTmpStr;
		CAXMLCostConfirmation* pCC;

		if(!checkConnectionStatus()) 
		{
			MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
			return E_NOT_CONNECTED;
		}
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);

		finalQuery = new UINT8[strlen(query)+strlen((char*)cascadeId)];
		sprintf( (char*)finalQuery, query, cascadeId);

#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "Getting Cost confirmations for cascade: ");
		CAMsg::printMsg(LOG_DEBUG, (const char*) finalQuery);
#endif
		
		result = monitored_PQexec(m_dbConn, (char *)finalQuery);
		
		delete[] finalQuery;
		finalQuery = NULL;
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

		//CAMsg::printMsg(LOG_DEBUG, "Stop get unsettled CC\n");
		return E_SUCCESS;
	}

SINT32 CAAccountingDBInterface::markAsSettled(UINT64 accountNumber, UINT8* cascadeId, UINT64 a_transferredBytes)
{
	if(checkOwner())
	{
		return __markAsSettled(accountNumber, cascadeId, a_transferredBytes);
	}
	else
	{
		return E_UNKNOWN;
	}
}
/**
	* Marks this account as settled.
	* @todo what to do if there was a new CC stored while we were busy settling the old one?
	* @retval E_NOT_CONNECTED if a connection to the DB could not be established
	* @retval E_UNKNOWN in case of a general error
	*/
SINT32 CAAccountingDBInterface::__markAsSettled(UINT64 accountNumber, UINT8* cascadeId, UINT64 a_transferredBytes)
	{
		const char* queryF = "UPDATE COSTCONFIRMATIONS SET SETTLED=1 WHERE ACCOUNTNUMBER=%s AND BYTES=%s AND CASCADE='%s'";
		UINT8 * query;
		PGresult * result;
		
		if(!checkConnectionStatus()) 
		{
			MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
			return E_NOT_CONNECTED;
		}
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);

		UINT8 tmp[32], tmp2[32];
		print64(tmp,accountNumber);
		print64(tmp2,a_transferredBytes);
		query = new UINT8[strlen(queryF) + 32 + 32 + strlen((char*)cascadeId)];
		sprintf((char *)query, queryF, tmp, tmp2, cascadeId);
		result = monitored_PQexec(m_dbConn, (char *)query);
		
		delete[] query;
		query = NULL;
		if(PQresultStatus(result) != PGRES_COMMAND_OK)
			{
				PQclear(result);
				return E_UNKNOWN;
			}
		PQclear(result);	

		return E_SUCCESS;
	}

SINT32 CAAccountingDBInterface::deleteCC(UINT64 accountNumber, UINT8* cascadeId)
{
	if(checkOwner())
	{
		return __deleteCC(accountNumber, cascadeId);
	}
	else
	{
		return E_UNKNOWN;
	}
}

SINT32 CAAccountingDBInterface::__deleteCC(UINT64 accountNumber, UINT8* cascadeId)
{
	const char* deleteQuery = "DELETE FROM COSTCONFIRMATIONS WHERE ACCOUNTNUMBER = %s AND CASCADE='%s'";
	UINT8* finalQuery;
	PGresult* result;
	SINT32 ret;
	UINT8 temp[32];
	print64(temp,accountNumber);
	
	if (!checkConnectionStatus())
	{
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
		ret = E_NOT_CONNECTED;	
	}
	else
	{						
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
		
		finalQuery = new UINT8[strlen(deleteQuery)+ 32 + strlen((char*)cascadeId)];
		sprintf((char *)finalQuery,deleteQuery,temp, cascadeId);
		result = monitored_PQexec(m_dbConn, (char*)finalQuery);
		
		//CAMsg::printMsg(LOG_DEBUG, "%s\n",finalQuery);
		delete[] finalQuery;
		finalQuery = NULL;
		if (PQresultStatus(result) != PGRES_COMMAND_OK)
		{
			PQclear(result);
			ret = E_UNKNOWN;
		}		
		else
		{
			PQclear(result);
			ret = E_SUCCESS;
		}
	}
	
	if (ret == E_SUCCESS)
	{
		CAMsg::printMsg(LOG_INFO, "CAAccountingDBInterface: Costconfirmation for account %s was deleted!\n", temp);
	}
	else
	{	
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Could not delete account %s!\n", temp);
	}	

	return ret;
}	

SINT32 CAAccountingDBInterface::storePrepaidAmount(UINT64 accountNumber, SINT32 prepaidBytes, UINT8* cascadeId)
{
	if(checkOwner())
	{
		return __storePrepaidAmount(accountNumber, prepaidBytes, cascadeId);
	}
	else
	{
		return E_UNKNOWN;
	}
}
/*
 *  When terminating a connection, store the amount of bytes that the JAP account has already paid for, but not used 
 */	
SINT32 CAAccountingDBInterface::__storePrepaidAmount(UINT64 accountNumber, SINT32 prepaidBytes, UINT8* cascadeId)
{
	const char* selectQuery = "SELECT COUNT(*) FROM PREPAIDAMOUNTS WHERE ACCOUNTNUMBER=%s AND CASCADE='%s'";
	const char* insertQuery = "INSERT INTO PREPAIDAMOUNTS(PREPAIDBYTES, ACCOUNTNUMBER, CASCADE) VALUES (%d, %s, '%s')";
	const char* updateQuery = "UPDATE PREPAIDAMOUNTS SET PREPAIDBYTES=%d WHERE ACCOUNTNUMBER=%s AND CASCADE='%s'";
	//const char* deleteQuery = "DELETE FROM PREPAIDAMOUNTS WHERE ACCOUNTNUMBER = %s AND CASCADE='%s'";
	const char* query;
	
	PGresult* result;
	UINT8* finalQuery;
	UINT8 tmp[32];
	UINT32 len;
	UINT32 count;
	print64(tmp,accountNumber);

	if(!checkConnectionStatus()) 
	{
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
		return E_NOT_CONNECTED;
	}
	
	MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
	
	len = max(strlen(selectQuery), strlen(insertQuery));
	len = max(len, strlen(updateQuery));
	finalQuery = new UINT8[len + 32 + 32 + strlen((char*)cascadeId)];
	sprintf( (char *)finalQuery, selectQuery, tmp, cascadeId);
	
	if (__checkCountAllQuery(finalQuery, count) != E_SUCCESS)
	{
		delete[] finalQuery;
		finalQuery = NULL;
		/*if(!checkConnectionStatus()) 
		{
			MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
			return E_NOT_CONNECTED;
		}*/
		return E_UNKNOWN;
	}
	
	// put query together (either insert or update)
	if(count == 0)
	{			
		query = insertQuery;
	}
	else
	{
		query = updateQuery;
	}
	sprintf((char*)finalQuery, query, prepaidBytes, tmp, cascadeId);
	result = monitored_PQexec(m_dbConn, (char *)finalQuery);
	
	if (PQresultStatus(result) != PGRES_COMMAND_OK) // || PQntuples(result) != 1)
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountungDBInterface: Saving to prepaidamounts failed!\n");
		//if (PQresultStatus(result) != PGRES_COMMAND_OK)
		{			
			CAMsg::printMsg(LOG_ERR, 
							"Database message '%s' while processing query '%s'\n", 
							PQresultErrorMessage(result), finalQuery
							);
		}
		delete[] finalQuery;
		finalQuery = NULL;		
		if (result)
		{
			PQclear(result);
		}
		return E_UNKNOWN;	
	}
	delete[] finalQuery;
	finalQuery = NULL;
	PQclear(result);
	CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: Stored %d prepaid bytes for account nr. %s \n",prepaidBytes, tmp); 
	return E_SUCCESS;
}

SINT32 CAAccountingDBInterface::getPrepaidAmount(UINT64 accountNumber, UINT8* cascadeId, bool a_bDelete)
{
	if(checkOwner())
	{
		return __getPrepaidAmount(accountNumber, cascadeId, a_bDelete);
	}
	else
	{
		return E_UNKNOWN;
	}
}
/*
 * When initializing a connection, retrieve the amount of prepaid, but unused, bytes the JAP account has left over from
 * a previous connection.
 * Will then delete this enty from the database table prepaidamounts
 * If the account has not been connected to this cascade before, will return zero 
 */	
SINT32 CAAccountingDBInterface::__getPrepaidAmount(UINT64 accountNumber, UINT8* cascadeId, bool a_bDelete)	
	{
		//check for an entry for this accountnumber
		const char* selectQuery = "SELECT PREPAIDBYTES FROM PREPAIDAMOUNTS WHERE ACCOUNTNUMBER=%s AND CASCADE='%s'";
		PGresult* result;
		UINT8* finalQuery;
		UINT8 accountNumberAsString[32];
		print64(accountNumberAsString,accountNumber);
		
		if(!checkConnectionStatus()) 
		{
			MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
			return E_NOT_CONNECTED;
		}
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
		
		finalQuery = new UINT8[strlen(selectQuery) + 32 + strlen((char*)cascadeId)];
		sprintf( (char *)finalQuery, selectQuery, accountNumberAsString, cascadeId);		
		result = monitored_PQexec(m_dbConn, (char *)finalQuery);
		
		if(PQresultStatus(result)!=PGRES_TUPLES_OK) 
		{
			CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Database error while trying to read prepaid bytes, Reason: %s\n", PQresultErrorMessage(result));
			PQclear(result);
			delete[] finalQuery;
			finalQuery = NULL;
			return E_UNKNOWN;
		}
		
		if(PQntuples(result)!=1) 
		{
			//perfectly normal, the user account simply hasnt been used with this cascade yet
			PQclear(result);
			delete[] finalQuery;
			finalQuery = NULL;
			return 0;
		}
		SINT32 nrOfBytes =  atoi(PQgetvalue(result, 0, 0)); //first row, first column
		PQclear(result);						

		if (a_bDelete)
		{
			//delete entry from db
			const char* deleteQuery = "DELETE FROM PREPAIDAMOUNTS WHERE ACCOUNTNUMBER=%s AND CASCADE='%s' ";
			PGresult* result2;
			print64(accountNumberAsString,accountNumber);
			sprintf( (char *)finalQuery, deleteQuery, accountNumberAsString, cascadeId);
			
			result2 = monitored_PQexec(m_dbConn, (char *)finalQuery);
			
			if (PQresultStatus(result2) != PGRES_COMMAND_OK)
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Deleting read prepaidamount failed.");	
			}
			PQclear(result2);			
		}
		
		delete[] finalQuery;
		finalQuery = NULL;
		
		return nrOfBytes;
	}	

SINT32 CAAccountingDBInterface::storeAccountStatus(UINT64 a_accountNumber, UINT32 a_statusCode, char *expires)
{
	if(checkOwner())
	{
		return __storeAccountStatus(a_accountNumber, a_statusCode, expires);
	}
	else
	{
		return E_UNKNOWN;
	}
}
/* upon receiving an ErrorMesage from the jpi, save the account status to the database table accountstatus
 * uses the same error codes defined in CAXMLErrorMessage
 */
SINT32 CAAccountingDBInterface::__storeAccountStatus(UINT64 accountNumber, UINT32 statuscode, char *expires)
{
	const char* previousStatusQuery = "SELECT COUNT(*) FROM ACCOUNTSTATUS WHERE ACCOUNTNUMBER='%s' ";
	//reverse order of columns, so insertQuery and updateQuery can be used with the same sprintf parameters
	const char* insertQuery         = "INSERT INTO ACCOUNTSTATUS(STATUSCODE,ACCOUNTNUMBER) VALUES ('%u', '%s')";
 	const char* updateQuery         = "UPDATE ACCOUNTSTATUS SET STATUSCODE=%u WHERE ACCOUNTNUMBER=%s";
 	const char* query;
 	
	PGresult* result;
	UINT8* finalQuery;
	UINT8 tmp[32];
	UINT32 len;
	UINT32 count;
	print64(tmp,accountNumber);

	if(!checkConnectionStatus()) 
	{
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
		return E_NOT_CONNECTED;
	}
	MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
	
	len = max(strlen(previousStatusQuery), strlen(insertQuery));
	len = max(len, strlen(updateQuery));
	finalQuery = new UINT8[len + 32 + 32 + 32];
	sprintf( (char *)finalQuery, previousStatusQuery, tmp);
	
	if (__checkCountAllQuery(finalQuery, count) != E_SUCCESS)
	{
		delete[] finalQuery;
		finalQuery = NULL;
		return E_UNKNOWN;
	}
	
	// put query together (either insert or update)
	if(count == 0)
	{			
		query = insertQuery;
	}
	else
	{
		query = updateQuery;
	}
	sprintf((char*)finalQuery, query, statuscode, tmp);
	result = monitored_PQexec(m_dbConn, (char *)finalQuery);	

	if (PQresultStatus(result) != PGRES_COMMAND_OK) // || PQntuples(result) != 1)
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountungDBInterface: Saving the account status failed!\n");
		//if (PQresultStatus(result) != PGRES_COMMAND_OK)
		{
			CAMsg::printMsg(LOG_ERR, 
							"Database Error '%s' while processing query '%s'\n", 
							PQresultErrorMessage(result), finalQuery
							);
		}
		delete[] finalQuery;
		finalQuery = NULL;
		if (result)
		{
			PQclear(result);
		}
		return E_UNKNOWN;	
	}
	delete[] finalQuery;
	finalQuery = NULL;
	PQclear(result);
	CAMsg::printMsg(LOG_DEBUG, "Stored status code %u and expire date %s, for account nr. %s \n",statuscode, "none", tmp); 
	return E_SUCCESS;	 	
}
	
	
	SINT32 CAAccountingDBInterface::getAccountStatus(UINT64 a_accountNumber, UINT32& a_statusCode, char *expires)
	{
		if(checkOwner())
		{
			return __getAccountStatus(a_accountNumber, a_statusCode, expires);
		}
		else
		{
			return E_UNKNOWN;
		}
	}
	
	/* retrieve account status, e.g. to see if the user's account is empty
	 * will return 0 if everything is OK (0 is defined as status ERR_OK, but is also returned if no entry is found for this account)
	 */
	SINT32 CAAccountingDBInterface::__getAccountStatus(UINT64 accountNumber, UINT32& a_statusCode, char *expires)
	{
		const char* selectQuery = "SELECT STATUSCODE FROM ACCOUNTSTATUS WHERE ACCOUNTNUMBER = %s";
		PGresult* result;
		UINT8* finalQuery;
		UINT8 accountNumberAsString[32];
		print64(accountNumberAsString,accountNumber);
		
		if(!checkConnectionStatus()) 
		{
			MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
			return E_NOT_CONNECTED;
		}
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
	
		a_statusCode =  CAXMLErrorMessage::ERR_OK;
		
		finalQuery = new UINT8[strlen(selectQuery) + 32];
		sprintf( (char *)finalQuery, selectQuery, accountNumberAsString);		
		result = monitored_PQexec(m_dbConn, (char *)finalQuery);
		
		delete[] finalQuery;
		finalQuery = NULL;
		if(PQresultStatus(result)!=PGRES_TUPLES_OK) 
		{
			CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Database error while trying to read account status, Reason: %s\n", PQresultErrorMessage(result));
			PQclear(result);
			return E_UNKNOWN;
		}
		
		if(PQntuples(result) == 1) 
		{
			int statusCodeIndex = PQfnumber(result,"STATUSCODE");
			//int expiresIndex = PQfnumber(result,"EXPIRES");
			if(statusCodeIndex != -1 )
			{
				a_statusCode = atoi(PQgetvalue(result, 0, statusCodeIndex)); //first row, first column
			}
			else
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: no status code found while reading account status\n");
			}
			
			/*if(expiresIndex != -1)
			{
				expires = strncpy(expires, (const char*)PQgetvalue(result, 0, expiresIndex), 10);
			}
			else
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: no expire date found while reading account status\n");
			}*/
		}
		
		PQclear(result);		
		
		return E_SUCCESS;				
	}

CAAccountingDBInterface *CAAccountingDBInterface::getConnection()
{
	UINT32 i;
	UINT64 myWaitNr;
	CAAccountingDBInterface *returnedConnection = NULL;
	
	if(ms_pDBConnectionPool == NULL)
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: No DBConnection available. This should NEVER happen!\n");
		return NULL;
	}
	if(ms_pConnectionAvailable == NULL)
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: DBConnections not initialized. This should NEVER happen!\n");
		return NULL;
	}
	ms_pConnectionAvailable->lock();
	for(i=0; 1; i++)
	{
		if(ms_pDBConnectionPool[(i%MAX_DB_CONNECTIONS)] != NULL)
		{
			if(ms_pDBConnectionPool[(i%MAX_DB_CONNECTIONS)]->testAndSetOwner())
			{
				returnedConnection = ms_pDBConnectionPool[(i%MAX_DB_CONNECTIONS)];
				break;
			}
		}
		if((i % MAX_DB_CONNECTIONS) == (MAX_DB_CONNECTIONS - 1)  )
		{
			//wait until connection is free
			ms_threadWaitNr++;
			myWaitNr = ms_threadWaitNr;
#ifdef DEBUG			
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: Thread %x waits for connection with waitNr %Lu and %Lu Threads waiting before\n",
					pthread_self(), myWaitNr, (myWaitNr - ms_nextThreadNr));
#endif
			while(ms_nextThreadNr != myWaitNr)
			{
				ms_pConnectionAvailable->wait();
			}
#ifdef DEBUG			
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: Thread %x with waitNr %Lu continues\n",
								pthread_self(), myWaitNr);
#endif
			if(ms_threadWaitNr == ms_nextThreadNr)
			{
				//was the last waiting thread: reset the variables
				ms_threadWaitNr = 0;
				ms_nextThreadNr = 0;
			}
			//next complete array loop turn
		}
	}
	ms_pConnectionAvailable->unlock();
	if(returnedConnection != NULL)
	{
		if(!(returnedConnection->checkConnectionStatus()))
		{
			if(returnedConnection->initDBConnection() != E_SUCCESS)
			{
				MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);
				CAMsg::printMsg(LOG_WARNING, "CAAccountingDBInterface: Warning requested DB connection can not be established\n"); 	
				/*returnedConnection->testAndResetOwner();
				return NULL;*/
			}
			else
			{
				MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
			}
		}
		else
		{
			MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
		}
	}
	return returnedConnection;
}
			
/*CAAccountingDBInterface *CAAccountingDBInterface::getConnectionNB()
{
	//@todo: implementation
	return NULL;
}*/
			
SINT32 CAAccountingDBInterface::releaseConnection(CAAccountingDBInterface *dbIf)
{
	SINT32 ret;
	//@todo: implementation
	if(dbIf == NULL)
	{
		return E_UNKNOWN;
	}
	if(dbIf->m_pConnectionMutex == NULL)
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: DBConnection structure corrupt!\n");
		return E_UNKNOWN;
	}
	if(ms_pConnectionAvailable == NULL)
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: DBConnections not initialized. This should NEVER happen!\n");
		return E_UNKNOWN;
	}
	
	ms_pConnectionAvailable->lock();
	if(dbIf->testAndResetOwner())
	{
		if(ms_threadWaitNr != ms_nextThreadNr)
		{
			ms_nextThreadNr++;
#ifdef DEBUG			
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: There are %Lu Threads waiting. Waking up thread with waitNr. %Lu\n",
											 (ms_threadWaitNr - ms_nextThreadNr), ms_nextThreadNr);
#endif
			//There are threads waiting
			ms_pConnectionAvailable->broadcast();
		}
		ret = E_SUCCESS;
	}
	else
	{
		ret = E_UNKNOWN;
	}
	ms_pConnectionAvailable->unlock();
	return ret;
}

bool CAAccountingDBInterface::testAndSetOwner()
{
	bool success = false;
	m_pConnectionMutex->lock();
	if(m_free)
	{
		m_free = false;
		m_owner = pthread_self();
		success = true;
	}
	m_pConnectionMutex->unlock();
	return success;
}

bool CAAccountingDBInterface::testAndResetOwner()
{
	bool success = false;
	m_pConnectionMutex->lock();
	if(!m_free && (m_owner == pthread_self()))
	{
		m_free = true;
		m_owner = 0;
		success = true;
	}
	m_pConnectionMutex->unlock();
	return success;
}

SINT32 CAAccountingDBInterface::init()
{
	UINT32 i;
	SINT32 dbConnStatus;
	
	ms_pConnectionAvailable = new CAConditionVariable();
	
	ms_threadWaitNr = 0;
	ms_nextThreadNr = 0;
	
	if(ms_pDBConnectionPool != NULL)
	{
		for(i=0; i < MAX_DB_CONNECTIONS; i++)
		{
			if(ms_pDBConnectionPool[i] != NULL)
			{
				CAMsg::printMsg(LOG_WARNING, "CAAccountingDBInterface: DBConnection initialization: "
						"Already initialized connections !?! Or someone forgot to do cleaunp?\n");
			}
			ms_pDBConnectionPool[i] = new CAAccountingDBInterface();
			dbConnStatus = ms_pDBConnectionPool[i]->initDBConnection();
			
			if(dbConnStatus != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: DBConnection initialization: "
						"could not connect to Database.\n");
				return E_UNKNOWN;
			}
		}
	}
	else
	{ //should never happen
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: DBConnection initialization failed.\n");
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}

SINT32 CAAccountingDBInterface::cleanup()
{
	UINT32 i;
	SINT32 dbConnStatus, ret = E_SUCCESS;
	if(ms_pDBConnectionPool != NULL)
	{
		for(i=0; i < MAX_DB_CONNECTIONS; i++)
		{
			if(ms_pDBConnectionPool[i] == NULL)
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: DBConnection cleanup: Already cleaned up !?! "
						"Or someone forgot to initialize?\n");
			}
			else
			{
				dbConnStatus = ms_pDBConnectionPool[i]->terminateDBConnection();
				delete ms_pDBConnectionPool[i];
				ms_pDBConnectionPool[i] = NULL;
			}
			if(dbConnStatus != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: DBConnection cleanup: "
						"an error occured while closing DBConnection.\n");
				ret = dbConnStatus;
			}
		}
	}
	if(ms_pConnectionAvailable != NULL)
	{
		delete ms_pConnectionAvailable;
		ms_pConnectionAvailable = NULL;
	}
	return ret;
}

PGresult *CAAccountingDBInterface::monitored_PQexec(PGconn *conn, const char *query)
{
	PGresult* result = NULL;
	result = PQexec(conn, query);
	if(!checkConnectionStatus()) 
	{		
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionFailure);	
	}
	else
	{
		MONITORING_FIRE_PAY_EVENT(ev_pay_dbConnectionSuccess);
	}
	return result;
}

#endif


