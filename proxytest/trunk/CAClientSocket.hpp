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
#ifndef __CA_CLIENT_SOCKET__
#define __CA_CLIENT_SOCKET__
#include "CAUtil.hpp"

class CASocket;

class CAClientSocket
	{
		public:
			virtual ~CAClientSocket()
				{
				}
			/** Sends all data over the network. This may block until all data is send.
					@param buff - the buffer of data to send
					@param len - content length
					@retval E_UNKNOWN, if an error occured
					@retval E_SUCCESS, if successful
			*/			
			virtual SINT32 sendFully(const UINT8* buff,UINT32 len)=0;
			
			/** Will receive some bytes from the socket. May block or not depending on the implementation.
				* @param buff the buffer which get the received data
				* @param len size of buff
				*	@return SOCKET_ERROR if an error occured
				* @retval E_AGAIN, if socket was in non-blocking mode and 
				*													receive would block or a timeout was reached
				* @retval 0 if socket was gracefully closed
				* @return the number of bytes received (always >0)
			***/
			virtual	SINT32 receive(UINT8* buff,UINT32 len)=0;
			
			/** Receives all len bytes. This blocks until all bytes are received or an error occured.
				* @return E_UNKNOWN, in case of an error
				* @return E_SUCCESS otherwise
			***/
			SINT32 receiveFully(UINT8* buff,UINT32 len)
			{
				SINT32 ret;
				UINT32 pos=0;
				do
					{
						ret=receive(buff+pos,len);
						if(ret<=0)
						{
							if(ret==E_AGAIN)
							{
								msSleep(100);
								continue;
							}
							else
							{
								return E_UNKNOWN;
							}
						}
						pos+=ret;
						len-=ret;
					}
				while(len>0);
				return E_SUCCESS;	    	    
			}
	};
#endif
