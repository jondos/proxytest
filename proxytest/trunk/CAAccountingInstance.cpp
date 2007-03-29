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

//for testing purposes only
#define JAP_DIGEST_LENGTH 28


DOM_Document CAAccountingInstance::m_preparedCCRequest;

/**
 * Singleton: This is the reference to the only instance of this class
 */
CAAccountingInstance * CAAccountingInstance::ms_pInstance = NULL;



/**
 * private Constructor
 */
CAAccountingInstance::CAAccountingInstance(CAMix* callingMix)
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
	
		prepareCCRequest(callingMix, m_AiName);
	
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
		CAMsg::printMsg( LOG_DEBUG, "AccountingInstance dying finished\n" );
	}



/**
 * Called by FirstMix for each incoming JAP packet.
 * Determines whether the packet should be let through or not
 * 
 * Possible return values, and FirstMix's reaction: 
 * @return 1: everything is OK, forward packet to next mix
 * @return 2: we need something (cert, CC,...) from the user, hold packet and start timeout
 * @return 3: fatal error, or timeout exceeded -> kick the user out
 * 
 */
SINT32 CAAccountingInstance::handleJapPacket(fmHashTableEntry *pHashEntry)
	{
		tAiAccountingInfo* pAccInfo = pHashEntry->pAccountingInfo;
		ms_pInstance->m_Mutex.lock();
		pAccInfo->transferredBytes += MIXPACKET_SIZE; // count the packet	
		
		//kick user out after previous error
		if(pAccInfo->authFlags & (AUTH_FATAL_ERROR))
		{
			// there was an error earlier.
			CAMsg::printMsg( LOG_DEBUG, "AccountingInstance: should kick out user now...\n");
			return returnKickout(pAccInfo);
		}
		
		//kick user out if a timeout was set and has since run out
		if (pAccInfo->authFlags & (AUTH_TIMEOUT_STARTED))
		{
			time_t now = time(NULL);
			if (now > pAccInfo->goodwillTimeoutStarttime + GOODWILL_TIMEOUT)
			{
				CAMsg::printMsg(LOG_DEBUG, "Goodwill timeout has runout, will kick out user now...\n");
				return returnKickout(pAccInfo);
			}	
		}	
	
		if(!(pAccInfo->authFlags & AUTH_GOT_ACCOUNTCERT) )
		{ 
			return returnWait(pAccInfo); //dont let the packet through for now, but still wait for an account cert
		}
		else 
		{
			// authentication process not properly finished
			if( pAccInfo->authFlags & AUTH_FAKE )
			{
				ms_pInstance->m_Mutex.unlock();
				pAccInfo->authFlags |= AUTH_FATAL_ERROR;
				CAMsg::printMsg( LOG_DEBUG, "AccountingInstance: AUTH_FAKE flag is set ... byebye\n");
				return returnKickout(pAccInfo);
			}
			if( !(pAccInfo->authFlags & AUTH_ACCOUNT_OK) )
			{
				// we did not yet receive the response to the challenge...
				time_t theTime=time(NULL);	
				if(theTime >= pAccInfo->lastRequestSeconds + REQUEST_TIMEOUT)
				{
					CAMsg::printMsg( LOG_DEBUG, "AccountingInstance: Jap refused to send response to challenge (Request Timeout)...\n");
					CAXMLErrorMessage msg(CAXMLErrorMessage::ERR_NO_ACCOUNTCERT);
					DOM_Document doc;
					msg.toXmlDocument(doc);
					pAccInfo->pControlChannel->sendXMLMessage(doc);
					pAccInfo->authFlags |= AUTH_FATAL_ERROR;
					return returnHold(); //timeout over -> kick out
				}
				else //timeout still running
				{
					return returnHold();
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
				return returnHold(); //no need to start goodwill timeout, setting AUTH_FATAL_ERROR will kick the user out next time 
			}

			//----------------------------------------------------------
			// ******     Hardlimit cost confirmation check **********
			//counting unconfirmed bytes is not necessary anymore, since we deduct from prepaid bytes
			//UINT32 unconfirmedBytes=diff64(pAccInfo->transferredBytes,pAccInfo->confirmedBytes);
			
			//confirmed and transferred bytes are cumulative, so they use UINT64 to store potentially huge values
			//prepaid Bytes as the difference will be much smaller, but might be negative, so we cast to signed int
			UINT64 prepaidBytesUnsigned = (UINT64) (pAccInfo->confirmedBytes - pAccInfo->transferredBytes);
			SINT32 prepaidBytes = (SINT32) prepaidBytesUnsigned;
#ifdef DEBUG		
			UINT64 confirmedBytes = pAccInfo->confirmedBytes;
			UINT64 transferred = pAccInfo->transferredBytes;
			CAMsg::printMsg(LOG_ERR, "Confirmed: %u \n",confirmedBytes);
			CAMsg::printMsg(LOG_ERR, "transferrred: %u \n",transferred);	
			CAMsg::printMsg(LOG_ERR, "prepaidBytes: %d \n",prepaidBytes);
#endif					
			if (prepaidBytes <= (SINT32) ms_pInstance->m_iHardLimitBytes)
			{
#ifdef DEBUG					
				CAMsg::printMsg(LOG_ERR, "hard limit of %d bytes triggered \n", ms_pInstance->m_iHardLimitBytes);
#endif					
					
				//currently nothing happens if prepaidBytes go below zero during timeout?
				//should not be a problem, TODO: think about this more  
				time_t theTime=time(NULL);	
				if ((pAccInfo->authFlags & AUTH_HARD_LIMIT_REACHED) == 0)
				{
					pAccInfo->authFlags |= AUTH_HARD_LIMIT_REACHED;
					pAccInfo->lastHardLimitSeconds = theTime;
				}
				if(theTime >= pAccInfo->lastHardLimitSeconds + HARD_LIMIT_TIMEOUT)
				{
					CAMsg::printMsg( LOG_DEBUG, "Accounting instance: User refused "
																				"to send cost confirmation (HARDLIMIT EXCEEDED).\n");
					ms_pInstance->m_pIPBlockList->insertIP( pHashEntry->peerIP );
					CAXMLErrorMessage msg(CAXMLErrorMessage::ERR_NO_CONFIRMATION);
					DOM_Document doc;
					msg.toXmlDocument(doc);
					pAccInfo->pControlChannel->sendXMLMessage(doc);
					pAccInfo->authFlags |= AUTH_FATAL_ERROR;
					pAccInfo->lastHardLimitSeconds = 0;
					return returnHold();
				}
			}
			else
			{
				pAccInfo->authFlags &= ~AUTH_HARD_LIMIT_REACHED;
			}

			//-------------------------------------------------------
			// *** SOFT LIMIT CHECK *** is it time to request a new cost confirmation?
			if ( prepaidBytes <= (SINT32) ms_pInstance->m_iSoftLimitBytes )
			{
#ifdef DEBU
				CAMsg::printMsg(LOG_ERR, "soft limit of %d bytes triggered \n",ms_pInstance->m_iSoftLimitBytes);
#endif						
				if( (pAccInfo->authFlags & AUTH_SENT_CC_REQUEST) )
				{//we have sent a first CC request
					//still waiting for the answer to the CC reqeust
					CAMsg::printMsg(LOG_ERR, "Waiting for CC request for account nr %u\n", pAccInfo.accountNumber);
					returnOK(pAccInfo);
				}//we have sent a CC request
				// no CC request sent yet --> send a first CC request
				DOM_Document doc;
				
                //send CC to jap
                UINT32 prepaidInterval;
                options.getPrepaidIntervalKbytes(&prepaidInterval);
                UINT64 transferredBytes = pAccInfo->transferredBytes;
                UINT64 bytesToConfirm = transferredBytes + (prepaidInterval * 1024); 				
				makeCCRequest(pAccInfo->accountNumber, bytesToConfirm, doc);
				CAMsg::printMsg(LOG_DEBUG, "AccountingInstance sending first CC request for account nr %u.\n", pAccInfo->accountNumber);
				pAccInfo->authFlags |= AUTH_SENT_CC_REQUEST;
				pAccInfo->pControlChannel->sendXMLMessage(doc);
#ifdef DEBUG	
				CAMsg::printMsg(LOG_DEBUG, "CC request sent for %u bytes \n",bytesToConfirm);
				CAMsg::printMsg(LOG_DEBUG, "transferrred bytes: %u bytes \n",transferredBytes);
				CAMsg::printMsg(LOG_DEBUG, "prepaid Interval: %u \n",prepaidInterval);	
				UINT32 debuglen = 3000;
				UINT8 debugout[3000];
				DOM_Output::dumpToMem(doc,debugout,&debuglen);
				debugout[debuglen] = 0;			
				CAMsg::printMsg(LOG_DEBUG, "the CC sent looks like this: %s \n",debugout);
#endif						
				return returnOK(pAccInfo);
			}// end of soft limit exceeded

			//everything is fine! let the packet pass thru
			return returnOK(pAccInfo);
		} //end of AUTH_GOT_ACCOUNTCERT		
	}
	
/******************************************************************/	
//methods to provide a unified point of exit for handleJapPacket
/******************************************************************/

/**
 * everything is fine, let the packet pass
 */
SINT32 CAAccountingInstance::returnOK(tAiAccountingInfo* pAccInfo)
{
	ms_pInstance->m_Mutex.unlock();
	pAccInfo->authFlags &= ~AUTH_TIMEOUT_STARTED;
	pAccInfo->goodwillTimeoutStarttime = 0;
	return 1;	
}

/*
 * we need a message from the Jap, hold packet and start a timeout
 */
SINT32 CAAccountingInstance::returnWait(tAiAccountingInfo* pAccInfo)
{
	ms_pInstance->m_Mutex.unlock();
	pAccInfo->authFlags |= AUTH_TIMEOUT_STARTED;
	CAMsg::printMsg(LOG_DEBUG, "Wait: %u\n", pAccInfo->goodwillTimeoutStarttime);
	pAccInfo->goodwillTimeoutStarttime = time(NULL);		
	return 2;
}	
/**
 *  When receiving this message, the Mix should kick the user out immediately
 */
SINT32 CAAccountingInstance::returnKickout(tAiAccountingInfo* pAccInfo)
{
	CAMsg::printMsg(LOG_DEBUG, "Kickout: %u\n", pAccInfo->goodwillTimeoutStarttime);
	ms_pInstance->m_Mutex.unlock();
	return 3;
}

/*
 * hold packet, no timeout started 
 * (Usage: send an error message before kicking out the user:
 * set AUTH_FATAL_ERROR yourself, and then returnHold()   )
 */
SINT32 CAAccountingInstance::returnHold()
{
	ms_pInstance->m_Mutex.unlock();
	return 2;
}



/**
 * @todo make this faster by not using DOM!
 * creating the xml of a new CC is really the responsability of the CAXMLCostConfirmation class
 * knowledge about the structure of a CC's XML should be encapsulated in it
 * TODO: add constructor to that class that takes accountnumber, transferredbytes etc as params
 *       (should use a template internally into which it only inserts accNumber and bytes,to speed things up
 * TODO: add toXMLElement method
 * then replace manually building the xml here with contructing a CAXMLCostConfirmation
 * and just add the xml returned by its toXMLElement method
 * @param callingMix: the Mix instance to which the AI belongs
 * (needed to get cascadeInfo to extract the price certificates to include in cost confirmations) 
 */
SINT32 CAAccountingInstance::prepareCCRequest(CAMix* callingMix, UINT8* a_AiName)
{	
	m_preparedCCRequest = DOM_Document::createDocument();
	
	DOM_Element elemRoot = m_preparedCCRequest.createElement("PayRequest");
	elemRoot.setAttribute("version", "1.0");
	m_preparedCCRequest.appendChild(elemRoot);
	DOM_Element elemCC = m_preparedCCRequest.createElement("CC");
	elemCC.setAttribute("version", "1.1");
	elemRoot.appendChild(elemCC);
	DOM_Element elemAiName = m_preparedCCRequest.createElement("AiID");
	setDOMElementValue(elemAiName, a_AiName);
	elemCC.appendChild(elemAiName);	

	//extract price certificate elements from cascadeInfo
	//get cascadeInfo from CAMix(which makeCCRequest needs to extract the price certs
	DOM_Document cascadeInfoDoc; 
	callingMix->getMixCascadeInfo(cascadeInfoDoc);
	
	DOM_Element cascadeInfoElem = cascadeInfoDoc.getDocumentElement();
	DOM_NodeList allMixes = cascadeInfoElem.getElementsByTagName("Mix");
	int nrOfMixes = allMixes.getLength();
	DOM_Node* mixNodes = new DOM_Node[nrOfMixes]; //so we can use separate loops for extracting, hashing and appending
	DOM_Node curMixNode;
	for (int i = 0; i<nrOfMixes; i++){
		//cant use getDOMChildByName from CAUtil here yet, since it will always return the first child
		curMixNode = allMixes.item(i); 
		getDOMChildByName(curMixNode,(UINT8*)"PriceCertificate",mixNodes[i],true);	
	}	 
	
	//hash'em, and get subjectkeyidentifiers
		UINT8 digest[SHA_DIGEST_LENGTH];
		UINT8** allHashes=new UINT8*[nrOfMixes];
		UINT8** allSkis=new UINT8*[nrOfMixes];
		DOM_Node skiNode;
		for (int i = 0; i < nrOfMixes; i++){
			UINT8* out=new UINT8[5000];
			UINT32 outlen=5000;
			
			DOM_Output::makeCanonical(mixNodes[i],out,&outlen);		
			out[outlen] = 0;
#ifdef DEBUG			
			CAMsg::printMsg(LOG_DEBUG, "price cert to be hashed: %s",out);
#endif				
			SHA1(out,outlen,digest);	
			delete[] out;
			
			UINT8* tmpBuff = new UINT8[1024];
			UINT32 len=1024;
			if(CABase64::encode(digest,SHA_DIGEST_LENGTH,tmpBuff,&len)!=E_SUCCESS)
				return E_UNKNOWN;
			tmpBuff[len]=0;
						
			allHashes[i] = tmpBuff;
			//do not delete tmpBuff here, since we're using allHashes below

		if (getDOMChildByName(mixNodes[i],(UINT8*)"SubjectKeyIdentifier",skiNode,true) != E_SUCCESS)
			{
				CAMsg::printMsg(LOG_CRIT,"Could not get mix id from price cert");	
			}	
		
		allSkis[i] =  (UINT8*) skiNode.getFirstChild().getNodeValue().transcode();

	}
	//and append to CC
	DOM_Element elemPriceCerts = m_preparedCCRequest.createElement("PriceCertificates");
	DOM_Element elemCert;
	for (int i = 0; i < nrOfMixes; i++) {
		elemCert = m_preparedCCRequest.createElement("PriceCertHash");
		//CAMsg::printMsg(LOG_DEBUG,"hash to be inserted in cc: index %d, value %s\n",i,allHashes[i]);
		setDOMElementValue(elemCert,allHashes[i]);
		delete[] allHashes[i];
		elemCert.setAttribute("id", DOMString( (const char*)allSkis[i]));
		if (i == 0) {
			elemCert.setAttribute("isAI","true");	
			}
			elemPriceCerts.appendChild(elemCert);
		}
		elemCC.appendChild(elemPriceCerts);
#ifdef DEBUG 		
		CAMsg::printMsg(LOG_DEBUG, "finished method makeCCRequest\n");
#endif		

        delete[] mixNodes;
		delete[] allHashes;
		delete[] allSkis;	
		return E_SUCCESS;
}


SINT32 CAAccountingInstance::makeCCRequest(const UINT64 accountNumber, const UINT64 transferredBytes, DOM_Document& doc)
	{
		DOM_Node elemCC;
		
		doc = DOM_Document::createDocument();
		doc.appendChild(doc.importNode(m_preparedCCRequest.getDocumentElement(),true));
		
	    getDOMChildByName(doc.getDocumentElement(),(UINT8*)"CC",elemCC);
		
		DOM_Element elemAccount = doc.createElement("AccountNumber");
		setDOMElementValue(elemAccount, accountNumber);
		elemCC.appendChild(elemAccount);
		DOM_Element elemBytes = doc.createElement("TransferredBytes");
		setDOMElementValue(elemBytes, transferredBytes);
		elemCC.appendChild(elemBytes);

		return E_SUCCESS;
	}




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
					CAMsg::printMsg( LOG_DEBUG, "Received an AccountCertificate. Calling handleAccountCertificate()\n" );
				ms_pInstance->handleAccountCertificate( pHashEntry, root );
			}
		else if ( strcmp( docElementName, "Response" ) == 0)
			{
					CAMsg::printMsg( LOG_DEBUG, "Received a Response (challenge-response)\n");
				ms_pInstance->handleChallengeResponse( pHashEntry, root );
			}
		else if ( strcmp( docElementName, "CC" ) == 0 )
			{
					CAMsg::printMsg( LOG_DEBUG, "Received a CC. Calling handleCostConfirmation()\n" );
				ms_pInstance->handleCostConfirmation( pHashEntry, root );
			}
		else
		{
			CAMsg::printMsg( LOG_ERR, 
					"AI Received XML message with unknown root element \"%s\". This is not accepted!\n",
											docElementName 
										);
			return E_UNKNOWN;
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
		CAMsg::printMsg(LOG_DEBUG, "started method handleAccountCertificate\n");
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
		
	CAMsg::printMsg(LOG_ERR, "Checking database for previously prepaid bytes...\n");
	SINT32 prepaidAmount = m_dbInterface->getPrepaidAmount(pAccInfo->accountNumber);
	if (prepaidAmount > 0)
	{
		pAccInfo->confirmedBytes += prepaidAmount;	
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: Got %d prepaid bytes\n",prepaidAmount);
	}	
	else
	{
		CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: No database record for prepaid bytes found for this account \n");	
	}	
	CAMsg::printMsg(LOG_DEBUG, "number of prepaid (confirmed-transferred) bytes: %d \n",pAccInfo->confirmedBytes-pAccInfo->transferredBytes);
		
	UINT8 * arbChallenge;
	UINT8 b64Challenge[ 512 ];
	UINT32 b64Len = 512;

	CAMsg::printMsg(LOG_DEBUG, "almost finished handleAccountCertificate, preparing challenge\n");

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
 * Also gets the last CC of the user, and sends it to the JAP
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
	
	// fetch cost confirmation from last session if available, and send it
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
			pAccInfo->pControlChannel->sendXMLMessage(pCC->getXMLDocument());
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
#ifdef DEBUG
	CAMsg::printMsg(LOG_DEBUG, "started method handleCostConfirmation\n");
#endif
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
	
/************ TODO: check pricecerthash with isAI-attribute instead *******
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
********************/

	// parse & set transferredBytes
	m_Mutex.lock();
	//when using Prepayment, this check is outdated, but left in to notice the most crude errors/cheats
	//The CC's transferredBytes should be equivalent to 
	//AccInfo's confirmed bytes + the Config's PrepaidInterval - the number of bytes transferred between
	//requesting and receiving the CC
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
	CAMsg::printMsg(LOG_DEBUG, "after store \n");
	delete pCC;
	return;
}





