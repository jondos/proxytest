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
#ifndef __CA_CHAINTABLE__
#define __CA_CHAINTABLE__
#ifndef ONLY_LOCAL_PROXY
#include "../CAMutex.hpp"
#include "../CAThread.hpp"
#include "CAChain.hpp"

struct t_chaintableEntry {
  t_chaintableEntry* rightEntry;
  t_chaintableEntry** rightEntryPointerOfLeftEntry;
  CAChain* chain;
}; 

struct t_chaintableIterator {
  t_chaintableEntry* currentEntry;
  bool removeEntry;
  UINT16 nextHashkey;
};


class CAChainTable {
  public:
    CAChainTable(void);
    ~CAChainTable(void);
    CAChain* getEntry(UINT8* a_chainId);
    CAChain* createEntry();
    void deleteEntry(UINT8* a_chainId);
    UINT32 getSize();
    CAChain* getFirstEntry();
    CAChain* getNextEntry();
    #ifdef DELAY_CHANNELS
      void setDelayParameters(UINT32 a_initialBucketSize, UINT32 a_delayBucketGrow, UINT32 a_delayBucketGrowInterval);
    #endif

  private:
    t_chaintableEntry** m_pChainTable;
    CAMutex* m_pMutex;
    UINT32 m_chaintableSize;
    t_chaintableIterator* m_pChaintableIterator;

    t_chaintableEntry* getEntryInternal(UINT8* a_chainId);
    
    #ifdef DELAY_CHANNELS
      CAMutex* m_pDelayBucketMutex;
      SINT32* m_pDelayBuckets;
      CAThread* m_pDelayBucketsLoop;
      volatile bool m_delayBucketsLoopRun;
		  volatile UINT32 m_initialBucketSize;
      volatile UINT32 m_delayBucketGrow;
      volatile UINT32 m_delayBucketGrowInterval;

      friend THREAD_RETURN lml_chaintableDelayBucketsLoop(void* a_param);
    #endif

    void removeEntryInternal(t_chaintableEntry* a_entry);
    void getNextEntryInternal(t_chaintableIterator* a_iterator);

};
#endif //__CA_CHAINTABLE__
#endif //ONLY_LOCAL_PROXY
