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

#ifndef CATEMPIPBLOCKLIST_HPP
#define CATEMPIPBLOCKLIST_HPP

#ifndef ONLY_LOCAL_PROXY

#include "CAThread.hpp"
#include "CAMutex.hpp"

struct _tempipblocklist_t
	{
		/** Next element, NULL if element is the last one */
		struct _tempipblocklist_t* next; 
	
		/** Entry is valid until getCurrentTimeMillis() > validTimeMillis */
		UINT64 validTimeMillis; 
		/** First two Bytes of the IP-Address */
		UINT8 ip[2];
	};

typedef struct _tempipblocklist_t		TEMPIPBLOCKLISTENTRY;
typedef struct _tempipblocklist_t*	PTEMPIPBLOCKLIST;

/**
 * The purpose of this class is storing the IPs of JAP users who tried to
 * hack/attack the payment system. Their IP should stay in this block list for a 
 * limited time period (e.g. 10 minutes or so). During this time a JAP 
 * cannot connect to the mixcascade from this IP.
 *
 * The implementation uses Mutex locking and is thus threadsafe
 *
 * @author Bastian Voigt <bavoigt@inf.fu-berlin.de>
 */
class CATempIPBlockList
{
	public:
		CATempIPBlockList(UINT64 validTimeMillis);

		~CATempIPBlockList();
		
		/** 
		 * inserts an IP into the blocklist 
		 *
		 * @retval E_SUCCESS if successful
		 * @retval E_UNKNOWN if IP was already in blocklist
		 */
		SINT32 insertIP(const UINT8 ip[4]);
		
		/** 
		 * check whether an IP is blocked 
		 *
		 * @retval 1, if the IP is blocked
		 * @retval 0, if the IP is not blocked
		 */
		SINT32 checkIP(const UINT8 ip[4]);
		
		/** 
		 * set the time (in Milliseconds) that each blocked IP should stay valid 
		 * in the list 
		 */
		void setValidTimeMillis(UINT64 millis);

		UINT32 count()
		{
			return m_iEntries;
		}

	private:
		/** as long as true the clenaupthread does his job. If false the thread will exit.*/
		volatile bool m_bRunCleanupThread;

		/** this thread cleans up the hashtable and removes old entries */
		CAThread * m_pCleanupThread;
		
		/** the cleanup thread main loop */
		static THREAD_RETURN cleanupThreadMainLoop(void *param);
		
		/** the time that each blocked IP should stay in the List */
		UINT64 m_validTimeMillis;
		
		/** the buffer where the entries are stored */
		PTEMPIPBLOCKLIST* m_hashTable;
			
		/** Used for locking the datastructure to make it threadsafe */
		CAMutex * m_pMutex;

		UINT32 m_iEntries;
};

#endif //ONLY_LOCAL_PROXY
#endif
