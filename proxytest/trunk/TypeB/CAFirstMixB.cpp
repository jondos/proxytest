/*
Copyright (c) 2000, The JAP-Team 
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.

  - Redistributions in binary form must reproduce the above copyright notice, 
    this list of conditions and the following disclaimer in the documentation and/or 
    other materials provided with the distribution.

  - Neither the name of the University of Technology Dresden, Germany nor the names of its contributors 
    may be used to endorse or promote products derived from this software without specific 
    prior written permission. 

  
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS 
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/
#include "../StdAfx.h"
#ifndef ONLY_LOCAL_PROXY
#include "CAFirstMixB.hpp"
#include "../CAThread.hpp"
#include "../CASingleSocketGroup.hpp"
#include "../CAInfoService.hpp"
#include "../CAPool.hpp"
#include "../CACmdLnOptions.hpp"
#include "../CAAccountingInstance.hpp"
#ifdef HAVE_EPOLL
  #include "../CASocketGroupEpoll.hpp"
#endif
extern CACmdLnOptions* pglobalOptions;

SINT32 CAFirstMixB::loop()
  {
#ifdef NEW_MIX_TYPE
    /* should only be compiled if new NEW_MIX_TYPE is defined */
#ifdef DELAY_USERS
    m_pChannelList->setDelayParameters(  pglobalOptions->getDelayChannelUnlimitTraffic(),
                                      pglobalOptions->getDelayChannelBucketGrow(),
                                      pglobalOptions->getDelayChannelBucketGrowIntervall());  
#endif    

  //  CASingleSocketGroup osocketgroupMixOut;
    SINT32 countRead;
    //#ifdef LOG_PACKET_TIMES
    //  tPoolEntry* pPoolEntry=new tPoolEntry;
    //  MIXPACKET* pMixPacket=&pPoolEntry->mixpacket;
    //#else
    tQueueEntry* pQueueEntry=new tQueueEntry;
    MIXPACKET* pMixPacket=&pQueueEntry->packet;
    //#endif  
    m_nUser=0;
    SINT32 ret;
    //osocketgroupMixOut.add(*m_pMuxOut);
  
    UINT8* tmpBuff=new UINT8[sizeof(tQueueEntry)];
    CAMsg::printMsg(LOG_DEBUG,"Starting Message Loop... \n");
    bool bAktiv;
    UINT8 rsaBuff[RSA_SIZE];
#ifdef LOG_TRAFFIC_PER_USER
    UINT64 current_time;
    UINT32 diff_time;
    CAMsg::printMsg(LOG_DEBUG,"Channel log formats:\n");
    CAMsg::printMsg(LOG_DEBUG,"1. Close received from user (times in micros) - 1:Channel-ID,Connection-ID,PacketsIn (only data and open),PacketsOut (only data),ChannelDuration (open packet received --> close packet put into send queue to next mix)\n");
    CAMsg::printMsg(LOG_DEBUG,"2. Channel close from Mix(times in micros)- 2.:Channel-ID,Connection-ID,PacketsIn (only data and open), PacketsOut (only data),ChannelDuration (open packet received)--> close packet put into send queue to next user\n");
