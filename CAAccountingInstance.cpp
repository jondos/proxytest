/*
Copyright (c) 2000, The JAP-Team All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
 
- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
 
- Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided
with the distribution.
 
- Neither the name of the University of Technology Dresden,
Germany nor the names of its contributors may be used to endorse
or promote products derived from this software without specific
prior written permission.
 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE 
*/
#include "StdAfx.h"
#ifdef PAYMENT


#include "CAAccountingInstance.hpp"
#include "CAAccountingControlChannel.hpp"
#include "CABase64.hpp"
#include "CAMsg.hpp"
#include "CAUtil.hpp"
#include "CASignature.hpp"
#include "CAXMLErrorMessage.hpp"


/** Helper function for debugging */
/*void hexDump(UINT8 *pDataIn, UINT32 lenDataIn) {
	int numLines=0;
	int i=0;
	numLines = lenDataIn/16;
	for(i=0; i<numLines-1; i++) {
		printf("%08x: %02x %02x %02x %02x %02x %02x %02x %02x		%02x %02x %02x %02x %02x %02x %02x %02x\n", 
						i*16, pDataIn[i*16], pDataIn[i*16+1], pDataIn[i*16+2], pDataIn[i*16+3], pDataIn[i*16+4],
						pDataIn[i*16+5], pDataIn[i*16+6], pDataIn[i*16+7], pDataIn[i*16+8], pDataIn[i*16+9],
						pDataIn[i*16+10], pDataIn[i*16+11], pDataIn[i*16+12], pDataIn[i*16+13], pDataIn[i*16+14],
						pDataIn[i*16+15]);
	}
	// last line
	// weglassen	
	}
*/
/**
 * Singleton: This is the reference to the only instance of this class
 */
CAAccountingInstance * CAAccountingInstance::m_spInstance = NULL;



/**
 * private Constructor
 */
CAAccountingInstance::CAAccountingInstance()
{
	CAMsg::printMsg( LOG_DEBUG, "AccountingInstance initialising\n" );
	m_pQueue = new CAQueue();
//	m_pIPBlockList = new CATempIPBlockList();
	
	// initialize some configuration settings

	// initialize JPI connection
	m_biInterface = new CAAccountingBIInterface();

	// initialize Database connection
	m_dbInterface = new CAAccountingDBInterface();

	// launch AI thread
	m_pThread = new CAThread();
	m_pThread->setMainLoop( aiThreadMainLoop );
	m_pThread->start( this );
	
}



/**
 * private desctructor
 */
CAAccountingInstance::~CAAccountingInstance()
{
	CAMsg::printMsg( LOG_DEBUG, "AccountingInstance dying\n" );
	delete m_biInterface;
	delete m_dbInterface;
	delete m_pIPBlockList;
	delete m_pQueue;
}



/**
 * Sets the in/out AES keys for communicating with one JAP
 * TODO: add Challenge / Response
 */
/*UINT32 CAAccountingInstance::setJapKeys( fmHashTableEntry *pHashEntry,
		UINT8 * out, UINT8 * in )
{
	CAMsg::printMsg( LOG_ERR, "AI setting Cipher keys\n" );
	aiAccountingInfo * pAccInfo;
	pAccInfo = pHashEntry->pAccountingInfo;
	m_Mutex.lock();

	// if necessary, create new CASymCipher objects
	if ( pAccInfo->pCipherIn == 0 )
		pAccInfo->pCipherIn = new CASymCipher();
	if ( pAccInfo->pCipherOut == 0 )
		pAccInfo->pCipherOut = new CASymCipher();

	// set the keys
	pHashEntry->pAccountingInfo->pCipherIn->setKey( in );
	pHashEntry->pAccountingInfo->pCipherOut->setKey( out );
	m_Mutex.unlock();
	return 0;
}*/




