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



/**
 * Singleton: This is the reference to the only instance of this class
 */
CAAccountingInstance * CAAccountingInstance::ms_pInstance = NULL;



/**
 * private Constructor
 */
CAAccountingInstance::CAAccountingInstance()
	{
	
		CAMsg::printMsg( LOG_DEBUG, "AccountingInstance initialising\n" );
		m_pQueue = new CAQueue();
		m_pIPBlockList = new CATempIPBlockList(60000);
		
		// initialize Database connection
		m_dbInterface = new CAAccountingDBInterface();
		if(m_dbInterface->initDBConnection() != E_SUCCESS)
		{
			CAMsg::printMsg( LOG_ERR, "**************** AccountingInstance: Could not connect to DB!");
			exit(1);
		}
	
		// initialize JPI signataure tester
		m_AiName = new UINT8[256];
		options.getAiID(m_AiName, 256);
		m_pJpiVerifyingInstance = options.getBI()->getVerifier();
		options.getPaymentHardLimit(&m_iHardLimitBytes);
		options.getPaymentSoftLimit(&m_iSoftLimitBytes);
	
		// launch AI thread
		m_pThread = new CAThread();
		m_pThread->setMainLoop( aiThreadMainLoop );
		m_bThreadRunning = true;
		m_pThread->start( this );
		
		// launch BI settleThread
		m_pSettleThread = new CAAccountingSettleThread();
	}



/**
 * private desctructor
 */
