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
#ifndef __CAACCOUNTINGDBINTERFACE__
#define __CAACCOUNTINGDBINTERFACE__
#include "doxygen.h"
#ifdef PAYMENT
#include "CAQueue.hpp"
#include "CAXMLCostConfirmation.hpp"
#include "CAThread.hpp"

#define MAX_DB_CONNECTIONS 3
#define PG_PROTOCOL_VERSION_3 3

#define RESULT_FORMAT_TEXT 0

#define PREPARED_STMT_NAME_STORE_CC "storeCCStatement"
#define PREPARED_STMT_QUERY_STORE_CC \
	"UPDATE COSTCONFIRMATIONS SET BYTES = $1::bigint, XMLCC = $2::varchar(2000), SETTLED = $3::integer WHERE ACCOUNTNUMBER = $4::bigint AND CASCADE = $5::varchar(200)"
#define PREPARED_STMT_PARAMS_STORE_CC 5

#define STMT_CLEAR_ACCOUNT_STATUS "DELETE FROM ACCOUNTSTATUS WHERE ACCOUNTNUMBER = '%llu'"

/**
  * @author Bastian Voigt
  *
  * This class is used to store cost confirmations
  * in a postgresql database
  */
class CAAccountingDBInterface
	{
	public:

		/**
		 * Creates the tables we need in the DB
		 *
		 * @return E_SUCCESS if all is OK
		 * @return E_UNKNOWN if the query could not be executed
		 * @return E_NOT_CONNECTED if we are not connected to the DB
		 */
		//SINT32 createTables();
		//SINT32 dropTables();

		SINT32 storeCostConfirmation(CAXMLCostConfirmation &cc, UINT8* ccCascade);

		SINT32 getCostConfirmation(UINT64 accountNumber, UINT8* cascadeId, CAXMLCostConfirmation **pCC, bool& a_bSettled);


		/**
		* Fills the CAQueue with all non-settled cost confirmations
		*
		*/
		//SINT32 getUnsettledCostConfirmations(CAQueue &q, UINT8* cascadeId);
		SINT32 getUnsettledCostConfirmations(CAXMLCostConfirmation ***resultCCs, UINT8* cascadeId, UINT32 *nrOfCCs);

		/**
		* Marks this account as settled.
		* @todo what to do if there was a new CC stored while we were busy settling the old one?
		*/
		SINT32 markAsSettled(UINT64 accountNumber, UINT8* cascadeId, UINT64 a_transferredBytes);

		/**
		 * if the BI reports an error while trying to settle a CC, this will be called to delete it from the database
		 * (otherwise the AI would try forever in vain to settle the unusable CC)
		 */
		SINT32 deleteCC(UINT64 accountNumber, UINT8* cascadeId);

		SINT32 storePrepaidAmount(UINT64 accountNumber, SINT32 prepaidBytes, UINT8* cascadeId);
		SINT32 getPrepaidAmount(UINT64 accountNumber, UINT8* cascadeId, bool a_bDelete);

		SINT32 storeAccountStatus(UINT64 a_accountNumber, UINT32 a_statusCode);
		SINT32 getAccountStatus(UINT64 a_accountNumber, UINT32& a_statusCode);
		SINT32 clearAccountStatus(UINT64 a_accountNumber);

		/**
		 * Takes and executes a query that counts databae records and tests if the result
		 * is valid.
		 * @param a_query a query that should return the count of database rows
		 * @param r_count number of database rows; only valid if E_SUCCESS is returned
		 */
		SINT32 checkCountAllQuery(UINT8* a_query, UINT32& r_count);

		/* Requests a DB connection. Blocks until it is available and returns a
		 * DB Interface which is then owned by the requesting thread. In general the returned
		 * interface is already connected but it is also returned if a connection could not be
		 * established
		 */
		static CAAccountingDBInterface *getConnection();

		/* Requests a DB connection. Returns a connected DB Interface
		 * if it is available, which is then owned by the requesting thread.
		 * Returns NULL otherwise.
		 */
		//static CAAccountingDBInterface *getConnectionNB();

		/* Release the DBConnection which must be owned by the calling thread
		 * Connection will not be disconnected
		 */
		static SINT32 releaseConnection(CAAccountingDBInterface *dbIf);

		/* static initialization of the DBConnections */
		static SINT32 init();
		/* removes all DBconnections */
		static SINT32 cleanup();

		private:

			CAAccountingDBInterface();
			~CAAccountingDBInterface();

			/* thread unsafe DB query functions */
			SINT32 __storeCostConfirmation(CAXMLCostConfirmation &cc, UINT8* ccCascade);
			SINT32 __getCostConfirmation(UINT64 accountNumber, UINT8* cascadeId, CAXMLCostConfirmation **pCC, bool& a_bSettled);

			//SINT32 __getUnsettledCostConfirmations(CAQueue &q, UINT8* cascadeId);
			SINT32 __getUnsettledCostConfirmations(CAXMLCostConfirmation ***resultCCs, UINT8* cascadeId, UINT32 *nrOfCCs);

			SINT32 __markAsSettled(UINT64 accountNumber, UINT8* cascadeId, UINT64 a_transferredBytes);
			SINT32 __deleteCC(UINT64 accountNumber, UINT8* cascadeId);

			SINT32 __storePrepaidAmount(UINT64 accountNumber, SINT32 prepaidBytes, UINT8* cascadeId);
			SINT32 __getPrepaidAmount(UINT64 accountNumber, UINT8* cascadeId, bool a_bDelete);

			SINT32 __storeAccountStatus(UINT64 a_accountNumber, UINT32 a_statusCode);
			SINT32 __getAccountStatus(UINT64 a_accountNumber, UINT32& a_statusCode);
			SINT32 __clearAccountStatus(UINT64 a_accountNumber);

			SINT32 __checkCountAllQuery(UINT8* a_query, UINT32& r_count);

			/**
			 * Initiates the database connection.
			 * This function is called inside the aiThread
			 *
			 * @return E_NOT_CONNECTED if the connection could not be established
			 * @return E_UNKNOWN if we are already connected
			 * @return E_SUCCESS if all is OK
			 */
			SINT32 initDBConnection();

			/**
			* Terminates the database connection
			* @return E_SUCCESS
			*/
			SINT32 terminateDBConnection();
			friend class CAAccountingInstance;

			bool isDBConnected();

			/**
			 * Checks if the connection still exists and tries to reconnect if not.
			 * @return if the database connection is active after the call or not
			 */
			bool checkConnectionStatus();

			bool checkOwner();

			bool testAndSetOwner();
			bool testAndResetOwner();

			PGresult *monitored_PQexec(PGconn *conn, const char *query);

			/** connection to postgreSQL database */
			PGconn * m_dbConn;
			bool m_bConnected;

			/* The owner of the connection */
			volatile thread_id_t m_owner;
			/* indicates wether this connection is not owned by a thread.
			 * (There is no reliable value of m_owner to indicate this).
			 */
			volatile bool m_free;
			int m_protocolVersion;
#ifdef PG_PROTO_VERSION_3
			char *m_pStoreCCStmt;
#endif
			/* to ensure atomic access to m_owner and m_free */
			CAMutex *m_pConnectionMutex;


			static CAConditionVariable *ms_pConnectionAvailable;
			/* WaitNr for a thread requesting a connection: if the ms_nextThreadNr equals the thread's
			 * waitNumber it's his turn to obtain the next free connection
			 */
			static volatile UINT64 ms_threadWaitNr;
			/* Indicates which thread obtains the next available connection */
			static volatile UINT64 ms_nextThreadNr;

			static CAAccountingDBInterface *ms_pDBConnectionPool[];
	};
#endif //PAYMENT
#endif