/**
 * This function is called by the FirstMix for each incoming JAP packet.
 * It counts the payload of normal packets and tells the mix when a connection 
 * should be closed because the user is not willing to pay.
 * 
 * @return 0 if the packet is an JAP->AI packet (caller should drop it)
 * @return 1 everything is OK, packet should be forwarded to next mix
 * @return 2 user did not send accountcertificate, connection should be closed
 * @return 3 user  did not send a cost confirmation 
 * or somehow tried to fake authentication, connection should be closed
 * @return 4 AuthState unknown (internal error, should not happen)
 */
SINT32 CAAccountingInstance::handleJapPacket(
	MIXPACKET *pPacket,
	fmHashTableEntry *pHashEntry
)
{
	aiAccountingInfo * pAccInfo;
//	UINT8 * pData;
	timespec now;	
	getcurrentTime(now);
	pAccInfo = pHashEntry->pAccountingInfo;
//	pData = pPacket->data;
	// set the lock
	m_Mutex.lock();

	// count the packet
	pAccInfo->transferredBytes += sizeof(MIXPACKET);
	
	if(pAccInfo->authFlags & (AUTH_GOT_ACCOUNTCERT) )
	{
		// we have an account certificate
		// did the user try to betray us?
		if( !(pAccInfo->authFlags & AUTH_ACCOUNT_OK) )
		{
			m_Mutex.unlock();
			return 3;
		}

		//----------------------------------------------------------
		// if the JAP refuses to send a cost confirmation -> byebye!
		if ((pAccInfo->transferredBytes-pAccInfo->confirmedBytes) >= HARDLIMIT_UNCONFIRMED_BYTES)
		{
			m_Mutex.unlock();
			CAMsg::printMsg( LOG_DEBUG, "Accounting instance: Account %d, IP %d.%d.%d.%d refused "
												"to send cost confirmation.",
												pAccInfo->accountNumber, pHashEntry->peerIP[ 0 ], pHashEntry->peerIP[ 1 ],
												pHashEntry->peerIP[ 2 ], pHashEntry->peerIP[ 3 ] );
			m_pIPBlockList->insertIP( pHashEntry->peerIP );
			return 3;
		}
	
		//-------------------------------------------------------
		// check: is it time to request a new cost confirmation?
		else if ( (pAccInfo->transferredBytes-pAccInfo->confirmedBytes) >= SOFTLIMIT_UNCONFIRMED_BYTES)
		{
			if( (pAccInfo->authFlags & AUTH_SENT_CC_REQUEST) )
			{
				if( (pAccInfo->authFlags & AUTH_SENT_SECOND_CC_REQUEST))
				{
					if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
					{
						m_Mutex.unlock();
						CAMsg::printMsg( LOG_DEBUG, "Accounting instance: Account %d, IP %d.%d.%d.%d refused "
															"to send cost confirmation(2).",
															pAccInfo->accountNumber, pHashEntry->peerIP[ 0 ], pHashEntry->peerIP[ 1 ],
															pHashEntry->peerIP[ 2 ], pHashEntry->peerIP[ 3 ] );
						m_pIPBlockList->insertIP( pHashEntry->peerIP );
						return 3;
					}
				}
				else if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
				{
					// send a reminder...
					DOM_Document doc;
					makeCCRequest(pAccInfo->accountNumber, pAccInfo->transferredBytes, doc);
					pAccInfo->pControlChannel->sendMessage(doc);
					pAccInfo->authFlags |= AUTH_SENT_SECOND_CC_REQUEST;
					pAccInfo->lastRequestSeconds = now.tv_sec;
					m_Mutex.unlock();
					return 1;
				}
			}
			else
			{
				// send a first CC request
				DOM_Document doc;
				makeCCRequest(pAccInfo->accountNumber, pAccInfo->transferredBytes, doc);
				pAccInfo->pControlChannel->sendMessage(doc);
				pAccInfo->authFlags |= AUTH_SENT_CC_REQUEST;
				pAccInfo->lastRequestSeconds = now.tv_sec;
				m_Mutex.unlock();
				return 1;
			}
		}
		else
		{
			//--------------------------------------------------------------
			// check: do we need a new balance certificate for the account?
	
			if( ( (pAccInfo->transferredBytes - pAccInfo->lastbalTransferredBytes) >=
				  ((pAccInfo->lastbalDeposit - pAccInfo->lastbalSpent) / 4)) ||
					(pAccInfo->lastbalDeposit == 0))
			{
				if( (pAccInfo->authFlags & AUTH_SENT_BALANCE_REQUEST) )
				{
					if( (pAccInfo->authFlags & AUTH_SENT_SECOND_BALANCE_REQUEST))
					{
						if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
						{
							m_Mutex.unlock();
							CAMsg::printMsg( LOG_DEBUG, "Accounting instance: Account %d, IP %d.%d.%d.%d refused "
																"to send balance cert.",
																pAccInfo->accountNumber, pHashEntry->peerIP[ 0 ], pHashEntry->peerIP[ 1 ],
																pHashEntry->peerIP[ 2 ], pHashEntry->peerIP[ 3 ] );
							m_pIPBlockList->insertIP( pHashEntry->peerIP );
							return 3;
						}
					}
					else if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
					{
						// send a reminder...
						// TODO adjust "newerThan" value
						DOM_Document doc;
						makeBalanceRequest((SINT32)now.tv_sec-600, doc);
						pAccInfo->pControlChannel->sendMessage(doc);
						pAccInfo->authFlags |= AUTH_SENT_SECOND_BALANCE_REQUEST;
						pAccInfo->lastRequestSeconds = now.tv_sec;
						m_Mutex.unlock();
						return 1;
					}
				}
				else
				{
					// send a first CC request
					DOM_Document doc;
					makeBalanceRequest(now.tv_sec-600, doc);
					pAccInfo->pControlChannel->sendMessage(doc);
					pAccInfo->authFlags |= AUTH_SENT_BALANCE_REQUEST;
					pAccInfo->lastRequestSeconds = now.tv_sec;
					m_Mutex.unlock();
					return 1;
				}
			}
			else
			{
				// let the packet pass thru
				m_Mutex.unlock();
				return 1;
			}
		}
	}
	else
	{
		//---------------------------------------------------------
		// we have no accountcert from the user, let's request one
		
		if(pAccInfo->authFlags & AUTH_SENT_ACCOUNT_REQUEST)
		{
			if(pAccInfo->authFlags & AUTH_SENT_SECOND_ACCOUNT_REQUEST)
			{
				// just wait for answer
				if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
				{
					// timeout
					m_Mutex.unlock();
					return 2;
				}
				m_Mutex.unlock();
				return 1;
			}
			if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
			{
				// send reminder
				DOM_Document doc;
				makeAccountRequest(doc);
				pAccInfo->pControlChannel->sendMessage(doc);
				pAccInfo->authFlags |= AUTH_SENT_SECOND_ACCOUNT_REQUEST;
				pAccInfo->lastRequestSeconds = now.tv_sec;
				m_Mutex.unlock();
				return 1;
			}
		}
		else
		{
			// send first request
			DOM_Document doc;
			makeAccountRequest(doc);
			pAccInfo->pControlChannel->sendMessage(doc);
			pAccInfo->authFlags |= AUTH_SENT_SECOND_ACCOUNT_REQUEST;
			pAccInfo->lastRequestSeconds = now.tv_sec;
			m_Mutex.unlock();
			return 1;
		}
	}

}


