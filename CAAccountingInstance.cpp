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
#include "CAMsg.hpp"
#include "CAUtil.hpp"


/** Helper function for debugging */
/*void hexDump(UINT8 *pDataIn, UINT32 lenDataIn) {
	int numLines=0;
	int i=0;
	numLines = lenDataIn/16;
	for(i=0; i<numLines-1; i++) {
		printf("%08x: %02x %02x %02x %02x %02x %02x %02x %02x    %02x %02x %02x %02x %02x %02x %02x %02x\n", 
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
CAAccountingInstance * CAAccountingInstance::m_pInstance = 0;



/**
 * private Constructor
 */
CAAccountingInstance::CAAccountingInstance()
{
	CAMsg::printMsg(LOG_DEBUG, "AccountingInstance initialising\n");
	m_pQueue = new CAQueue();
	m_pIPBlockList = new CATempIPBlockList();

	// initialize JPI connection
	m_biInterface = new CAAccountingBIInterface();
	
	// initialize Database connection
	m_dbInterface = new CAAccountingDBInterface();

	// launch AI thread
	m_pThread = new CAThread();
	m_pThread->setMainLoop(aiThreadMainLoop);
	m_pThread->start(this);
}



/**
 * private desctructor
 */
CAAccountingInstance::~CAAccountingInstance()
{
	CAMsg::printMsg(LOG_DEBUG, "AccountingInstance dying\n");
	delete m_biInterface;
	delete m_dbInterface;
	delete m_pIPBlockList;
	delete m_pQueue;
}



/**
 * Sets the in/out AES keys for communicating with one JAP
 * TODO: add Challenge / Response
 */
UINT32 CAAccountingInstance::setJapKeys( fmHashTableEntry *pHashEntry,
																				 UINT8 * out, UINT8 * in) 
{
	CAMsg::printMsg(LOG_ERR, "AI setting Cipher keys\n");
	aiAccountingInfo * pAccInfo;
	pAccInfo = pHashEntry->pAccountingInfo;
	m_Mutex.lock();
	
	// if necessary, create new CASymCipher objects
	if( pAccInfo->pCipherIn == 0 )
		pAccInfo->pCipherIn = new CASymCipher();
	if( pAccInfo->pCipherOut == 0 )
		pAccInfo->pCipherOut = new CASymCipher();

	// set the keys
	pHashEntry->pAccountingInfo->pCipherIn->setKey(in);
	pHashEntry->pAccountingInfo->pCipherOut->setKey(out);
	m_Mutex.unlock();
	return 0;
}




/**
 * This function is called by the FirstMix for each incoming JAP packet.
 * It filters out JAP->AI packets, counts the payload of normal packets and 
 * tells the mix when a connection should be closed.
 * 
 * @return 0 if the packet is an JAP->AI packet (caller should drop it)
 * @return 1 everything is OK, packet should be forwarded to next mix
 * @return 2 user did not send certificate, connection should be closed
 * @return 3 user is an evil hax0r, i.e. did not send a cost confirmation 
 * or somehow tried to fake authentication, connection should be closed
 * @return 4 AuthState unknown (internal error, should not happen)
 */
