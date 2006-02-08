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
				CAMsg::printMsg( LOG_ERR, "**************** AccountingInstance: Could not connect to DB!\n");
				exit(1);
			}
	
		// initialize JPI signataure tester
		m_AiName = new UINT8[256];
		options.getAiID(m_AiName, 256);
		if (options.getBI() != NULL)
			{
				m_pJpiVerifyingInstance = options.getBI()->getVerifier();
			}
		options.getPaymentHardLimit(&m_iHardLimitBytes);
		options.getPaymentSoftLimit(&m_iSoftLimitBytes);
	
		// launch AI thread
		/*m_pThread = new CAThread();
		m_pThread->setMainLoop( aiThreadMainLoop );
		m_bThreadRunning = true;
		m_pThread->start( this );
		*/
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
		//m_pThread->join();
		//delete m_pThread;
		delete m_pSettleThread;
		//delete m_biInterface;
		delete m_dbInterface;
		delete m_pIPBlockList;
		delete m_pQueue;
		delete[] m_AiName;
	}



/**
 * This function is called by the FirstMix for each incoming JAP packet.
 * It counts the payload of normal packets and tells the mix when a connection 
 * should be closed because the user is not willing to pay.
 * 
 * @return 1 everything is OK, packet should be forwarded to next mix
 * @return 2 user did not send accountcertificate, connection should be closed
 * @return 3 user  did not send a cost confirmation 
 *						or somehow tried to fake authentication, connection should be closed
 */