#endif
//    CAThread threadReadFromUsers;
//    threadReadFromUsers.setMainLoop(loopReadFromUsers);
//    threadReadFromUsers.start(this);

    while(!m_bRestart)                                                            /* the main mix loop as long as there are things that are not handled by threads. */
      {
        bAktiv=false;
//LOOP_START:

//First Step
//Checking for new connections    
// Now in a separat Thread.... 

// Second Step 
// Checking for data from users
// Now in a separate Thread (see loopReadFromUsers())
//Only proccess user data, if queue to next mix is not to long!!
#define MAX_NEXT_MIX_QUEUE_SIZE 10000000 //10 MByte
        if(m_pQueueSendToMix->getSize()<MAX_NEXT_MIX_QUEUE_SIZE)
          {
            countRead=m_psocketgroupUsersRead->select(/*false,*/0);        // how many JAP<->mix connections have received data from their coresponding JAP
            if(countRead>0)
              bAktiv=true;
#ifdef HAVE_EPOLL
            //if we have epool we do not need to search the whole list
            //of connected JAPs to find the ones who have sent data
            //as epool will return ONLY these connections.
            fmHashTableEntry* pHashEntry=(fmHashTableEntry*)m_psocketgroupUsersRead->getFirstSignaledSocketData();
            while(pHashEntry!=NULL)
              {
                CAMuxSocket* pMuxSocket=pHashEntry->pMuxSocket;
#else
            //if we do not have epoll we have to go to the whole
            //list of open connections to find the ones which
            //actually have sent some data
            fmHashTableEntry* pHashEntry=m_pChannelList->getFirst();
            while(pHashEntry!=NULL&&countRead>0)                      // iterate through all connections as long as there is at least one active left
              {
                CAMuxSocket* pMuxSocket=pHashEntry->pMuxSocket;
                if(m_psocketgroupUsersRead->isSignaled(*pMuxSocket))  // if this one seems to have data
                  {
                    countRead--;
#endif
                    ret=pMuxSocket->receive(pMixPacket,0);
                    #if defined LOG_PACKET_TIMES||defined(LOG_CHANNEL)
                      getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start);
                      set64(pQueueEntry->timestamp_proccessing_start_OP,pQueueEntry->timestamp_proccessing_start);
                    #endif  
                    if(ret==SOCKET_ERROR/*||pHashEntry->accessUntil<time()*/) 
                      {  
                                                      // remove dead connections
                        #ifdef LOG_TRAFFIC_PER_USER
                          getcurrentTimeMillis(current_time);
                          diff_time=diff64(current_time,pHashEntry->timeCreated);
                          m_pIPList->removeIP(pHashEntry->peerIP,diff_time,pHashEntry->trafficIn,pHashEntry->trafficOut);
                        #else
                          m_pIPList->removeIP(pHashEntry->peerIP);
                        #endif
                        m_psocketgroupUsersRead->remove(pMuxSocket->getSocket());
                        m_psocketgroupUsersWrite->remove(pMuxSocket->getSocket());
                        ASSERT(pHashEntry->pQueueSend!=NULL,"Send queue is NULL");
                        delete pHashEntry->pQueueSend;
                        pHashEntry->pQueueSend = NULL;
                        delete pHashEntry->pSymCipher;
                        pHashEntry->pSymCipher = NULL;
                        #ifdef COUNTRY_STATS
                          decUsers(pHashEntry);
                        #else
                          decUsers();
                        #endif
                        /* remove the client part (MuxSocket) but keep the
                         * outgoing channels until a CHANNEL-CLOSE from the
                         * last mix is received
                         */
                        m_pChannelList->removeClientPart(pMuxSocket);
                        delete pMuxSocket;
                        pMuxSocket = NULL;
                      }
                    else if(ret==MIXPACKET_SIZE)                       // we've read enough data for a whole mix packet. nice!
                      {
                        #ifdef LOG_TRAFFIC_PER_USER
                          pHashEntry->trafficIn++;
                        #endif
                        #ifdef COUNTRY_STATS
                          m_PacketsPerCountryIN[pHashEntry->countryID]++;
                        #endif  
                        //New control channel code...!
						SINT32 ret = 0;
						if(pMixPacket->channel>0&&pMixPacket->channel<256)
						{
							if (pHashEntry->pControlChannelDispatcher->proccessMixPacket(pMixPacket))
							{
								goto NEXT_USER;
							}
							else
							{
								CAMsg::printMsg(LOG_DEBUG, "Packet is invalid and could not be processed!\n");
								ret = 3;
							}
						}
#ifdef PAYMENT
												// payment code added by Bastian Voigt
						if (ret == 0)
						{
							ret = CAAccountingInstance::handleJapPacket(pHashEntry);  
						}
						if (ret == 2)
						{
							goto NEXT_USER;
						}		
#endif													                        
                        if(ret == 3) 
                          {
                            // this jap is evil! terminate connection and add IP to blacklist
                            CAMsg::printMsg(LOG_DEBUG, "CAFirstMixB: Detected evil Jap.. closing connection! Removing IP..\n", ret);
                            fmChannelListEntry* pEntry;
                            pEntry=m_pChannelList->getFirstChannelForSocket(pMuxSocket);
                            while(pEntry!=NULL)
                              {
                                getRandom(pMixPacket->data,DATA_SIZE);
                                pMixPacket->flags=CHANNEL_CLOSE;
                                pMixPacket->channel=pEntry->channelOut;
                                #ifdef LOG_PACKET_TIMES
                                  setZero64(pQueueEntry->timestamp_proccessing_start);
                                #endif
                                m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));
                                delete pEntry->pCipher;
                                pEntry->pCipher = NULL;
                                pEntry=m_pChannelList->getNextChannel(pEntry);
                              }
                            m_pIPList->removeIP(pHashEntry->peerIP);
                            m_psocketgroupUsersRead->remove(pMuxSocket->getSocket());
                            m_psocketgroupUsersWrite->remove(pMuxSocket->getSocket());
                            delete pHashEntry->pQueueSend;
                            pHashEntry->pQueueSend = NULL;
                            delete pHashEntry->pSymCipher;
                            pHashEntry->pSymCipher = NULL;
                            m_pChannelList->remove(pMuxSocket);
                            delete pMuxSocket;
                            pMuxSocket = NULL;
                            decUsers();
                            goto NEXT_USER;
                          }

                        if(pMixPacket->flags==CHANNEL_DUMMY)          // just a dummy to keep the connection alife in e.g. NAT gateways 
                          { 
                            getRandom(pMixPacket->data,DATA_SIZE);
                            #ifdef LOG_PACKET_TIMES
                              setZero64(pQueueEntry->timestamp_proccessing_start);
                            #endif
                            pHashEntry->pQueueSend->add(pMixPacket,sizeof(tQueueEntry));
                            #ifdef HAVE_EPOLL
                              m_psocketgroupUsersWrite->add(*pMuxSocket,pHashEntry); 
                            #else
                              m_psocketgroupUsersWrite->add(*pMuxSocket); 
                            #endif
                          }
                        else                                         // finally! a normal mix packet
                          {
                            CASymCipher* pCipher=NULL;
                            fmChannelListEntry* pEntry;
                            pEntry=m_pChannelList->get(pMuxSocket,pMixPacket->channel);
                            if(pEntry!=NULL&&pMixPacket->flags==CHANNEL_DATA)
                              {
                                pMixPacket->channel=pEntry->channelOut;
                                pCipher=pEntry->pCipher;
                                pCipher->crypt1(pMixPacket->data,pMixPacket->data,DATA_SIZE);
                                                     // queue the packet for sending to the next mix.
                                #ifdef LOG_PACKET_TIMES
                                  getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
                                #endif
                                m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));
                                incMixedPackets();
                                #ifdef LOG_CHANNEL
                                  pEntry->packetsInFromUser++;
                                #endif
                              }
                            else if(pEntry==NULL&&pMixPacket->flags==CHANNEL_OPEN)  // open a new mix channel
                              { // stefan: muesste das nicht vor die behandlung von CHANNEL_DATA? oder gilt OPEN => !DATA ? 
                                //es gilt: open -> data
                                pHashEntry->pSymCipher->crypt1(pMixPacket->data,rsaBuff,KEY_SIZE);
                                pCipher= new CASymCipher();
                                pCipher->setKey(rsaBuff);
                                for(int i=0;i<16;i++)
                                  rsaBuff[i]=0xFF;
                                pCipher->setIV2(rsaBuff);
                                pCipher->crypt1(pMixPacket->data+KEY_SIZE,pMixPacket->data,DATA_SIZE-KEY_SIZE);
                                getRandom(pMixPacket->data+DATA_SIZE-KEY_SIZE,KEY_SIZE);
                                #ifdef LOG_CHANNEL
                                  HCHANNEL tmpC=pMixPacket->channel;
                                #endif
                                if(m_pChannelList->addChannel(pMuxSocket,pMixPacket->channel,pCipher,&pMixPacket->channel)!=E_SUCCESS)
                                  { //todo move up ?
                                    delete pCipher;
                                    pCipher = NULL;
                                  }
                                else
                                  {
                                    #ifdef LOG_PACKET_TIMES
                                      getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
                                    #endif
                                    #ifdef LOG_CHANNEL
                                      fmChannelListEntry* pTmpEntry=m_pChannelList->get(pMuxSocket,tmpC);
                                      pTmpEntry->packetsInFromUser++;
                                      set64(pTmpEntry->timeCreated,pQueueEntry->timestamp_proccessing_start);
                                    #endif
                                    m_pQueueSendToMix->add(pMixPacket,sizeof(tQueueEntry));
                                    incMixedPackets();
                                    #ifdef _DEBUG
//                                      CAMsg::printMsg(LOG_DEBUG,"Added out channel: %u\n",pMixPacket->channel);
                                    #endif
                                  }
                              }
                          }
                      }
                #ifdef HAVE_EPOLL