UINT32 CAAccountingInstance::handleJapPacket( MIXPACKET *pPacket,
																							fmHashTableEntry *pHashEntry
																							)
{
	aiAccountingInfo * pAccInfo;
	CASymCipher *pCipherIn;
	CASymCipher *pCipherOut;
	UINT8 * pData;

	pAccInfo = pHashEntry->pAccountingInfo;
	pCipherIn = pAccInfo->pCipherIn;
	pData = pPacket->data;
//	pCipherOut = pAccInfo->pCipherOut;

	
	// is this packet an JAP->AI packet?
	if(pPacket->channel == 0xFFFFFFFF) {
		if(pCipherIn != 0) { // decrypt the packet
			pCipherIn->crypt1(	pData, 
															pData, DATA_SIZE);
		}
		else {
			CAMsg::printMsg(LOG_ERR, "Could not decrypt JAP->AI packet: key not initialized\n");
		}
		
		addToReceiveBuffer( pData, pHashEntry); // add it to the AI msg buffer
		return 0;
	}
	else {

		// user is authenticated normally
		m_Mutex.lock();
		if(pAccInfo->authState == AUTH_OK) {
			// count the payload
			pAccInfo->mixedPackets++;
			
			// if the JAP refuses to send a cost confirmation -> byebye!
			if(pAccInfo->mixedPackets >= pAccInfo->confirmedPackets+1000) {
				m_Mutex.unlock();
				CAMsg::printMsg(LOG_DEBUG, "Accounting instance: Account %d, IP %d.%d.%d.%d refused "
						"to send cost confirmation.", 
						pAccInfo->accountNumber, pHashEntry->peerIP[0], pHashEntry->peerIP[1],
						pHashEntry->peerIP[2], pHashEntry->peerIP[3]);
				m_pIPBlockList->insertIP(pHashEntry->peerIP);
				return 3;
			}
			
			// let the packet pass thru
			m_Mutex.unlock();
			return 1;
		}
		
		// user has not sent his/her certificate
		else if(pAccInfo->authState == AUTH_UNKNOWN) {
			pAccInfo->mixedPackets++;
			// new (not yet authenticated) users can surf a bit (up to 500 Packets~=500KB) 
			// before they get kicked out
			if(pAccInfo->mixedPackets >= 500) {
				m_Mutex.unlock();
				CAMsg::printMsg(LOG_DEBUG, "Accounting instance: Unknown User at IP %d.%d.%d.%d has sent/received " 
						"more than 500 packets and will be kicked",
						pHashEntry->peerIP[0], pHashEntry->peerIP[1],
						pHashEntry->peerIP[2], pHashEntry->peerIP[3]);
				m_pIPBlockList->insertIP(pHashEntry->peerIP);
				return 2;
			}
			else {
				// let the packet pass thru
				m_Mutex.unlock();
				return 1;
			}
		}
		
		// user is a bad guy
		else if(pAccInfo->authState == AUTH_BAD) {
			m_Mutex.unlock();
			CAMsg::printMsg(LOG_DEBUG, "Accounting instance: Account %d, IP %d.%d.%d.%d is evil (AuthState BAD)!", 
				pAccInfo->accountNumber, pHashEntry->peerIP[0], pHashEntry->peerIP[1],
				pHashEntry->peerIP[2], pHashEntry->peerIP[3]);
			m_pIPBlockList->insertIP(pHashEntry->peerIP);
			return 3;
		}
		else {
			m_Mutex.unlock();
			CAMsg::printMsg(LOG_ERROR, "Accounting instance: Account %d, IP %d.%d.%d.%d has invalid AuthState!",
				pAccInfo->accountNumber, pHashEntry->peerIP[0], pHashEntry->peerIP[1],
				pHashEntry->peerIP[2], pHashEntry->peerIP[3]);
			return 4;
		}
	}
}





/**
 * Adds an incoming JAP->AI packet to the corresponding per-user
 * buffer. When a complete message was received, this function puts it
 * in the AI queue, so the AI thread can deal with it
 */
void CAAccountingInstance::addToReceiveBuffer(UINT8 *pData, 
																							fmHashTableEntry *pHashEntry) 
{
	// Note: We don't need to use m_Mutex in this function because only this
	// thread reads/writes pReceiveBuffer, msgTotalSize, msgCurrentSize
	// the queue is implemented threadsafe by itself
	
	int numBytes = 0;
	aiQueueItem *pQueueItem;
	aiAccountingInfo * pAccInfo;

	pAccInfo = pHashEntry->pAccountingInfo;
	
  // start a new message?
	if( pAccInfo->pReceiveBuffer == 0 ) {

		// first 4bytes are msg size
		pAccInfo->msgTotalSize = ntohl(*((UINT32*) pData));
		pAccInfo->pReceiveBuffer = (UINT8 *)malloc(pAccInfo->msgTotalSize+sizeof(char));
		if( (pAccInfo->msgTotalSize >= DATA_SIZE-sizeof(UINT32) )) {
			numBytes = DATA_SIZE - sizeof(UINT32);
		} 
		else {
			numBytes = pAccInfo->msgTotalSize;
		}
		
		memcpy( pAccInfo->pReceiveBuffer, pData + sizeof(UINT32), 
						DATA_SIZE - sizeof(UINT32));
		pAccInfo->msgCurrentSize = DATA_SIZE - sizeof(UINT32);
	}

	// or continue an old one?
	else { 

		// calculate number of bytes to copy
		if( (pAccInfo->msgTotalSize - pAccInfo->msgCurrentSize) < DATA_SIZE ) {
			numBytes = pAccInfo->msgTotalSize - pAccInfo->msgCurrentSize;
		}
		else {
			numBytes = DATA_SIZE;
		}

		// copy data to our per-user buffer
		memcpy( pAccInfo->pReceiveBuffer + pAccInfo->msgCurrentSize, 
						pData, numBytes );
		pAccInfo->msgCurrentSize += numBytes;
	}

	// is our message ready (completely transmitted)?
	if(pAccInfo->msgCurrentSize >= pAccInfo->msgTotalSize) {
		pAccInfo->pReceiveBuffer[pAccInfo->msgTotalSize] = 0; // add 0 to the end of the string
		
		CAMsg::printMsg(LOG_DEBUG, "Accounting Instance: msg finished\n");
		CAMsg::printMsg(LOG_DEBUG, "msg dump:\n%s\n", (char *)(pAccInfo->pReceiveBuffer));

		// enqueue it, so the Accounting Thread can deal with it
		pQueueItem = new aiQueueItem;
		pQueueItem->pHashEntry = pHashEntry;
		pQueueItem->pData = pAccInfo->pReceiveBuffer;
		pQueueItem->dataLen = pAccInfo->msgTotalSize;
		
		m_pQueue->add( pQueueItem, sizeof(aiQueueItem) );

		// free QItem (the queue made a copy)
		free(pQueueItem);
		pAccInfo->pReceiveBuffer = 0;
		pAccInfo->msgTotalSize = 0;
		pAccInfo->msgCurrentSize = 0;
	}
}



