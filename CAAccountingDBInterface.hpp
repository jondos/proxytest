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
#ifndef CAACCOUNTINGDBINTERFACE_HPP
#define CAACCOUNTINGDBINTERFACE_HPP

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

	SINT32 initDBConnection(const UINT8 * host, UINT32 tcp_port, 
													const UINT8 * dbName, const UINT8 * userName,
													const UINT8 * password);
	SINT32 terminateDBConnection();
	SINT32 createTables();
	SINT32 dropTables();
	SINT32 storeCostConfirmation(UINT64 accountNumber, UINT64 bytes, UINT8 * xmlCC, UINT32 isSettled);
//	SINT32 storeCostConfirmation(UINT64 accountNumber, char *buf, UINT32 *len);
	SINT32 getCostConfirmation(UINT64 accountNumber, UINT8 *buf, UINT32 *len);

private:
	/** connection to postgreSQL database */
	PGconn * m_dbConn;
	bool m_bConnected;
};

#endif
