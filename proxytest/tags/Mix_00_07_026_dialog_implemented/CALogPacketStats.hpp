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
#if !defined(__CA_LOG_PACKET_TIMES__)
#define __CA_LOG_PACKET_TIMES__

#ifndef ONLY_LOCAL_PROXY

#include "CAMutex.hpp"
#include "CAThread.hpp"

class CALogPacketStats
	{
		public:
			CALogPacketStats()
				{
					m_pthreadLog=NULL;
					m_iLogIntervallInHalfMinutes=30;
					resetTimeingStats();
				}

			~CALogPacketStats()
				{
					stop();
				}

			SINT32 setLogIntervallInMinutes(UINT32 minutes)
				{
					m_iLogIntervallInHalfMinutes=minutes<<1;
					return E_SUCCESS;
				}

			SINT32 start();
			SINT32 stop();
			SINT32 addToTimeingStats(	const tQueueEntry& oQueueEntry,
																UINT32 uType,bool bUpstream);
			SINT32 resetTimeingStats();
			SINT32 logTimeingStats();
		
		private:
			CAMutex m_csTimeingStats;
			UINT32 m_iLogIntervallInHalfMinutes;
			UINT32 m_timingMaxDataPacketUpstream,m_timingMaxDataPacketDownStream;
			UINT32 m_timingMinDataPacketUpstream,m_timingMinDataPacketDownStream;
			UINT32 m_timingCountDataPacketsUpstream,m_timingCountDataPacketsDownStream;
			UINT64 m_timingSumDataPacketUpstream,m_timingSumDataPacketDownStream;
			UINT32 m_timingMaxClosePacketUpstream;
			UINT32 m_timingMinClosePacketUpstream;
			UINT32 m_timingCountClosePacketsUpstream;
			UINT64 m_timingSumClosePacketUpstream;
			UINT32 m_timingMaxOpenPacketUpstream,m_timingMinOpenPacketUpstream;
			UINT32 m_timingCountOpenPacketsUpstream;
			UINT64 m_timingSumOpenPacketUpstream;

			//OP means "only proccessing" (without send or queueing times)
			UINT32 m_timingMaxDataPacketUpstreamOP,m_timingMaxDataPacketDownStreamOP;
			UINT32 m_timingMinDataPacketUpstreamOP,m_timingMinDataPacketDownStreamOP;
			UINT64 m_timingSumDataPacketUpstreamOP,m_timingSumDataPacketDownStreamOP;
			UINT32 m_timingMaxClosePacketUpstreamOP;
			UINT32 m_timingMinClosePacketUpstreamOP;
			UINT64 m_timingSumClosePacketUpstreamOP;
			UINT32 m_timingMaxOpenPacketUpstreamOP,m_timingMinOpenPacketUpstreamOP;
			UINT64 m_timingSumOpenPacketUpstreamOP;

			#ifdef USE_POOL
				UINT32 m_timingMaxPoolPacketUpstream,m_timingMaxPoolPacketDownStream;
				UINT32 m_timingMinPoolPacketUpstream,m_timingMinPoolPacketDownStream;
				UINT32 m_timingCountPoolPacketsUpstream;
				UINT64 m_timingSumPoolPacketUpstream;
				UINT32 m_timingCountPoolPacketsDownStream;
				UINT64 m_timingSumPoolPacketDownStream;
			#endif
			friend THREAD_RETURN	loopLogPacketStats(void*);
			volatile bool					m_bRunLog;
			CAThread*							m_pthreadLog;
	};
#endif //ONLY_LOCAL_PROXY
#endif
