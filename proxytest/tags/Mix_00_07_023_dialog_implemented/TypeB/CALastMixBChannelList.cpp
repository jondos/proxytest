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
#include "CALastMixBChannelList.hpp"
#include "../CAUtil.hpp"

CALastMixBChannelList::CALastMixBChannelList() {
  m_pChannelTable=new (t_lastMixBChannelListEntry*[0x10000]);
  for (UINT32 i = 0; i < 0x10000; i++) {
    m_pChannelTable[i] = NULL;
  }
  m_channelTableSize = 0;
  m_pMutex = new CAMutex();
}

CALastMixBChannelList::~CALastMixBChannelList() {
  delete m_pMutex;
  delete []m_pChannelTable;
}

t_lastMixBChannelListEntry* CALastMixBChannelList::add(HCHANNEL a_channelId, CASymCipher* a_channelCipher, CAChain* a_associatedChain) {
  m_pMutex->lock();
  /* check whether we have not already an entry with the same ID */
  if (getEntryInternal(a_channelId) != NULL) {
    m_pMutex->unlock();
    return NULL;
  }
  t_lastMixBChannelListEntry* newEntry = new t_lastMixBChannelListEntry;
  newEntry->channelId = a_channelId;
  newEntry->channelCipher = a_channelCipher;
  newEntry->associatedChain = a_associatedChain;
  newEntry->associatedChannelList = this;
  newEntry->firstResponseDeadline = NULL;
  /* take the lower 16 bits of the ID as key for the hashtable */
  UINT16 hashKey = (UINT16)(a_channelId & 0x0000FFFF);
  /* now add the entry at the begin of the hashtable-line */
  newEntry->rightEntry = m_pChannelTable[hashKey];
  newEntry->rightEntryPointerOfLeftEntry = &(m_pChannelTable[hashKey]);
  m_pChannelTable[hashKey] = newEntry;
  if (newEntry->rightEntry != NULL) {
    newEntry->rightEntry->rightEntryPointerOfLeftEntry = &(newEntry->rightEntry);
  }
  m_channelTableSize++;
  m_pMutex->unlock();
  return newEntry;
}

void CALastMixBChannelList::removeFromTable(t_lastMixBChannelListEntry* a_channelEntry) {
  m_pMutex->lock();
  if (a_channelEntry->associatedChannelList == this) {
    /* only remove entries which are in the table */
    *(a_channelEntry->rightEntryPointerOfLeftEntry) = a_channelEntry->rightEntry;
    if (a_channelEntry->rightEntry != NULL) {
      a_channelEntry->rightEntry->rightEntryPointerOfLeftEntry = a_channelEntry->rightEntryPointerOfLeftEntry;
    }
    a_channelEntry->rightEntry = NULL;
    a_channelEntry->rightEntryPointerOfLeftEntry = NULL;
    a_channelEntry->associatedChannelList = NULL;
  }
  m_pMutex->unlock();
}

t_lastMixBChannelListEntry* CALastMixBChannelList::get(HCHANNEL a_channelId) {
  t_lastMixBChannelListEntry* returnedEntry = NULL;
  m_pMutex->lock();
  returnedEntry = getEntryInternal(a_channelId);
  m_pMutex->unlock();
  return returnedEntry;
}


t_lastMixBChannelListEntry* CALastMixBChannelList::getEntryInternal(HCHANNEL a_channelId) {
  /* mutex must be already locked */
  /* take the lower 16 bits of the ID as key for the hashtable */
  UINT16 hashKey = (UINT16)(a_channelId & 0x0000FFFF);
  bool entryFound = false;
  t_lastMixBChannelListEntry* currentEntry = m_pChannelTable[hashKey];
  while ((currentEntry != NULL) && !entryFound) {
    /* compare the Channel-IDs */
    if (currentEntry->channelId == a_channelId) {
      /* we have found the entry */
      entryFound = true;
    }
    else {
      currentEntry = currentEntry->rightEntry;
    }
  }
  return currentEntry;
}
