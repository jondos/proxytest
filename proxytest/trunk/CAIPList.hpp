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
#ifndef __CA_IP_LIST
#define __CA_IP_LIST
#include "CAMutex.hpp"
/** This structure is used for building the IP-List. 
It stores the first two bytes of an IP-Address, how often this IP-Address was inserted
and a pointer to the next element of the list*/
struct _iplist_t
	{
		UINT8 ip[2]; /** First to Bytes of the IP-Address*/
		UINT8 count; /** Count of insertions*/ 
		struct _iplist_t* next; /**Next element, NULL if element is the last one*/
	};

typedef struct _iplist_t IPLISTENTRY;
typedef struct _iplist_t* PIPLIST;

/** The default value of allowed insertions, until insertIP() will return an error*/
#define MAX_IP_CONNECTIONS 10

/** The purpose of this class is to store a list of IP-Addresses. If an
IP-Address is inserted more than 'x' times, than an error is returned.
The First mix uses this functionalty to do some basic Denial Of Service defens.
If some one tries to do connection flooding to the First Mix, only 'x' connections
are accepted and the others are droped. 
The internal organisation is a hash-table with overrun lists. The hashtable has
0x10000 buckets. The last two bytes of an IP-Address are the hash-key.
@note This class only supports IPv4.
@warning If there is less memory, CAIPList will crash!
@warning This class is NOT thread safe.
@version 1.0 first version
 */

class CAIPList
	{
		public:
			CAIPList();
			CAIPList(UINT32 allowedConnections);
			~CAIPList();
			SINT32 insertIP(UINT8 ip[4]);
			SINT32 removeIP(UINT8 ip[4]);
		protected:
			UINT32 m_allowedConnections;
			PIPLIST* m_HashTable;
			UINT8 m_Random[56]; //seams to be the best value for MD5, which operats on x*512-64 bit (52*8+4*8=512-64)
			CAMutex m_Mutex;
	};
#endif
