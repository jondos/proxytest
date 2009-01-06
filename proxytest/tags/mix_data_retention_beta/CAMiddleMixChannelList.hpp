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
#ifndef __CAMIDDLEMIXCHANNELLIST__
#define __CAMIDDLEMIXCHANNELLIST__
#ifndef ONLY_LOCAL_PROXY
#include "CASymCipher.hpp"
#include "CAMuxSocket.hpp"
#include "CAMutex.hpp"


struct t_middlemixchannellist
	{
		HCHANNEL channelIn;
		HCHANNEL channelOut;
		CASymCipher* pCipher;
		struct dl_in
			{
				struct t_middlemixchannellist* next,*prev;
			} list_HashTableIn; 
		
		struct dl_out
			{
				struct t_middlemixchannellist* next,*prev;
			} list_HashTableOut; 

	};
typedef struct t_middlemixchannellist mmChannelList;
typedef struct t_middlemixchannellist mmChannelListEntry;
typedef struct t_middlemixchannellist mmHashTableEntry;
typedef struct t_middlemixchannellist* LP_mmHashTableEntry;

/**  \page pageMiddleMixChannelList Data structures for storing the Mix channel table of a middle Mix
	*  \section docMiddleMixChannelList Data structures for storing the Mix channel table of a middle Mix
	*	 The following data structures are used to store and access all information necassary to process a MixPacket.
	*
	* \image html MiddleMixChannelTable.png "Table 1: Necessary data stored in the Channel Table"
	*
	*  Table 1 gives an idea about the information which must be stored.
	*  In different situations the information stored in the table has to be access
	*  in different way for instance using different search criterias. The following situations are common:
	* - if the Mix receives a MixPacket from the previous Mix, he has to find out, if the incoming ChannelID already exists. 
	* - if the Mix receives a MixPacket from the next Mix, he has to find the belonging Cipher and incoming Channel-ID 
	* using the outgoing Channel-ID as search criteria
	*
	* In order to satisfy all this different requirements, the data is organized in different 
	* search structures like Hashtables and linked Lists (see Fig. 1).
  *
	* \image html DataStructuresOfMiddleMix.png "Figure 1: Data structures used for accessing the Channel Table Data"
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
	* See \ref pageMiddleMixChannelList "[MiddleMixChannelList]" for more information.
	*/
class CAMiddleMixChannelList
	{
		public:
			CAMiddleMixChannelList()
				{
					m_pHashTableIn=new LP_mmHashTableEntry[0x10000];
					memset(m_pHashTableIn,0,0x10000*sizeof(LP_mmHashTableEntry));
					m_pHashTableOut=new LP_mmHashTableEntry[0x10000];
					memset(m_pHashTableOut,0,0x10000*sizeof(LP_mmHashTableEntry));
				}
			
			~CAMiddleMixChannelList();
		
			SINT32 add(HCHANNEL channelIn,CASymCipher* pCipher,HCHANNEL* channelOut);
			
			SINT32 getInToOut(HCHANNEL channelIn, HCHANNEL* channelOut,CASymCipher** ppCipher);
			SINT32 remove(HCHANNEL channelIn);
			static SINT32 test();  
		private:
			SINT32 getOutToIn_intern_without_lock(HCHANNEL* channelIn, HCHANNEL channelOut,CASymCipher** ppCipher)
				{
					mmChannelListEntry* pEntry=m_pHashTableOut[channelOut&0x0000FFFF];
					while(pEntry!=NULL)
						{
							if(pEntry->channelOut==channelOut)
								{
									if(channelIn!=NULL)
										*channelIn=pEntry->channelIn;
									if(ppCipher!=NULL)
										{
											*ppCipher=pEntry->pCipher;
											(*ppCipher)->lock();
										}
									return E_SUCCESS;
								}
							pEntry=pEntry->list_HashTableOut.next;
						}
					return E_UNKNOWN;
				}
		public:
			SINT32 getOutToIn(HCHANNEL* channelIn, HCHANNEL channelOut,CASymCipher** ppCipher)
				{
					m_Mutex.lock();
					SINT32 ret=getOutToIn_intern_without_lock(channelIn,channelOut,ppCipher);
					m_Mutex.unlock();
					return ret;
				}

		private:
			LP_mmHashTableEntry* m_pHashTableIn;
			LP_mmHashTableEntry* m_pHashTableOut;
			CAMutex m_Mutex;
	};
#endif
#endif //ONLY_LOCAL_PROXY