/**
 * The Main Loop of the accounting instance thread.
 * TODO: terminate the whole firstMix on db connection errors
 */
THREAD_RETURN CAAccountingInstance::aiThreadMainLoop(void *param)
{
	CAAccountingInstance *instance;
	CAQueue * pQueue;
	aiQueueItem * pQueueItem;
	UINT32 itemSize;

	pQueueItem = new aiQueueItem;
	itemSize = sizeof(aiQueueItem);
	instance = (CAAccountingInstance *)param;
	pQueue = instance->m_pQueue;
	
	CAMsg::printMsg(LOG_DEBUG, "AI Thread starting\n");

	// initiate DB connection
	if( instance->initDBConnection() != 0) {
		CAMsg::printMsg(LOG_ERR, "Could not connect to Postgres Database! AI thread aborting\n");
		return 0;
	}

	
	while(true) {	
		CAMsg::printMsg(LOG_DEBUG, "AI Thread waiting for message .. \n");
		pQueue->getOrWait((UINT8 *)pQueueItem, &itemSize);
		CAMsg::printMsg(LOG_DEBUG, "aiThread(): got message\n");
		instance->processJapMessage( pQueueItem->pHashEntry, pQueueItem->pData, pQueueItem->dataLen );
  }
}




/**
 * Handle a user (xml) message sent to us by the Jap. 
 *  
 * This function is running inside the AiThread. It determines 
 * what type of message we have and calls the appropriate handle...() 
 * function
 */
void CAAccountingInstance::processJapMessage( fmHashTableEntry * pHashEntry, 
										  												UINT8 * pData, UINT32 dataLen ) 
{
  // parse XML - get tagname of document element
  MemBufInputSource oInput(pData, dataLen, "AccountingMessage");
  DOMParser oParser;
  oParser.parse(oInput);
	
  DOM_Document doc=oParser.getDocument();
  DOM_Element root=doc.getDocumentElement();
	
	DOMString dStr = root.getTagName();
	DOM_Element elem;
  char *docElementName = dStr.transcode();

	// what type of message is it?
  if(strcmp(docElementName, "AccountCertificate")==0) {
		CAMsg::printMsg(LOG_DEBUG, "AI: It is an AccountCertificate\n");
		handleAccountCertificate(pHashEntry, root, pData, dataLen);
  }
  else if(strcmp(docElementName, "CC")==0) {
		CAMsg::printMsg(LOG_DEBUG, "AI: It is a CostConfirmation .. nice but atm not necessary\n");
		handleCostConfirmation(pHashEntry, root, pData, dataLen);				
  }
	else if(strcmp(docElementName, "Balance")==0) {
		CAMsg::printMsg(LOG_DEBUG, "AI: It is a balance certificate .. yuppi!\n");
		handleBalanceCertificate(pHashEntry, root, pData, dataLen);
	}
  else {
		CAMsg::printMsg(LOG_ERR, "AI: XML message with root element \"%s\" is unknown! Ignoring\n",
										docElementName);
	// Error
  }
	CAMsg::printMsg(LOG_DEBUG, "Deleting\n");
	delete docElementName;
	//free(docElementName); ??
	CAMsg::printMsg(LOG_DEBUG, "Ende von procesJapMsg\n");
}


