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

#include "StdAfx.h"
#include "CALogPacketStats.hpp"
#include "CAUtil.hpp"
#include "CAMuxSocket.hpp"
#include "CAMsg.hpp"

#ifdef LOG_PACKET_TIMES
/** How to end this thread
	* Set m_bRunLog=false;
	*/

THREAD_RETURN loopLogPacketStats(void* param)
	{
		CALogPacketStats* pLog=(CALogPacketStats*)param;
		UINT32 countLog=0;
		while(pLog->m_bRunLog)
			{
				if(countLog==0)
					{
						pLog->logTimeingStats();
						pLog->resetTimeingStats();
						countLog=pLog->m_iLogIntervallInHalfMinutes;
					}
				sSleep(30);
				countLog--;
			}
		THREAD_RETURN_SUCCESS;
	}

SINT32 CALogPacketStats::start()
	{
		resetTimeingStats();
		m_pthreadLog=new CAThread();
		m_pthreadLog->setMainLoop(loopLogPacketStats);
		m_bRunLog=true;
		return m_pthreadLog->start(this);
	}

SINT32 CALogPacketStats::stop()
	{
		m_bRunLog=false;
		if(m_pthreadLog!=NULL)
			{
				m_pthreadLog->join();
				delete m_pthreadLog;
			}
		m_pthreadLog=NULL;
		return E_SUCCESS;
	}

SINT32 CALogPacketStats::addToTimeingStats(const tQueueEntry& oQueueEntry,UINT32 uType,bool bUpstream)
{
	m_csTimeingStats.lock();
	UINT32 proccessingTime=diff64(oQueueEntry.timestamp_proccessing_end,oQueueEntry.timestamp_proccessing_start);
	UINT32 proccessingTimeOP=diff64(oQueueEntry.timestamp_proccessing_end_OP,oQueueEntry.timestamp_proccessing_start_OP);
	if(bUpstream)
	{
		if(uType==CHANNEL_DATA)
			{
				m_timingCountDataPacketsUpstream++;
				add64(m_timingSumDataPacketUpstream,proccessingTime);
				if(proccessingTime>m_timingMaxDataPacketUpstream)
					m_timingMaxDataPacketUpstream=proccessingTime;
				else if(m_timingMinDataPacketUpstream>proccessingTime)
					m_timingMinDataPacketUpstream=proccessingTime;	
				add64(m_timingSumDataPacketUpstreamOP,proccessingTimeOP);
				if(proccessingTimeOP>m_timingMaxDataPacketUpstreamOP)
					m_timingMaxDataPacketUpstreamOP=proccessingTimeOP;
				else if(m_timingMinDataPacketUpstreamOP>proccessingTimeOP)
					m_timingMinDataPacketUpstreamOP=proccessingTimeOP;	
			}
		else if(uType==CHANNEL_CLOSE)
			{
				m_timingCountClosePacketsUpstream++;
				add64(m_timingSumClosePacketUpstream,proccessingTime);
				if(proccessingTime>m_timingMaxClosePacketUpstream) 
					m_timingMaxClosePacketUpstream=proccessingTime;
				else if(m_timingMinClosePacketUpstream>proccessingTime)
					m_timingMinClosePacketUpstream=proccessingTime;	
				add64(m_timingSumClosePacketUpstreamOP,proccessingTimeOP);
				if(proccessingTimeOP>m_timingMaxClosePacketUpstreamOP) 
					m_timingMaxClosePacketUpstreamOP=proccessingTimeOP;
				else if(m_timingMinClosePacketUpstreamOP>proccessingTimeOP)
					m_timingMinClosePacketUpstreamOP=proccessingTimeOP;	
			}
		else
			{//open
				m_timingCountOpenPacketsUpstream++;
				add64(m_timingSumOpenPacketUpstream,proccessingTime);
				if(proccessingTime>m_timingMaxOpenPacketUpstream)
					m_timingMaxOpenPacketUpstream=proccessingTime;
				else if(m_timingMinOpenPacketUpstream>proccessingTime)
					m_timingMinOpenPacketUpstream=proccessingTime;	
				add64(m_timingSumOpenPacketUpstreamOP,proccessingTimeOP);
				if(proccessingTimeOP>m_timingMaxOpenPacketUpstreamOP)
					m_timingMaxOpenPacketUpstreamOP=proccessingTimeOP;
				else if(m_timingMinOpenPacketUpstreamOP>proccessingTimeOP)
					m_timingMinOpenPacketUpstreamOP=proccessingTimeOP;	
			}
		#ifdef USE_POOL
			UINT32 poolTime=diff64(oQueueEntry.pool_timestamp_out,oQueueEntry.pool_timestamp_in);
			m_timingCountPoolPacketsUpstream++;
			add64(m_timingSumPoolPacketUpstream,poolTime);
			if(poolTime>m_timingMaxPoolPacketUpstream)
				m_timingMaxPoolPacketUpstream=poolTime;
			else if(m_timingMinPoolPacketUpstream>poolTime)
				m_timingMinPoolPacketUpstream=poolTime;	
		#endif
		#ifdef _DEBUG
			#ifndef USE_POOL
				CAMsg::printMsg(LOG_CRIT,"Upload Packet processing time (arrival --> send): %u 탎 -- Queue out --> Queue in %u 탎\n",
																	proccessingTime,proccessingTimeOP);
			#else
				CAMsg::printMsg(LOG_CRIT,"Upload Packet processing time (arrival --> send): %u 탎 -- Queue out --> Queue in %u 탎 -- Pool Time: %u 탎\n",
																	proccessingTime,proccessingTimeOP,poolTime);
			#endif
		#endif
	}
	else //downstream
		{
			//always data packets
			m_timingCountDataPacketsDownStream++;
			add64(m_timingSumDataPacketDownStream,proccessingTime);
			if(proccessingTime>m_timingMaxDataPacketDownStream)
				m_timingMaxDataPacketDownStream=proccessingTime;
			else if(m_timingMinDataPacketDownStream>proccessingTime)
				m_timingMinDataPacketDownStream=proccessingTime;	
			add64(m_timingSumDataPacketDownStreamOP,proccessingTimeOP);
			if(proccessingTimeOP>m_timingMaxDataPacketDownStreamOP)
				m_timingMaxDataPacketDownStreamOP=proccessingTimeOP;
			else if(m_timingMinDataPacketDownStreamOP>proccessingTimeOP)
				m_timingMinDataPacketDownStreamOP=proccessingTimeOP;	
			#ifdef USE_POOL
				UINT32 poolTime=diff64(oQueueEntry.pool_timestamp_out,oQueueEntry.pool_timestamp_in);
				m_timingCountPoolPacketsDownStream++;
				add64(m_timingSumPoolPacketDownStream,poolTime);
				if(poolTime>m_timingMaxPoolPacketDownStream)
					m_timingMaxPoolPacketDownStream=poolTime;
				else if(m_timingMinPoolPacketDownStream>poolTime)
					m_timingMinPoolPacketDownStream=poolTime;	
			#endif
			#ifdef _DEBUG
				#ifndef USE_POOL
					CAMsg::printMsg(LOG_CRIT,"Download Packet processing time (arrival --> send): %u 탎 -- Queue out --> Queue in %u 탎\n",
																		proccessingTime,proccessingTimeOP);
				#else
					CAMsg::printMsg(LOG_CRIT,"Download Packet processing time (arrival --> send): %u 탎 -- Queue out --> Queue in %u 탎 -- Pool Time: %u 탎\n",
																		proccessingTime,proccessingTimeOP,poolTime);
				#endif
			#endif
		}
	
	m_csTimeingStats.unlock();
	return E_SUCCESS;
}

