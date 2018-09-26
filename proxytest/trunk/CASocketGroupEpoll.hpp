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
#ifndef __CASOCKETGOROPEPOLL__
#define __CASOCKETGOROPEPOLL__
#include "doxygen.h"
#ifdef HAVE_EPOLL
#include "CASocket.hpp"
#include "CAMuxSocket.hpp"
#include "CAMutex.hpp"
#ifdef DEBUG
	#include "CAMsg.hpp"
#endif

/*** A socket group implemented using epoll() call. You can use a socket group to query a et of sockets
for new events.
***/
class CASocketGroupEpoll
	{
		public:
			CASocketGroupEpoll(bool bWrite);
			~CASocketGroupEpoll();
			SINT32 setPoolForWrite(bool bWrite);
			
			/** Adds the socket s to the socket group. */
			SINT32 add(CASocket&s)
			{
				return add(s,NULL);
			}

			/** Adds the socket s to the socket group. */
			SINT32 add(CAMuxSocket&s)
			{
				return add(s, NULL);
			}

			/*** Adds the socket s to the socket group. Additional one can set a parameter datapointer, which is
			  * assoziated with the socket s. 
				Note: if datapointer!=NULL than you cannot use the isSignaled(socket) functions.!
				***/
			SINT32 add(CASocket&s,void * datapointer)
				{
					SINT32 ret=E_SUCCESS;
					m_pcsFD_SET->lock();
					SOCKET socket=s.getSocket();
					if (datapointer != NULL)
						m_pEpollEvent->data.ptr = datapointer;
					else
						m_pEpollEvent->data.fd = socket;

					if (epoll_ctl(m_hEPFD, EPOLL_CTL_ADD, socket, m_pEpollEvent) != 0)
					{
						CAMsg::printMsg(LOG_DEBUG, "Failed to add socket to epoll group\n");
						ret = E_UNKNOWN;
					}
					m_pcsFD_SET->unlock();
					return ret;
				}


			/** Adds the socket s to the socket group. Additional one can set a parameter datapointer, which is
			  * assoziated with the socket s*/
			SINT32 add(CAMuxSocket&s,void* datapointer)
				{
					return add(*(s.getCASocket()),datapointer);
				}

			SINT32 remove(CASocket&s)
				{
					SINT32 ret=E_SUCCESS;
					m_pcsFD_SET->lock();
					if(epoll_ctl(m_hEPFD,EPOLL_CTL_DEL,s.getSocket(),m_pEpollEvent)!=0)
						ret=E_UNKNOWN;
					ASSERT(ret==E_SUCCESS,"Error in Epoll socket group remove")
						m_pcsFD_SET->unlock();
					return ret;
				}

			SINT32 remove(CAMuxSocket&s)
				{
					return remove(*(s.getCASocket()));
				}

			SINT32 select()
				{
				m_pcsFD_SET->lock();
					m_iNumOfReadyFD=epoll_wait(m_hEPFD,m_pEvents,MAX_POLLFD,-1);
					if(m_iNumOfReadyFD>0)
						{
							m_pcsFD_SET->unlock();
							return m_iNumOfReadyFD;
						}
					m_pcsFD_SET->unlock();
					return E_UNKNOWN;
				}
			
			/** Waits for events on the sockets. 
				* If after ms milliseconds no event occurs, E_TIMEDOUT is returned
				* @param time_ms - maximum milliseconds to wait
				* @retval E_TIMEDOUT, if other ms milliseconds no event occurs
				* @retval E_UNKNOWN, if an error occured
				* @return number of read/writeable sockets
			***/
			SINT32 select(UINT32 time_ms)
				{
					m_pcsFD_SET->lock();
					m_iNumOfReadyFD=epoll_wait(m_hEPFD,m_pEvents,MAX_POLLFD,time_ms);
					if(m_iNumOfReadyFD>0)
						{
							m_pcsFD_SET->unlock();
							return m_iNumOfReadyFD;
						}
					else if(m_iNumOfReadyFD==0)
						{
							m_pcsFD_SET->unlock();
							return E_TIMEDOUT;
						}
					m_pcsFD_SET->unlock();
					return E_UNKNOWN;
				}

    /*** Note: only works, if datapointer was not set!***/
		bool isSignaled(CASocket&s)
				{
					SINT32 socket=s.getSocket();
					for(SINT32 i=0;i<m_iNumOfReadyFD;i++)
						{
							if(socket==m_pEvents[i].data.fd)
								return true;
						}
					return false;
				}

		/*** Note: only works, if datapointer was not set!***/
		bool isSignaled(CASocket*ps)
				{
				SINT32 socket=ps->getSocket();
					for(SINT32 i=0;i<m_iNumOfReadyFD;i++)
						{
							if(socket==m_pEvents[i].data.fd)
								return true;
						}
					return false;
				}

		/*** Note: only works, if datapointer was not set!***/
		bool isSignaled(CAMuxSocket&s)
				{
					SINT32 socket=s.getSocket();
					for(SINT32 i=0;i<m_iNumOfReadyFD;i++)
						{
							if(socket==m_pEvents[i].data.fd)
								return true;
						}
					return false;
				}
		
		/*** Note: only works, if datapointer was set!***/
			bool isSignaled(void* datapointer)
				{
					for(SINT32 i=0;i<m_iNumOfReadyFD;i++)
						{
							if(datapointer==m_pEvents[i].data.ptr)
								return true;
						}
					return false;
				}

			/*** Note: only works, if datapointer was set!***/
			void* getFirstSignaledSocketData()
				{
					m_iAktSignaledSocket=0;
					if(m_iNumOfReadyFD>0)
						return m_pEvents[0].data.ptr;
					return NULL;
				}

			/*** Note: only works, if datapointer was set!***/
			void* getNextSignaledSocketData()
				{
					m_iAktSignaledSocket++;
					if(m_iNumOfReadyFD>m_iAktSignaledSocket)
						return m_pEvents[m_iAktSignaledSocket].data.ptr;
					return NULL;
				}

		private:
			EPOLL_HANDLE m_hEPFD; //the EPoll file descriptor
			struct epoll_event* m_pEvents;
			struct epoll_event* m_pEpollEvent;
			SINT32 m_iNumOfReadyFD;
			SINT32 m_iAktSignaledSocket;
			CAMutex* m_pcsFD_SET;
	};
#endif
#endif