SINT32 CAAccountingInstance::handleJapPacket(fmHashTableEntry *pHashEntry)
	{
		tAiAccountingInfo* pAccInfo=pHashEntry->pAccountingInfo;
	
		ms_pInstance->m_Mutex.lock();

		//Handle free surfing period
		time_t theTime =time(NULL);
		/*if (pAccInfo->surfingFree == 0)
		{
			CAMsg::printMsg( LOG_DEBUG, "New user may surf for free.\n");
			pAccInfo->connectionTime = theTime;
			pAccInfo->surfingFree = 1;
		}
		
		if (pAccInfo->surfingFree == 1 && (pAccInfo->connectionTime+120 < theTime))
		{
			pAccInfo->surfingFree = 2;
			CAMsg::printMsg( LOG_DEBUG, "Free surfing period exceeded.\n");
		}
		
		if (pAccInfo->surfingFree == 1)
		{
			ms_pInstance->m_Mutex.unlock();
			return 1;
		}*/
		
		pAccInfo->transferredBytes += MIXPACKET_SIZE; // count the packet	
		
		if(pAccInfo->authFlags & (AUTH_FATAL_ERROR))
			{
				// there was an error earlier..
				ms_pInstance->m_Mutex.unlock();
				return 3;
			}
	
		if(pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT )
			{
				//----------------------------------------------------------
				// authentication process not properly finished
				if( pAccInfo->authFlags & AUTH_FAKE )
					{
						ms_pInstance->m_Mutex.unlock();
						pAccInfo->authFlags |= AUTH_FATAL_ERROR;
						CAMsg::printMsg( LOG_DEBUG, "AccountingInstance: AUTH_FAKE flag is set ... byebye\n");
						return 3;
					}
				if( !(pAccInfo->authFlags & AUTH_ACCOUNT_OK) )
					{
						// we did not yet receive the response to the challenge...
						time_t theTime=time(NULL);	
						if(theTime >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
							{
								ms_pInstance->m_Mutex.unlock();
								pAccInfo->authFlags |= AUTH_FATAL_ERROR;
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
						pAccInfo->pControlChannel->sendXMLMessage(doc);
						pAccInfo->authFlags |= AUTH_FATAL_ERROR;
						ms_pInstance->m_Mutex.unlock();
						return 3;
					}

				//----------------------------------------------------------
				// Hardlimit cost confirmation check
				UINT32 unconfirmedBytes=diff64(pAccInfo->transferredBytes,pAccInfo->confirmedBytes);
				if (unconfirmedBytes >= ms_pInstance->m_iHardLimitBytes)
					{
						#ifdef DEBUG
							CAMsg::printMsg( LOG_DEBUG, "Accounting instance: User refused "
																					"to send cost confirmation (HARDLIMIT EXCEEDED).\n");
						#endif
						ms_pInstance->m_pIPBlockList->insertIP( pHashEntry->peerIP );
						CAXMLErrorMessage msg(CAXMLErrorMessage::ERR_NO_CONFIRMATION);
						DOM_Document doc;
						msg.toXmlDocument(doc);
						pAccInfo->pControlChannel->sendXMLMessage(doc);
						pAccInfo->authFlags |= AUTH_FATAL_ERROR;
						ms_pInstance->m_Mutex.unlock();
						return 3;
					}
	
				//-------------------------------------------------------
				// check: is it time to request a new cost confirmation?
				if( unconfirmedBytes >= ms_pInstance->m_iSoftLimitBytes)
					{
						if( (pAccInfo->authFlags & AUTH_SENT_CC_REQUEST) )
							{//we have sent a first CC request
								//still waiting for the answer to the CC reqeust
								ms_pInstance->m_Mutex.unlock();
								return 1;
							}//we have sent a CC request
						// no CC request sent yet --> send a first CC request
						DOM_Document doc;
						#ifdef DEBUG
							CAMsg::printMsg(LOG_DEBUG, "AccountingInstance sending first CC request.\n");
						#endif
						makeCCRequest(pAccInfo->accountNumber, pAccInfo->transferredBytes, doc);
						pAccInfo->pControlChannel->sendXMLMessage(doc);
						pAccInfo->authFlags |= AUTH_SENT_CC_REQUEST;
						ms_pInstance->m_Mutex.unlock();
						return 1;
					}//soft limit exceeded
				//everything is fine --> but do we need a new balance cert?
				//@todo move this to a separate thread?!
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
								ms_pInstance->m_Mutex.unlock();
								return 1;
							}
						// send a first CC request
						DOM_Document doc;
						time_t theTime=time(NULL);
						CAAccountingInstance::makeBalanceRequest(theTime-600, doc);
						#ifdef DEBUG
							CAMsg::printMsg(LOG_DEBUG, "AccountingInstance sending balance request.\n");
						#endif
						pAccInfo->reqbalMinSeconds = theTime - 600;
						pAccInfo->pControlChannel->sendXMLMessage(doc);
						pAccInfo->authFlags |= AUTH_SENT_BALANCE_REQUEST;
						pAccInfo->lastRequestSeconds = theTime;
						ms_pInstance->m_Mutex.unlock();
						return 1;
					}
				//really everything is fine! let the packet pass thru
				ms_pInstance->m_Mutex.unlock();
				return 1;
			}
		
		//---------------------------------------------------------
		// we have no accountcert from the user, but we have already sent the request
		if(pAccInfo->authFlags & AUTH_SENT_ACCOUNT_REQUEST)
			{
				int ret=2;
				if(pAccInfo->lastRequestSeconds+PAYMENT_ACCOUNT_CERT_TIMEOUT<theTime)
					{
						ret = 3;
					}
  			ms_pInstance->m_Mutex.unlock();
				return ret;
			}
		// send first request
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "AccountingInstance sending account request.\n");
		#endif
		//time_t theTime=time(NULL);
		DOM_Document doc;
		CAAccountingInstance::makeAccountRequest(doc);
		pAccInfo->pControlChannel->sendXMLMessage(doc);
		pAccInfo->authFlags |= AUTH_SENT_ACCOUNT_REQUEST;
		pAccInfo->lastRequestSeconds = theTime;
		ms_pInstance->m_Mutex.unlock();
		return 1;
	}

/** @todo makt the faster by not using DOM!*/
SINT32 CAAccountingInstance::makeCCRequest(const UINT64 accountNumber, const UINT64 transferredBytes, DOM_Document& doc)
	{
		// create a DOM CostConfirmation document
		doc = DOM_Document::createDocument();
		DOM_Element elemRoot = doc.createElement("PayRequest");
		elemRoot.setAttribute("version", "1.0");
		doc.appendChild(elemRoot);
		DOM_Element elemCC = doc.createElement("CC");
		elemCC.setAttribute("version", "1.1");
		elemRoot.appendChild(elemCC);
		DOM_Element elemAiName = doc.createElement("AiID");
		setDOMElementValue(elemAiName, ms_pInstance->m_AiName);
		elemCC.appendChild(elemAiName);	
		DOM_Element elemAccount = doc.createElement("AccountNumber");
		setDOMElementValue(elemAccount, accountNumber);
		elemCC.appendChild(elemAccount);
		DOM_Element elemBytes = doc.createElement("TransferredBytes");
		setDOMElementValue(elemBytes, transferredBytes);
		elemCC.appendChild(elemBytes);
		return E_SUCCESS;
	}