SINT32 CALogPacketStats::logTimeingStats()
{
	m_csTimeingStats.lock();	
	UINT32 aveDataUpstream=0;
	UINT32 aveCloseUpstream=0;
	UINT32 aveOpenUpstream=0;
	UINT32 aveDataDownStream=0;
	UINT32 aveDataUpstreamOP=0;
	UINT32 aveCloseUpstreamOP=0;
	UINT32 aveOpenUpstreamOP=0;
	UINT32 aveDataDownStreamOP=0;
	if(m_timingCountOpenPacketsUpstream>0)
		{
			aveOpenUpstream=div64(m_timingSumOpenPacketUpstream,m_timingCountOpenPacketsUpstream);
			aveOpenUpstreamOP=div64(m_timingSumOpenPacketUpstreamOP,m_timingCountOpenPacketsUpstream);
		}
	if(m_timingCountDataPacketsUpstream>0)
		{
			aveDataUpstream=div64(m_timingSumDataPacketUpstream,m_timingCountDataPacketsUpstream);
			aveDataUpstreamOP=div64(m_timingSumDataPacketUpstreamOP,m_timingCountDataPacketsUpstream);
		}
	if(m_timingCountClosePacketsUpstream>0)
		{
			aveCloseUpstream=div64(m_timingSumClosePacketUpstream,m_timingCountClosePacketsUpstream);
			aveCloseUpstreamOP=div64(m_timingSumClosePacketUpstreamOP,m_timingCountClosePacketsUpstream);
		}
	if(m_timingCountDataPacketsDownStream>0)
		{
			aveDataDownStream=div64(m_timingSumDataPacketDownStream,m_timingCountDataPacketsDownStream);
			aveDataDownStreamOP=div64(m_timingSumDataPacketDownStreamOP,m_timingCountDataPacketsDownStream);
		}

	#ifdef USE_POOL
		UINT32 avePoolUpstream=0;
		UINT32 avePoolDownStream=0;
		if(m_timingCountPoolPacketsUpstream>0)
			avePoolUpstream=div64(m_timingSumPoolPacketUpstream,m_timingCountPoolPacketsUpstream);
		if(m_timingCountPoolPacketsDownStream>0)
			aveDataDownStream=div64(m_timingSumPoolPacketDownStream,m_timingCountPoolPacketsDownStream);
	#endif
	CAMsg::printMsg(LOG_DEBUG,"Packet timeing stats [탎] -- Data Packets Upstream [%u] (Min/Max/Ave): %u/%u/%u -- Open Packets Upstream [%u]: %u/%u/%u Close Packets Upstream [%u] %u/%u/%u -- Data Packets Downstream [%u]: %u/%u/%u \n",
	m_timingCountDataPacketsUpstream,m_timingMinDataPacketUpstream,m_timingMaxDataPacketUpstream,aveDataUpstream,
	m_timingCountOpenPacketsUpstream,m_timingMinOpenPacketUpstream,m_timingMaxOpenPacketUpstream,aveOpenUpstream,
	m_timingCountClosePacketsUpstream,m_timingMinClosePacketUpstream,m_timingMaxClosePacketUpstream,aveCloseUpstream,
	m_timingCountDataPacketsDownStream,m_timingMinDataPacketDownStream,m_timingMaxDataPacketDownStream,aveDataDownStream);

	CAMsg::printMsg(LOG_DEBUG,"Packet timeing stats (only Queue out --> Queue in)[탎] -- Data Packets Upstream [%u] (Min/Max/Ave): %u/%u/%u -- Open Packets Upstream [%u]: %u/%u/%u Close Packets Upstream [%u] %u/%u/%u -- Data Packets Downstream [%u]: %u/%u/%u \n",
	m_timingCountDataPacketsUpstream,m_timingMinDataPacketUpstreamOP,m_timingMaxDataPacketUpstreamOP,aveDataUpstreamOP,
	m_timingCountOpenPacketsUpstream,m_timingMinOpenPacketUpstreamOP,m_timingMaxOpenPacketUpstreamOP,aveOpenUpstreamOP,
	m_timingCountClosePacketsUpstream,m_timingMinClosePacketUpstreamOP,m_timingMaxClosePacketUpstreamOP,aveCloseUpstreamOP,
	m_timingCountDataPacketsDownStream,m_timingMinDataPacketDownStreamOP,m_timingMaxDataPacketDownStreamOP,aveDataDownStreamOP);
	#ifdef USE_POOL
		CAMsg::printMsg(LOG_DEBUG,"Pool timeing stats [탎] -- Upstream [%u] (Min/Max/Ave): %u/%u/%u -- Downstream [%u]: %u/%u/%u\n",
		m_timingCountPoolPacketsUpstream,m_timingMinPoolPacketUpstream,m_timingMaxPoolPacketUpstream,avePoolUpstream,
		m_timingCountPoolPacketsDownStream,m_timingMinPoolPacketDownStream,m_timingMaxPoolPacketDownStream,avePoolDownStream);
	#endif
	m_csTimeingStats.unlock();
	return E_SUCCESS;
}