/** @todo maybe make this an own CAAbstractXMLEncodable class */
SINT32 CAAccountingInstance::makeCCRequest(
		const UINT64 accountNumber, 
		const UINT64 transferredBytes, 
		DOM_Document& doc
	)
{
	// create a DOM CostConfirmation document
	doc = DOM_Document::createDocument();
	DOM_Element elemRoot = doc.createElement("PayRequest");
	elemRoot.setAttribute("version", "1.0");
	doc.appendChild(elemRoot);
	
	DOM_Element elemCC = doc.createElement("CC");
	elemCC.setAttribute("version", "1.0");
	elemRoot.appendChild(elemCC);
	
	DOM_Element elemAccount = doc.createElement("AccountNumber");
	setDOMElementValue(elemAccount, accountNumber);
	elemCC.appendChild(elemAccount);

	DOM_Element elemBytes = doc.createElement("TransferredBytes");
	setDOMElementValue(elemBytes, transferredBytes);
	elemCC.appendChild(elemBytes);
	
	return E_SUCCESS;
}

/** @todo maybe make this an own CAAbstractXMLEncodable class */
SINT32 CAAccountingInstance::makeBalanceRequest(const SINT32 seconds, DOM_Document &doc)
{
	UINT8 timeBuf[128];
	UINT32 timeBufLen = 128;
	
	doc = DOM_Document::createDocument();
	DOM_Element elemRoot = doc.createElement("PayRequest");
	elemRoot.setAttribute("version", "1.0");
	doc.appendChild(elemRoot);
	DOM_Element elemAcc = doc.createElement("BalanceRequest");
	elemRoot.appendChild(elemAcc);
	DOM_Element elemDate = doc.createElement("NewerThan");
	elemAcc.appendChild(elemDate);
	formatJdbcTimestamp(seconds, timeBuf, timeBufLen);
	setDOMElementValue(elemDate, timeBuf);
	return E_SUCCESS;
}

