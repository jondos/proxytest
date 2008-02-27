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

#ifndef __CAACCOUNTINGSETTLETHREAD__
#define __CAACCOUNTINGSETTLETHREAD__

#include "CAThread.hpp"
#include "Hashtable.hpp"
#include "CAConditionVariable.hpp"

struct t_aiSettleItem
	{
		DOM_Document doc;
	};

typedef struct t_aiSettleItem aiSettleItem;

/** This structure holds information about known Payment Instances */
struct t_paymentInstanceListNode
{
	t_paymentInstanceListNode* next;
	CACertificate* pCertificate;
	UINT32 u32Port;
	UINT8 arstrPIID[256];
	UINT8 arstrHost[256];
};

typedef struct t_paymentInstanceListNode tPaymentInstanceListEntry; 


/**
 * A thread that settles CCs with the BI.
 * 
 * @author Bastian Voigt
 * @todo make SLEEP_SECONDS a configure option
 */
class CAAccountingSettleThread
{
	public:
		CAAccountingSettleThread(Hashtable* a_accountingHashtable, UINT8* currentCascade);
		~CAAccountingSettleThread();
		
		void settle();
		
	private:	
		CAConditionVariable* m_pCondition;
		UINT8* m_settleCascade;
		static THREAD_RETURN mainLoop(void * param);
		CAThread* m_pThread;
		volatile bool m_bRun;
		Hashtable* m_accountingHashtable;	
		bool m_bSleep;
		
		struct SettleEntry
		{
			UINT64 accountNumber;
			UINT32 authFlags;
			UINT32 authRemoveFlags;
			UINT64 confirmedBytes;
			UINT64 diffBytes;
			SettleEntry* nextEntry;
		};			
	
};
#endif
