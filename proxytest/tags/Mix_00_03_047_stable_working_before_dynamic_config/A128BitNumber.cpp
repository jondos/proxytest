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
#include "A128BitNumber.hpp"

A128BitNumber::A128BitNumber(UINT8* a_key)          //As we need Access to a 128Bit
	{                                                   //long number, we convert 16
		for (UINT8 i = 0; i<4; i++)                  //8 Bit values by shifting and
			{                                           //adding to form 4 32Bit values
				m_segmente[i] = 0;                          //with identical bit sequences
				for (UINT8 j = 0; j<4; j++)               //thus creating access to our 128Bit
					{                                         //value...
						m_segmente[i]<<=8;
						m_segmente[i]+=*a_key;
						a_key++;
					}
			}
	}


UINT32 A128BitNumber::getPosition()										//To find the position in the
	{                                                   //hash table we use the
		UINT32 r_position = m_segmente[0];                //first 20 bits as an array index
		r_position >>= 12;                                //We grab the first segment of 32 bits
		return r_position;                                //and shift the extra 12 away...
	}


UINT32 A128BitNumber::getSegment(UINT8 a_segmentNumber)
	{
		return m_segmente[a_segmentNumber];
	}

