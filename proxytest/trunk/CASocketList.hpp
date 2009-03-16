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
		CASymCipher* pCiphers;
		CASocket* pSocket;
		connlist* next;
		HCHANNEL outChannel;
		UINT32 currentSendMeCounter;
		UINT32 upstreamBytes;
	} CONNECTIONLIST,CONNECTION;
		
struct t_MEMBLOCK;

class CASocketList
	{
		public:
			CASocketList();
			CASocketList(bool bThreadSafe);
			~CASocketList();
			SINT32 add(CASocket* pSocket,CASymCipher* pCiphers);
			SINT32 get(HCHANNEL in,CONNECTION* out);
			
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
			SINT32 addSendMeCounter(HCHANNEL in,SINT32 value);
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