/** @todo maybe make this an own CAAbstractXMLEncodable class */
SINT32 CAAccountingInstance::makeAccountRequest(DOM_Document &doc)
{
	doc = DOM_Document::createDocument();
	DOM_Element elemRoot = doc.createElement("PayRequest");
	elemRoot.setAttribute("version", "1.0");
	doc.appendChild(elemRoot);
	DOM_Element elemAcc = doc.createElement("AccountRequest");
	elemRoot.appendChild(elemAcc);
	return E_SUCCESS;
}

/**
 * The Main Loop of the accounting instance thread.
 * @todo terminate the whole firstMix on DB connection errors
 */
THREAD_RETURN CAAccountingInstance::aiThreadMainLoop( void *param )
{
	CAAccountingInstance * instance;
	CAQueue * pQueue;
	aiQueueItem * pQueueItem;
	UINT32 itemSize;

	pQueueItem = new aiQueueItem;
	itemSize = sizeof( aiQueueItem );
	instance = ( CAAccountingInstance * ) param;
	pQueue = instance->m_pQueue;

	CAMsg::printMsg( LOG_DEBUG, "AI Thread starting\n" );

	// initiate DB connection
/*	if ( instance->initDBConnection() != 0 )
		{
			CAMsg::printMsg( LOG_ERR, "Could not connect to Postgres Database! AI thread aborting\n" );
			return 0;
		}*/


	while ( true )
		{
			CAMsg::printMsg( LOG_DEBUG, "AI Thread waiting for message .. \n" );
			pQueue->getOrWait( ( UINT8 * ) pQueueItem, &itemSize );
			CAMsg::printMsg( LOG_DEBUG, "aiThread(): got message\n" );
			instance->processJapMessage( pQueueItem->pHashEntry, pQueueItem->pDomDoc );
			delete pQueueItem->pDomDoc;
			delete pQueueItem;
		}
	return (THREAD_RETURN)0;
}




/**
 * Handle a user (xml) message sent to us by the Jap. 
 *	
 * This function is running inside the AiThread. It determines 
 * what type of message we have and calls the appropriate handle...() 
 * function
 */
