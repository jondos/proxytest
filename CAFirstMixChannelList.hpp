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
#ifndef __CAFRISTMIXCHANNELLIST__
#define __CAFRISTMIXCHANNELLIST__
#include "doxygen.h"
#ifndef ONLY_LOCAL_PROXY
#include "CAMuxSocket.hpp"
#include "CAQueue.hpp"
#include "CASymCipher.hpp"
#include "CAMutex.hpp"
#include "CAMsg.hpp"
#include "CAControlChannelDispatcher.hpp"
#ifdef DELAY_USERS
	#include "CAThread.hpp"
#endif	
#ifdef PAYMENT

class CAAccountingControlChannel;

#endif

struct t_fmhashtableentry
	{
		public:
			CAMuxSocket*	pMuxSocket;
			CAQueue*		pQueueSend;
			CAControlChannelDispatcher* pControlChannelDispatcher;
			SINT32			uAlreadySendPacketSize;
			tQueueEntry		oQueueEntry;
			UINT32				cSuspend;
#ifdef LOG_TRAFFIC_PER_USER
			UINT32				trafficIn;
			UINT32				trafficOut;
			UINT64				timeCreated;
#endif			
			UINT64				id;

			CASymCipher*  pSymCipher;
			UINT8					peerIP[4]; //needed for flooding control
#ifdef COUNTRY_STATS
			UINT32 countryID; /** CountryID of this IP Address*/
#endif				

#ifdef LOG_DIALOG
			UINT8*				strDialog;
#endif

#ifdef DELAY_USERS
			UINT32				delayBucket;
			UINT32				delayBucketID;
#endif						
			// if false, the entry should be deleted the next time it is read from the queue
			bool bRecoverTimeout;
	
		private:
			UINT32				cNumberOfChannels;
			struct t_firstmixchannellist* pChannelList;

			struct
				{
					struct t_fmhashtableentry* prev;
					struct t_fmhashtableentry* next;
				} list_HashEntries;
				
			// the timeout list
			// At the moement only enabled for payment Mixes (to be changed iff new mix protcol supports this for all clients)
#ifdef PAYMENT
			struct
			{
				struct t_fmhashtableentry* prev;
				struct t_fmhashtableentry* next;
				SINT32 timoutSecs;
			} list_TimeoutHashEntries;
#endif
		friend class CAFirstMixChannelList;
#ifdef PAYMENT
		public:
			bool bCountPacket;
 		private:
			tAiAccountingInfo* pAccountingInfo;
		friend class CAAccountingInstance;
		friend class CAAccountingControlChannel;
#endif
	};

typedef struct t_fmhashtableentry fmHashTableEntry;
typedef fmHashTableEntry* LP_fmHashTableEntry;



struct t_firstmixchannellist
	{
		public:
			fmHashTableEntry* pHead;

			HCHANNEL channelIn;
			HCHANNEL channelOut;
		
			CASymCipher* pCipher;
			bool bIsSuspended;

#ifdef LOG_CHANNEL
			UINT32				packetsInFromUser;
			UINT64				timeCreated;
			UINT32				packetsOutToUser;
#endif
#ifdef SSL_HACK
			UINT32				downStreamBytes; /* a hack to solve the SSL problem */
#endif
		private:
			struct
				{
					struct t_firstmixchannellist* prev;
					struct t_firstmixchannellist* next;
				} list_OutChannelHashTable;

			struct
				{
					struct t_firstmixchannellist* prev;
					struct t_firstmixchannellist* next;
				} list_InChannelPerSocket;
		
		friend class CAFirstMixChannelList;
	};

typedef struct t_firstmixchannellist fmChannelList; 
typedef struct t_firstmixchannellist fmChannelListEntry; 
typedef fmChannelListEntry* LP_fmChannelListEntry;

