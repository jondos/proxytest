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
#include "CAMuxSocket.hpp"
#include "CAQueue.hpp"
#include "CASymCipher.hpp"
#include "CAMutex.hpp"
#include "CAMsg.hpp"
#include "CAControlChannelDispatcher.hpp"
#ifdef PAYMENT
/**
 * Structure that holds all per-user payment information
 * Included in CAFirstMixChannelList (struct fmHashTableEntry)
 */
struct t_accountinginfo
{
	UINT8 * pReceiveBuffer; //buffer for storing incoming msgs
	UINT32  msgTotalSize;   //total size of the incoming msg
	UINT32  msgCurrentSize; //number of bytes received

	// for encrypting/decryptinh  payment packets
	CASymCipher * pCipherIn; //decrypt JAP->AI packets
	CASymCipher * pCipherOut; //encrypt AI->JAP packets

	UINT8 * pLastXmlCostConfirmation;

	UINT64 accountNumber;
	UINT64 maxBalance;

	UINT64 balanceLeft;
//	UINT64 balance
	
	UINT32 authState;				// state of the authentication process
	// this can be one of AUTH_NEW, AUTH_BAD, AUTH_OK
};
typedef struct t_accountinginfo aiAccountingInfo;
typedef aiAccountingInfo * LP_aiAccountingInfo;
#endif

struct t_fmhashtableentry
	{
		public:
			CAMuxSocket*	pMuxSocket;
			CAQueue*			pQueueSend;
			CAControlChannelDispatcher* pControlChannelDispatcher;
			UINT32        uAlreadySendPacketSize;
			tQueueEntry		oQueueEntry;
			UINT32				cSuspend;
#ifdef LOG_CHANNEL
			UINT32				trafficIn;
			UINT32				trafficOut;
			UINT64				timeCreated;
			UINT64				id;
#endif
#ifdef PAYMENT
	aiAccountingInfo * pAccountingInfo;
#endif
#ifdef FIRST_MIX_SYMMETRIC
			CASymCipher*  pSymCipher;
#endif			
			UINT8					peerIP[4]; //needed for flooding control
		private:
			UINT32				cNumberOfChannels;
			struct t_firstmixchannellist* pChannelList;

			struct
				{
					struct t_fmhashtableentry* prev;
					struct t_fmhashtableentry* next;
				} list_HashEntries;

		friend class CAFirstMixChannelList;
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
			UINT32				packetsOutToUser;
			UINT64				timeCreated;	
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
		
			SINT32 CAFirstMixChannelList::add(CAMuxSocket* pMuxSocket,UINT8 peerIP[4],CAQueue* pQueueSend);
			SINT32 addChannel(CAMuxSocket* pMuxSocket,HCHANNEL channelIn,CASymCipher* pCipher,HCHANNEL* channelOut);
			
			fmChannelListEntry* get(CAMuxSocket* pMuxSocket,HCHANNEL channelIn);


			SINT32 remove(CAMuxSocket* pMuxSocket);
			SINT32 removeChannel(CAMuxSocket* pMuxSocket,HCHANNEL channelIn);
					
			fmHashTableEntry* getFirst();
			fmHashTableEntry* getNext();
			fmHashTableEntry* get(CAMuxSocket* pMuxSocket);

		
			fmChannelListEntry* getFirstChannelForSocket(CAMuxSocket* pMuxSocket);
			fmChannelListEntry* getNextChannel(fmChannelListEntry* pEntry);
		
			static SINT32 test();
		private:
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
			///The Hash-Table of all connections.
			LP_fmHashTableEntry* m_HashTable;
			///The Hash-Table of all out-channels.
			LP_fmChannelListEntry* m_HashTableOutChannels;
			
			///Pointer to the head of a list of all connections.
			fmHashTableEntry* m_listHashTableHead;
			///Next Element in the enumeration of all connections.
			fmHashTableEntry* m_listHashTableNext;
			///This mutex is used in all functions and makes them thread safe.
			CAMutex m_Mutex;
//#ifdef PAYMENT
//			CAAccountingInstance *m_pAccountingInstance;
//#endif

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
				}
#endif
	};
#endif