/** @todo makt the faster by not using DOM!*/
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

/** @todo makt the faster by not using DOM!*/
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
/*THREAD_RETURN CAAccountingInstance::aiThreadMainLoop( void *param )
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
			}
		THREAD_RETURN_SUCCESS;
	}
*/


/**
 * Handle a user (xml) message sent to us by the Jap. 
 *	
 * This function is running inside the AiThread. It determines 
 * what type of message we have and calls the appropriate handle...() 
 * function
 */
SINT32 CAAccountingInstance::processJapMessage(fmHashTableEntry * pHashEntry,const DOM_Document& a_DomDoc)
	{
		DOM_Element root = a_DomDoc.getDocumentElement();
		char * docElementName = root.getTagName().transcode();

		// what type of message is it?
		if ( strcmp( docElementName, "AccountCertificate" ) == 0 )
			{
				#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Received an AccountCertificate. Calling handleAccountCertificate()\n" );
				#endif
				ms_pInstance->handleAccountCertificate( pHashEntry, root );
			}
		else if ( strcmp( docElementName, "Response" ) == 0)
			{
				#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Received a Response (challenge-response)\n");
				#endif
				ms_pInstance->handleChallengeResponse( pHashEntry, root );
			}
		else if ( strcmp( docElementName, "CC" ) == 0 )
			{
				#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Received a CC. Calling handleCostConfirmation()\n" );
				#endif
				ms_pInstance->handleCostConfirmation( pHashEntry, root );
			}
		else if ( strcmp( docElementName, "Balance" ) == 0 )
			{
				#ifdef DEBUG
					CAMsg::printMsg( LOG_DEBUG, "Received a BalanceCertificate. Calling handleBalanceCertificate()\n" );
				#endif
				ms_pInstance->handleBalanceCertificate( pHashEntry, root );
			}
		else
			{
				CAMsg::printMsg( LOG_ERR, 
													"AI Received XML message with unknown root element \"%s\". Ignoring...\n",
														docElementName 
												);
			}
		//delete pDomDoc;
		delete [] docElementName;
		return E_SUCCESS;
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
		tAiAccountingInfo* pAccInfo = pHashEntry->pAccountingInfo;
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
				pAccInfo->pControlChannel->sendXMLMessage(errDoc);
				m_Mutex.unlock();
				return ;
			}

		// parse & set accountnumber
		if ( getDOMChildByName( root, (UINT8 *)"AccountNumber", elGeneral, false ) != E_SUCCESS ||
					getDOMElementValue( elGeneral, pAccInfo->accountNumber ) != E_SUCCESS)
			{
				CAMsg::printMsg( LOG_ERR, "AccountCertificate has wrong or no accountnumber. Ignoring\n");
				CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_FORMAT);
				DOM_Document errDoc;
				err.toXmlDocument(errDoc);
				pAccInfo->pControlChannel->sendXMLMessage(errDoc);
				m_Mutex.unlock();
				return ;
			}

		// parse & set payment instance id
		UINT32 len=256;
		pAccInfo->pstrBIID=new UINT8[256];
		if ( getDOMChildByName( root, (UINT8 *)"BiID", elGeneral, false ) != E_SUCCESS ||
			 getDOMElementValue( elGeneral,pAccInfo->pstrBIID, &len ) != E_SUCCESS)
			{
				delete[] pAccInfo->pstrBIID;
				pAccInfo->pstrBIID=NULL;
				CAMsg::printMsg( LOG_ERR, "AccountCertificate has no Payment Instance ID. Ignoring\n");
				CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_FORMAT);
				DOM_Document errDoc;
				err.toXmlDocument(errDoc);
				pAccInfo->pControlChannel->sendXMLMessage(errDoc);
				m_Mutex.unlock();
				return ;
			}
		#ifdef DEBUG
			CAMsg::printMsg(LOG_DEBUG, "Stored payment instance ID: %s\n", pAccInfo->pstrBIID);
		#endif

	// parse & set public key
	if ( getDOMChildByName( root, (UINT8 *)"JapPublicKey", elGeneral, false ) != E_SUCCESS )
		{
			CAMsg::printMsg( LOG_ERR, "AccountCertificate contains no public key. Ignoring\n");
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_KEY_NOT_FOUND);
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
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
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
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
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
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
	pAccInfo->pControlChannel->sendXMLMessage(doc);
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
	tAiAccountingInfo* pAccInfo = pHashEntry->pAccountingInfo;

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
		#pragma message (__FILE__ "(665) Signature verifying must be implemented here !!!!!!!!!! ")
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
				UINT8 tmp[32];
				print64(tmp,pAccInfo->transferredBytes);
				CAMsg::printMsg(LOG_DEBUG, "TransferredBytes is now %s\n", tmp);
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
	tAiAccountingInfo*pAccInfo=pHashEntry->pAccountingInfo;
	m_Mutex.lock();
	
	// check authstate	
	if( (pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT)==0 ||
			(pAccInfo->authFlags & AUTH_ACCOUNT_OK)==0)
		{
			m_Mutex.unlock();
			return ;
		}
		
	CAXMLCostConfirmation* pCC = CAXMLCostConfirmation::getInstance(root);
	if(pCC==NULL)
		{
			m_Mutex.unlock();
			return ;
		}
	// for debugging only: test signature the oldschool way
	// warning this removes the signature from doc!!!
	if ( pAccInfo->pPublicKey==NULL||
			(pAccInfo->pPublicKey->verifyXML( (DOM_Node &)root ) != E_SUCCESS ))
		{
			// wrong signature
			CAMsg::printMsg( LOG_INFO, "CostConfirmation has INVALID SIGNATURE!\n" );
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_BAD_SIGNATURE, (UINT8*)"CostConfirmation has bad signature");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
			delete pCC;
			m_Mutex.unlock();
			return ;
		}
	#ifdef DEBUG
	else
		{
			CAMsg::printMsg( LOG_DEBUG, "CostConfirmation Signature is OK.\n");
		}
	#endif
	m_Mutex.unlock();
	

	// parse and check AI name
	UINT8* pAiID = pCC->getAiID();
	if( strcmp( (char *)pAiID, (char *)m_AiName ) != 0)
		{
			CAMsg::printMsg( LOG_INFO, "CostConfirmation has wrong AIName %s. Ignoring...\n", pAiID );
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_DATA, (UINT8*)"Your CostConfirmation has a wrong AI name");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
			delete[] pAiID;
			delete pCC;
			return ;
		}
	delete[] pAiID;

	// parse & set transferredBytes
	m_Mutex.lock();
	if(pCC->getTransferredBytes() < pAccInfo->confirmedBytes )
		{
			UINT8 tmp[32];
			print64(tmp,pCC->getTransferredBytes());
			CAMsg::printMsg( LOG_INFO, "CostConfirmation has Wrong Number of Bytes (%s). Ignoring...\n", tmp );
			CAXMLErrorMessage err(CAXMLErrorMessage::ERR_WRONG_DATA, 
				(UINT8*)"Your CostConfirmation has a wrong number of transferred bytes");
			DOM_Document errDoc;
			err.toXmlDocument(errDoc);
			pAccInfo->pControlChannel->sendXMLMessage(errDoc);
			delete pCC;
			m_Mutex.unlock();
			return ;
		}
	pAccInfo->confirmedBytes = pCC->getTransferredBytes();
	if(pAccInfo->confirmedBytes >= pAccInfo->reqConfirmBytes)
	{
		pAccInfo->authFlags &= ~AUTH_SENT_CC_REQUEST;
	}
	m_Mutex.unlock();
	
	m_dbInterface->storeCostConfirmation(*pCC);
	delete pCC;
	return ;
}



