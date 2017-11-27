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

#ifndef __CAFIRSTMIXA__
#define __CAFIRSTMIXA__
#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_FIRST_MIX
#include "../CAFirstMix.hpp"
#include "../CASocketAddrINet.hpp"
#include "../CAIPAddrWithNetmask.hpp"

THREAD_RETURN fm_loopPacketProcessing(void *params);

class CAFirstMixA:public CAFirstMix
{
		public:
			virtual void shutDown();

		protected:
			SINT32 loop();
#ifndef MULTI_THREADED_PACKET_PROCESSING
			SINT32 closeConnection(fmHashTableEntry* pHashEntry);
#else
#ifdef HAVE_EPOLL
			SINT32 closeConnection(fmHashTableEntry* pHashEntry, CASocketGroupEpoll* psocketgroupUsersRead, CASocketGroupEpoll* psocketgroupUsersWrite, CAFirstMixChannelList* pChannelList);
#else
			SINT32 closeConnection(fmHashTableEntry* pHashEntry, CASocketGroup* psocketgroupUsersRead, CASocketGroup* psocketgroupUsersWrite, CAFirstMixChannelList* pChannelList);
#endif
#endif
		
		private:
#ifndef MULTI_THREADED_PACKET_PROCESSING
			bool sendToUsers();
#else
#ifdef HAVE_EPOLL
		bool sendToUsers(CASocketGroupEpoll* psocketgroupUsersWrite,CASocketGroupEpoll* psocketgroupUsersRead, CAFirstMixChannelList* pChannelList);
#else
		bool sendToUsers(CASocketGroup* psocketgroupUsersWrite,CASocketGroup* psocketgroupUsersRead, CAFirstMixChannelList* pChannelList);
#endif
			void notifyAllUserChannels(fmHashTableEntry *pfmHashEntry, UINT16 flags);
#ifdef SSL_HACK
			void finishPacket(fmHashTableEntry *pfmHashEntry);
#endif
#ifdef PAYMENT
			void checkUserConnections();
			SINT32 accountTrafficUpstream(fmHashTableEntry* pHashEntry);
			SINT32 accountTrafficDownstream(fmHashTableEntry* pfmHashEntry);
#endif
	#ifdef LOG_CRIME
			void crimeSurveillance(CAIPAddrWithNetmask* surveillanceIPs, UINT32 nrOfSureveillanceIPs,	UINT8 *peerIP, SINT32 peerPort,MIXPACKET *pMixPacket);
	#endif
			friend THREAD_RETURN fm_loopPacketProcessing(void *params);
};

#endif
#endif //ONLY_LOCAL_PROXY