void CAAccountingInstance::processJapMessage(
	fmHashTableEntry * pHashEntry,
	DOM_Document * pDomDoc
)
{
	// parse XML and get tagname of document element (=type of message)
/*	MemBufInputSource oInput( pData, dataLen, "AccountingMessage" );
	DOMParser oParser;
	oParser.parse( oInput );
	DOM_Document doc = oParser.getDocument();*/
	DOM_Element root = pDomDoc->getDocumentElement();
	char * docElementName = root.getTagName().transcode();

	// what type of message is it?
	if ( strcmp( docElementName, "AccountCertificate" ) == 0 )
		{
			CAMsg::printMsg( LOG_DEBUG, "Received an AccountCertificate. Calling handleAccountCertificate()\n" );
			handleAccountCertificate( pHashEntry, root );
		}
	else if ( strcmp( docElementName, "CC" ) == 0 )
		{
			CAMsg::printMsg( LOG_DEBUG, "Received a CC. Calling handleCostConfirmation()\n" );
			handleCostConfirmation( pHashEntry, root );
		}
	else if ( strcmp( docElementName, "Balance" ) == 0 )
		{
			CAMsg::printMsg( LOG_DEBUG, "Received a BalanceCertificate. Calling handleBalanceCertificate()\n" );
			handleBalanceCertificate( pHashEntry, root );
		}
	else
		{
			CAMsg::printMsg( LOG_ERR, "Received XML message with unknown root element \"%s\". Ignoring...\n",
											 docElementName );
		}

	delete [] docElementName;
}


/**
 * Handles an account certificate of a newly connected Jap.
 * Parses accountnumber and publickey, checks the signature
 * and generates and sends a challenge XML structure to the 
 * Jap.
 * @todo think about switching account without changing mixcascade
 *   (receive a new acc.cert. though we already have one)
 * @todo set authFlags correctly
 */
void CAAccountingInstance::handleAccountCertificate(
		fmHashTableEntry *pHashEntry,
		DOM_Element &root
	)
{
	aiAccountingInfo * pAccInfo = pHashEntry->pAccountingInfo;
	DOM_Element elGeneral;

	// check authstate of this user
	m_Mutex.lock();
	if(pAccInfo->authFlags&AUTH_GOT_ACCOUNTCERT)
		{
			CAMsg::printMsg(LOG_DEBUG, "Already got an account cert. Ignoring!");
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_REQUEST, (UINT8*)"You have already sent an Account Certificate");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			m_Mutex.unlock();
			return ;
		}

	// parse & set accountnumber
	if ( getDOMChildByName( root, (UINT8 *)"AccountNumber", elGeneral, false ) != E_SUCCESS ||
			getDOMElementValue( elGeneral, pAccInfo->accountNumber ) != E_SUCCESS ||
			pAccInfo->accountNumber == 0 )
		{
			CAMsg::printMsg( LOG_ERR, "AccountCertificate has wrong or no accountnumber. Ignoring\n");
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_FORMAT);
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			m_Mutex.unlock();
			return ;
		}

	// parse & set public key
	if ( getDOMChildByName( root, (UINT8 *)"JapPublicKey", elGeneral, false ) != E_SUCCESS )
		{
			CAMsg::printMsg( LOG_ERR, "AccountCertificate contains not public key. Ignoring\n");
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_KEY_NOT_FOUND);
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			m_Mutex.unlock();
			return ;
		}
	pAccInfo->pPublicKey = new CASignature();
	if ( pAccInfo->pPublicKey->setVerifyKey( elGeneral ) != E_SUCCESS )
		{
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_INTERNAL_SERVER_ERROR);
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			m_Mutex.unlock();
			return ;
		}

	if ( m_pJpiVerifyingInstance->verifyXML( (DOM_Node &)root, (CACertStore *)NULL ) != E_SUCCESS )
		{
			// signature invalid. mark this user as bad guy
			CAMsg::printMsg( LOG_INFO, "CAAccountingInstance::handleAccountCertificate(): Bad Jpi signature" );
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_SIGNATURE, (UINT8*)"Your account certificate is invalid");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			pAccInfo->authFlags |= AUTH_FAKE | AUTH_GOT_ACCOUNTCERT;
			pAccInfo->authFlags &= ~AUTH_ACCOUNT_OK;
			m_Mutex.unlock();
			return ;
		}
		
	UINT8 * arbChallenge;
	UINT8 b64Challenge[ 512 ];
	UINT32 b64Len = 512;

	// generate random challenge data and Base64 encode it
	arbChallenge = ( UINT8* ) malloc( 222 );
	getRandom( arbChallenge, 222 );
	CABase64::encode( arbChallenge, 222, b64Challenge, &b64Len );
	if ( pAccInfo->pChallenge != NULL )
		delete[] pAccInfo->pChallenge;
	pAccInfo->pChallenge = arbChallenge; // store challenge for later..

	// generate XML challenge structure
	DOM_Document doc = DOM_Document::createDocument();
	DOM_Element elemRoot = doc.createElement( "Challenge" );
	DOM_Element elemPanic = doc.createElement( "DontPanic" );
	elemPanic.setAttribute( "version", "1.0" );
	doc.appendChild( elemRoot );
	elemRoot.appendChild( elemPanic );
	setDOMElementValue( elemPanic, b64Challenge );

	// send XML struct to Jap & set auth flags
	pAccInfo->pControlChannel->sendMessage(doc);
	pAccInfo->authFlags = AUTH_CHALLENGE_SENT | AUTH_GOT_ACCOUNTCERT;
	m_Mutex.unlock();
}




