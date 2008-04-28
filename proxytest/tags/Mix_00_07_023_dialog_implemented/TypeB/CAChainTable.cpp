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
#include "CAChainTable.hpp"
#include "typedefsb.hpp"
#include "../CAUtil.hpp"


CAChainTable::CAChainTable(void) {
  m_pChainTable = new (t_chaintableEntry*[0x10000]);
  for (UINT32 i = 0; i < 0x10000; i++) {
    m_pChainTable[i] = NULL;
  }
  m_pMutex = new CAMutex();
  m_chaintableSize = 0;
  m_pChaintableIterator = NULL;
  #ifdef DELAY_CHANNELS
    m_initialBucketSize = DELAY_CHANNEL_TRAFFIC;
    m_delayBucketGrow = DELAY_BUCKET_GROW;
    m_delayBucketGrowInterval = DELAY_BUCKET_GROW_INTERVALL;
    m_pDelayBucketMutex = new CAMutex();
    m_pDelayBuckets = new SINT32[MAX_POLLFD];
    for (UINT32 i = 0; i < MAX_POLLFD; i++) {
      m_pDelayBuckets[i] = -1;
    }
    /* start the delay-buckets-refill loop */
    m_delayBucketsLoopRun = true;
    m_pDelayBucketsLoop = new CAThread((UINT8*)"CAChainTable: delay-buckets refill thread");
    m_pDelayBucketsLoop->setMainLoop(lml_chaintableDelayBucketsLoop);
    m_pDelayBucketsLoop->start(this);
  #endif
}

CAChainTable::~CAChainTable(void) {
  /* use the iterator to clean the table */
  getFirstEntry();
  while (m_pChaintableIterator != NULL) {
    /* chaintable iterator is removed, if last entry is reached */
    m_pChaintableIterator->removeEntry = true;
    /* entry is removed automatically, when the iterator points to the next
     * entry
     */
    getNextEntry();
  }
  #ifdef DELAY_CHANNELS
    m_delayBucketsLoopRun = false;
    /* wait for the delay-buckets-refill loop */
    m_pDelayBucketsLoop->join();
    delete m_pDelayBucketsLoop;
    delete m_pDelayBucketMutex;
    delete []m_pDelayBuckets;
  #endif
  delete m_pMutex;
  delete []m_pChainTable;
}

CAChain* CAChainTable::getEntry(UINT8* a_chainId) {
  m_pMutex->lock();
  CAChain* returnedChain = NULL;
  t_chaintableEntry* chaintableEntry = getEntryInternal(a_chainId);
  if (chaintableEntry != NULL) {
    returnedChain = chaintableEntry->chain;
  }
  m_pMutex->unlock();
  return returnedChain;
}

void CAChainTable::deleteEntry(UINT8* a_chainId) {
  m_pMutex->lock();
  t_chaintableEntry* chaintableEntry = getEntryInternal(a_chainId);
  if (chaintableEntry != NULL) {
    /* we have found an entry with the specified ID -> check whether the
     * iterator points to it
     */
    if (m_pChaintableIterator != NULL) {
      if (m_pChaintableIterator->currentEntry == chaintableEntry) {
        /* don't remove the entry but set the remove-flag for the iterator */
        m_pChaintableIterator->removeEntry = true;
      }
      else {
        removeEntryInternal(chaintableEntry);
      }
    }
    else {
      removeEntryInternal(chaintableEntry);
    }
  }
  m_pMutex->unlock();
}

CAChain* CAChainTable::getFirstEntry() {
  CAChain* firstChain = NULL;
  m_pMutex->lock();
  if (m_pChaintableIterator == NULL) {
    m_pChaintableIterator = new t_chaintableIterator;
  }
  else {
    /* look whether we shall remove our last entry */
    if (m_pChaintableIterator->removeEntry) {
      removeEntryInternal(m_pChaintableIterator->currentEntry);
    }
  }
  m_pChaintableIterator->nextHashkey = 0;
  m_pChaintableIterator->removeEntry = false;
  m_pChaintableIterator->currentEntry = NULL;
  getNextEntryInternal(m_pChaintableIterator);
  if (m_pChaintableIterator->currentEntry == NULL) {
    /* no entries in the table */
    delete m_pChaintableIterator;
    m_pChaintableIterator = NULL;
  }
  else {
    firstChain = m_pChaintableIterator->currentEntry->chain;
  }
  m_pMutex->unlock();
  return firstChain;
}

