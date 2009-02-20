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
#include "CAMuxSocket.hpp"
#include "CASocketList.hpp"

typedef struct ReverseMuxList_t
	{
		CAMuxSocket* pMuxSocket;
		HCHANNEL inChannel;
		HCHANNEL outChannel;
		CASymCipher* pCipher;
		ReverseMuxList_t* next;
	} REVERSEMUXLIST,REVERSEMUXLISTENTRY;

typedef struct MuxList_t
	{
		CAMuxSocket* pMuxSocket;
		CASocketList* pSocketList;
		MuxList_t* next;
	} MUXLIST,MUXLISTENTRY;



class CAMuxChannelList
	{
		public:
			CAMuxChannelList();
			int add(CAMuxSocket* pMuxSocket);
			MUXLISTENTRY* get(CAMuxSocket* pMuxSocket);
			bool remove(CAMuxSocket* pMuxSocket,MUXLISTENTRY* pEntry);
			int add(MUXLISTENTRY* pEntry,HCHANNEL in,HCHANNEL out,CASymCipher* pCipher);
			bool get(MUXLISTENTRY* pEntry,HCHANNEL in,CONNECTION* out);
			REVERSEMUXLISTENTRY* get(HCHANNEL out);
			bool remove(HCHANNEL out,REVERSEMUXLISTENTRY* reverseEntry);
			MUXLISTENTRY* getFirst();
			MUXLISTENTRY* getNext();

		protected:
			MUXLIST* list;
			MUXLISTENTRY* aktEnumPos;
			REVERSEMUXLIST* reverselist;
	};