/**
 * Handles the response to our challenge.
 * Checks the validity of the response and sets the user's authFlags
 * accordingly.
 */
void CAAccountingInstance::handleChallengeResponse(
	fmHashTableEntry *pHashEntry,
	const DOM_Element &root
)
{
	UINT8 decodeBuffer[ 512 ];
	UINT32 decodeBufferLen = 512;
	UINT32 usedLen;
	DOM_Element elemPanic;
	DSA_SIG * pDsaSig;
	aiAccountingInfo * pAccInfo = pHashEntry->pAccountingInfo;

	// check current authstate
	m_Mutex.lock();
	if( (!(pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT)) ||
			(!(pAccInfo->authFlags & AUTH_CHALLENGE_SENT))
		)
		{
			m_Mutex.unlock();
			return ;
		}
	pAccInfo->authFlags &= ~AUTH_CHALLENGE_SENT;
	m_Mutex.unlock();

	// get raw bytes of response
	if ( getDOMChildByName( root, (UINT8*)"DontPanic", elemPanic, false ) != E_SUCCESS ||
			 getDOMElementValue( elemPanic, decodeBuffer, &decodeBufferLen ) != E_SUCCESS )
		{
			CAMsg::printMsg( LOG_DEBUG, "ChallengeResponse has wrong XML format\n" );
			return ;
		}
	usedLen = decodeBufferLen;
	decodeBufferLen = 512;
	CABase64::decode( decodeBuffer, usedLen, decodeBuffer, &decodeBufferLen );
	pDsaSig = DSA_SIG_new();
	
	m_Mutex.lock();
	// check signature
	CASignature * sigTester = pHashEntry->pAccountingInfo->pPublicKey;
	sigTester->encodeRS( decodeBuffer, &usedLen, pDsaSig );
	if ( sigTester->verify( pHashEntry->pAccountingInfo->pChallenge, 222, pDsaSig ) != E_SUCCESS )
		{
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_SIGNATURE, (UINT8*)"Challenge-response authentication failed");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			pAccInfo->authFlags |= AUTH_FAKE;
			pAccInfo->authFlags &= ~AUTH_ACCOUNT_OK;
			m_Mutex.unlock();
			return ;
		}
		
	pAccInfo->authFlags |= AUTH_ACCOUNT_OK;
	if ( pHashEntry->pAccountingInfo->pChallenge != NULL ) // free mem
		{
			delete[] pHashEntry->pAccountingInfo->pChallenge;
			pHashEntry->pAccountingInfo->pChallenge = NULL;
		}
	m_Mutex.unlock();
}





/**
 * Handles a cost confirmation sent by a jap
 */
