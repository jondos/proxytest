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

	// initialize JPI connection
	m_biInterface = new CAAccountingBIInterface();
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

	// if necessary, create new CASymCipher objects
	if( pAccInfo->pCipherIn == 0 )
		pAccInfo->pCipherIn = new CASymCipher();
	if( pAccInfo->pCipherOut == 0 )
		pAccInfo->pCipherOut = new CASymCipher();

	// set the keys
	pHashEntry->pAccountingInfo->pCipherIn->setKey(in);
	pHashEntry->pAccountingInfo->pCipherOut->setKey(out);
	return 0;
}




/**
 * This is called by the FirstMix for each incoming JAP packet.
 * Filters out JAP->AI packets.
 * 
 * @return 0 if everything is OK
 * @return >0 means this jap is evil and the connection should be closed
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
		//CAMsg::printMsg(LOG_DEBUG, "CAAccountingInstance: received normal packet, dump:\n");
		//hexDump( (UINT8 *)pPacket, DATA_SIZE+6);
		
		// no, it's a normal user packet.
		// check if the user is authorized
		// if not, close the connection
		//if(userOK) 
			return 0;
		//else
		//	return 1;
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
		handleAccountCertificate(pHashEntry, root);
  }
  else if(strcmp(docElementName, "CC")==0) {
		CAMsg::printMsg(LOG_DEBUG, "AI: It is a CostConfirmation .. nice but atm not necessary\n");
		handleCostConfirmation(pHashEntry, root);				
  }
	else if(strcmp(docElementName, "Balance")==0) {
		CAMsg::printMsg(LOG_DEBUG, "AI: It is a balance certificate .. yuppi!\n");
		handleBalanceCertificate(pHashEntry, root);
	}
  else {
		CAMsg::printMsg(LOG_ERR, "AI: XML message with root element \"%s\" is unknown! Ignoring\n",
										docElementName);
	// Error
  }
	CAMsg::printMsg(LOG_DEBUG, "Deletingi\n");
	delete docElementName;
	//free(docElementName); ??
	CAMsg::printMsg(LOG_DEBUG, "Ende von procesJapMsg\n");
}


/**
 * Handles an account certificate of a newly connected Jap.
 */
void CAAccountingInstance::handleAccountCertificate(fmHashTableEntry *pHashEntry, const DOM_Element &root)
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

	// TODO: validate RESPONSE
	pAccInfo->authState =

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


/**
 * Handles a cost confirmation sent by a jap (INCOMPLETE)
 */
void CAAccountingInstance::handleCostConfirmation(fmHashTableEntry *pHashEntry, const DOM_Element &root)
{
	PGresult *dbResult;

	// strings for db query
	// analyse XML
/*	<CC>
		<ID>=AIname
		<AN>=Accountnummer
		<C>=Cost
		<D>=hash
		<w>=tickprice
	</CC>*/
	
	// parse ai name	
	DOM_Element elemAiName = root.getElementsByTagName(aiNameTagname);
	DOM_Node node = elemAiName.getFirstChild();
	if(node.getNodeType() != TEXT_NODE) {
		CAMsg::printMsg(LOG_DEBUG, "CostConfirmation has WRONG FORMAT!! Ignoring\n");
		return;
	}
	DOMString dstrAiName = ((DOM_CharacterData)node).getData();
	char *aiName = dstrAiName.transcode();	


	// parse cost
	DOM_Element elemAiName = root.getElementsByTagName(aiNameTagname);
	DOM_Node node = elemAiName.getFirstChild();
	if(node.getNodeType() != TEXT_NODE) {
		CAMsg::printMsg(LOG_DEBUG, "CostConfirmation has WRONG FORMAT!! Ignoring\n");
		return;
	}
	DOMString dstrAiName = ((DOM_CharacterData)node).getData();
	char *aiName = dstrAiName.transcode();

		
	// store to db
	m_dbInterface->storeCostConfirmation(...);
}



/**
 * This should always be called when closing a JAP connection
 * to cleanup the data structures
 */
UINT32 CAAccountingInstance::cleanupTableEntry(fmHashTableEntry *pHashEntry) 
{
	CAMsg::printMsg(LOG_DEBUG, "cleanupTableEntry() Hello World\n");
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
 * Tells the Jap which data this AI expects to receive from the Jap.
 * This can be sent to every user at the beginning of the connection,
 * before any user packets were sent
 *
 * INCOMPLETE
 */
void CAAccountingInstance::sendAIRequest(fmHashTableEntry pHashEntry, bool certNeeded,
																				bool balanceNeeded, bool costConfirmationNeeded) 
{
	char str1[] = "<AIRequest>\n";
	char str2[] = "<CertNeeded>";
	char str3[] = "</CertNeeded>\n";
	char str4[] = "<BalanceNeeded>";
	char str5[] = "</BalanceNeeded>\n";
	char str6[] = "<CostConfirmationNeeded>";
	char str7[] = "</CostConfirmationNeeded>\n";
	char str8[] = "<Random>";
	char str9[] = "</Random>\n";
	char str10[] = "</AIRequest>\n";

//	sprintf(...);
//	sendAIMessageToJap(pHashEntry, msgString, strlen(msgString));
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
	
	
	UINT32 msgTotalLength = msgLen + sizeof(UINT32); // add 4 bytes for the msg length 
	UINT32 numPackets = msgTotalLength / DATA_SIZE;
	if( (msgTotalLength%DATA_SIZE) != 0 ) numPackets++;
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
	
	// send remaining packets
	for(i=1; i<numPackets; i++) {
		index += numBytes;	
		memcpy(pMixPacket->DATA+sizeof(UINT32)+index, msgString+index, numBytes);
		pCipherOut->encrypt(pMixPacket->DATA, pMixPacket->DATA, DATA_SIZE);	
	}
}



/**
 * This must be called when a JAP is connecting to init our data structures
 */
UINT32 CAAccountingInstance::initTableEntry(fmHashTableEntry * pHashEntry)
{
	pHashEntry->pAccountingInfo = new aiAccountingInfo;
	memset(pHashEntry->pAccountingInfo, 0, sizeof(aiAccountingInfo) );
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