CAChain* CAChainTable::getNextEntry() {
  CAChain* nextChain = NULL;
  m_pMutex->lock();
  if (m_pChaintableIterator != NULL) {
    getNextEntryInternal(m_pChaintableIterator);
    if (m_pChaintableIterator->currentEntry == NULL) {
      /* no more entries in the table */
      delete m_pChaintableIterator;
      m_pChaintableIterator = NULL;
    }
    else {
      nextChain = m_pChaintableIterator->currentEntry->chain;
    }
  }
  m_pMutex->unlock();
  return nextChain;
}

CAChain* CAChainTable::createEntry() {
  m_pMutex->lock();
  if (m_chaintableSize >= MAX_POLLFD) {
    /* we cannot create more chains because we are not able to handle more
     * sockets
     */
    m_pMutex->unlock();
    return NULL;
  }
  /* create a unique chain-id */
  UINT8* chainId = new UINT8[CHAIN_ID_LENGTH];
  do {
    if (getRandom(chainId, CHAIN_ID_LENGTH) != E_SUCCESS) {
      m_pMutex->unlock();
      delete []chainId;
      return NULL;
    }
  }
  while (getEntryInternal(chainId) != NULL);
  t_chaintableEntry* newEntry = new t_chaintableEntry;
  #ifndef DELAY_CHANNELS
    newEntry->chain = new CAChain(chainId);
  #else
    /* find an unused delay-bucket */
    m_pDelayBucketMutex->lock();
    UINT32 i = 0;
    bool bucketFound = false;
    while ((!bucketFound) && (i < MAX_POLLFD)) {
      if (m_pDelayBuckets[i] == -1) {
        bucketFound = true;
      }
      else {
        i++;
      }
    }
    if (!bucketFound) {
      /* we found no bucket -> this shouldn't happen because there cannot be
       * more chains than buckets
       */
      m_pDelayBucketMutex->unlock();
      delete newEntry;
      delete []chainId;
      m_pMutex->unlock();
      return NULL;
    }
    /* initalize our bucket */
    m_pDelayBuckets[i] = (SINT32)m_initialBucketSize;
    m_pDelayBucketMutex->unlock();
    newEntry->chain = new CAChain(chainId, m_pDelayBucketMutex, &(m_pDelayBuckets[i]));
  #endif
  /* take the lower 16 bits as key for the hashtable */
  UINT16 hashKey = (((UINT16)(chainId[CHAIN_ID_LENGTH - 2])) << 8) | (UINT16)(chainId[CHAIN_ID_LENGTH - 1]);
  /* now add the entry at the begin of the hashtable-line */
  newEntry->rightEntry = m_pChainTable[hashKey];
  newEntry->rightEntryPointerOfLeftEntry = &(m_pChainTable[hashKey]);
  m_pChainTable[hashKey] = newEntry;
  if (newEntry->rightEntry != NULL) {
    newEntry->rightEntry->rightEntryPointerOfLeftEntry = &(newEntry->rightEntry);
  }
  m_chaintableSize++;
  m_pMutex->unlock();
  return (newEntry->chain);
}

UINT32 CAChainTable::getSize() {
  UINT32 chaintableSize;
  m_pMutex->lock();
  chaintableSize = m_chaintableSize;
  m_pMutex->unlock();
  return chaintableSize;
}


t_chaintableEntry* CAChainTable::getEntryInternal(UINT8* a_chainId) {
  /* mutex must be already locked */
  /* take the lower 16 bits as key for the hashtable */
  UINT16 hashKey = (((UINT16)(a_chainId[CHAIN_ID_LENGTH - 2])) << 8) | (UINT16)(a_chainId[CHAIN_ID_LENGTH - 1]);
  bool entryFound = false;
  t_chaintableEntry* currentEntry = m_pChainTable[hashKey];
  while ((currentEntry != NULL) && !entryFound) {
    /* a removed entry could be in the table, if the iterator points to it ->
     * we have to check it
     */
    bool entryVirtualRemoved = false;
    if (m_pChaintableIterator != NULL) {
      if ((m_pChaintableIterator->currentEntry == currentEntry) && (m_pChaintableIterator->removeEntry)) {
        entryVirtualRemoved = true;
        /* skip this entry */
        currentEntry = currentEntry->rightEntry;
      }
    }
    if (!entryVirtualRemoved) {
      /* compare the Chain-IDs (lower 2 bytes are identical because of the same
       * hashkey -> don't compare them)
       */
      if (memcmp(currentEntry->chain->getChainId(), a_chainId, CHAIN_ID_LENGTH - 2) == 0) {
        /* we have found the entry */
        entryFound = true;
      }
      else {
        currentEntry = currentEntry->rightEntry;
      }
    }
  }
  return currentEntry;
}