NEXT_USER:
                  pHashEntry=(fmHashTableEntry*)m_psocketgroupUsersRead->getNextSignaledSocketData();
                #else
                  }//if is signaled
NEXT_USER:
                  pHashEntry=m_pChannelList->getNext();
                #endif
              }
          }
//Third step
//Sending to next mix

// Now in a separate Thread (see loopSendToMix())

//Step 4
//Stepa 4a Receiving form Mix to Queue now in separat Thread
//Step 4b Proccesing MixPackets received from Mix
//todo check for error!!!
        countRead=m_nUser+1;
        while(countRead>0&&m_pQueueReadFromMix->getSize()>=sizeof(tQueueEntry))
          {
            bAktiv=true;
            countRead--;
            ret=sizeof(tQueueEntry);
            m_pQueueReadFromMix->get((UINT8*)pQueueEntry,(UINT32*)&ret);
            #ifdef LOG_PACKET_TIMES
              getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_start_OP);
            #endif
            if(pMixPacket->flags==CHANNEL_CLOSE) //close event
              {
                #if defined(_DEBUG) && !defined(__MIX_TEST)
//                  CAMsg::printMsg(LOG_DEBUG,"Closing Channel: %u ...\n",pMixPacket->channel);
                #endif
                fmChannelList* pEntry=m_pChannelList->get(pMixPacket->channel);
                if(pEntry!=NULL)
                  {
                    if (pEntry->pHead != NULL) {
                      pMixPacket->channel=pEntry->channelIn;
                      getRandom(pMixPacket->data,DATA_SIZE);
                      #ifdef LOG_PACKET_TIMES
                        getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
                      #endif
                      pEntry->pHead->pQueueSend->add(pMixPacket,sizeof(tQueueEntry));
                      #ifdef LOG_TRAFFIC_PER_USER
                        pEntry->pHead->trafficOut++;
                      #endif
                      #ifdef COUNTRY_STATS
                        m_PacketsPerCountryOUT[pEntry->pHead->countryID]++;
                      #endif  
                      #ifdef LOG_CHANNEL  
                        //pEntry->packetsOutToUser++;
                        getcurrentTimeMicros(current_time);
                        diff_time=diff64(current_time,pEntry->timeCreated);
                        CAMsg::printMsg(LOG_DEBUG,"2:%u,%Lu,%u,%u,%u\n",
                                                pEntry->channelIn,pEntry->pHead->id,pEntry->packetsInFromUser,pEntry->packetsOutToUser,
                                                diff_time);
                      #endif
                    
                      #ifdef HAVE_EPOLL
                        m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket,pEntry->pHead); 
                      #else
                        m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket); 
                      #endif                    
                      delete pEntry->pCipher;
                      pEntry->pCipher = NULL;
                      m_pChannelList->removeChannel(pEntry->pHead->pMuxSocket,pEntry->channelIn);
                    }
                    else {
                      /* the client has already closed the connection but we
                       * have waited for the CHANNEL-CLOSE packet from the
                       * last mix -> now we have it
                       */
                      #ifdef LOG_PACKET_TIMES
                        getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
                      #endif
                      delete pEntry->pCipher;
                      pEntry->pCipher = NULL;
                      m_pChannelList->removeVacantOutChannel(pEntry);
                      pEntry = NULL;
                    }
                }
              }
            else
              {//flag !=close
                #if defined(_DEBUG) && !defined(__MIX_TEST)
//                  CAMsg::printMsg(LOG_DEBUG,"Sending Data to Browser!\n");
                #endif
                fmChannelList* pEntry=m_pChannelList->get(pMixPacket->channel);
                if(pEntry!=NULL) {
                  if (pEntry->pHead != NULL) {
                    #ifdef LOG_CRIME
                      if((pMixPacket->flags&CHANNEL_SIG_CRIME)==CHANNEL_SIG_CRIME)
                        {
                          UINT32 id=(pMixPacket->flags>>8)&0x000000FF;
                          int log=LOG_ENCRYPTED;
                          if(!pglobalOptions->isEncryptedLogEnabled())
                            log=LOG_CRIT;
                          CAMsg::printMsg(log,"Detecting crime activity - ID: %u -- In-IP is: %u.%u.%u.%u \n",id,pEntry->pHead->peerIP[0],pEntry->pHead->peerIP[1],pEntry->pHead->peerIP[2],pEntry->pHead->peerIP[3]);
                          continue;
                        }
                    #endif
                    pMixPacket->channel=pEntry->channelIn;
                    pEntry->pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
                    
                    #ifdef LOG_PACKET_TIMES
                      getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
                    #endif
                    pEntry->pHead->pQueueSend->add(pMixPacket,sizeof(tQueueEntry));
                    #ifdef LOG_TRAFFIC_PER_USER
                      pEntry->pHead->trafficOut++;
                    #endif
                    #ifdef COUNTRY_STATS
                      m_PacketsPerCountryOUT[pEntry->pHead->countryID]++;
                    #endif  
                    #ifdef LOG_CGANNEL  
                      pEntry->packetsOutToUser++;
                    #endif
                    #ifdef HAVE_EPOLL
                      m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket,pEntry->pHead); 
                    #else
                      m_psocketgroupUsersWrite->add(*pEntry->pHead->pMuxSocket); 
                    #endif    
                    incMixedPackets();
                  }
                  else {
                    /* connection to client is already closed -> we wait for
                     * CLOSE-CHANNEL from last mix (but this is no CLOSE-
                     * CHANNEL packet -> do only some compatibility things and
                     * ignore the packet)
                     */
                    #ifdef LOG_CRIME
                      /* we don't have the user-information any more (but also
                       * the user cannot receive any data); nevertheless write
                       * a log-message to show what happened
                       */
                      if((pMixPacket->flags&CHANNEL_SIG_CRIME)==CHANNEL_SIG_CRIME) {
                        UINT32 id = (pMixPacket->flags>>8)&0x000000FF;
                        int log = LOG_ENCRYPTED;
                        if (!pglobalOptions->isEncryptedLogEnabled()) {
                          log=LOG_CRIT;
                        }
                        CAMsg::printMsg(log,"Detecting crime activity - ID: %u -- In-IP is: not available (user has already closed connection)\n",id);
                        continue;
                      }
                    #endif
                    #ifdef LOG_PACKET_TIMES
                      getcurrentTimeMicros(pQueueEntry->timestamp_proccessing_end_OP);
                    #endif
                  }
                }
                else
                  {                  
                    #ifdef _DEBUG
                      if(pMixPacket->flags!=CHANNEL_DUMMY)
                        {
/*                          CAMsg::printMsg(LOG_DEBUG,"Error Sending Data to Browser -- "
                              "Channel-Id %u not valid!\n",pMixPacket->channel
                            );*/
                          #ifdef LOG_CHANNEL
                            CAMsg::printMsg(LOG_INFO,"Packet late arrive for channel: %u\n",pMixPacket->channel);
                          #endif
                        }
                    #endif
                  }
              }
          }

