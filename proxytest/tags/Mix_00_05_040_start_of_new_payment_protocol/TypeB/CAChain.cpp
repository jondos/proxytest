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
#include "CAChain.hpp"
#include "typedefsb.hpp"
#include "CALastMixBChannelList.hpp"
#include "../CAUtil.hpp"

#ifndef DELAY_CHANNELS
CAChain::CAChain(UINT8* a_chainId) {
#else
CAChain::CAChain(UINT8* a_chainId, CAMutex* a_delayBucketMutex, SINT32* a_delayBucket) {
#endif
  m_chainId = a_chainId;
  m_firstChannel = NULL;
  m_socket = NULL;
  m_lastAccessTime = -1;
  m_upstreamSendQueue = new CAQueue(DATA_SIZE);
  m_upstreamClosed = false;
  m_downstreamClosed = false;
  m_firstSocketGroup = NULL;
  m_connectionError = false;
  m_unknownChainId = false;
  m_firstDownstreamPacket = true;
  #ifdef LOG_CHAIN_STATISTICS
    m_packetsFromUser = 0;
    m_bytesFromUser = 0;
    m_packetsToUser = 0;
    m_bytesToUser = 0;
    getcurrentTimeMicros(m_creationTime);
  #endif
  #ifdef DELAY_CHANNELS
    m_pDelayBucketMutex = a_delayBucketMutex;
    m_pDelayBucket = a_delayBucket;
  #endif
}

CAChain::~CAChain(void) {  
  if (m_socket != NULL) {
    removeFromAllSocketGroupsInternal();
    m_socket->close();
    delete m_socket;
  }
  m_upstreamSendQueue->clean();
  delete m_upstreamSendQueue;
  /* remove all associated channels (normally there shouldn't be any, but in
   * case of a shutdown, some channels may be still open)
   */
  while (m_firstChannel != NULL) {
    t_channelEntry* channelEntry = m_firstChannel;
    /* remove the entry from the channel-table */
    channelEntry->channel->associatedChannelList->removeFromTable(channelEntry->channel);
    /* remove all deadlines */
    while (channelEntry->channel->firstResponseDeadline != NULL) {
      t_deadlineEntry* currentDeadline = channelEntry->channel->firstResponseDeadline;
      channelEntry->channel->firstResponseDeadline = currentDeadline->nextDeadline;
      delete currentDeadline;
    }
    /* remove the channel-cipher */
    delete channelEntry->channel->channelCipher;
    m_firstChannel = channelEntry->nextChannel;
    delete channelEntry;
  }
  #ifdef LOG_CHAIN_STATISTICS
    /* log chain-statistics with format:
     * Chain-ID, Chain duration [micros], Upload (bytes), Download (bytes), Packets from user, Packets to user
     */
     UINT64 currentTime;
     getcurrentTimeMicros(currentTime);
     UINT32 duration = diff64(currentTime, m_creationTime);
     UINT8* chainId = getPrintableChainId();
     CAMsg::printMsg(LOG_DEBUG, "%s,%u,%u,%u,%u,%u\n", chainId, duration, m_bytesFromUser, m_bytesToUser, m_packetsFromUser, m_packetsToUser);
     delete []chainId;
  #endif
  delete []m_chainId;
  #ifdef DELAY_CHANNELS
    /* free the delay-bucket (set it to -1), don't delete the mutex because it
     * is used for all delay-buckets
     */
    m_pDelayBucketMutex->lock();
    *m_pDelayBucket = -1;
    m_pDelayBucketMutex->unlock();
  #endif
}

UINT8* CAChain::getChainId() {
  return m_chainId;
}

#ifdef LOG_CHAIN_STATISTICS
  void CAChain::setSocket(CASocket* a_socket, UINT32 a_alreadyProcessedPackets, UINT32 a_alreadyProcessedBytes) {
    m_socket = a_socket;
    m_bytesFromUser = a_alreadyProcessedBytes;
    m_packetsFromUser = a_alreadyProcessedPackets;
  }
#else
  void CAChain::setSocket(CASocket* a_socket) {
    m_socket = a_socket;
  }
#endif

/**
 * Returns: 0, if a packet was created.
 *          1, if currently nothing can be done.
 *          2, if a packet was created and the chain can be removed from the chaintable.
 *          3, if no packet was created, but the chain can be removed from the chaintable.
 */
#ifdef HAVE_EPOLL
SINT32 CAChain::processDownstream(CASocketGroupEpoll* a_signalingGroup, MIXPACKET* a_downstreamPacket, UINT32* a_processedBytes) {
#else
SINT32 CAChain::processDownstream(CASocketGroup* a_signalingGroup, MIXPACKET* a_downstreamPacket, UINT32* a_processedBytes) {
#endif
  *a_processedBytes = 0;
  /* first: get the time - we will need it */
  timespec currentTime;
  getcurrentTime(currentTime);
  if (m_lastAccessTime != -1) {
    /* currently we dont't have an associated channel -> check whether the
     * access-timeout is reached
     */
    if (m_lastAccessTime + CHAIN_TIMEOUT < currentTime.tv_sec) {
      /* timeout is reached */
      closeChainInternal();
      return 3;
    }
    /* there is currently no channel associated -> we can't do anything */
    return 1;
  }
  /* we have at least one associated channel */
  /* check whether we have to drop packages because of outdated deadlines */
  t_deadlineEntry* testedDeadlineEntry = m_firstChannel->channel->firstResponseDeadline;
  if (((testedDeadlineEntry->deadline.tv_sec + DEADLINE_TIMEOUT) < currentTime.tv_sec) || (((testedDeadlineEntry->deadline.tv_sec + DEADLINE_TIMEOUT) == currentTime.tv_sec) && (testedDeadlineEntry->deadline.tv_nsec <= currentTime.tv_nsec))) {
    /* we are too late, it wouldn't make sense to send the packet -> we will
     * reduce traffic by dropping the packet (and all following packets of the
     * channel) -> currently we have to send at least a CHANNEL-CLOSE, so keep
     * one packet in the channel
     */
    if (m_firstChannel->channel->remainingDownstreamPackets > 1) {
      /* we will really loose packets -> synchronization between client and
       * server is destroyed -> signal connection error and close the chain
       */
      signalConnectionError();
      UINT8* chainId = getPrintableChainId();
      CAMsg::printMsg(LOG_INFO, "Dropped downstream-packets from chain '%s'!\n", chainId);
      delete []chainId;
      while (m_firstChannel->channel->remainingDownstreamPackets > 1) {
        m_firstChannel->channel->remainingDownstreamPackets--;
        m_firstChannel->channel->firstResponseDeadline = testedDeadlineEntry->nextDeadline;
        delete testedDeadlineEntry;
        testedDeadlineEntry = m_firstChannel->channel->firstResponseDeadline;
      }
    }
  }
  /* now try to send something */
  t_downstreamChainCell* pChainCell = (t_downstreamChainCell*)(a_downstreamPacket->data);
  if ((m_socket != NULL) && (!m_downstreamClosed) && (m_firstChannel != NULL)) {
    if (m_firstChannel->channel->remainingDownstreamPackets > 1) {
      /* we are able to send data to the client -> look whether data is
       * available at the socket
       */
      if (isSignaledInSocketGroup(a_signalingGroup)) {
        /* there is something available -> check how much data we can process
         */
        UINT16 payloadData = MAX_SEQUEL_DOWNSTREAM_CHAINCELL_PAYLOAD;
        if (m_firstDownstreamPacket) {
          payloadData = MAX_FIRST_DOWNSTREAM_CHAINCELL_PAYLOAD;
        }
        #ifdef DELAY_CHANNELS
          payloadData = min(payloadData, (UINT16)getDelayBucketInternal());
        #endif
        if (payloadData > 0) {
          /* we will receive something */
          /* if the packet isn't filled fully, some randomness for the
           * remainging space would be great
           */
          getRandom(a_downstreamPacket->data, DATA_SIZE);
          SINT32 bytesReceived;
          if (m_firstDownstreamPacket) {
            bytesReceived = m_socket->receive(pChainCell->firstCell.data, payloadData);
          }
          else {
            bytesReceived = m_socket->receive(pChainCell->sequelCell.data, payloadData);
          }
          if (bytesReceived >= 0) {
            if (bytesReceived == 0) {
              /* seems to be the end of the data-stream */
              closeDownstream();
            }
            else {
              /* we have received some bytes -> create the packet */
              #ifdef DELAY_CHANNELS
                removeFromDelayBucketInternal(bytesReceived);
              #endif
              if (m_firstDownstreamPacket) {
                /* also we have to send the Chain-ID */
                memcpy(pChainCell->firstCell.chainId, m_chainId, CHAIN_ID_LENGTH);
                m_firstDownstreamPacket = false;
              }
              pChainCell->lengthAndFlags = htons((UINT16)bytesReceived);
              a_downstreamPacket->channel = m_firstChannel->channel->channelId;
              a_downstreamPacket->flags = CHANNEL_DATA;
              m_firstChannel->channel->channelCipher->crypt2(a_downstreamPacket->data, a_downstreamPacket->data, DATA_SIZE);
              m_firstChannel->channel->remainingDownstreamPackets--;
              t_deadlineEntry* currentDeadline = m_firstChannel->channel->firstResponseDeadline;
              m_firstChannel->channel->firstResponseDeadline = currentDeadline->nextDeadline;
              delete currentDeadline;
              *a_processedBytes = (UINT32)bytesReceived;
              #ifdef LOG_CHAIN_STATISTICS
                m_packetsToUser++;
                m_bytesToUser = m_bytesToUser + (UINT32)bytesReceived;
              #endif
              return 0;
            }
          }
          else {
            /* there was a connection error */
            signalConnectionError();
          }
        }
      }
    }
  }
  /* we cannot send any real data, but maybe we have to send some protocol data */
  if (m_firstChannel->channel->remainingDownstreamPackets == 1) {
    /* currently we have to send a CHANNEL-CLOSE */
    getRandom(a_downstreamPacket->data, DATA_SIZE);
    a_downstreamPacket->channel = m_firstChannel->channel->channelId;
    a_downstreamPacket->flags = CHANNEL_CLOSE;
    /* delete channel-resources */
    t_lastMixBChannelListEntry* currentChannel = m_firstChannel->channel;
    currentChannel->associatedChannelList->removeFromTable(currentChannel);
    delete currentChannel->firstResponseDeadline;
    delete currentChannel->channelCipher;
    delete currentChannel;
    t_channelEntry* currentChannelEntry = m_firstChannel;
    /* change to the next channel */
    m_firstChannel = m_firstChannel->nextChannel;
    delete currentChannelEntry;
    #ifdef LOG_CHAIN_STATISTICS
      /* a packet (CHANNEL_CLOSE) without payload is sent */
      m_packetsToUser++;
    #endif
    if (m_firstChannel == NULL) {
      if (m_downstreamClosed && m_upstreamClosed) {
        /* it was the last channel and the chain is closed -> it can be
         * removed from the table
         */
        return 2;
      }
      else {
        /* it was the last channel, but the chain isn't closed -> start the
         * access timeout
         */
        timespec currentTime;
        getcurrentTime(currentTime);
        m_lastAccessTime = currentTime.tv_sec;
        return 0;
      }
    }
    /* we've sent a close but it wasn't the last channel */
    return 0;
  }
  /* no data, no channel-close, but maybe we have to send a packet because of
   * a deadline
   */
  if ((m_firstChannel->channel->firstResponseDeadline->deadline.tv_sec < currentTime.tv_sec) || ((m_firstChannel->channel->firstResponseDeadline->deadline.tv_sec == currentTime.tv_sec) && (m_firstChannel->channel->firstResponseDeadline->deadline.tv_nsec <= currentTime.tv_nsec))) {
    /* deadline reached */
    getRandom(a_downstreamPacket->data, DATA_SIZE);
    pChainCell->lengthAndFlags = 0;
    if (m_firstDownstreamPacket) {
      /* also we have to send the Chain-ID */
      memcpy(pChainCell->firstCell.chainId, m_chainId, CHAIN_ID_LENGTH);
      m_firstDownstreamPacket = false;
    }
    /* maybe we have to set some flags */
    if (m_unknownChainId) {
      pChainCell->lengthAndFlags = pChainCell->lengthAndFlags | CHAINFLAG_UNKNOWN_CHAIN;
      /* reset the flag */
      m_unknownChainId = false;
    }
    if (m_connectionError) {
      pChainCell->lengthAndFlags = pChainCell->lengthAndFlags | CHAINFLAG_CONNECTION_ERROR;
      /* reset the flag */
      m_connectionError = false;
    }
    if (m_downstreamClosed) {
      pChainCell->lengthAndFlags = pChainCell->lengthAndFlags | CHAINFLAG_STREAM_CLOSED;
      /* don't reset the flag */
    }
    /* ensure correct byte order */
    pChainCell->lengthAndFlags = htons(pChainCell->lengthAndFlags);
    /* finalize packet */
    a_downstreamPacket->channel = m_firstChannel->channel->channelId;
    a_downstreamPacket->flags = CHANNEL_DATA;
    m_firstChannel->channel->channelCipher->crypt2(a_downstreamPacket->data, a_downstreamPacket->data, DATA_SIZE);
    /* clean up */
    m_firstChannel->channel->remainingDownstreamPackets--;
    t_deadlineEntry* currentDeadline = m_firstChannel->channel->firstResponseDeadline;
    m_firstChannel->channel->firstResponseDeadline = currentDeadline->nextDeadline;
    delete currentDeadline;
    #ifdef LOG_CHAIN_STATISTICS
      /* a packet without payload is sent */
      m_packetsToUser++;
    #endif
    return 0;
  }
  /* no deadline reached and nothing else to do */
  return 1;
}

#ifdef HAVE_EPOLL
bool CAChain::isSignaledInSocketGroup(CASocketGroupEpoll* a_socketGroup) {
  if (m_socket != NULL) {
    return a_socketGroup->isSignaled(this);
  }
  return false;
}
#else
bool CAChain::isSignaledInSocketGroup(CASocketGroup* a_socketGroup) {
  if (m_socket != NULL) {
    return a_socketGroup->isSignaled(*m_socket);
  }
  return false;
}
#endif

#ifdef HAVE_EPOLL
void CAChain::addToSocketGroup(CASocketGroupEpoll* a_socketGroup) {
#else
void CAChain::addToSocketGroup(CASocketGroup* a_socketGroup) {
#endif
  if (m_socket != NULL) {
    /* check whether our socket isn't already in the specified socket-group
    */
    t_socketGroupEntry* currentEntry = m_firstSocketGroup;
    t_socketGroupEntry** previousNextEntryPointer = &m_firstSocketGroup;
    bool alreadyIncluded = false;
    while ((currentEntry != NULL) && (!alreadyIncluded)) {
      if (currentEntry->socketGroup == a_socketGroup) {
        alreadyIncluded = true;
      }
      else {
        previousNextEntryPointer = &(currentEntry->nextSocketGroup);
        currentEntry = currentEntry->nextSocketGroup;
      }
    }
    if (!alreadyIncluded) {
      #ifdef HAVE_EPOLL
        a_socketGroup->add(*m_socket, this);
      #else
        a_socketGroup->add(*m_socket);
      #endif
      currentEntry = new t_socketGroupEntry;
      currentEntry->nextSocketGroup = NULL;
      currentEntry->socketGroup = a_socketGroup;
      *previousNextEntryPointer = currentEntry;
    }
  }
}

#ifdef HAVE_EPOLL
void CAChain::removeFromSocketGroup(CASocketGroupEpoll* a_socketGroup) {
#else
void CAChain::removeFromSocketGroup(CASocketGroup* a_socketGroup) {
#endif
  if (m_socket != NULL) {
    /* check whether our socket is in the specified socket-group */
    t_socketGroupEntry* currentEntry = m_firstSocketGroup;
    t_socketGroupEntry** previousNextEntryPointer = &m_firstSocketGroup;
    while (currentEntry != NULL) {
      if (currentEntry->socketGroup == a_socketGroup) {
        /* we are in the specified socket group -> remove occurance */
        a_socketGroup->remove(*m_socket);
        *previousNextEntryPointer = currentEntry->nextSocketGroup;
        delete currentEntry;
        currentEntry = NULL;
      }
      else {
        previousNextEntryPointer = &(currentEntry->nextSocketGroup);
        currentEntry = currentEntry->nextSocketGroup;
      }
    }
  }
}

#ifdef HAVE_EPOLL
UINT32 CAChain::sendUpstreamData(UINT32 a_maxLength, CASocketGroupEpoll* a_removedSocketGroup) {
#else
UINT32 CAChain::sendUpstreamData(UINT32 a_maxLength, CASocketGroup* a_removedSocketGroup) {
#endif
  UINT32 processedBytes = sendUpstreamDataInternal(a_maxLength);
  if (m_upstreamSendQueue->isEmpty()) {
    /* queue is empty -> we can remove the entry from the socketgroup */
    removeFromSocketGroup(a_removedSocketGroup);
  }
  return processedBytes;
}


void CAChain::addDataToUpstreamQueue(UINT8* a_buffer, UINT32 a_size) {
  if (!m_upstreamClosed) {
    /* only add data if upstream isn't closed */
    m_upstreamSendQueue->add(a_buffer, a_size);
    #ifdef LOG_CHAIN_STATISTICS
      m_packetsFromUser++;
      m_bytesFromUser = m_bytesFromUser + a_size;
    #endif
  }
}

void CAChain::closeUpstream() {
  /* currently we will close the whole chain immediately */
  closeChainInternal();
}

void CAChain::closeDownstream() {
  /* currently we will close the whole chain immediately */
  closeChainInternal();
}

void CAChain::signalConnectionError() {
  m_connectionError = true;
  /* we will also close the chain */
  closeChainInternal();
}

void CAChain::signalUnknownChain() {
  m_unknownChainId = true;
  /* we will not send any chain-id -> disable m_firstDownstreamPacket */
  m_firstDownstreamPacket = false;
  /* we will also close the chain */
  closeChainInternal();
}

UINT8* CAChain::getPrintableChainId() {
  UINT8* printableChainId = bytes2hex(m_chainId, CHAIN_ID_LENGTH);
  strtrim(printableChainId);
  return printableChainId;
}

void CAChain::addChannel(t_lastMixBChannelListEntry* a_channel, bool a_fastResponse) {
  t_channelEntry* lastChannel = NULL;
  bool invalidChannel = false;
  if (m_firstChannel != NULL) {
    if (m_firstChannel->nextChannel != NULL) {
      /* somebody is trying to add a third channel to the chain but currently
       * only 2 channels can be associated to a data-chain -> ignore the new
       * channel (attention: currently we have to send at least a
       * CHANNEL-CLOSE because there is no channel-timeout at first and middle
       * mixes), send an IOException and close the chain
       */
      invalidChannel = true;
      signalConnectionError();
      /* find the last associated channel */
      lastChannel = m_firstChannel->nextChannel;
      while (lastChannel->nextChannel != NULL) {
        lastChannel = lastChannel->nextChannel;
      }   
    }
    else {
      lastChannel = m_firstChannel;
    }
  }
  t_channelEntry* newChannel = new t_channelEntry;
  /* initialize the fields */
  newChannel->nextChannel = NULL;
  newChannel->channel = a_channel;
  if (lastChannel != NULL) {
    /* close all previous channels immediately */
    forceImmediateResponsesInternal();
    /* now add the new channel */
    lastChannel->nextChannel = newChannel;
  }
  else {
    m_firstChannel = newChannel;
  }
  timespec currentTime;
  getcurrentTime(currentTime);
  if (!invalidChannel) {
    if ((!(m_upstreamClosed && m_downstreamClosed)) && (m_lastAccessTime != -1)) {
      /* if not downstream and upstream is closed and also an access-timeout
       * is running -> stop that access-timeout
       */
      m_lastAccessTime = -1;
    }
    a_channel->remainingDownstreamPackets = CHANNEL_DOWNSTREAM_PACKETS;
    /* create deadlines for the new downstream-packets */
    t_deadlineEntry** lastNextDeadlinePointer = &(a_channel->firstResponseDeadline);
    for (UINT32 i = 0; i < CHANNEL_DOWNSTREAM_PACKETS; i++) {
      t_deadlineEntry* currentDeadline = new t_deadlineEntry;
      currentDeadline->nextDeadline = NULL;
      if (!m_downstreamClosed) {
        if (a_fastResponse && (i == 0)) {
          /* we shall send a fast response -> send back the first packet of
           * the new channel immediately
           */
          currentDeadline->deadline.tv_sec = currentTime.tv_sec;
        }
        else {
          /* use normal channel-timeout */
          currentDeadline->deadline.tv_sec = currentTime.tv_sec + CHANNEL_TIMEOUT;
        }
      }
      else {
        /* downstream is already closed -> send packets back immediately */
        currentDeadline->deadline.tv_sec = currentTime.tv_sec;
      }
      currentDeadline->deadline.tv_nsec = currentTime.tv_nsec;
      *lastNextDeadlinePointer = currentDeadline;
      lastNextDeadlinePointer = &(currentDeadline->nextDeadline);
    }
  }
  else {
    /* send only one packet (will be CHANNEL-CLOSE) */
    a_channel->remainingDownstreamPackets = 1;
    a_channel->firstResponseDeadline = new t_deadlineEntry;
    a_channel->firstResponseDeadline->nextDeadline = NULL;
    a_channel->firstResponseDeadline->deadline.tv_sec = currentTime.tv_sec;
    a_channel->firstResponseDeadline->deadline.tv_nsec = currentTime.tv_nsec;
  }
}

void CAChain::closeChainInternal() {
  m_upstreamClosed = true;
  m_downstreamClosed = true;
  m_upstreamSendQueue->clean();
  if (m_socket != NULL) {
    removeFromAllSocketGroupsInternal();
    m_socket->close();
    delete m_socket;
    m_socket = NULL;
  }
  /* send back all response-packets immediately */
  forceImmediateResponsesInternal();
}

void CAChain::forceImmediateResponsesInternal() {
  /* set all deadlines to the current time (if they don't have a previous
   * time) */
  timespec currentTime;
  getcurrentTime(currentTime);
  t_channelEntry* currentChannel = m_firstChannel;
  while (currentChannel != NULL) {     
    t_deadlineEntry* currentDeadline = currentChannel->channel->firstResponseDeadline;
    while (currentDeadline != NULL) {
      if (currentDeadline->deadline.tv_sec > currentTime.tv_sec) {
        currentDeadline->deadline.tv_sec = currentTime.tv_sec;
        currentDeadline->deadline.tv_nsec = currentTime.tv_nsec;
      }
      else {
        if ((currentDeadline->deadline.tv_sec == currentTime.tv_sec) && (currentDeadline->deadline.tv_nsec > currentTime.tv_nsec)) {
          currentDeadline->deadline.tv_nsec = currentTime.tv_nsec;
        }
      }
      currentDeadline = currentDeadline->nextDeadline;
    }
    currentChannel = currentChannel->nextChannel;
  }
}  

void CAChain::removeFromAllSocketGroupsInternal() {
  t_socketGroupEntry* currentEntry = m_firstSocketGroup;
  m_firstSocketGroup = NULL;
  while (currentEntry != NULL) {
    currentEntry->socketGroup->remove(*m_socket);
    t_socketGroupEntry* nextEntry = currentEntry->nextSocketGroup;
    delete currentEntry;
    currentEntry = nextEntry;
  }
}

UINT32 CAChain::sendUpstreamDataInternal(UINT32 a_maxLength) {
  UINT32 bytesSent = 0;
  if ((!m_upstreamClosed) && (m_socket != NULL)) {
    UINT32 length = a_maxLength;
    UINT8* buffer = new UINT8[length];
    if (m_upstreamSendQueue->peek(buffer, &length) == E_SUCCESS) {
      /* queue has filled the buffer */
      SINT32 errorCode = m_socket->send(buffer, length);
      if (errorCode >= 0) {
        length = (UINT32)errorCode;
        bytesSent = length;
        m_upstreamSendQueue->remove(&length);
      }
      else {
        /* error while sending data */
        signalConnectionError();
      }
    }
  }
  return bytesSent;
}

#ifdef DELAY_CHANNELS
  SINT32 CAChain::getDelayBucketInternal() {
    SINT32 delayBucket;
    m_pDelayBucketMutex->lock();
    delayBucket = *m_pDelayBucket;
    m_pDelayBucketMutex->unlock();
    return delayBucket;
  }

  void CAChain::removeFromDelayBucketInternal(SINT32 a_bytesToRemove) {
    m_pDelayBucketMutex->lock();
    *m_pDelayBucket = (*m_pDelayBucket) + a_bytesToRemove;
    m_pDelayBucketMutex->unlock();
  }
#endif
#endif //ONLY_LOCAL_PROXY
