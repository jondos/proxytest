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
#include "CAReplay.hpp"
#include "A128BitNumber.hpp"

SINT32 CAReplay::init()
{
	m_uOldEpoch = 0;       //we start out with the first current epoch as 1...
	m_uCurrentEpoch = 1;
	m_uNewEpoch = 2;

	for (UINT32 i= 0; i<HASH_SIZE; i++)
		{
			m_arHash_1[i].seg1=0;    //Filling all arrays with 0 to mark all positions as empty
			m_arHash_2[i].seg1=0;
			m_arHash_3[i].seg1=0;

			m_arHash_1[i].seg2=0;    //Filling the second segment of all arrays with 0 to mark all positions as empty
			m_arHash_2[i].seg2=0;
			m_arHash_3[i].seg2=0;
		}

	m_arHashAssignment[OLD_HASH]=m_arHash_1;      //We start out with this configuration
	m_arHashAssignment[CURRENT_HASH]=m_arHash_2;  //After the first change of epoch it
	m_arHashAssignment[NEW_HASH]=m_arHash_3;      // will naturally be altered

	return E_SUCCESS;

}

SINT32 CAReplay::changeEpochs ()                          //If the timerthread changes the epoch,
	{                                                         //we save the location of the OLD_HASH
		tHashEntry *tempPointer = m_arHashAssignment[OLD_HASH];    	//in a temporary pointer, cycle the following
		m_arHashAssignment[OLD_HASH]=m_arHashAssignment[CURRENT_HASH];  //hashes backward and add the former OLD_HASH
		m_arHashAssignment[CURRENT_HASH]=m_arHashAssignment[NEW_HASH];  //After this we delete all old entries in the
		m_arHashAssignment[NEW_HASH]=tempPointer;                   //newly added NEW_HASH
		memset(tempPointer,0,sizeof(tHashEntry)*HASH_SIZE);
		return E_SUCCESS;
	}

SINT32 CAReplay::checkAndInsert (UINT32 hash, A128BitNumber* key, bool* isNoReplay)
{
  UINT32 position = key->getPosition();
  tHashEntry* pActiveHash= &(m_arHashAssignment[hash][position]);
  UINT32 fragment1 = key->getSegment(2);
	UINT32 fragment2 = key->getSegment(3);

	              //ActiveHash now points at the position
	                            //of the array where the key is to
	                           //be inserted


  if (pActiveHash->seg1 == 0 && pActiveHash->seg2 == 0)         //if no value is located at this position
		{
			pActiveHash->seg1 = fragment1;                 // insert the key fragment
			pActiveHash->seg2 = fragment2;
		}
  else
		{
      if (pActiveHash->seg1 == fragment1 && pActiveHash->seg2 == fragment2)            //if not compare the fragments to prevent
				{                                     //a replay attack and if not identical
					*isNoReplay = false;
					return E_SUCCESS;
				}
			else                                    //continue to the next array field and repreat
				{                                       //until a free array field is found
					pActiveHash++;
					position++;
					while ((position<HASH_SIZE) && (pActiveHash->seg1 != 0 || pActiveHash->seg2 != 0))
						{
							if (pActiveHash->seg1 == fragment1 && pActiveHash->seg2 == fragment2)
								{
									*isNoReplay = false;
									return E_SUCCESS;
								}
							else
								{
									pActiveHash++;
									position++;
								}
						}
      if (position == HASH_SIZE)             //in case an array out of bounds exception
				{                                      // has been prevented, return to the start
					position=0;
					pActiveHash = m_arHashAssignment[hash];
					while ((position<HASH_SIZE) && (pActiveHash->seg1 != 0 || pActiveHash->seg2 != 0))
						{
							if (pActiveHash->seg1 == fragment1 && pActiveHash->seg2 == fragment2)      //if we ran out of the array in the first
								{
									*isNoReplay = false;
									return E_SUCCESS;
								}						                      //cycle, we try once more and return
							else                              //false if this failes, as the array
								{                               //is full, however the probability
									pActiveHash++;                 //of this is extremely low
									position++;
								}
						}
					if (position == HASH_SIZE)
						{
							*isNoReplay = false;
							return E_SUCCESS;
						}
				}
			pActiveHash->seg1 = fragment1;
		  pActiveHash->seg2 = fragment2;              //in that case insert the fragments here
    }
  }

	*isNoReplay = true;
  return E_SUCCESS;
}



SINT32 CAReplay::validEpochAndKey( A128BitNumber* key, UINT32 keylen, UINT32 epoch_buff, bool* isNoReplay)
{
  //It is illegal to change the length of the key during operation
  if (keylen != KEYLENGTH)
		{
			*isNoReplay = false;
			return E_SUCCESS;
		}

  //depending on which Epoch the message was sent in we have it inserted
  //in the array or discard it if invalid

  if (epoch_buff == m_uOldEpoch)
	  {
		  checkAndInsert(OLD_HASH, key, isNoReplay);
			return E_SUCCESS;
	  }
  else if (epoch_buff == m_uCurrentEpoch)
	  {
			checkAndInsert(CURRENT_HASH, key, isNoReplay);
			return E_SUCCESS;
	  }
  else if (epoch_buff == m_uNewEpoch)
	  {
		  checkAndInsert(NEW_HASH, key, isNoReplay);
			return E_SUCCESS;
	  }
  else
	  *isNoReplay = false;
    return E_SUCCESS;
}

UINT32 CAReplay::getCurrentEpoch()
{
	return m_uCurrentEpoch;
}
