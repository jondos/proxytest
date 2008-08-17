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
#ifndef __CASYMCIPHER__
#define __CASYMCIPHER__

#define KEY_SIZE 16

#include "aes/rijndael-api-fst.h"
#include "CALockAble.hpp"
class CASymCipher:public CALockAble
	{
		public:
			CASymCipher()
				{
					m_bEncKeySet=false;
					m_keyAES=new keyInstance[1];
					m_iv=new UINT8[16];
					m_iv2=new UINT8[16];
				}

			~CASymCipher()
				{
					waitForDestroy();
					delete[] m_keyAES;
					delete[] m_iv;
					delete[] m_iv2;
				}
			bool isEncyptionKeyValid()
				{
					return m_bEncKeySet;
				}

			SINT32 setKeyAES(UINT8* key);
			SINT32 setIV(UINT8* iv)
				{// TODO m_iv2 ???
					memcpy(m_iv,iv,16);
					memcpy(m_iv2,iv,16);
					return E_SUCCESS;
				}

			SINT32 decryptAES(UINT8* in,UINT8* out,UINT32 len);
			SINT32 decryptAES2(UINT8* in,UINT8* out,UINT32 len);
			SINT32 encryptAES(UINT8* in,UINT8* out,UINT32 len);
		protected:
			keyInstance* m_keyAES;
			UINT8* m_iv;
			UINT8* m_iv2;
			bool m_bEncKeySet;
	};

#endif