//Step 5 
//Writing to users...
        countRead=m_psocketgroupUsersWrite->select(/*true,*/0);
#ifdef HAVE_EPOLL    
        fmHashTableEntry* pfmHashEntry=(fmHashTableEntry*)m_psocketgroupUsersWrite->getFirstSignaledSocketData();
        while(pfmHashEntry!=NULL)
          {
#else
        fmHashTableEntry* pfmHashEntry=m_pChannelList->getFirst();
        while(countRead>0&&pfmHashEntry!=NULL)
          {
            if(m_psocketgroupUsersWrite->isSignaled(*pfmHashEntry->pMuxSocket))
              {
                countRead--;
#endif
#ifdef DELAY_USERS
                if(pfmHashEntry->delayBucket>0)
                {
#endif
                if(pfmHashEntry->pQueueSend->getSize()>0)
                {
                bAktiv=true;
                UINT32 len=sizeof(tQueueEntry);
                if(pfmHashEntry->uAlreadySendPacketSize==-1)
                  {
                    pfmHashEntry->pQueueSend->get((UINT8*)&pfmHashEntry->oQueueEntry,&len); 
                    #ifdef PAYMENT
                      //do not count control channel packets!
                      if(pfmHashEntry->oQueueEntry.packet.channel>0&&pfmHashEntry->oQueueEntry.packet.channel<256)
                        pfmHashEntry->bCountPacket=false;
                      else
                        pfmHashEntry->bCountPacket=true;
                    #endif
                    pfmHashEntry->pMuxSocket->prepareForSend(&(pfmHashEntry->oQueueEntry.packet));
                    pfmHashEntry->uAlreadySendPacketSize=0;
                  }
                len=MIXPACKET_SIZE-pfmHashEntry->uAlreadySendPacketSize;
                ret=pfmHashEntry->pMuxSocket->getCASocket()->send(((UINT8*)&(pfmHashEntry->oQueueEntry))+pfmHashEntry->uAlreadySendPacketSize,len);
                if(ret>0)
                  {
                    pfmHashEntry->uAlreadySendPacketSize+=ret;
                    if(pfmHashEntry->uAlreadySendPacketSize==MIXPACKET_SIZE)
                      {
                        #ifdef PAYMENT
                          if(pfmHashEntry->bCountPacket)
                            {
                              // count packet for payment
								if (CAAccountingInstance::handleJapPacket(pfmHashEntry) == 2)
								{
									goto NEXT_USER_WRITING;
								}
                            }
                        #endif
                        #ifdef DELAY_USERS
                          pfmHashEntry->delayBucket--;
                        #endif
                        pfmHashEntry->uAlreadySendPacketSize=-1;
                        #ifdef LOG_PACKET_TIMES
                          if(!isZero64(pfmHashEntry->oQueueEntry.timestamp_proccessing_start))
                            {
                              getcurrentTimeMicros(pfmHashEntry->oQueueEntry.timestamp_proccessing_end);
                              m_pLogPacketStats->addToTimeingStats(pfmHashEntry->oQueueEntry,CHANNEL_DATA,false);
                            }
                        #endif
                      }
                   }
                }
#ifdef DELAY_USERS
                }
#endif
                  //todo error handling
#ifdef HAVE_EPOLL
NEXT_USER_WRITING:
            pfmHashEntry=(fmHashTableEntry*)m_psocketgroupUsersWrite->getNextSignaledSocketData();
#else
              }//if is socket signaled
NEXT_USER_WRITING:              
            pfmHashEntry=m_pChannelList->getNext();
#endif
          }
        if(!bAktiv)
          msSleep(100);
      }
