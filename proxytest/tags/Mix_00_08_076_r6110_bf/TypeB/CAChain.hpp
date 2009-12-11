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
#ifndef __CA_CHAIN__
#define __CA_CHAIN__
#include "../doxygen.h"
#ifndef ONLY_LOCAL_PROXY
#include "CALastMixBChannelList.hpp"
#include "../CASocket.hpp"
#include "../CAQueue.hpp"
#ifdef HAVE_EPOLL	
  #include "../CASocketGroupEpoll.hpp"
#else
  #include "../CASocketGroup.hpp"
#endif
#ifdef DELAY_CHANNELS
  #include "../CAMutex.hpp"
#endif


struct t_channelEntry {
  struct t_lastMixBChannelListEntry* channel;
  t_channelEntry* nextChannel;
};

struct t_socketGroupEntry {
  #ifdef HAVE_EPOLL	
    CASocketGroupEpoll* socketGroup;
  #else
    CASocketGroup* socketGroup;
  #endif
  t_socketGroupEntry* nextSocketGroup;
};


class CAChain {
  public:
    #ifndef DELAY_CHANNELS
      CAChain(UINT8* a_chainId);
    #else
      CAChain(UINT8* a_chainId, CAMutex* a_delayBucketMutex, SINT32* a_delayBucket);
    #endif
    ~CAChain(void);
    UINT8* getChainId();
    #ifdef LOG_CHAIN_STATISTICS
      void setSocket(CASocket* a_socket, UINT32 a_alreadyProcessedPackets, UINT32 a_alreadyProcessedBytes);
    #else
      void setSocket(CASocket* a_socket);
    #endif
    void addChannel(t_lastMixBChannelListEntry* a_channel, bool a_fastResponse);
    void addDataToUpstreamQueue(UINT8* a_buffer, UINT32 a_size);
    void signalConnectionError();
    void signalUnknownChain();
    void closeUpstream();
    void closeDownstream();
    UINT8* getPrintableChainId();
    #ifdef HAVE_EPOLL
      void addToSocketGroup(CASocketGroupEpoll* a_socketGroup);
      void removeFromSocketGroup(CASocketGroupEpoll* a_socketGroup);
      bool isSignaledInSocketGroup(CASocketGroupEpoll* a_socketGroup);
      UINT32 sendUpstreamData(UINT32 a_maxLength, CASocketGroupEpoll* a_removedSocketGroup);
      SINT32 processDownstream(CASocketGroupEpoll* a_signalingGroup, MIXPACKET* a_downstreamPacket, UINT32* a_processedBytes);
    #else
      void addToSocketGroup(CASocketGroup* a_socketGroup);
      void removeFromSocketGroup(CASocketGroup* a_socketGroup);
      bool isSignaledInSocketGroup(CASocketGroup* a_socketGroup);
      UINT32 sendUpstreamData(UINT32 a_maxLength, CASocketGroup* a_removedSocketGroup);
      SINT32 processDownstream(CASocketGroup* a_signalingGroup, MIXPACKET* a_downstreamPacket, UINT32* a_processedBytes);
    #endif

  private:
    UINT8* m_chainId;
    SINT32 m_lastAccessTime;
    CASocket* m_socket;
    CAQueue* m_upstreamSendQueue;
    t_channelEntry* m_firstChannel;
    t_socketGroupEntry* m_firstSocketGroup;
    bool m_connectionError;
    bool m_downstreamClosed;
    bool m_upstreamClosed;
    bool m_firstDownstreamPacket;
    bool m_unknownChainId;

    #ifdef LOG_CHAIN_STATISTICS
      /* some statistics */
      UINT32 m_packetsFromUser;
      UINT32 m_bytesFromUser;
      UINT32 m_packetsToUser;
      UINT32 m_bytesToUser;
      UINT64 m_creationTime;
    #endif

    #ifdef DELAY_CHANNELS
      CAMutex* m_pDelayBucketMutex;
      SINT32* m_pDelayBucket;

      SINT32 getDelayBucketInternal();
      void removeFromDelayBucketInternal(SINT32 a_bytesToRemove);
    #endif

    void closeChainInternal();
    void forceImmediateResponsesInternal();
    void removeFromAllSocketGroupsInternal();
    UINT32 sendUpstreamDataInternal(UINT32 a_maxLength);
};
#endif //__CA_CHAIN__
#endif //ONLY_LOCAL_PROXY
