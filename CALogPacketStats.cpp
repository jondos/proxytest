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
			}
		else if(uType==CHANNEL_CLOSE)
			{
				m_timingCountClosePacketsUpstream++;
				add64(m_timingSumClosePacketUpstream,proccessingTime);
				if(proccessingTime>m_timingMaxClosePacketUpstream) 
					m_timingMaxClosePacketUpstream=proccessingTime;
				else if(m_timingMinClosePacketUpstream>proccessingTime)
					m_timingMinClosePacketUpstream=proccessingTime;	
			}
		else
			{//open
				m_timingCountOpenPacketsUpstream++;
				add64(m_timingSumOpenPacketUpstream,proccessingTime);
				if(proccessingTime>m_timingMaxOpenPacketUpstream)
					m_timingMaxOpenPacketUpstream=proccessingTime;
				else if(m_timingMinOpenPacketUpstream>proccessingTime)
					m_timingMinOpenPacketUpstream=proccessingTime;	
			}
		#ifdef USE_POOL
			UINT32 poolTime=diff64(oQueueEntry.pool_timestamp_out,oQueueEntry.pool_timestamp_out);
			m_timingCountPoolPacketsUpstream++;
			add64(m_timingSumPoolPacketUpstream,poolTime);
			if(poolTime>m_timingMaxPoolPacketUpstream)
				m_timingMaxPoolPacketUpstream=poolTime;
			else if(m_timingMinPoolPacketUpstream>poolTime)
				m_timingMinPoolPacketUpstream=poolTime;	
		#endif
		#ifdef _DEBUG
			#ifndef USE_POOL
				CAMsg::printMsg(LOG_CRIT,"Upload Packet processing time (arrival <--> send): %u �s\n",
																	proccessingTime);
			#else
				CAMsg::printMsg(LOG_CRIT,"Upload Packet processing time (arrival <--> send): %u �s [Pool Time: %u �s]\n",
																	proccessingTime,poolTime);
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
			#ifdef USE_POOL
				UINT32 poolTime=diff64(oQueueEntry.pool_timestamp_out,oQueueEntry.pool_timestamp_out);
				m_timingCountPoolPacketsDownStream++;
				add64(m_timingSumPoolPacketDownStream,poolTime);
				if(poolTime>m_timingMaxPoolPacketDownStream)
					m_timingMaxPoolPacketDownStream=poolTime;
				else if(m_timingMinPoolPacketDownStream>poolTime)
					m_timingMinPoolPacketDownStream=poolTime;	
			#endif
			#ifdef _DEBUG
				#ifndef USE_POOL
					CAMsg::printMsg(LOG_CRIT,"Download Packet processing time (arrival <--> send): %u �s\n",
																		proccessingTime);
				#else
					CAMsg::printMsg(LOG_CRIT,"Download Packet processing time (arrival <--> send): %u �s [Pool Time: %u �s]\n",
																		proccessingTime,poolTime);
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
	if(m_timingCountOpenPacketsUpstream>0)
			aveOpenUpstream=div64(m_timingSumOpenPacketUpstream,m_timingCountOpenPacketsUpstream);
	if(m_timingCountDataPacketsUpstream>0)
			aveDataUpstream=div64(m_timingSumDataPacketUpstream,m_timingCountDataPacketsUpstream);
	if(m_timingCountClosePacketsUpstream>0)
			aveCloseUpstream=div64(m_timingSumClosePacketUpstream,m_timingCountClosePacketsUpstream);
	if(m_timingCountDataPacketsDownStream>0)
			aveDataDownStream=div64(m_timingSumDataPacketDownStream,m_timingCountDataPacketsDownStream);
	#ifdef USE_POOL
		UINT32 avePoolUpstream=0;
		UINT32 avePoolDownStream=0;
		if(m_timingCountPoolPacketsUpstream>0)
			avePoolUpstream=div64(m_timingSumPoolPacketUpstream,m_timingCountPoolPacketsUpstream);
		if(m_timingCountPoolPacketsDownStream>0)
			aveDataDownStream=div64(m_timingSumPoolPacketDownStream,m_timingCountPoolPacketsDownStream);
	#endif
	CAMsg::printMsg(LOG_DEBUG,"Packet timeing stats [�s] -- Data Packets Upstream [%u] (Min/Max/Ave): %u/%u/%u -- Open Packets Upstream [%u]: %u/%u/%u Close Packets Upstream [%u] %u/%u/%u -- Data Packets Downstream [%u]: %u/%u/%u \n",
	m_timingCountDataPacketsUpstream,m_timingMinDataPacketUpstream,m_timingMaxDataPacketUpstream,aveDataUpstream,
	m_timingCountOpenPacketsUpstream,m_timingMinOpenPacketUpstream,m_timingMaxOpenPacketUpstream,aveOpenUpstream,
	m_timingCountClosePacketsUpstream,m_timingMinClosePacketUpstream,m_timingMaxClosePacketUpstream,aveCloseUpstream,
	m_timingCountDataPacketsDownStream,m_timingMinDataPacketDownStream,m_timingMaxDataPacketDownStream,aveDataDownStream);
	#ifdef USE_POOL
		CAMsg::printMsg(LOG_DEBUG,"Pool timeing stats [�s] -- Upstream [%u] (Min/Max/Ave): %u/%u/%u -- Downstream [%u]: %u/%u/%u\n",
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