//ERR:
//@todo move cleanup to clean() !
    CAMsg::printMsg(LOG_CRIT,"Seams that we are restarting now!!\n");
    m_bRestart=true;
    m_pMuxOut->close();
    for(UINT32 i=0;i<m_nSocketsIn;i++)
      m_arrSocketsIn[i].close();
    //writng some bytes to the queue...
    UINT8 b[sizeof(tQueueEntry)+1];
    m_pQueueSendToMix->add(b,sizeof(tQueueEntry)+1);
//#if !defined(_DEBUG) && !defined(NO_LOOPACCEPTUSER)
    CAMsg::printMsg(LOG_CRIT,"Wait for LoopAcceptUsers!\n");
    m_pthreadAcceptUsers->join();
//#endif
    CAMsg::printMsg(LOG_CRIT,"Wait for LoopSendToMix!\n");
    m_pthreadSendToMix->join(); //will not join if queue is empty (and so wating)!!!
    CAMsg::printMsg(LOG_CRIT,"Wait for LoopReadFromMix!\n");
    m_pthreadReadFromMix->join();
    #ifdef LOG_PACKET_TIMES
      CAMsg::printMsg(LOG_CRIT,"Wait for LoopLogPacketStats to terminate!\n");
      m_pLogPacketStats->stop();
    #endif  
    //waits until all login threads terminates....
    // we have to be sure that the Accept thread was alread stoped!
    m_pthreadsLogin->destroy(true);
    CAMsg::printMsg(LOG_CRIT,"Before deleting CAFirstMixChannelList()!\n");
    CAMsg::printMsg  (LOG_CRIT,"Memeory usage before: %u\n",getMemoryUsage());  
    fmHashTableEntry* pHashEntry=m_pChannelList->getFirst();
    while(pHashEntry!=NULL)
      {
        CAMuxSocket * pMuxSocket=pHashEntry->pMuxSocket;
        delete pHashEntry->pQueueSend;
        pHashEntry->pQueueSend = NULL;
        delete pHashEntry->pSymCipher;
        pHashEntry->pSymCipher = NULL; 

        fmChannelListEntry* pEntry=m_pChannelList->getFirstChannelForSocket(pHashEntry->pMuxSocket);
        while(pEntry!=NULL)
          {
            delete pEntry->pCipher;
            pEntry->pCipher = NULL;
  
            pEntry=m_pChannelList->getNextChannel(pEntry);
          }
        m_pChannelList->remove(pHashEntry->pMuxSocket);
        pMuxSocket->close();
        delete pMuxSocket;
        pMuxSocket = NULL;
        pHashEntry=m_pChannelList->getNext();
      }
    /* clean all vacant out-channels (the connection from the client was
     * closed but we've waited for the CHANNEL-CLOSE from the last mix)
     */
    m_pChannelList->cleanVacantOutChannels();
    CAMsg::printMsg  (LOG_CRIT,"Memory usage after: %u\n",getMemoryUsage());  
    delete pQueueEntry;
    pQueueEntry = NULL;
    delete []tmpBuff;
    tmpBuff = NULL;
    CAMsg::printMsg(LOG_CRIT,"Main Loop exited!!\n");
#endif // NEW_MIX_TYPE
    return E_UNKNOWN;
  }
#endif //ONLY_LOCAL_PROXY