/**  \page pageFirstMixChannelList Data structures for storing the Mix channel table of the first Mix
	*  \section docFirstMixChannelList Data structures for storing the Mix channel table of the first Mix
	*	 The following data structures are used to store and access all information necassary to process a MixPacket.
	*
	* \image html FirstMixChannelTable.png "Table 1: Necessary data stored in the Channel Table"
	*
	*  Table 1 gives an idea about the information which must be stored.
	*  In different situations the information stored in the table has to be access
	*  in different way for instance using different search criterias. The following situations are common:
	* - if the Mix polls for new data from the users, he has to serial traverse all
	*  TCP/IP connections from the JAPs. 
	* - if the Mix receives a MixPacket from a JAP, he has to find the belonging Cipher and outgoing Channel-ID 
	* using the incoming Channel-ID as search criteria
	* - if the Mix receives a MixPacket from the following Mix, he has to find the belonging Chipher, incoming Channel-Id and
	* Send Queue using the outgoing Channel-ID as search criteria.
	*
	* In order to satisfy all this different requirements, the data is organized in different 
	* search structures like Hashtables and linked Lists (see Fig. 1).
  *
	* \image html DataStructuresOfFirstMix.png "Figure 1: Data structures used for accessing the Channel Table Data"
	* 
	* There exists a Hashtable ((\link CAFirstMixChannelList#m_HashTable m_HashTable \endlink) of all TCP/IP connections from JAPs, 
	* where each TCP/IP connections represents exactly one JAP.
	* The SOCKET identifier of such a TCP/IP connection is used as Hashkey. The elements of this Hashtable are of
	* the type fmHashTableEntry. In order to travers the TCP/IP connections from all JAPs, each Hashtableentry
	* stores a pointer (next) to the next (non empty) Hashtableentry, thus forming a linked list 
	* (actually it is a double linked list).
	* Each Hashtableentry stores a reference to the belonging TCP/IP socket object (pMuxSocket), to a Queue
	* used for storing the data which has to be send to the JAP (pSendQueue) and 
	* a pointer (pChannelList) to a List of Channels belonging to the JAP.
	* Each entry of the Channellist stores a reference (pCipher) to the Cipher object used for en-/decryption of 
	* the MixPackets belonging to the MixChannel, the incoming (channelIn) and outgoing (channelOut) Channel-ID.
	* Also each Channellistentry contains a reference (pHead) to the belonging Hashtableentry. 
	* In order to traverse all Channels which belong to a JAP, the Channels are organised in a double linked list.
	* The elements list_InChannelPersocket.next and list_InChannelPersocket.prev could be used to traverse the Channels. 
	* This way the Mix could find the outgoing Channel-ID (and Cipher) if he receives a MixPacket from a JAP. (Note that the
	* Mix already knows the HashtableEntry, because he has read the MixPacket from the belonging Socket).
	* 
	* In order to find a Channel using the outgoing Channel-ID as search criteria there exists a Hashtable with
	* overrun lists (\link CAFirstMixChannelList#m_HashTableOutChannels m_HashTableOutChannels \endlink). The last two bytes from the Channel-ID are used as Hashkey.
	* The elements of this Hashtable are pointers to Channellistentries. The overrun lists are 
	* stored in the list_OutChannelHashTable element of each Channellistentry.
	*/

/** Data structure that stores all information about the currently open Mix channels.
	* See \ref pageFirstMixChannelList "[FirstMixChannelList]" for more information.
	*/
