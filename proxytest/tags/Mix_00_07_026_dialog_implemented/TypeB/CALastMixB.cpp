/*
 * Copyright (c) 2006, The JAP-Team 
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *   - Neither the name of the University of Technology Dresden, Germany nor
 *     the names of its contributors may be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission. 
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE
 */
#include "../StdAfx.h"
#ifndef ONLY_LOCAL_PROXY
#include "CALastMixB.hpp"
#include "typedefsb.hpp"
#include "CALastMixBChannelList.hpp"
#include "../CASingleSocketGroup.hpp"
#include "../CAPool.hpp"
#include "../CACmdLnOptions.hpp"
#include "../CAUtil.hpp"

#ifdef HAVE_EPOLL
  #include "../CASocketGroupEpoll.hpp"
#endif

extern CACmdLnOptions* pglobalOptions;


CALastMixB::CALastMixB() {
  m_pChainTable = NULL;
  m_pChannelTable = NULL;
}

void CALastMixB::reconfigureMix() {
  #ifdef DELAY_CHANNELS
    CAMsg::printMsg(LOG_DEBUG, "CALastMixB: Set new resources limitation parameters.\n");
    if (m_pChainTable != NULL) {
      m_pChainTable->setDelayParameters(pglobalOptions->getDelayChannelUnlimitTraffic(), pglobalOptions->getDelayChannelBucketGrow(), pglobalOptions->getDelayChannelBucketGrowIntervall());
    }
  #endif
}


