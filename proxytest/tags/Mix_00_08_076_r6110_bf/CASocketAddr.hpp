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
#ifndef __CASOCKETADDR__
#define __CASOCKETADDR__

/** This is an abstract class for representing a socket address used in CASocket, CADatagramSocket and CAMuxSocket.*/
class CASocketAddr
	{
		public:
			virtual ~CASocketAddr(){}

			/**Creates a copy of the Address*/
			virtual CASocketAddr* clone() const=0;

			/** The type (family) of socket for which this address is useful.
			  * Must be overwritten in subclasses. **/
			virtual SINT32  getType() const =0;

			/** The size of the SOCKADDR struct needed by function of CASocket and other.*/
			virtual SINT32 getSize() const =0;

			/** Casts to a SOCKADDR struct **/
			virtual	const SOCKADDR* LPSOCKADDR() const =0;
			//	virtual operator LPSOCKADDR()=0;

			/** Returns a string which describes this address in a human readable form.
				* @param buff buffer which stores the result
				* @param bufflen size of buff
				* @retval E_SUCCESS if successful
				* @retval E_SPACE if the buffer is to small
				* @retval E_UNKNOWN in case of an other error
				*/
			virtual SINT32 toString(UINT8* buff,UINT32 bufflen)const=0;

	};

typedef CASocketAddr* LPCASOCKETADDR;
#endif