CAAccountingInstance::~CAAccountingInstance()
	{
		CAMsg::printMsg( LOG_DEBUG, "AccountingInstance dying\n" );
		m_bThreadRunning = false;
		m_pThread->join();
		delete m_pThread;
		//delete m_biInterface;
		delete m_dbInterface;
		delete m_pIPBlockList;
		delete m_pQueue;
	}



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
SINT32 CAAccountingInstance::handleJapPacket( MIXPACKET *pPacket, fmHashTableEntry *pHashEntry)
{
	aiAccountingInfo * pAccInfo;
	timespec now;	
	getcurrentTime(now);
	pAccInfo = pHashEntry->pAccountingInfo;
	
	
	m_Mutex.lock();
	pAccInfo->transferredBytes += sizeof(MIXPACKET); // count the packet	
	
	if(pAccInfo->authFlags & (AUTH_FATAL_ERROR))
	{
		// there was an error earlier..
		return 3;
	}
	
	if(pAccInfo->authFlags & (AUTH_GOT_ACCOUNTCERT) )
	{
	
		//----------------------------------------------------------
		// authentication process not properly finished
		if( pAccInfo->authFlags & AUTH_FAKE )
			{
				m_Mutex.unlock();
				CAMsg::printMsg( LOG_DEBUG, "AccountingInstance: AUTH_FAKE flag is set ... byebye\n");
				return 3;
			}
		if( !(pAccInfo->authFlags & AUTH_ACCOUNT_OK) )
		{
			// we did not yet receive the response to the challenge...
			if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
			{
				m_Mutex.unlock();
				CAMsg::printMsg( LOG_DEBUG, "AccountingInstance: Jap refused to send response to challenge (Request Timeout)...\n");
				return 3;
			}
		}
		
		if( pAccInfo->authFlags & AUTH_ACCOUNT_EMPTY )
			{
				CAMsg::printMsg( LOG_DEBUG, "AccountingInstance: Account is empty. Closing connection.\n");
				CAXMLErrorMessage msg(CAXMLErrorMessage::ERR_ACCOUNT_EMPTY);
				DOM_Document doc;
				msg.toXmlDocument(doc);
				pAccInfo->pControlChannel->sendMessage(doc);
				pAccInfo->authFlags |= AUTH_FATAL_ERROR;
				m_Mutex.unlock();
				return 3;
			}

		//----------------------------------------------------------
		// Hardlimit corstconfirmation check
		else if ((pAccInfo->transferredBytes-pAccInfo->confirmedBytes) >= m_iHardLimitBytes)
			{
				#ifdef DEBUG
				CAMsg::printMsg( LOG_DEBUG, "Accounting instance: User refused "
													"to send cost confirmation (HARDLIMIT EXCEEDED).\n");
				#endif
				m_pIPBlockList->insertIP( pHashEntry->peerIP );
				CAXMLErrorMessage msg(CAXMLErrorMessage::ERR_NO_CONFIRMATION);
				DOM_Document doc;
				msg.toXmlDocument(doc);
				pAccInfo->pControlChannel->sendMessage(doc);
				pAccInfo->authFlags |= AUTH_FATAL_ERROR;
				m_Mutex.unlock();
				return 3;
			}
	
		//-------------------------------------------------------
		// check: is it time to request a new cost confirmation?
		else if( (pAccInfo->transferredBytes-pAccInfo->confirmedBytes) >= m_iSoftLimitBytes)
		{
			if( (pAccInfo->authFlags & AUTH_SENT_CC_REQUEST) )
			{
				if( (pAccInfo->authFlags & AUTH_SENT_SECOND_CC_REQUEST))
				{
					if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
					{
						#ifdef DEBUG
						CAMsg::printMsg( LOG_DEBUG, "Accounting instance: User refused "
															"to send cost confirmation (REQUEST TIMEOUT).\n");
						#endif
						m_pIPBlockList->insertIP( pHashEntry->peerIP );
						CAXMLErrorMessage msg(CAXMLErrorMessage::ERR_NO_CONFIRMATION);
						DOM_Document doc;
						msg.toXmlDocument(doc);
						pAccInfo->pControlChannel->sendMessage(doc);
						pAccInfo->authFlags |= AUTH_FATAL_ERROR;
						m_Mutex.unlock();
						return 3;
					}
					else
					{
						m_Mutex.unlock();
						return 1;
					}
				}
				else if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
				{
					// send a reminder...
					DOM_Document doc;
					#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG, "AccountingInstance sending REMINDER CC request.\n");
					#endif
					makeCCRequest(pAccInfo->accountNumber, pAccInfo->transferredBytes, doc);
					pAccInfo->pControlChannel->sendMessage(doc);
					pAccInfo->authFlags |= AUTH_SENT_SECOND_CC_REQUEST;
					pAccInfo->lastRequestSeconds = now.tv_sec;
					m_Mutex.unlock();
					return 1;
				}
				else
				{
					m_Mutex.unlock();
					return 1;
				}
			}
			else
			{
				// send a first CC request
				DOM_Document doc;
				#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "AccountingInstance sending CC request.\n");
				#endif
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
			/// TODO: Make the numbers vvv configurable
			if( (( (pAccInfo->transferredBytes - pAccInfo->lastbalTransferredBytes) >=
					((pAccInfo->lastbalDeposit - pAccInfo->lastbalSpent) / 33))&&
					((pAccInfo->transferredBytes - pAccInfo->lastbalTransferredBytes) >= 256*1024))
					|| (pAccInfo->lastbalDeposit == 0))
			{
				if( (pAccInfo->authFlags & AUTH_SENT_BALANCE_REQUEST) )
				{
					if( (pAccInfo->authFlags & AUTH_SENT_SECOND_BALANCE_REQUEST))
					{
						if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
						{
							#ifdef DEBUG
							CAMsg::printMsg( LOG_DEBUG, "Accounting instance: User refused "
																"to send balance cert (request Timeout).\n",
																pAccInfo->accountNumber, pHashEntry->peerIP[ 0 ], pHashEntry->peerIP[ 1 ],
																pHashEntry->peerIP[ 2 ], pHashEntry->peerIP[ 3 ] );
							#endif
							m_pIPBlockList->insertIP( pHashEntry->peerIP );
							CAXMLErrorMessage msg(CAXMLErrorMessage::ERR_NO_BALANCE);
							DOM_Document doc;
							msg.toXmlDocument(doc);
							pAccInfo->pControlChannel->sendMessage(doc);
							pAccInfo->authFlags |= AUTH_FATAL_ERROR;
							m_Mutex.unlock();
							return 3;
						}
						else
						{
							m_Mutex.unlock();
							return 1;
						}
					}
					else if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
					{
						// send a reminder...
						// TODO adjust "newerThan" value
						DOM_Document doc;
						#ifdef DEBUG
						CAMsg::printMsg(LOG_DEBUG, "AccountingInstance sending REMINDER balance request.\n");
						#endif
						makeBalanceRequest((SINT32)now.tv_sec-600, doc);
						pAccInfo->reqbalMinSeconds = now.tv_sec - 600;
						pAccInfo->pControlChannel->sendMessage(doc);
						pAccInfo->authFlags |= AUTH_SENT_SECOND_BALANCE_REQUEST;
						pAccInfo->lastRequestSeconds = now.tv_sec;
						m_Mutex.unlock();
						return 1;
					}
					else
					{
						m_Mutex.unlock();
						return 1;
					}
				}
				else
				{
					// send a first CC request
					DOM_Document doc;
					makeBalanceRequest(now.tv_sec-600, doc);
					#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG, "AccountingInstance sending balance request.\n");
					#endif
					pAccInfo->reqbalMinSeconds = now.tv_sec - 600;
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
					#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Accounting instance: User refused "
												"to send account certificate.(request timeout)\n");
					#endif
					CAXMLErrorMessage msg(CAXMLErrorMessage::ERR_NO_ACCOUNTCERT);
					DOM_Document doc;
					msg.toXmlDocument(doc);
					pAccInfo->pControlChannel->sendMessage(doc);
					pAccInfo->authFlags |= AUTH_FATAL_ERROR;
					m_Mutex.unlock();
					return 2;
				}
				m_Mutex.unlock();
				return 1;
			}
			if(now.tv_sec >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
			{
				// send reminder
				#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "AccountingInstance sending REMINDER account request.\n");
				#endif
				DOM_Document doc;
				makeAccountRequest(doc);
				pAccInfo->pControlChannel->sendMessage(doc);
				pAccInfo->authFlags |= AUTH_SENT_SECOND_ACCOUNT_REQUEST;
				pAccInfo->lastRequestSeconds = now.tv_sec;
			}
			m_Mutex.unlock();
			return 1;
		}
		else
		{
			// send first request
			#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "AccountingInstance sending account request.\n");
			#endif
			DOM_Document doc;
			makeAccountRequest(doc);
			pAccInfo->pControlChannel->sendMessage(doc);
			pAccInfo->authFlags |= AUTH_SENT_ACCOUNT_REQUEST;
			pAccInfo->lastRequestSeconds = now.tv_sec;
			m_Mutex.unlock();
			return 1;
		}
	}
	CAMsg::printMsg(LOG_ERR, 
			"Unknown error in CAAccountingInstance::handleJapPacket()."
			"..... this should never happen!\n"
		);
	return 4;
}


/** TODO: maybe make this an own CAAbstractXMLEncodable subclass */
SINT32 CAAccountingInstance::makeCCRequest(const UINT64 accountNumber, const UINT64 transferredBytes, DOM_Document& doc)
{
	// create a DOM CostConfirmation document
	doc = DOM_Document::createDocument();
	DOM_Element elemRoot = doc.createElement("PayRequest");
	elemRoot.setAttribute("version", "1.0");
	doc.appendChild(elemRoot);
	
	DOM_Element elemCC = doc.createElement("CC");
	elemCC.setAttribute("version", "1.0");
	elemRoot.appendChild(elemCC);
	
	DOM_Element elemAiName = doc.createElement("AiID");
	setDOMElementValue(elemAiName, m_AiName);
	elemCC.appendChild(elemAiName);
	
	DOM_Element elemAccount = doc.createElement("AccountNumber");
	setDOMElementValue(elemAccount, (UINT64)accountNumber);
	elemCC.appendChild(elemAccount);

	DOM_Element elemBytes = doc.createElement("TransferredBytes");
	setDOMElementValue(elemBytes, transferredBytes);
	elemCC.appendChild(elemBytes);
	
	return E_SUCCESS;
}

/** TODO: maybe make this an own CAAbstractXMLEncodable subclass */
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

/** TODO: maybe make this an own CAAbstractXMLEncodable subclass */
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
 * Reads messages out of the queue and processes them
 */
THREAD_RETURN CAAccountingInstance::aiThreadMainLoop( void *param )
{
	CAAccountingInstance * instance;
	aiQueueItem item;
	UINT32 itemSize;
	instance = ( CAAccountingInstance * ) param;
	CAMsg::printMsg( LOG_DEBUG, "AI Thread starting\n" );

	while ( instance->m_bThreadRunning )
		{
			itemSize = sizeof( aiQueueItem );
			instance->m_pQueue->getOrWait( (UINT8*)&item, &itemSize );
			instance->processJapMessage( item.pHashEntry, item.pDomDoc );
			//delete item.domDoc; (??)
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
void CAAccountingInstance::processJapMessage(fmHashTableEntry * pHashEntry,DOM_Document * pDomDoc)
{
	DOM_Element root = pDomDoc->getDocumentElement();
	char * docElementName = root.getTagName().transcode();

	// what type of message is it?
	if ( strcmp( docElementName, "AccountCertificate" ) == 0 )
		{
			#ifdef DEBUG
				CAMsg::printMsg( LOG_DEBUG, "Received an AccountCertificate. Calling handleAccountCertificate()\n" );
			#endif
			handleAccountCertificate( pHashEntry, root );
		}
	else if ( strcmp( docElementName, "Response" ) == 0)
		{
			#ifdef DEBUG
				CAMsg::printMsg( LOG_DEBUG, "Received a Response (challenge-response)\n");
			#endif
			handleChallengeResponse( pHashEntry, root );
		}
	else if ( strcmp( docElementName, "CC" ) == 0 )
		{
			#ifdef DEBUG
				CAMsg::printMsg( LOG_DEBUG, "Received a CC. Calling handleCostConfirmation()\n" );
			#endif
			handleCostConfirmation( pHashEntry, root );
		}
	else if ( strcmp( docElementName, "Balance" ) == 0 )
		{
			#ifdef DEBUG
				CAMsg::printMsg( LOG_DEBUG, "Received a BalanceCertificate. Calling handleBalanceCertificate()\n" );
			#endif
			handleBalanceCertificate( pHashEntry, root );
		}
	else
		{
			CAMsg::printMsg( 
					LOG_ERR, 
					"AI Received XML message with unknown root element \"%s\". Ignoring...\n",
					docElementName 
				);
		}
	delete pDomDoc;
	delete [] docElementName;
}


/**
 * Handles an account certificate of a newly connected Jap.
 * Parses accountnumber and publickey, checks the signature
 * and generates and sends a challenge XML structure to the 
 * Jap.
 * TODO: think about switching account without changing mixcascade
 *   (receive a new acc.cert. though we already have one)
 */
void CAAccountingInstance::handleAccountCertificate(fmHashTableEntry *pHashEntry, DOM_Element &root)
{
	aiAccountingInfo * pAccInfo = pHashEntry->pAccountingInfo;
	DOM_Element elGeneral;
	timespec now;
	getcurrentTime(now);

	// check authstate of this user
	m_Mutex.lock();
	if(pAccInfo->authFlags&AUTH_GOT_ACCOUNTCERT)
		{
			#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "Already got an account cert. Ignoring!");
			#endif
			CAXMLErrorMessage err(
					CAXMLErrorMessage::ERR_BAD_REQUEST, 
					(UINT8*)"You have already sent an Account Certificate"
				);
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
			CAMsg::printMsg( LOG_ERR, "AccountCertificate contains no public key. Ignoring\n");
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_KEY_NOT_FOUND);
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			m_Mutex.unlock();
			return ;
		}
	#ifdef DEBUG
		UINT8* aij;
		UINT32 aijsize;
		aij = DOM_Output::dumpToMem(elGeneral, &aijsize);
		aij[aijsize-1]=0;
		CAMsg::printMsg( LOG_DEBUG, "Setting user public key %s>\n", aij );
		delete[] aij;
	#endif
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

	if ( (!m_pJpiVerifyingInstance)||
			(m_pJpiVerifyingInstance->verifyXML( (DOM_Node &)root, (CACertStore *)NULL ) != E_SUCCESS ))
		{
			// signature invalid. mark this user as bad guy
			CAMsg::printMsg( LOG_INFO, "CAAccountingInstance::handleAccountCertificate(): Bad Jpi signature\n" );
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
	pAccInfo->lastRequestSeconds = now.tv_sec;
	m_Mutex.unlock();
}




/**
 * Handles the response to our challenge.
 * Checks the validity of the response and sets the user's authFlags
 * accordingly.
 */
void CAAccountingInstance::handleChallengeResponse(fmHashTableEntry *pHashEntry, const DOM_Element &root)
{
	UINT8 decodeBuffer[ 512 ];
	UINT32 decodeBufferLen = 512;
	UINT32 usedLen;
	DOM_Element elemPanic;
	//DSA_SIG * pDsaSig;
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
	//m_Mutex.unlock();

	// get raw bytes of response
	if ( getDOMElementValue( root, decodeBuffer, &decodeBufferLen ) != E_SUCCESS )
		{
			CAMsg::printMsg( LOG_DEBUG, "ChallengeResponse has wrong XML format. Ignoring\n" );
			return ;
		}
	usedLen = decodeBufferLen;
	decodeBufferLen = 512;
	CABase64::decode( decodeBuffer, usedLen, decodeBuffer, &decodeBufferLen );
//	pDsaSig = DSA_SIG_new();
	
	//m_Mutex.lock();
	// check signature
	CASignature * sigTester = pHashEntry->pAccountingInfo->pPublicKey;
		#pragma message Signature verifying must be implemented here !!!!!!!!!!
	//sigTester->decodeRS( decodeBuffer, decodeBufferLen, pDsaSig );
	/// TODO: Really do signature checking here...
/*	if ( sigTester->verifyDER( pHashEntry->pAccountingInfo->pChallenge, 222, decodeBuffer, decodeBufferLen ) 
		!= E_SUCCESS )
		{
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_SIGNATURE, 
					(UINT8*)"Challenge-response authentication failed!!\n");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			pAccInfo->authFlags |= AUTH_FAKE;
			pAccInfo->authFlags &= ~AUTH_ACCOUNT_OK;
			m_Mutex.unlock();
			return ;
		}*/
		
	pAccInfo->authFlags |= AUTH_ACCOUNT_OK;
	
	// fetch cost confirmation from last session if available
	CAXMLCostConfirmation * pCC = NULL;
	m_dbInterface->getCostConfirmation(pAccInfo->accountNumber, &pCC);
	if(pCC!=NULL)
		{
			pAccInfo->transferredBytes += pCC->getTransferredBytes();
			#ifdef DEBUG
				CAMsg::printMsg(LOG_DEBUG, "TransferredBytes is now %lld\n", pAccInfo->transferredBytes);
			#endif
			pAccInfo->confirmedBytes = pCC->getTransferredBytes();
			delete pCC;
		}
	
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
void CAAccountingInstance::handleCostConfirmation(fmHashTableEntry *pHashEntry,DOM_Element &root)
{
	aiAccountingInfo *pAccInfo;
	
	pAccInfo = pHashEntry->pAccountingInfo;
	m_Mutex.lock();
	
	// check authstate	
	if( (!pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT) ||
			(!pAccInfo->authFlags & AUTH_ACCOUNT_OK))
		{
			m_Mutex.unlock();
			return ;
		}
		
		
	// for debugging only: test signature the oldschool way
	// warning this removes the signature from doc!!!
/*	if ( (!pAccInfo->pPublicKey)||
			(pAccInfo->pPublicKey->verifyXML( (DOM_Node &)root ) != E_SUCCESS ))
		{
			// wrong signature
			CAMsg::printMsg( LOG_INFO, "CostConfirmation has INVALID SIGNATURE!\n" );
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_SIGNATURE, (UINT8*)"CostConfirmation has bad signature");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			m_Mutex.unlock();
			return ;
		}*/
		

	CAXMLCostConfirmation cc(root);
	
	// TODO: Make this work.... :-//
	if( cc.verifySignature( *(pAccInfo->pPublicKey)) != E_SUCCESS)
		{
			// wrong signature
			CAMsg::printMsg( LOG_INFO, "CostConfirmation has INVALID SIGNATURE! (IGNORING)\n" );
			/*CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_SIGNATURE, (UINT8*)"CostConfirmation has bad signature");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			m_Mutex.unlock();
			return ;*/
		}
	#ifdef DEBUG
	else
		{
			CAMsg::printMsg( LOG_DEBUG, "CostConfirmation Signature is OK.\n");
		}
	#endif
	m_Mutex.unlock();
	

	// parse and check AI name
	UINT8 * pAiID = cc.getAiID();
	if( strcmp( (char *)pAiID, (char *)m_AiName ) != 0)
		{
			CAMsg::printMsg( LOG_INFO, "CostConfirmation has wrong AIName %s. Ignoring...\n", pAiID );
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_DATA, (UINT8*)"Your CostConfirmation has a wrong AI name");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			delete[] pAiID;
			return ;
		}
	delete[] pAiID;

	// parse & set transferredBytes
	m_Mutex.lock();
	if(cc.getTransferredBytes() < pAccInfo->confirmedBytes )
		{
			CAMsg::printMsg( LOG_INFO, "CostConfirmation has Wrong Number of Bytes (%lld). Ignoring...\n", cc.getTransferredBytes() );
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_DATA, 
				(UINT8*)"Your CostConfirmation has a wrong number of transferred bytes");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendMessage(errDoc);
			m_Mutex.unlock();
			return ;
		}
	pAccInfo->confirmedBytes = cc.getTransferredBytes();
	if(pAccInfo->confirmedBytes >= pAccInfo->reqConfirmBytes)
	{
		pAccInfo->authFlags &= ~AUTH_SENT_CC_REQUEST;
		pAccInfo->authFlags &= ~AUTH_SENT_SECOND_CC_REQUEST;
	}
	m_Mutex.unlock();
	
	m_dbInterface->storeCostConfirmation( cc );
	return ;
}



/**
 * Handles a balance certificate sent by the JAP.
 * Checks signature and age.
 * @todo set authFlags
 * @todo send back XMLErrorMessages on failure
 */
void CAAccountingInstance::handleBalanceCertificate(fmHashTableEntry *pHashEntry, const DOM_Element &root)
{
	CASignature * pSigTester;
	UINT8 strGeneral[ 256 ];
	UINT32 strGeneralLen = 256;
	UINT64 newDeposit, newSpent;
	SINT32 seconds;
	DOM_Element elGeneral;
	aiAccountingInfo * pAccInfo = pHashEntry->pAccountingInfo;
	

	// test signature
	m_Mutex.lock();
	//pSigTester = pHashEntry->pAccountingInfo->pPublicKey;
	if( !m_pJpiVerifyingInstance || 
			(m_pJpiVerifyingInstance->verifyXML( (DOM_Node &)root, (CACertStore *)NULL ) != E_SUCCESS) 
		)
		{
			CAMsg::printMsg( LOG_INFO, "BalanceCertificate has BAD SIGNATURE! Ignoring\n" );
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
			(getDOMElementValue( elGeneral, newDeposit ) != E_SUCCESS) ||
			(newDeposit < pAccInfo->lastbalDeposit) )
	{
		m_Mutex.unlock();
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "invalid deposit value...");
		#endif
		return;
	}
	#ifdef DEBUG
	else {
		CAMsg::printMsg(LOG_DEBUG, "Balance: deposit=%lld\n", newDeposit);
	}
	#endif
	
	// parse & set spent
	if( (getDOMChildByName( root, (UINT8*)"Spent", elGeneral, false ) != E_SUCCESS) ||
			(getDOMElementValue( elGeneral, newSpent ) != E_SUCCESS) ||
			(newSpent < pAccInfo->lastbalSpent) )
	{
		m_Mutex.unlock();
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "invalid spent value...");
		#endif
		return;
	}
	#ifdef DEBUG
	else {
		CAMsg::printMsg(LOG_DEBUG, "Balance: Spent=%lld\n", newSpent);
	}
	#endif
	
	// some checks for empty accounts
	if(	(newDeposit-newSpent <= MIN_BALANCE ) ||
			((newDeposit-newSpent-pAccInfo->transferredBytes) <= MIN_BALANCE ) 
		)
	{
		// mark account as empty
		pAccInfo->authFlags |= AUTH_ACCOUNT_EMPTY;
	}
	
	pAccInfo->lastbalDeposit = newDeposit;
	pAccInfo->lastbalSpent = newSpent;
	pAccInfo->lastbalTransferredBytes = pAccInfo->transferredBytes;
	
	// everything is OK, situation normal
	pAccInfo->authFlags &= ~(AUTH_SENT_BALANCE_REQUEST|AUTH_SENT_SECOND_BALANCE_REQUEST);
	m_Mutex.unlock();
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
	aiAccountingInfo * pAccInfo;
	if ( pHashEntry->pAccountingInfo != 0 )
		{
			pAccInfo = pHashEntry->pAccountingInfo;
			if ( pAccInfo->pPublicKey )
				{
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



#endif /* ifdef PAYMENT */