void CAAccountingInstance::handleCostConfirmation(
	fmHashTableEntry *pHashEntry,
	const DOM_Element &root
)
{
	CASignature * pSigTester;
	aiAccountingInfo *pAccInfo;
	pAccInfo = pHashEntry->pAccountingInfo;
	DOM_Element elemGeneral;	
	UINT8 strGeneral[ 1024 ];
	UINT32 strGeneralLen = 1024;
	UINT64 bytes;

	
	m_Mutex.lock();
	
	// check authstate	
	if( (!pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT) ||
			(!pAccInfo->authFlags & AUTH_ACCOUNT_OK))
		{
			m_Mutex.unlock();
			return ;
		}

	// check signature
	pSigTester = pAccInfo->pPublicKey;
	if ( pSigTester->verifyXML( (DOM_Node &)root, (CACertStore *)NULL ) != E_SUCCESS )
		{
			// wrong signature
			CAMsg::printMsg( LOG_DEBUG, "CostConfirmation has INVALID SIGNATURE!\n" );
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_SIGNATURE, (UINT8*)"CostConfirmation has bad signature");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			m_Mutex.unlock();
			return ;
		}
	m_Mutex.unlock();

	// parse and check AI name
	if ( getDOMChildByName( root, (UINT8*)"AIName", elemGeneral, false ) != E_SUCCESS ||
			 getDOMElementValue( elemGeneral, strGeneral, &strGeneralLen ) != E_SUCCESS ||
			 strcmp( (char *)strGeneral, (char *)m_AiName ) != 0 )
		{
			CAMsg::printMsg( LOG_DEBUG, "CostConfirmation has wrong AIName %s. Ignoring...\n", strGeneral );
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_DATA, (UINT8*)"Your CostConfirmation has a wrong AI name");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			return ;
		}

	// parse & set transferredBytes
	m_Mutex.lock();
	if ( getDOMChildByName( root, (UINT8*)"Bytes", elemGeneral, false ) != E_SUCCESS ||
			 getDOMElementValue( elemGeneral, bytes ) != E_SUCCESS ||
			 bytes < pAccInfo->confirmedBytes )
		{
			CAMsg::printMsg( LOG_DEBUG, "CostConfirmation has Wrong Number of Bytes (%lld). Ignoring...\n", bytes );
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_DATA, 
				(UINT8*)"Your CostConfirmation has a wrong number of transferred bytes");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			m_Mutex.unlock();
			return ;
		}
	pAccInfo->confirmedBytes = bytes;
	if(pAccInfo->confirmedBytes == pAccInfo->reqConfirmBytes)
	{
		pAccInfo->authFlags &= ~AUTH_SENT_CC_REQUEST;
		pAccInfo->authFlags &= ~AUTH_SENT_SECOND_CC_REQUEST;
	}

	// store XMLCC in database
	strGeneralLen = 1024;
	if( (DOM_Output::dumpToMem((DOM_Node &)root, strGeneral, &strGeneralLen) != E_SUCCESS) ||
			(strGeneralLen >= 1024))
	{
		m_Mutex.unlock();
		CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_DATA, 
			(UINT8*)"Your CostConfirmation is too long");
		DOM_Document errDoc;
		err.toXmlDocument(errDoc);
		pAccInfo->pControlChannel->sendMessage(errDoc);
		return ;
	}
	strGeneral[strGeneralLen] = '\0'; // make null-terminated string
	m_dbInterface->storeCostConfirmation( pAccInfo->accountNumber, bytes, strGeneral, false );
	m_Mutex.unlock();
	return ;
}



/**
 * Handles a balance certificate sent by the JAP.
 * Checks signature and age.
 * @todo set authFlags
 * @todo send back XMLErrorMessages on failure
 */
