/*
Copyright (c) 2000, The JAP-Team
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

        - Redistributions of source code must retain the above copyright notice,
          this list of conditions and the following disclaimer.

        - Redistributions in binary form must reproduce the above copyright
notice,
          this list of conditions and the following disclaimer in the
documentation and/or
                other materials provided with the distribution.

        - Neither the name of the University of Technology Dresden, Germany nor
the names of its contributors
          may be used to endorse or promote products derived from this software
without specific
                prior written permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE
*/
#ifndef __CAASYMCHANNELCIPHER__
#define __CAASYMCHANNELCIPHER__
#include "CALockAble.hpp"
/*
#ifdef SYM_CHANNEL_CIPHER_CTR
#define CASymChannelCipher CASymCipherCTR
#include "CASymCipherCTR.hpp"


#else
#define CASymChannelCipher CASymCipherOFB
#endif

#define CASymCipherMuxSocket CASymCipherOFB
#include "CASymCipherOFB.hpp"
*/

const UINT8* const SYMCHANNELCIPHER_ALG_NAME_OFB = (const UINT8* const) "AES/OFB/ANON";
const UINT8* const SYMCHANNELCIPHER_ALG_NAME_CTR = (const UINT8* const) "AES/CTR";
const UINT8* const SYMCHANNELCIPHER_ALG_NAME_NULL = (const UINT8* const) "NULL";

enum SYMCHANNELCIPHER_ALGORITHM { OFB,CTR,NULL_CIPHER,UNDEFINED_CIPHER };

class CASymChannelCipher
	#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX
	:public CALockAble
#endif
	{
		public:
			

			static const UINT8* const getAlgorithmName(SYMCHANNELCIPHER_ALGORITHM alg)
				{
					switch (alg)
						{
							case OFB:
								return SYMCHANNELCIPHER_ALG_NAME_OFB;
							case CTR:
								return SYMCHANNELCIPHER_ALG_NAME_CTR;
							case NULL_CIPHER:
								return SYMCHANNELCIPHER_ALG_NAME_NULL;
							case UNDEFINED_CIPHER:
								return NULL;
						}	
					return NULL;
				}

			virtual	SINT32 crypt1(const UINT8* in,UINT8* out,UINT32 len)=0;
			virtual SINT32 crypt2(const UINT8* in,UINT8* out,UINT32 len)=0;
			/** Sets the keys for crypt1() and crypt2() to the same key*/
			virtual SINT32 setKey(const UINT8* key)=0;	
					/** Sets the keys for crypt1() and crypt2() either to the same key (if keysize==KEY_SIZE) or to
			 * different values, if keysize==2* KEY_SIZE*/
			virtual SINT32 setKeys(const UINT8* key,UINT32 keysize)=0;	

				/** Sets iv1 and iv2 to p_iv.
				* @param p_iv 16 random bytes used for new iv1 and iv2.
				* @retval E_SUCCESS
				*/
			virtual SINT32 setIVs(const UINT8* p_iv) = 0;

			/** Sets iv2 to p_iv.
				* @param p_iv 16 random bytes used for new iv2.
				* @retval E_SUCCESS
				*/
			virtual SINT32 setIV2(const UINT8* p_iv) = 0;

			virtual bool isKeyValid()=0;

	};


#define CASymCipherMuxSocket CASymChannelCipher



#endif //__CAASYMCHANNELCIPHER__