/**
 * Handles a balance certificate sent by the JAP.
 * Checks signature and age.
 * @todo set authFlags
 * @todo send back XMLErrorMessages on failure
 */
SINT32 CAAccountingInstance::handleBalanceCertificate(fmHashTableEntry *pHashEntry, const DOM_Element &root)
	{
		UINT8 strGeneral[ 256 ];
		UINT32 strGeneralLen = 256;
		UINT64 newDeposit, newSpent;
		SINT32 seconds;
		DOM_Element elGeneral;
		tAiAccountingInfo* pAccInfo = pHashEntry->pAccountingInfo;
	
		// test signature
		m_Mutex.lock();
		if( !m_pJpiVerifyingInstance || 
				(m_pJpiVerifyingInstance->verifyXML( (DOM_Node &)root, (CACertStore *)NULL ) != E_SUCCESS) 
			)
			{
				CAMsg::printMsg( LOG_INFO, "BalanceCertificate has BAD SIGNATURE! Ignoring\n" );
				m_Mutex.unlock();
				return E_UNKNOWN;
			}
		m_Mutex.unlock();

		// parse timestamp
		if ( (getDOMChildByName( root, (UINT8*)"Timestamp", elGeneral, false ) != E_SUCCESS) 
				|| (getDOMElementValue( elGeneral, strGeneral, &strGeneralLen ) != E_SUCCESS) )
			{
				return E_UNKNOWN;
			}
		parseJdbcTimestamp(strGeneral, seconds);
	
		m_Mutex.lock();
		if(seconds < pAccInfo->reqbalMinSeconds)
			{
				// todo send a request again...
				// TODO: if too old -> ask JPI for a new one
				// if JPI is not available at present -> let user proceed for a while
				m_Mutex.unlock();
				return E_UNKNOWN;
			}
		//pAccInfo->lastbalSeconds = seconds;
	
		// parse & set deposit
		if( (getDOMChildByName( root, (UINT8*)"Deposit", elGeneral, false ) != E_SUCCESS) ||
				(getDOMElementValue( elGeneral, newDeposit ) != E_SUCCESS) ||
				(newDeposit < pAccInfo->lastbalDeposit) )
			{
				m_Mutex.unlock();
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG, "invalid deposit value...\n");
				#endif
				return E_UNKNOWN;
			}