SINT32 CALastMixB::loop() {
#ifdef NEW_MIX_TYPE
  /* should only be compiled, if TypeB mixes are used */
  m_pChainTable = new CAChainTable();
  m_pChannelTable = new CALastMixBChannelList();
  #ifdef DELAY_CHANNELS
    m_pChainTable->setDelayParameters(pglobalOptions->getDelayChannelUnlimitTraffic(), pglobalOptions->getDelayChannelBucketGrow(), pglobalOptions->getDelayChannelBucketGrowIntervall());
  #endif  

  #ifdef HAVE_EPOLL
    CASocketGroupEpoll* psocketgroupCacheRead = new CASocketGroupEpoll(false);
    CASocketGroupEpoll* psocketgroupCacheWrite = new CASocketGroupEpoll(true);
  #else
    CASocketGroup* psocketgroupCacheRead = new CASocketGroup(false);
    CASocketGroup* psocketgroupCacheWrite = new CASocketGroup(true);
  #endif

  UINT8 rsaOutputBuffer[RSA_SIZE];
  m_logUploadedPackets = 0;
  m_logDownloadedPackets = 0;
  set64((UINT64&)m_logUploadedBytes,(UINT32)0);
  set64((UINT64&)m_logDownloadedBytes,(UINT32)0);

  /* start logging */
  CAThread* pLogThread = new CAThread((UINT8*)"CALastMixB - LogLoop");
  pLogThread->setMainLoop(lm_loopLog);
  pLogThread->start(this);

  /* initialize some pointers */
  tQueueEntry* currentQueueEntry = new tQueueEntry;
  MIXPACKET* currentMixPacket = &(currentQueueEntry->packet);
  t_upstreamChainCell* pChainCell = (t_upstreamChainCell*)(currentMixPacket->data);
  
  #ifdef LOG_CHAIN_STATISTICS
    CAMsg::printMsg(LOG_DEBUG, "Chain log format is: Chain-ID, Chain duration [micros], Upload (bytes), Download (bytes), Packets from user, Packets to user\n"); 
  #endif

  while(!m_bRestart) {
    /* begin of the mix-loop */
    bool bAktiv = false;

// Step 1a: reading from previous Mix --> now in separate thread
// Step 1b: processing MixPackets from previous mix

    if (m_pQueueReadFromMix->getSize() >= sizeof(tQueueEntry)) {
      /* there is something to do in upstream-direction */
      bAktiv = true;
      UINT32 chains = m_pChainTable->getSize();
      /* limit the number of processed upstream-packets depending on the
       * number of currently forwarded data-chains
       */
      for (UINT32 k = 0; (k < (chains + 1)) && (m_pQueueReadFromMix->getSize() >= sizeof(tQueueEntry)); k++) {
        UINT32 readBytes = sizeof(tQueueEntry);
        m_pQueueReadFromMix->get((UINT8*)currentQueueEntry, &readBytes);
        #ifdef LOG_PACKET_TIMES
          getcurrentTimeMicros(currentQueueEntry->timestamp_proccessing_start_OP);
        #endif
        if ((currentMixPacket->channel > 0) && (currentMixPacket->channel < 256)) {
          /* it's a control-channel packet */
          m_pMuxInControlChannelDispatcher->proccessMixPacket(currentMixPacket);
          /* process the next packet */
          continue;
        }
        /* a data-channel packet received */
        m_logUploadedPackets++;
        if (currentMixPacket->flags & CHANNEL_DUMMY) {
          /* it's only a dummy-packet -> ignore it */
          continue;
        }
        t_lastMixBChannelListEntry* pChannelListEntry = m_pChannelTable->get(currentMixPacket->channel);
        if (pChannelListEntry == NULL) {
          /* it's a new channel */
          #ifdef _DEBUG1
            CAMsg::printMsg(LOG_DEBUG, "New channel from previous Mix!\n");
          #endif                
          m_pRSA->decrypt(currentMixPacket->data, rsaOutputBuffer);
          #ifdef REPLAY_DETECTION
            if (m_pReplayDB->insert(rsaOutputBuffer) != E_SUCCESS) {
              /* we know such a packet already */
              CAMsg::printMsg(LOG_INFO, "Replay: Duplicate packet ignored.\n");
              /* currently we have to send at least a CHANNEL-CLOSE -> reuse
               * our buffers for the response
               */
              getRandom(currentMixPacket->data, DATA_SIZE);
              currentMixPacket->flags = CHANNEL_CLOSE;
              #ifdef LOG_PACKET_TIMES
                /* set invalid packet time for the response */
                setZero64(currentQueueEntry->timestamp_proccessing_start);
              #endif
              m_pQueueSendToMix->add(currentMixPacket, sizeof(tQueueEntry));
              m_logDownloadedPackets++;
              /* process the next packet */
              continue;
            }
          #endif
          /* copy the RSA output-buffer back in the mix-packet (without the
           * symmetric key -> decrypted data will start at the data-pointer
           * of the packet)
           */
          memcpy(currentMixPacket->data, rsaOutputBuffer + KEY_SIZE, RSA_SIZE - KEY_SIZE);
          /* initialize the channel-cipher */
          CASymCipher* channelCipher = new CASymCipher();
          channelCipher->setKey(rsaOutputBuffer);
          /* uncrypt the packet (because the symmetric key at the begin is
           * removed, we have to pull out the decrypted data)
           */
          channelCipher->crypt1((currentMixPacket->data) + RSA_SIZE, (currentMixPacket->data) + RSA_SIZE - KEY_SIZE, DATA_SIZE - RSA_SIZE);
          #ifdef LOG_PACKET_TIMES
            getcurrentTimeMicros(currentQueueEntry->timestamp_proccessing_end_OP);
          #endif
          UINT16 lengthAndFlags = ntohs(pChainCell->lengthAndFlags);
          UINT16 payloadLength = lengthAndFlags & CHAINFLAG_LENGTH_MASK;
          if (lengthAndFlags & CHAINFLAG_NEW_CHAIN) {
            #ifdef _DEBUG1
              CAMsg::printMsg(LOG_DEBUG, "Creating new chain.\n");
            #endif
            CAChain* currentChain = m_pChainTable->createEntry();
            if (currentChain == NULL) {
              /* we are unable to create a new chain (maximum number of
               * chains - defined in MAX_POLLFD - is reached)
               */
              CAMsg::printMsg(LOG_INFO, "Unable to create more than %u chains - dropped new chain.\n", MAX_POLLFD);
              delete channelCipher;
              /* currently we have to send at least a CHANNEL-CLOSE -> reuse
               * our buffers for the response
               */
              getRandom(currentMixPacket->data, DATA_SIZE);
              currentMixPacket->flags = CHANNEL_CLOSE;
              #ifdef LOG_PACKET_TIMES
                /* set invalid packet time for the response */
                setZero64(pQueueEntry->timestamp_proccessing_start);
              #endif
              m_pQueueSendToMix->add(currentMixPacket, sizeof(tQueueEntry));
              m_logDownloadedPackets++;
              /* process the next packet */
              continue;
            }
            currentChain->addChannel(m_pChannelTable->add(currentMixPacket->channel, channelCipher, currentChain), ((lengthAndFlags & CHAINFLAG_FAST_RESPONSE) == CHAINFLAG_FAST_RESPONSE));
            /* Attention: The type-field is handled as part of the payload-
             *            data --> to get the useable payload we have to
             *            subtract the size of the type-field.
             */
            payloadLength = payloadLength - 1;
            if (payloadLength <= MAX_FIRST_UPSTREAM_CHAINCELL_PAYLOAD) {
              /* it's a valid new chain */
              CASocket* tmpSocket = new CASocket;
              CACacheLoadBalancing* pLoadBalancing = m_pCacheLB;
              if (pChainCell->firstCell.type == MIX_PAYLOAD_SOCKS) {
                pLoadBalancing = m_pSocksLB;
              }
              SINT32 errorCode = E_UNKNOWN;
              /* build a new connection to one of the known proxy-servers */
              for (UINT32 count=0; count < pLoadBalancing->getElementCount(); count++) {
                tmpSocket->create();
                tmpSocket->setRecvBuff(50000);
                tmpSocket->setSendBuff(5000);
                errorCode = tmpSocket->connect(*pLoadBalancing->get(), LAST_MIX_TO_PROXY_CONNECT_TIMEOUT);
                if (errorCode == E_SUCCESS) {
                  break;
                }
                tmpSocket->close();
              }  
              if (errorCode != E_SUCCESS) {
                /* could not connect to any proxy */
                CAMsg::printMsg(LOG_DEBUG,"Cannot connect to Squid!\n");
                delete tmpSocket;
                tmpSocket = NULL;
                /* close the chain immediately */
                currentChain->signalConnectionError();
              }
              else {
                /* we have a connection to a proxy */
                #ifdef LOG_CHAIN_STATISTICS
                  currentChain->setSocket(tmpSocket, 1, payloadLength);
                #else
                  currentChain->setSocket(tmpSocket);
                #endif
                #ifdef _DEBUG1
                  /* log the first 30 byte of the chain-data */
                  UINT8 c = pChainCell->firstCell.data[30];
                  /* make a temporary string-cut after 30 byte */
                  pChainCell->firstCell.data[30] = 0;
                  CAMsg::printMsg(LOG_DEBUG, "Try sending data to Squid: %s\n", pChainCell->firstCell.data);
                  pChainCell->firstCell.data[30] = c;
                #endif
                #ifdef LOG_CRIME
                  if (checkCrime(pChainCell->firstCell.data, payloadLength)) {
                    /* we've captured a stupid gangsta, who sent a suspected 
                     * webaddress completely in the first packet
                     */
                    UINT8 crimeBuff[MAX_FIRST_UPSTREAM_CHAINCELL_PAYLOAD + 1];
                    /* ensure that there is a trailing 0 -> use one byte more
                     * than necessary for the plain data
                     */
                    memset(crimeBuff, 0, MAX_FIRST_UPSTREAM_CHAINCELL_PAYLOAD + 1);
                    memcpy(crimeBuff, pChainCell->firstCell.data, payloadLength);
                    /* for compatibility with the default mix-implementation
                     * we will send an extra-packet on the channel with a
                     * crime-signal (without using the channel-cipher)
                     */
                    tQueueEntry oSigCrimeQueueEntry;
                    memset(&oSigCrimeQueueEntry, 0, sizeof(tQueueEntry));
                    UINT32 id = m_pMuxIn->sigCrime(currentMixPacket->channel, &oSigCrimeQueueEntry.packet);
                    m_pQueueSendToMix->add(&oSigCrimeQueueEntry, sizeof(tQueueEntry));
                    m_logDownloadedPackets++;
                    int log = LOG_ENCRYPTED;
                    if (!pglobalOptions->isEncryptedLogEnabled()) {
                      log = LOG_CRIT;
                      CAMsg::printMsg(log,"Crime detected -- ID: %u -- Content: \n%s\n", id, crimeBuff);
                    }
                  }
                #endif
                if (tmpSocket->sendTimeOut(pChainCell->firstCell.data, payloadLength, LAST_MIX_TO_PROXY_SEND_TIMEOUT) == SOCKET_ERROR) {
                  #ifdef _DEBUG
                    CAMsg::printMsg(LOG_DEBUG,"Error sending data to Squid!\n");
                  #endif
                  currentChain->signalConnectionError();
                }
                else {
                  tmpSocket->setNonBlocking(true);
                  currentChain->addToSocketGroup(psocketgroupCacheRead);
                  #ifdef LOG_PACKET_TIMES
                    getcurrentTimeMicros(currentQueueEntry->timestamp_proccessing_end);
                    m_pLogPacketStats->addToTimeingStats(*currentQueueEntry, CHANNEL_OPEN, true);
                  #endif
                  if (lengthAndFlags & CHAINFLAG_STREAM_CLOSED) {
                    /* close upstream (after sending data) */
                    currentChain->closeUpstream();
                  }
                }
              }
            }
            else {
              /* invalid packet length */
              currentChain->signalConnectionError();
            }
          }
          else {
            /* new-chain flag is not set */
            CAChain* currentChain = m_pChainTable->getEntry(pChainCell->sequelCell.chainId);
            if (currentChain != NULL) {
              #ifdef _DEBUG1
                CAMsg::printMsg(LOG_DEBUG, "Continue existent chain.\n");
              #endif
              /* we've found the specified chain in the table */
              currentChain->addChannel(m_pChannelTable->add(currentMixPacket->channel, channelCipher, currentChain), ((lengthAndFlags & CHAINFLAG_FAST_RESPONSE) == CHAINFLAG_FAST_RESPONSE));
              if (payloadLength <= MAX_SEQUEL_UPSTREAM_CHAINCELL_PAYLOAD) {
                /* payload-length is valid */
                if (payloadLength > 0) {
                  currentChain->addDataToUpstreamQueue(pChainCell->sequelCell.data, payloadLength);
                  currentChain->addToSocketGroup(psocketgroupCacheWrite);
                }
                #ifdef LOG_CHAIN_STATISTICS
                /* also add empty packets to the queue (will do nothing, but
                 * adds the received packet to the statistics)
                 */
                else {
                  currentChain->addDataToUpstreamQueue(pChainCell->sequelCell.data, payloadLength);
                }
                #endif
                #ifdef LOG_PACKET_TIMES
                  getcurrentTimeMicros(currentQueueEntry->timestamp_proccessing_end);
                  m_pLogPacketStats->addToTimeingStats(*currentQueueEntry, CHANNEL_DATA, true);
                #endif
                if (lengthAndFlags & CHAINFLAG_STREAM_CLOSED) {
                  /* close upstream (after sending data) */
                  currentChain->closeUpstream();
                }
              }
              else {
                /* invalid payload-length */
                currentChain->signalConnectionError();
              }
            }
            else {
              #ifdef _DEBUG1
                CAMsg::printMsg(LOG_DEBUG, "Unknown chain - cannot continue chain.\n");
              #endif
              /* we don't know a chain with the specified ID -> create a
               * dummy-chain and signal an unknown-chain-error
               */
              currentChain = m_pChainTable->createEntry();
              if (currentChain == NULL) {
                /* we are unable to create a new chain (maximum number of
                 * chains - defined in MAX_POLLFD - is reached)
                 */
                CAMsg::printMsg(LOG_INFO, "Unable to create more than %u chains - cannot send 'unknown chain' response.\n", MAX_POLLFD);
                delete channelCipher;
                /* currently we have to send at least a CHANNEL-CLOSE -> reuse
                 * our buffers for the response
                 */
                getRandom(currentMixPacket->data, DATA_SIZE);
                currentMixPacket->flags = CHANNEL_CLOSE;
                #ifdef LOG_PACKET_TIMES
                  /* set invalid packet time for the response */
                  setZero64(pQueueEntry->timestamp_proccessing_start);
                #endif
                m_pQueueSendToMix->add(currentMixPacket, sizeof(tQueueEntry));
                m_logDownloadedPackets++;
                /* process the next packet */
                continue;
              }
              currentChain->addChannel(m_pChannelTable->add(currentMixPacket->channel, channelCipher, currentChain), ((lengthAndFlags & CHAINFLAG_FAST_RESPONSE) == CHAINFLAG_FAST_RESPONSE));
              currentChain->signalUnknownChain();
            }
          }
        }
        else {
          /* it's not the first channel-packet -> currently only one upstream-
           * packet is allowed -> ignore this one
           */
          CAMsg::printMsg(LOG_INFO, "Received more than one packet on a channel.\n");
        }
      }
    }

//end Step 1

//Step 2 Sending to Cache...

    /* check for chains which have data in the upstream-queue (only those
     * chains are in the socket-group) and having also a send-ready socket
     */
    SINT32 sendReadySockets = psocketgroupCacheWrite->select(0);
    if (sendReadySockets > 0) {
      bAktiv=true;
      #ifdef HAVE_EPOLL
        CAChain* currentChain = (CAChain*)psocketgroupCacheWrite->getFirstSignaledSocketData();
        while (currentChain != NULL) {
          add64((UINT64&)m_logUploadedBytes, currentChain->sendUpstreamData(MIXPACKET_SIZE, psocketgroupCacheWrite));
          currentChain = (CAChain*)(psocketgroupCacheWrite->getNextSignaledSocketData());
        }
      #else
        CAChain* currentChain = m_pChainTable->getFirstEntry();
        while ((currentChain != NULL) && (sendReadySockets > 0)) {
          if (currentChain->isSignaledInSocketGroup(psocketgroupCacheWrite)) {
            sendReadySockets--;
            add64((UINT64&)m_logUploadedBytes, currentChain->sendUpstreamData(MIXPACKET_SIZE, psocketgroupCacheWrite));
          }
          currentChain = m_pChainTable->getNextEntry();
        }
      #endif
    }

//End Step 2

//Step 3 Reading from Cache....

    #define MAX_MIXIN_SEND_QUEUE_SIZE 1000000
    psocketgroupCacheRead->select(0);
    if (m_pQueueSendToMix->getSize() < MAX_MIXIN_SEND_QUEUE_SIZE) {
      /* we are able to send data to the previos mix -> ask every chain
       * whether we can process something
       */
      CAChain* currentChain = m_pChainTable->getFirstEntry(); 
      while (currentChain != NULL) {
        #ifdef LOG_PACKET_TIMES
          /* timestamps are only meaningful, if a packet is created and sent,
           * in the other case they will be overwritten by the next chain
           */
          getcurrentTimeMicros(currentQueueEntry->timestamp_proccessing_start);
          set64(currentQueueEntry->timestamp_proccessing_start_OP, currentQueueEntry->timestamp_proccessing_start);
        #endif
        UINT32 processedBytes;
        SINT32 status = currentChain->processDownstream(psocketgroupCacheRead, currentMixPacket, &processedBytes);
        add64((UINT64&)m_logDownloadedBytes, processedBytes);
        if ((status == 0) || (status == 2)) {
          /* there was a packet created -> send it */
          #ifdef LOG_PACKET_TIMES
            getcurrentTimeMicros(currentQueueEntry->timestamp_proccessing_end_OP);
          #endif
          m_pQueueSendToMix->add(currentMixPacket, sizeof(tQueueEntry)); 
          m_logDownloadedPackets++;
        }
        if ((status == 2) || (status == 3)) {
          /* chain can be removed from the table */
          m_pChainTable->deleteEntry(currentChain->getChainId());
        }
        currentChain = m_pChainTable->getNextEntry();
      }
    }

//end Step 3

//Step 4 Writing to previous Mix
// Now in a separate Thread!
//end step 4

    if (!bAktiv) {
      /* there was no data to process in upstream and downstream direction
       * -> avoid senseless looping and sleep some time
       */
      msSleep(100);
    }
    /* go again to the begin */
  }
  
  /* we have leaved the mix-loop */
  CAMsg::printMsg(LOG_CRIT, "Seams that we are restarting now!\n");
  m_bRestart=true;
  m_pMuxIn->close();
  /* write some bytes to the queue (ensure that m_pthreadSendToMix will stop)
   */
  UINT8 b[sizeof(tQueueEntry)+1];
  m_pQueueSendToMix->add(b, sizeof(tQueueEntry)+1);
  CAMsg::printMsg(LOG_CRIT, "Wait for LoopSendToMix...\n");
  /* will not join if queue is empty (because thread is waiting)!!! */
  m_pthreadSendToMix->join(); 
  m_bRunLog = false;
  CAMsg::printMsg(LOG_CRIT, "Wait for LoopReadFromMix...\n");
  m_pthreadReadFromMix->join();
  #ifdef LOG_PACKET_TIMES
    CAMsg::printMsg(LOG_CRIT, "Wait for LoopLogPacketStats to terminate...\n");
    m_pLogPacketStats->stop();
  #endif
  /* delete the tables (will also remove all entries) */
  delete m_pChainTable;
  delete m_pChannelTable;
  delete currentQueueEntry;
  pLogThread->join();
  delete pLogThread;
  delete psocketgroupCacheWrite;
  delete psocketgroupCacheRead;
#endif //NEW_MIX_TYPE
  return E_UNKNOWN;
}
#endif //ONLY_LOCAL_PROXY
