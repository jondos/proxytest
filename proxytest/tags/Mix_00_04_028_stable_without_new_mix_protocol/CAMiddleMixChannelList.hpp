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