class CAFirstMixChannelList
	{
		public:
			CAFirstMixChannelList();
			~CAFirstMixChannelList();

#ifndef LOG_DIALOG
			fmHashTableEntry* add(CAMuxSocket* pMuxSocket,const UINT8 peerIP[4],CAQueue* pQueueSend);
#else
			fmHashTableEntry* add(CAMuxSocket* pMuxSocket,const UINT8 peerIP[4],CAQueue* pQueueSend,UINT8* strDialog);
#endif
			SINT32 addChannel(CAMuxSocket* pMuxSocket,HCHANNEL channelIn,CASymCipher* pCipher,HCHANNEL* channelOut);
			
			fmChannelListEntry* get(CAMuxSocket* pMuxSocket,HCHANNEL channelIn);			

#ifdef PAYMENT
			/** 
			 * @return pops the next expired entry from the queue or returns NULL
			 * if there are no exired entries
			 */
			fmHashTableEntry* popTimeoutEntry();
			
			/**
			 * @param if set to true, forces to return the next timeout enty or NULL if none
			 * are left in the queue
			 * @return if set to true, forces to return the next timeout enty or NULL if none
			 * are left in the queue
			 */
			fmHashTableEntry* popTimeoutEntry(bool a_bForce);
			
			/**
			 * adds the entry to the timeout queue with mutex
			 */
			SINT32 pushTimeoutEntry(fmHashTableEntry* pHashTableEntry);
#endif

			SINT32 remove(CAMuxSocket* pMuxSocket);
			SINT32 removeChannel(CAMuxSocket* pMuxSocket,HCHANNEL channelIn);
					
			fmHashTableEntry* getFirst();
			fmHashTableEntry* getNext();
			fmHashTableEntry* get(CAMuxSocket* pMuxSocket);

		
			fmChannelListEntry* getFirstChannelForSocket(CAMuxSocket* pMuxSocket);
			fmChannelListEntry* getNextChannel(fmChannelListEntry* pEntry);
		
			static SINT32 test();

      #ifdef NEW_MIX_TYPE
        /* additional methods for TypeB first mixes */
        SINT32 removeClientPart(CAMuxSocket* pMuxSocket);
        void removeVacantOutChannel(fmChannelListEntry* pEntry);
        void cleanVacantOutChannels();
      #endif

		private:
#ifdef PAYMENT
			SINT32 removeFromTimeoutList(fmHashTableEntry* pHashTableEntry);
			/**
			 * adds the entry to the timeout queue
			 */
			SINT32 pushTimeoutEntry_internal(fmHashTableEntry* pHashTableEntry);
			
			fmHashTableEntry* popTimeoutEntry_internal(bool a_bForce);
			
			UINT32 countTimeoutEntries();
#endif		
			/** Gets the in-channel and all associated information for the given out-channel.
				* This method is NOT thread safe (and so only for internal use)
				* @see get() 
				*	@param channelOut the out-channel id for which the information is requested
				* @return the in-channel and all associated information
				* @retval NULL if not found in list
				*/
			fmChannelListEntry* get_intern_without_lock(HCHANNEL channelOut)
				{
					fmChannelListEntry* pEntry=m_HashTableOutChannels[channelOut&0x0000FFFF];
					while(pEntry!=NULL)
						{
							if(pEntry->channelOut==channelOut)
								{
									return pEntry;
								}
							pEntry=pEntry->list_OutChannelHashTable.next;
						}
					return NULL;
				}	

		public:
			/** Gets the in-channel and all associated information for the given out-channel.
				* @param channelOut the out-channel id for which the information is requested
				* @return the in-channel and all associated information
				* @retval NULL if not found in list
				*/
			fmChannelListEntry* get(HCHANNEL channelOut)
				{
					m_Mutex.lock();
					fmChannelListEntry* pEntry=get_intern_without_lock(channelOut);
					m_Mutex.unlock();
					return pEntry;
				}	
		private:
			static const SINT32 EXPIRATION_TIME_SECS;
		
			///The Hash-Table of all connections.
			LP_fmHashTableEntry* m_HashTable;
			///The Hash-Table of all out-channels.
			LP_fmChannelListEntry* m_HashTableOutChannels;
			
			///Pointer to the head of a list of all connections.
			fmHashTableEntry* m_listHashTableHead;
			///Next Element in the enumeration of all connections.
			fmHashTableEntry* m_listHashTableNext;
#ifdef PAYMENT			
			///Pointer to the head of the timout list of all connections.
			fmHashTableEntry* m_listTimoutHead;
			fmHashTableEntry* m_listTimoutFoot;
#endif			
			///This mutex is used in all functions and makes them thread safe.
			CAMutex m_Mutex;
//#ifdef PAYMENT
//			CAAccountingInstance *m_pAccountingInstance;
//#endif
			#ifdef DELAY_USERS
				UINT32** m_pDelayBuckets;
				CAThread* m_pThreadDelayBucketsLoop;
				CAMutex* m_pMutexDelayChannel;
				bool m_bDelayBucketsLoopRun;
				friend THREAD_RETURN fml_loopDelayBuckets(void*);
				//Parameters
				volatile UINT32	m_u32DelayChannelUnlimitTraffic;  //how many packets without any delay?
				volatile UINT32 m_u32DelayChannelBucketGrow; //how many packets to put in each bucket per time intervall
				volatile UINT32 m_u32DelayChannelBucketGrowIntervall; //duration of one time intervall in ms
																															//therefore the allowed max bandwith=BucketGrow/Intervall*1000 *PAYLOAD_SIZE[bytes/s]
				public:
					void setDelayParameters(UINT32 unlimitTraffic,UINT32 bucketGrow,UINT32 intervall);																												
			#endif
#ifdef DO_TRACE
			UINT32 m_aktAlloc;
			UINT32 m_maxAlloc;
			LP_fmChannelListEntry newChannelListEntry()
				{
					m_aktAlloc+=sizeof(fmChannelListEntry);
					if(m_maxAlloc<m_aktAlloc)
						{
							m_maxAlloc=m_aktAlloc;
							CAMsg::printMsg(LOG_DEBUG,"FirstMixChannelList current alloc: %u\n",m_aktAlloc);
						}
					return (LP_fmChannelListEntry)new fmChannelListEntry;
				}
			void deleteChannelListEntry(LP_fmChannelListEntry entry)
				{
					m_aktAlloc-=sizeof(fmChannelListEntry);
					delete entry;
					entry = NULL;
				}
#endif
	};
#endif
#endif //ONLY_LOCAL_PROXY