/**
 * This must be called whenever a JAP is connecting to init our per-user
 * data structures
 */
SINT32 CAAccountingInstance::initTableEntry( fmHashTableEntry * pHashEntry )
	{
		pHashEntry->pAccountingInfo = new tAiAccountingInfo;
		memset( pHashEntry->pAccountingInfo, 0, sizeof( tAiAccountingInfo ) );
		pHashEntry->pAccountingInfo->authFlags |= AUTH_SENT_ACCOUNT_REQUEST;
		pHashEntry->pAccountingInfo->lastRequestSeconds = time(NULL);
		pHashEntry->pAccountingInfo->goodwillTimeoutStarttime = -1;
		//getting the JAP's previously prepaid bytes happens in handleAccountCert, could be moved here?
		return E_SUCCESS;
	}



/**
 * This should always be called when closing a JAP connection
 * to cleanup the payment data structures and store prepaid bytes
 
 */
SINT32 CAAccountingInstance::cleanupTableEntry( fmHashTableEntry *pHashEntry )
	{
		ms_pInstance->m_Mutex.lock();
		
		if ( pHashEntry->pAccountingInfo != NULL)
			{
				tAiAccountingInfo* pAccInfo = pHashEntry->pAccountingInfo;
				pHashEntry->pAccountingInfo=NULL;
				
				//store prepaid bytes in database, so the user wont lose the prepaid amount by disconnecting
				SINT32 prepaidBytes = pAccInfo->confirmedBytes - pAccInfo->transferredBytes;
				CAAccountingDBInterface* dbInterface = new CAAccountingDBInterface(); //local variable, since method is static, but m_dbInterface is a member variable
				if(dbInterface->initDBConnection() != E_SUCCESS)
				{
					CAMsg::printMsg( LOG_ERR, "Could not connect to DB, preapid bytes were lost\n");
					delete dbInterface;
					return E_UNKNOWN;
				}
				
				dbInterface->storePrepaidAmount(pAccInfo->accountNumber,prepaidBytes);
				delete dbInterface;

				//free memory of pAccInfo
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
				
				delete pAccInfo;
				pHashEntry->pAccountingInfo=NULL;
			}
			ms_pInstance->m_Mutex.unlock();
		
		return E_SUCCESS;
	}



#endif /* ifdef PAYMENT */