SINT32 CALogPacketStats::resetTimeingStats()
	{
		m_csTimeingStats.lock();
		m_timingMaxDataPacketUpstream=0;
		m_timingMaxDataPacketDownStream=0;
		m_timingMaxClosePacketUpstream=0;
		m_timingMinDataPacketUpstream=0xFFFFFFFF;
		m_timingMinDataPacketDownStream=0xFFFFFFFF;
		m_timingMinClosePacketUpstream=0xFFFFFFFF;
		m_timingCountDataPacketsUpstream=m_timingCountDataPacketsDownStream=0;
		m_timingCountClosePacketsUpstream=0;
		setZero64(m_timingSumDataPacketUpstream);
		setZero64(m_timingSumDataPacketDownStream);
		setZero64(m_timingSumClosePacketUpstream);
		m_timingMaxOpenPacketUpstream=0;
		m_timingMinOpenPacketUpstream=0xFFFFFFFF;
		m_timingCountOpenPacketsUpstream=0;
		setZero64(m_timingSumOpenPacketUpstream);

		m_timingMaxDataPacketUpstreamOP=0;
		m_timingMaxDataPacketDownStreamOP=0;
		m_timingMaxClosePacketUpstreamOP=0;
		m_timingMinDataPacketUpstreamOP=0xFFFFFFFF;
		m_timingMinDataPacketDownStreamOP=0xFFFFFFFF;
		m_timingMinClosePacketUpstreamOP=0xFFFFFFFF;
		setZero64(m_timingSumDataPacketUpstreamOP);
		setZero64(m_timingSumDataPacketDownStreamOP);
		setZero64(m_timingSumClosePacketUpstreamOP);
		m_timingMaxOpenPacketUpstreamOP=0;
		m_timingMinOpenPacketUpstreamOP=0xFFFFFFFF;
		setZero64(m_timingSumOpenPacketUpstreamOP);

		#ifdef USE_POOL
			m_timingMaxPoolPacketUpstream=0;
			m_timingMaxPoolPacketDownStream=0;
			m_timingMinPoolPacketUpstream=m_timingMinPoolPacketDownStream=0xFFFFFFFF;
			m_timingCountPoolPacketsUpstream=0;
			setZero64(m_timingSumPoolPacketUpstream);
			m_timingCountPoolPacketsDownStream=0;
			setZero64(m_timingSumPoolPacketDownStream);
		#endif
		m_csTimeingStats.unlock();
		return E_SUCCESS;
	}
#endif
