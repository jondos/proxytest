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

#ifndef __CAREPLAY__
#define __CAREPLAY__

#include "A128BitNumber.hpp"

#define HASH_SIZE     8388608   //Size of each array in byte
#define KEYLENGTH     128       //Keylength as transmitted in bit
#define TABLE_ENTRY   64        //Length of array entry (part of the key that is stored)
#define OLD_HASH      0
#define CURRENT_HASH  1
#define NEW_HASH      2

struct t_HashEntry
	{
		UINT32 seg1;
		UINT32 seg2;
	};

typedef t_HashEntry tHashEntry;

class CAReplay
	{
		public:
			SINT32 init();
			SINT32 changeEpochs();
			SINT32 checkAndInsert (UINT32 hash, A128BitNumber* key, bool*);
			SINT32 validEpochAndKey(A128BitNumber* key, UINT32 keylen, UINT32 epoch_buff, bool*);
			UINT32 getCurrentEpoch();
		
		private:
			//The three Hash tables are listed in the following - note they are
			//not bound to an order, as they will be reassigned during runtime.
			A128BitNumber*    m_pA128BitNumber;
			tHashEntry				m_arHash_1[HASH_SIZE];
			tHashEntry				m_arHash_2[HASH_SIZE];
			tHashEntry				m_arHash_3[HASH_SIZE];
			UINT32						m_uOldEpoch;
			UINT32						m_uCurrentEpoch;
			UINT32						m_uNewEpoch;
			tHashEntry*				m_arHashAssignment[3];   //These pointers define which array is currently used
																  //for old, current and new epoch
																  //HashAssignment[0] points to Old_Epoch
																  //HashAssignment[1] points to Current_Epoch
																  //HashAssignment[2] points to New_Epoch

	};
#endif
