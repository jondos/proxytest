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
	#ifndef __CASOCKETADDRUNIX__
	#define __CASOCKETADDRUNIX__
	#include "CASocketAddr.hpp"

	/** This is a class for Unix Domain Protocol Sockat Addresses.*/
	class CASocketAddrUnix:public CASocketAddr,private sockaddr_un
		{
			public:
				CASocketAddrUnix();
				CASocketAddrUnix(const CASocketAddrUnix& addr);
				
				/**Returns the type (family) of the socket this address is for (always AF_LOCAL)
					* @retval AF_LOCAL
					*/
				SINT32 getType() const
					{
						return AF_LOCAL;
					}
				

				/** Creates a new copy of this address
					*
					* @return a copy of this address
					*/
				CASocketAddr* clone() const
					{
						return new CASocketAddrUnix(*this);
					}
			

				/** Resturns the size of the SOCKADDR struct used.
					* return sizeof(sockaddr_un)
					*/
				SINT32 getSize() const
					{
						return sizeof(sockaddr_un);
					}
				
				/** Makes a cast to SOCKADDR* .*/
				const SOCKADDR* LPSOCKADDR() const
					{
						return (const ::LPSOCKADDR)(static_cast<const sockaddr_un*>(this));
					}			
				
				/** Sets the path of this unix address.
					* @param path the new path of this address
					* @retval E_SUCCESS if succesful
					* @retval E_UNKNOWN otherwise
					*/
				SINT32 setPath(const char* path);
				
				/** Retruns the path of this address
					*
					* @return the path of this address or NULL if not set
					*/
				UINT8* getPath() const;

				/** Returns a human readable string describing this address.
					* @param buff buffer which holds the string
					* @param bufflen size of the buffer
					* @retval E_SPACE if the bufvfer is to small for the string
					* @retval E_UNKNOWN if an error occured
					* @retval E_SUCCESS if successfull
					*/
				virtual SINT32 toString(UINT8* buff,UINT32 bufflen) const
					{
						UINT8* tmppath=getPath();
						if(tmppath==NULL)
							return E_UNKNOWN;
						SINT32 ret=snprintf((char*)buff,bufflen,"Unix address: %s",tmppath);
						delete[]tmppath;
						tmppath = NULL;
						if(ret<0)
							{
								return E_SPACE;
							}
						return E_SUCCESS;
					}

		};

	#endif