/**
 * Handles an account certificate of a newly connected Jap.
 */
void CAAccountingInstance::handleAccountCertificate(fmHashTableEntry *pHashEntry, 
						const DOM_Element &root,
						UINT8 * pData, UINT32 dataLen)
{
	aiAccountingInfo *pAccInfo;
	pAccInfo = pHashEntry->accountingInfo;
	DOM_Element elGeneral;
	UINT8 strGeneral[255];
	UINT32 strGeneralLen=255;

	// parse accountnumber
	if(getDOMChildByName(root, "AccountNumber", elGeneral, false)!=E_SUCCESS) {
		handleAccountCertificateError(1);
		return;
	}
	if(getDOMElementValue(elGeneral,strGeneral,&strGeneralLen)!=E_SUCCESS) {
		handleAccountCertificateError(2);
		return;
	}
	pAccInfo->accountNumber = atoi(strGeneral);
	
	// check signature
	
	// send challenge

	// TODO: validate RESPONSE
	m_Mutex.lock();
	pAccInfo->authState = 
	m_Mutex.unlock();

	return;
}


void CAAccountingInstance::handleAccountCertificateError(	fmHashTableEntry *pHashEntry,
																													UINT32 num) 
{
	aiAccountingInfo * pAccInfo = pHashEntry->pAccountingInfo;
	CAMsg::printMsg(LOG_ERR, "AI: AccountCertificate has wrong format(%i)! Ignoring\n", num);
	sendAIMessageToJap(pHashEntry, ...); // inform the jap of his evilness
	pAccInfo->closeConnection = 1;
}



void CAAccountingInstance::handleChallengeResponse(fmHashTableEntry *pHashEntry, 
						const DOM_Element &root,
						UINT8 * pData, UINT32 dataLen)
{
	// check it
	
	// set authstate accordingly
	
}





/**
 * Handles a cost confirmation sent by a jap (INCOMPLETE)
 */
void CAAccountingInstance::handleCostConfirmation(fmHashTableEntry *pHashEntry, 
							const DOM_Element &root,
							UINT8 * pData, UINT32 dataLen)
{
	aiAccountingInfo *pAccInfo;
	pAccInfo = pHashEntry->accountingInfo;

// in this version we are using XMLEasyCC
// without micropayment which has the following format:
 /*
 * <CC version="1.0">
 *   <AIName>aiName</AIName>
 *   <Bytes>Number of Bytes transferred</Bytes>
 *   <Number>accountNumber</Number>
 * </CC>
 */
 
	// TODO: check signature
	
	// parse ai name	
	DOM_Element elemAiName = root.getElementsByTagName("AIName");
	DOM_Node node = elemAiName.getFirstChild();
	if(node.getNodeType() != TEXT_NODE) {
		CAMsg::printMsg(LOG_DEBUG, "CostConfirmation has WRONG FORMAT!! Ignoring\n");
		return;
	}
	DOMString dstrAiName = ((DOM_CharacterData)node).getData();
	char *aiName = dstrAiName.transcode();	


	// parse Bytes
	DOM_Element elemAiName = root.getElementsByTagName("Bytes");
	DOM_Node node = elemAiName.getFirstChild();
	if(node.getNodeType() != TEXT_NODE) {
		CAMsg::printMsg(LOG_DEBUG, "CostConfirmation has WRONG FORMAT!! Ignoring\n");
		return;
	}
	DOMString dstrBytes = ((DOM_CharacterData)node).getData();
	char *strBytes = dstrBytes.transcode();
	UINT64 bytes = (UINT64)strtol(strBytes, 0, 10);
	

	// store to db
	m_dbInterface->storeCostConfirmation(pAccInfo->accountNumber, bytes, pData);
	delete [] strBytes;
	delete [] aiName;
}


void CAAccountingInstance::handleBalanceCertificate(fmHashTableEntry *pHashEntry, 
			const DOM_Element &root,
			UINT8 * pData, UINT32 dataLen)
{
	// TODO: validate signature
	// CASignature sig;
	// sig.setVerifyKey(CACertificate );
	// sig.verifyXML(root, pTrustedCerts);
	
	// TODO: check age
	
	// TODO: if too old -> ask JPI for a new one
	// if JPI is not available at present -> let user proceed for a while
}


/**
 * Tells the Jap which data this AI expects to receive from the Jap.
 * This can be sent to every user at the beginning of the connection,
 * before any user packets were sent, or it can be sent to users who 
 * refuse to send authentication / cost confirmation info before closing
 * the connection
 *
 * // TODO: Rewrite
 */
