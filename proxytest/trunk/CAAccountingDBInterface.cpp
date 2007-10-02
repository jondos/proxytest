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

extern CACmdLnOptions* pglobalOptions;

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

bool CAAccountingDBInterface::checkConnectionStatus()
{
	if (!m_bConnected)
	{
		return false;
	}
	
	if (PQstatus(m_dbConn) != CONNECTION_OK) 
	{
		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInteface: Connection to database lost! Reason: %s\n", 
			PQerrorMessage(m_dbConn));	
    	PQreset(m_dbConn);
    	
      	if (PQstatus(m_dbConn) != CONNECTION_OK) 
      	{
      		CAMsg::printMsg(LOG_ERR, "CAAccountingDBInteface: Could not reset database connection! Reason: %s\n", 
      			PQerrorMessage(m_dbConn));	
			terminateDBConnection();
      	}
      	else
      	{
      		CAMsg::printMsg(LOG_INFO, "CAAccountingDBInteface: Database connection has been reset successfully!");
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
SINT32 CAAccountingDBInterface::getCostConfirmation(UINT64 accountNumber, UINT8* cascadeId, CAXMLCostConfirmation **pCC, bool& a_bSettled)
	{
		if(!checkConnectionStatus()) 
		{
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
		
		result = PQexec(m_dbConn, (char *)query);
		delete[] query;
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
	r_count = 0;
	
	if (!a_query)
	{		
		return E_UNKNOWN;
	}
	
	PGresult * pResult = PQexec(m_dbConn, (char*)a_query);
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



/**
 * stores a cost confirmation in the DB
 * @todo optimize - maybe do check and insert/update in one step??
 */
SINT32 CAAccountingDBInterface::storeCostConfirmation( CAXMLCostConfirmation &cc, UINT8* ccCascade )
	{			
		#ifndef HAVE_NATIVE_UINT64
			#warning Native UINT64 type not available - CostConfirmation Database might be non-functional
		#endif
		const char* previousCCQuery = "SELECT COUNT(*) FROM COSTCONFIRMATIONS WHERE ACCOUNTNUMBER=%s AND CASCADE='%s'";
		const char* query2F =         "INSERT INTO COSTCONFIRMATIONS(BYTES, XMLCC, SETTLED, ACCOUNTNUMBER, CASCADE) VALUES (%s, '%s', %d, %s, '%s')";
	 	const char* query3F =         "UPDATE COSTCONFIRMATIONS SET BYTES=%s, XMLCC='%s', SETTLED=%d WHERE ACCOUNTNUMBER=%s AND CASCADE='%s'";
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
			return E_NOT_CONNECTED;
		}
		
		pStrCC = new UINT8[8192];
		size=8192;
		if(cc.toXMLString(pStrCC, &size)!=E_SUCCESS)
		{
			CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstanceDBInterface: Could not transform CC to XML string!\n");
			delete[] pStrCC;
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
		if (checkCountAllQuery(query, count) != E_SUCCESS)
		{
			delete[] pStrCC;
			delete[] query;
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
		pResult = PQexec(m_dbConn, (char*)query);
		delete[] pStrCC;
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
			PQclear(pResult);
			return E_UNKNOWN;
		}
		delete[] query;	
		PQclear(pResult);		

		#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstanceDBInterface: Finished storing CC in DB.\n");
		#endif
		return E_SUCCESS;
	}


/**
	* Fills the CAQueue with pointer to all non-settled cost confirmations. The caller is responsible for deleating this cost confirmations.
	* @retval E_NOT_CONNECTED if a connection to the DB could not be established
	* @retval E_UNKNOWN in case of a general error
	* @retval E_SUCCESS : database operation completed successfully (but queue might still be empty if there were no unsettled cost confirmation to find).
	* Param: cascadeId : String identifier of a cascade for which to return the cost confirmations (concatenated hashes of all the cascade's mixes' price certs) 
	* 
	*/
SINT32 CAAccountingDBInterface::getUnsettledCostConfirmations(CAQueue& q, UINT8* cascadeId)
	{
		const char* query= "SELECT XMLCC FROM COSTCONFIRMATIONS WHERE SETTLED=0 AND CASCADE = '%s' ";
		UINT8* finalQuery;
		PGresult* result;
		SINT32 numTuples, i;
		UINT8* pTmpStr;
		CAXMLCostConfirmation* pCC;

		if(!checkConnectionStatus()) 
		{
			return E_NOT_CONNECTED;
		}

		finalQuery = new UINT8[strlen(query)+strlen((char*)cascadeId)];
		sprintf( (char*)finalQuery, query, cascadeId);

#ifdef DEBUG
		CAMsg::printMsg(LOG_DEBUG, "Getting Cost confirmations for cascade: ");
		CAMsg::printMsg(LOG_DEBUG, (const char*) finalQuery);
#endif
		
		result = PQexec(m_dbConn, (char *)finalQuery);
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

/**
	* Marks this account as settled.
	* @todo what to do if there was a new CC stored while we were busy settling the old one?
	* @retval E_NOT_CONNECTED if a connection to the DB could not be established
	* @retval E_UNKNOWN in case of a general error
	*/
SINT32 CAAccountingDBInterface::markAsSettled(UINT64 accountNumber, UINT8* cascadeId, UINT64 a_transferredBytes)
	{
		const char* queryF = "UPDATE COSTCONFIRMATIONS SET SETTLED=1 WHERE ACCOUNTNUMBER=%s AND BYTES=%s AND CASCADE='%s'";
		UINT8 * query;
		PGresult * result;
		
		if(!checkConnectionStatus()) 
		{
			return E_NOT_CONNECTED;
		}

		UINT8 tmp[32], tmp2[32];
		print64(tmp,accountNumber);
		print64(tmp2,a_transferredBytes);
		query = new UINT8[strlen(queryF) + 32 + 32 + strlen((char*)cascadeId)];
		sprintf((char *)query, queryF, tmp, tmp2, cascadeId);
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
	
SINT32 CAAccountingDBInterface::deleteCC(UINT64 accountNumber, UINT8* cascadeId)
{
	const char* deleteQuery = "DELETE FROM COSTCONFIRMATIONS WHERE ACCOUNTNUMBER = %s AND CASCADE='%s'";
	UINT8* finalQuery;
	PGresult* result;
	SINT32 ret;
	UINT8 temp[32];
	print64(temp,accountNumber);
	
	if (!checkConnectionStatus())
	{
		ret = E_NOT_CONNECTED;	
	}
	else
	{						
		finalQuery = new UINT8[strlen(deleteQuery)+ 32 + strlen((char*)cascadeId)];
		sprintf((char *)finalQuery,deleteQuery,temp, cascadeId);
		result = PQexec(m_dbConn, (char*)finalQuery);
		//CAMsg::printMsg(LOG_DEBUG, "%s\n",finalQuery);
		delete[] finalQuery;
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


/*
 *  When terminating a connection, store the amount of bytes that the JAP account has already paid for, but not used 
 */	
SINT32 CAAccountingDBInterface::storePrepaidAmount(UINT64 accountNumber, SINT32 prepaidBytes, UINT8* cascadeId)
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
		return E_NOT_CONNECTED;
	}
	
	len = max(strlen(selectQuery), strlen(insertQuery));
	len = max(len, strlen(updateQuery));
	finalQuery = new UINT8[len + 32 + 32 + strlen((char*)cascadeId)];
	sprintf( (char *)finalQuery, selectQuery, tmp, cascadeId);
	
	if (checkCountAllQuery(finalQuery, count) != E_SUCCESS)
	{
		delete[] finalQuery;
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
	result = PQexec(m_dbConn, (char *)finalQuery);	
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
		if (result)
		{
			PQclear(result);
		}
		return E_UNKNOWN;	
	}
	delete[] finalQuery;
	PQclear(result);
	CAMsg::printMsg(LOG_DEBUG, "CAAccountingDBInterface: Stored %d prepaid bytes for account nr. %s \n",prepaidBytes, tmp); 
	return E_SUCCESS;
}
/*
 * When initializing a connection, retrieve the amount of prepaid, but unused, bytes the JAP account has left over from
 * a previous connection.
 * Will then delete this enty from the database table prepaidamounts
 * If the account has not been connected to this cascade before, will return zero 
 */	
SINT32 CAAccountingDBInterface::getPrepaidAmount(UINT64 accountNumber, UINT8* cascadeId, bool a_bDelete)	
	{
		//check for an entry for this accountnumber
		const char* selectQuery = "SELECT PREPAIDBYTES FROM PREPAIDAMOUNTS WHERE ACCOUNTNUMBER=%s AND CASCADE='%s'";
		PGresult* result;
		UINT8* finalQuery;
		UINT8 accountNumberAsString[32];
		print64(accountNumberAsString,accountNumber);
		
		if(!checkConnectionStatus()) 
		{
			return E_NOT_CONNECTED;
		}
		
		finalQuery = new UINT8[strlen(selectQuery) + 32 + strlen((char*)cascadeId)];
		sprintf( (char *)finalQuery, selectQuery, accountNumberAsString, cascadeId);		
		result = PQexec(m_dbConn, (char *)finalQuery);
		if(PQresultStatus(result)!=PGRES_TUPLES_OK) 
		{
			CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Database error while trying to read prepaid bytes, Reason: %s\n", PQresultErrorMessage(result));
			PQclear(result);
			delete[] finalQuery;
			return E_UNKNOWN;
		}
		
		if(PQntuples(result)!=1) 
		{
			//perfectly normal, the user account simply hasnt been used with this cascade yet
			PQclear(result);
			delete[] finalQuery;
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
			
			result2 = PQexec(m_dbConn, (char *)finalQuery);
			if (PQresultStatus(result2) != PGRES_COMMAND_OK)
			{
				CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Deleting read prepaidamount failed.");	
			}
			PQclear(result2);			
		}
		
		delete[] finalQuery;
		
		return nrOfBytes;
	}	
	/* upon receiving an ErrorMesage from the jpi, save the account status to the database table accountstatus
	 * uses the same error codes defined in CAXMLErrorMessage
	 */
	SINT32 CAAccountingDBInterface::storeAccountStatus(UINT64 accountNumber, UINT32 statuscode)
	{
		const char* previousStatusQuery = "SELECT COUNT(*) FROM ACCOUNTSTATUS WHERE ACCOUNTNUMBER=%s ";
		//reverse order of columns, so insertQuery and updateQuery can be used with the same sprintf parameters
		const char* insertQuery         = "INSERT INTO ACCOUNTSTATUS(STATUSCODE,ACCOUNTNUMBER) VALUES (%u, %s)";
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
			return E_NOT_CONNECTED;
		}
		
		len = max(strlen(previousStatusQuery), strlen(insertQuery));
		len = max(len, strlen(updateQuery));
		finalQuery = new UINT8[len + 32 + 32];
		sprintf( (char *)finalQuery, previousStatusQuery, tmp);
		
		if (checkCountAllQuery(finalQuery, count) != E_SUCCESS)
		{
			delete[] finalQuery;
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
		result = PQexec(m_dbConn, (char *)finalQuery);	
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
			if (result)
			{
				PQclear(result);
			}
			return E_UNKNOWN;	
		}
		delete[] finalQuery;
		PQclear(result);
		CAMsg::printMsg(LOG_DEBUG, "Stored status code %u for account nr. %s \n",statuscode, tmp); 
		return E_SUCCESS;	 	
	}
	
	/* retrieve account status, e.g. to see if the user's account is empty
	 * will return 0 if everything is OK (0 is defined as status ERR_OK, but is also returned if no entry is found for this account)
	 */
	SINT32 CAAccountingDBInterface::getAccountStatus(UINT64 accountNumber, UINT32& a_statusCode)
	{
		const char* selectQuery = "SELECT STATUSCODE FROM ACCOUNTSTATUS WHERE ACCOUNTNUMBER = %s";
		PGresult* result;
		UINT8* finalQuery;
		UINT8 accountNumberAsString[32];
		print64(accountNumberAsString,accountNumber);
		
		if(!checkConnectionStatus()) 
		{
			return E_NOT_CONNECTED;
		}
		
		
		a_statusCode =  CAXMLErrorMessage::ERR_OK;
		
		finalQuery = new UINT8[strlen(selectQuery) + 32];
		sprintf( (char *)finalQuery, selectQuery, accountNumberAsString);		
		result = PQexec(m_dbConn, (char *)finalQuery);
		delete[] finalQuery;
		if(PQresultStatus(result)!=PGRES_TUPLES_OK) 
		{
			CAMsg::printMsg(LOG_ERR, "CAAccountingDBInterface: Database error while trying to read account status, Reason: %s\n", PQresultErrorMessage(result));
			PQclear(result);
			return E_UNKNOWN;
		}
		
		if(PQntuples(result) == 1) 
		{
			a_statusCode = atoi(PQgetvalue(result, 0, 0)); //first row, first column
		}
		PQclear(result);		
		
		return E_SUCCESS;				
	}
	
	
#endif