void CAAccountingInstance::handleBalanceCertificate(
		fmHashTableEntry *pHashEntry,
		const DOM_Element &root
	)
{
	CASignature * pSigTester;
	UINT8 strGeneral[ 256 ];
	UINT32 strGeneralLen = 256;
	UINT64 u64General;
	SINT32 seconds;
	DOM_Element elGeneral;
	aiAccountingInfo * pAccInfo = pHashEntry->pAccountingInfo;
	

	// test signature
	m_Mutex.lock();
	pSigTester = pHashEntry->pAccountingInfo->pPublicKey;
	if ( !pSigTester || (pSigTester->verifyXML( (DOM_Node &)root, (CACertStore *)NULL ) != E_SUCCESS) )
		{
			CAMsg::printMsg( LOG_DEBUG, "BalanceCertificate has BAD SIGNATURE! Ignoring\n" );
			m_Mutex.unlock();
			return ;
		}
	m_Mutex.unlock();

	// parse timestamp
	if ( (getDOMChildByName( root, (UINT8*)"Timestamp", elGeneral, false ) != E_SUCCESS) 
		|| (getDOMElementValue( elGeneral, strGeneral, &strGeneralLen ) != E_SUCCESS) )
		{
			return ;
		}
	CAMsg::printMsg( LOG_DEBUG, "BalCert->Timestamp (unparsed): %s\n", strGeneral );
	parseJdbcTimestamp(strGeneral, seconds);
	
	m_Mutex.lock();
	if(seconds < pAccInfo->reqbalMinSeconds)
	{
		// todo send a request again...
		// TODO: if too old -> ask JPI for a new one
		// if JPI is not available at present -> let user proceed for a while
		m_Mutex.unlock();
		return ;
	}
	//pAccInfo->lastbalSeconds = seconds;
	
	// parse & set deposit
	if( (getDOMChildByName( root, (UINT8*)"Deposit", elGeneral, false ) != E_SUCCESS) ||
			(getDOMElementValue( elGeneral, u64General ) != E_SUCCESS) ||
			(u64General < pAccInfo->lastbalDeposit) )
	{
		m_Mutex.unlock();
		return;
	}
	pAccInfo->lastbalDeposit = u64General;
	
	// parse & set spent
	if( (getDOMChildByName( root, (UINT8*)"Spent", elGeneral, false ) != E_SUCCESS) ||
			(getDOMElementValue( elGeneral, u64General ) != E_SUCCESS) ||
			(u64General < pAccInfo->lastbalSpent) )
	{
		m_Mutex.unlock();
		return;
	}
	pAccInfo->lastbalSpent = u64General;
}


/**
 * This must be called whenever a JAP is connecting to init our per-user
 * data structures
 */
SINT32 CAAccountingInstance::initTableEntry( fmHashTableEntry * pHashEntry )
{
	pHashEntry->pAccountingInfo = new aiAccountingInfo;
	memset( pHashEntry->pAccountingInfo, 0, sizeof( aiAccountingInfo ) );
	return E_SUCCESS;
}



/**
 * This should always be called when closing a JAP connection
 * to cleanup the payment data structures
 * @todo rewrite
 */
SINT32 CAAccountingInstance::cleanupTableEntry( fmHashTableEntry *pHashEntry )
{
	CAMsg::printMsg( LOG_DEBUG, "cleanupTableEntry()\n" );
	aiAccountingInfo * pAccInfo;
	if ( pHashEntry->pAccountingInfo != 0 )
		{
			pAccInfo = pHashEntry->pAccountingInfo;
			if(pAccInfo->pLastXmlCC)
				{
					delete [] pAccInfo->pLastXmlCC;
				}
			
			if ( pAccInfo->pPublicKey )
				{
					// todo check is "delete" ok here?
					delete pAccInfo->pPublicKey;
				}
			if ( pAccInfo->pChallenge )
				{
					delete [] pAccInfo->pChallenge;
				}
			delete pHashEntry->pAccountingInfo;
		}
	return E_SUCCESS;
}



/**
 * Returns the singleton instance of the AccountingInstance.
 */
/*CAAccountingInstance * CAAccountingInstance::getInstance()
{
	if ( CAAccountingInstance::ms_pInstance == 0 )
		{
			CAAccountingInstance::ms_pInstance = new CAAccountingInstance();
		}
	return CAAccountingInstance::ms_pInstance;
}*/

#endif /* ifdef PAYMENT */