void CAChainTable::removeEntryInternal(t_chaintableEntry* a_entry) {
  /* mutex must be already locked */
  *(a_entry->rightEntryPointerOfLeftEntry) = a_entry->rightEntry;
  if (a_entry->rightEntry != NULL) {
    a_entry->rightEntry->rightEntryPointerOfLeftEntry = a_entry->rightEntryPointerOfLeftEntry;
  }
  /* delete the chain */
  delete a_entry->chain;
  /* delete the table-entry */
  delete a_entry;
  m_chaintableSize--;
}

void CAChainTable::getNextEntryInternal(t_chaintableIterator* a_iterator) {
  /* mutex must be already locked */
  t_chaintableEntry* nextEntry;
  if (a_iterator->currentEntry == NULL) {
    nextEntry = NULL;
  }
  else {
    nextEntry = a_iterator->currentEntry->rightEntry;
    /* look whether we shall remove our last entry */
    if (a_iterator->removeEntry) {
      removeEntryInternal(a_iterator->currentEntry);
      a_iterator->removeEntry = false;
    }
  }
  if (nextEntry == NULL) {
    /* we have to start at the current hashtable-line until we find a
     * hashtable-line with entries or reach again the top of the table
     */
    do {
      a_iterator->currentEntry = m_pChainTable[a_iterator->nextHashkey];
      (a_iterator->nextHashkey)++;
    }
    while ((a_iterator->currentEntry == NULL) && (a_iterator->nextHashkey != 0));
    if (a_iterator->nextHashkey == 0) {
      /* we have reached the top of the table again */
      a_iterator->currentEntry = NULL;
    }
  }
  else {
    /* we have another entry in the same hashtable-line */
    a_iterator->currentEntry = nextEntry;
  }
}

#ifdef DELAY_CHANNELS
  THREAD_RETURN lml_chaintableDelayBucketsLoop(void* a_param) {
    /* get the chaintable we are running on from the parameter */
    CAChainTable* pChainTable = (CAChainTable*)a_param;
    while (pChainTable->m_delayBucketsLoopRun) {
      pChainTable->m_pDelayBucketMutex->lock();
      SINT32 bucketGrow = (SINT32)(pChainTable->m_delayBucketGrow);
      for (UINT32 i = 0; i < MAX_POLLFD; i++) {
        if ((pChainTable->m_pDelayBuckets[i]) != -1) {
          /* let only grow used buckets, but don't make them too large */
          pChainTable->m_pDelayBuckets[i] = min((pChainTable->m_pDelayBuckets[i]) + bucketGrow, MAX_DELAY_BUCKET_SIZE);
        }
      }
      pChainTable->m_pDelayBucketMutex->unlock();
      /* sleep some time before the next grow-round starts */
      msSleep((UINT16)(pChainTable->m_delayBucketGrowInterval));
    }
    THREAD_RETURN_SUCCESS;
  }

  void CAChainTable::setDelayParameters(UINT32 a_initialBucketSize, UINT32 a_delayBucketGrow, UINT32 a_delayBucketGrowInterval) {
    CAMsg::printMsg(LOG_DEBUG, "CAChainTable - Set new traffic limit per channel: inital size: %u bucket grow: %u interval: %u\n", a_initialBucketSize, a_delayBucketGrow, a_delayBucketGrowInterval);
    m_pDelayBucketMutex->lock();
    m_initialBucketSize = a_initialBucketSize;
    m_delayBucketGrow = a_delayBucketGrow;
    m_delayBucketGrowInterval = a_delayBucketGrowInterval;
    /* re-initialize all used buckets */
    for (UINT32 i = 0; i < MAX_POLLFD; i++) {
      if (m_pDelayBuckets[i] != -1) {
        /* re-initialize only used buckets */
        m_pDelayBuckets[i] = a_initialBucketSize;
      }
    }
    m_pDelayBucketMutex->unlock();    
  }                                                        
#endif //DELAY_CHANNELS
#endif//onlyLOCAL_PROXY
