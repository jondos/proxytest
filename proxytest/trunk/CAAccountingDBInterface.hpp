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

#include "StdAfx.h"
#include "CAQueue.hpp"
#include "CAXMLCostConfirmation.hpp"

/**
  * @author Bastian Voigt
  *
  * This class is used to store cost confirmations
  * in a postgresql database
  */
class CAAccountingDBInterface
{
public: 
	CAAccountingDBInterface();
	~CAAccountingDBInterface();

	/**
	* Initiates the database connection. 
	* This function is called inside the aiThread
	*
	* @return E_NOT_CONNECTED if the connection could not be established
	* @return E_UNKNOWN if we are already connected
	* @return E_SUCCESS if all is OK
	*/
	SINT32 initDBConnection(
			const UINT8 * host, UINT32 tcp_port, 
			const UINT8 * dbName, const UINT8 * userName,
			const UINT8 * password
		);
	
	/**
	* Terminates the database connection
	* @return E_SUCCESS
	*/
	SINT32 terminateDBConnection();
	
	/**
	* Creates the tables we need in the DB
	*
	* @return E_SUCCESS if all is OK
	* @return E_UNKNOWN if the query could not be executed
	* @return E_NOT_CONNECTED if we are not connected to the DB
	*/
	SINT32 createTables();
	
	
	SINT32 dropTables();
	
	SINT32 storeCostConfirmation(CAXMLCostConfirmation &cc);

	SINT32 getCostConfirmation(UINT64 accountNumber, CAXMLCostConfirmation *pCC);
	
	
	/**
	 * Fills the CAQueue with all non-settled cost confirmations
	 *
	 */
	SINT32 getUnsettledCostConfirmations(CAQueue &q);
	
	/**
	 * Marks this account as settled.
	 * @todo what to do if there was a new CC stored while we were busy settling the old one?
	 */
	SINT32 markAsSettled(UINT64 accountNumber);

private:
	/** connection to postgreSQL database */
	PGconn * m_dbConn;
	bool m_bConnected;
};

#endif