#ifdef DEBUG
		else 
			{
				UINT8 tmp[32];
				print64(tmp,newDeposit);
				CAMsg::printMsg(LOG_DEBUG, "Balance: deposit=%s\n", tmp);
			}
#endif
	
	// parse & set spent
		if( (getDOMChildByName( root, (UINT8*)"Spent", elGeneral, false ) != E_SUCCESS) ||
				(getDOMElementValue( elGeneral, newSpent ) != E_SUCCESS) ||
				(newSpent < pAccInfo->lastbalSpent) )
			{
				m_Mutex.unlock();
				#ifdef DEBUG
					CAMsg::printMsg(LOG_DEBUG, "invalid spent value...\n");
				#endif
				return E_UNKNOWN;
			}
#ifdef DEBUG
		else 
			{
				UINT8 tmp[32];
				print64(tmp,newSpent);
				CAMsg::printMsg(LOG_DEBUG, "Balance: Spent=%s\n", tmp);
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
		pAccInfo->authFlags &= ~AUTH_SENT_BALANCE_REQUEST;
		m_Mutex.unlock();
		return E_SUCCESS;
	}


/**
 * This must be called whenever a JAP is connecting to init our per-user
 * data structures
 */
SINT32 CAAccountingInstance::initTableEntry( fmHashTableEntry * pHashEntry )
	{
		pHashEntry->pAccountingInfo = new tAiAccountingInfo;
		memset( pHashEntry->pAccountingInfo, 0, sizeof( tAiAccountingInfo ) );
		return E_SUCCESS;
	}



/**
 * This should always be called when closing a JAP connection
 * to cleanup the payment data structures
 * @todo rewrite
 */
SINT32 CAAccountingInstance::cleanupTableEntry( fmHashTableEntry *pHashEntry )
	{
		tAiAccountingInfo* pAccInfo;
		if ( pHashEntry->pAccountingInfo != NULL)
			{
				pAccInfo = pHashEntry->pAccountingInfo;
				if ( pAccInfo->pPublicKey!=NULL )
					{
						delete pAccInfo->pPublicKey;
					}
				if ( pAccInfo->pChallenge!=NULL )
					{
						delete [] pAccInfo->pChallenge;
					}
				if ( pAccInfo->pstrBIID!=NULL )
					{
						delete [] pAccInfo->pstrBIID;
					}
				delete pHashEntry->pAccountingInfo;
				pHashEntry->pAccountingInfo=NULL;
			}
		return E_SUCCESS;
	}



#endif /* ifdef PAYMENT */
