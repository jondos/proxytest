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
#ifndef __CASYMCIPHERNULL__
#define __CASYMCIPHERNULL__


#include "CALockAble.hpp"
#include "CAMutex.hpp"
#include "CASymChannelCipher.hpp"
/** This class implemtns the symmetric channel cipher interface - but does not do eny encryption!.
	*/
class CASymCipherNull:public CASymChannelCipher

	{
		public:
			CASymCipherNull()
				{
				}

			~CASymCipherNull()
				{
#ifndef ONLY_LOCAL_PROXY
					waitForDestroy();
#endif
				}
			bool isKeyValid()
				{
					return true;
				}

			/** Sets the keys for crypt1() and crypt2() to the same key*/
			SINT32 setKey(const UINT8* key)
				{
					return E_SUCCESS;
				}
			
			/** Sets the keys for crypt1() and crypt2() either to the same key (if keysize==KEY_SIZE) or to
			 * different values, if keysize==2* KEY_SIZE*/
			SINT32 setKeys(const UINT8* key,UINT32 keysize)
				{
					return E_SUCCESS;
				}

			
			/** Sets iv1 and iv2 to p_iv.
				* @param p_iv 16 random bytes used for new iv1 and iv2.
				* @retval E_SUCCESS
				*/
			SINT32 setIVs(const UINT8* p_iv)
				{
					return E_SUCCESS;
				}

			/** Sets iv2 to p_iv.
				* @param p_iv 16 random bytes used for new iv2.
				* @retval E_SUCCESS
				*/
			SINT32 setIV2(const UINT8* p_iv)
				{
					return E_SUCCESS;
				}

			SINT32 crypt1(const UINT8* in,UINT8* out,UINT32 len)
				{
					memmove(out, in, len);
					return E_SUCCESS;
				}

			SINT32 crypt2(const UINT8* in,UINT8* out,UINT32 len)
				{
					memmove(out, in, len);
					return E_SUCCESS;
				}


	};

#endif
