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
#ifndef __CAASYMCHANNELCIPHERFACTORY__
#define __CAASYMCHANNELCIPHERFACTROY__
#include "CASymChannelCipher.hpp"
#include "CASymCipherCTR.hpp"
#include "CASymCipherOFB.hpp"
#include "CASymCipherNull.hpp"
class CASymChannelCipherFactory
	{
		public:
			static CASymChannelCipher* createCipher(SYMCHANNELCIPHER_ALGORITHM alg)
					{
						switch (alg)
							{
								case OFB:
									return new CASymCipherOFB();
								case CTR:
									return new CASymCipherCTR();
								case NULL_CIPHER:
									return new CASymCipherNull();
								case UNDEFINED_CIPHER:
									return NULL;
							}	
						return NULL;
					}

			static SYMCHANNELCIPHER_ALGORITHM getAlgIDFromString(UINT8* strAlgID)
			{
				if (strAlgID == NULL)
					return UNDEFINED_CIPHER;
				if (strcmp((const char*)SYMCHANNELCIPHER_ALG_NAME_OFB, (const char*)strAlgID) == 0)
				{
					return OFB;
				}
				if (strcmp((const char*)SYMCHANNELCIPHER_ALG_NAME_CTR, (const char*)strAlgID) == 0)
				{
					return CTR;
				}
				if (strcmp((const char*)SYMCHANNELCIPHER_ALG_NAME_NULL, (const char*)strAlgID) == 0)
				{
					return NULL_CIPHER;
				}
				return UNDEFINED_CIPHER;

			}
	};


#endif //__CASYMCHANNELCIPHERFACTORY__