void CAAccountingInstance::sendAIRequest(fmHashTableEntry pHashEntry, bool certNeeded,
																				bool balanceNeeded, bool costConfirmationNeeded) 
{
	char msgString[500];
	sprintf(msgString, "<AIRequest>\n<CertNeeded>%s</CertNeeded>\n<BalanceNeeded>%s</BalanceNeeded>"
			"\n<CostConfirmationNeeded>%s</CostConfirmationNeeded>\n<Random></Random>\n</AIRequest>\n",
			(certNeeded?"true":"false"), (balanceNeeded?"true":"false"), 
			(costConfirmationNeeded?"true":"false"));
	sendAIMessageToJap(pHashEntry, msgString, strlen(msgString));
}



/**
 * Sends a message to the Jap. The message is automatically splitted into 
 * several mixpackets if necessary and is encrypted using the pCipherOut
 * AES cipher
 */
void CAAccountingInstance::sendAIMessageToJap(fmHashTableEntry pHashEntry,
																						UINT8 *msgString, UINT32 msgLen)
{
	aiAccountingInfo * pAccInfo = pHashEntry->pAccountingInfo;
	CASymCipher * pCipherOut = pAccInfo->pCipherOut;
	MIXPACKET *pMixPacket = (MIXPACKET *) malloc(MIXPACKET_SIZE);

	// set packet header
	pMixPacket->channel = 0xffffffff; //special AI channel number
	pMixPacket->flags = 0;
	
	// send first packet (with length information)
	(UINT32)pMixPacket->DATA[0] = ntohl(msgLen);
	UINT32 numBytes;
	UINT32 index=0;
	if(msgLen < (DATA_SIZE-sizeof(UINT32)) ) 
		numBytes=msgLen;
	else 
		numBytes = DATA_SIZE-sizeof(UINT32);
	memcpy(pMixPacket->DATA+sizeof(UINT32), msgString, numBytes);
	pCipherOut->encrypt(pMixPacket->DATA, pMixPacket->DATA, DATA_SIZE);
	pHashEntry->pMuxSocket->send(pMixPacket);
	
	
	// send remaining packets
	index += numBytes;
	do {
		numBytes = (msgLen-index>=DATA_SIZE ? DATA_SIZE : msgLen-index );
		memcpy(pMixPacket->DATA, msgString+index, numBytes);
		index += numBytes;
		pCipherOut->encrypt(pMixPacket->DATA, pMixPacket->DATA, DATA_SIZE);
		pHashEntry->pMuxSocket->send(pMixPacket);
	} while(msgLen>index);
	free(pMixPacket);
}



/**
 * This must be called whenever a JAP is connecting to init our per-user
 * data structures
 */
UINT32 CAAccountingInstance::initTableEntry(fmHashTableEntry * pHashEntry)
{
	pHashEntry->pAccountingInfo = new aiAccountingInfo;
	memset(pHashEntry->pAccountingInfo, 0, sizeof(aiAccountingInfo) );
	return 0;
}



/**
 * This should always be called when closing a JAP connection
 * to cleanup the data structures
 */
UINT32 CAAccountingInstance::cleanupTableEntry(fmHashTableEntry *pHashEntry) 
{
	CAMsg::printMsg(LOG_DEBUG, "cleanupTableEntry()\n");
	aiAccountingInfo * pAccInfo;
	if(pHashEntry->pAccountingInfo != 0) {
		pAccInfo = pHashEntry->pAccountingInfo;
		if(pAccInfo->pCipherIn != 0) {
			delete pAccInfo->pCipherIn;	
		}
		if(pAccInfo->pCipherOut != 0) {
			delete pAccInfo->pCipherOut;
		}
		if(pAccInfo->pReceiveBuffer != 0) {
			free(pAccInfo->pReceiveBuffer);
		}
		if(pAccInfo->pLastXmlCostConfirmation != 0) {
			free(pAccInfo->pLastXmlCostConfirmation);
		}
		delete pHashEntry->pAccountingInfo;
	}
	return 0;
}



/**
 * Returns the singleton instance of the AccountingInstance.
 */
CAAccountingInstance * CAAccountingInstance::getInstance() 
{
	if( m_pInstance == 0 ) {
		CAAccountingInstance::m_pInstance = new CAAccountingInstance();
	}
	return CAAccountingInstance::m_pInstance;
}

#endif
