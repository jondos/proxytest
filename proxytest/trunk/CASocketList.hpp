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
#ifndef __CASOCKETLIST__
#define __CASOCKETLIST__
#include "CAMuxSocket.hpp"
#include "CASymCipher.hpp"
#include "CAMutex.hpp"
#include "CAQueue.hpp"

typedef struct connlist
	{
		CASymCipher* pCipher;
		CAQueue* pSendQueue;
		connlist* next;
		union
			{
				CASocket* pSocket;
				HCHANNEL outChannel;
			};
		HCHANNEL id;
#ifdef LOG_CHANNEL
		UINT32	u32Upload;
		UINT32	u32Download;
		UINT64	time_created;
#endif
	} CONNECTIONLIST,CONNECTION;
		
struct t_MEMBLOCK;

class CASocketList
	{
		public:
			CASocketList();
			CASocketList(bool bThreadSafe);
			~CASocketList();
#ifdef LOG_CHANNEL
			SINT32 add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher,CAQueue* pQueue,UINT64 time=0,UINT32 initalUpLoad=0);
#else
			SINT32 add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher,CAQueue* pQueue);
#endif
			SINT32 add(HCHANNEL in,HCHANNEL out,CASymCipher* pCipher);
			bool	get(HCHANNEL in,CONNECTION* out);
			bool	get(CONNECTION* in,HCHANNEL out);
			bool	get(CONNECTION* in,CASocket* pSocket);
			
			CASocket* remove(HCHANNEL id);
			SINT32 clear();
			/** Gets the first entry of the channel-list.
			*	@return the first entry of the channel list (this is not a copy!!)
			*
			*/	 
			CONNECTION*  getFirst()
				{
					m_AktEnumPos=m_Connections;
					return m_AktEnumPos;
				}

			/** Gets the next entry of the channel-list.
			*	@return the next entry of the channel list (this is not a copy!!)
			*
			*/	 
			CONNECTION* getNext()
				{
					if(m_AktEnumPos!=NULL)
						m_AktEnumPos=m_AktEnumPos->next;
					return m_AktEnumPos;
				}

			UINT32 getSize(){return m_Size;}
			SINT32 setThreadSafe(bool b);
		protected:
			SINT32 increasePool();
			CONNECTIONLIST* m_Connections;
			CONNECTIONLIST* m_Pool;
			CONNECTIONLIST* m_AktEnumPos;
			t_MEMBLOCK* m_Memlist;
			CAMutex cs;
			bool m_bThreadSafe;
			UINT32 m_Size;
	};	
#endif
