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
#include "CASymCipherCTR.hpp"
#include "CAMsg.hpp"
//AES


/** Sets the key1 and key2 used for encryption/decryption to the same value of key. Also resets the IVs to zero!
	* @param key 16 random bytes used as key
	* @param bEncrypt if true, the key should be used for encryption (otherwise it will be used for decryption)
	* @retval E_SUCCESS
	*/
SINT32 CASymCipherCTR::setKey(const UINT8* key)
{
	memset(m_iv1,0,16);
	memset(m_iv2,0,16);

	EVP_DecryptInit_ex(m_ctxAES1,EVP_aes_128_ctr(), NULL, key, m_iv1);
	EVP_EncryptInit_ex(m_ctxAES2, EVP_aes_128_ctr(), NULL, key, m_iv2);
	memcpy(key1, key, 16);
	memcpy(key2, key, 16);

	m_bKeySet=true;
	return E_SUCCESS;
}

SINT32 CASymCipherCTR::setKeys(const UINT8* key,UINT32 keysize)
{
	if(keysize==KEY_SIZE)
		{
			return setKey(key);
		}
	else if(keysize==2*KEY_SIZE)
		{
			memset(m_iv1,0,16);
			memset(m_iv2,0,16);
			EVP_DecryptInit_ex(m_ctxAES1,EVP_aes_128_ctr(), NULL, key, m_iv1);
			EVP_EncryptInit_ex(m_ctxAES2, EVP_aes_128_ctr(), NULL, key+KEY_SIZE, m_iv2);
			memcpy(key1, key, 16);
			memcpy(key2, key + KEY_SIZE, 16);
			m_bKeySet=true;
			return E_SUCCESS;
		}
	return E_UNKNOWN;
}

/** Encryptes/Decrpytes in to out using iv1 and key1. AES is used for encryption and the encryption
	* is done with a special
	* 128bit-OFB mode: In the case that (len mod 16 !=0) the unused cipher
	* output bits are discarded and NOT used next time encryptAES() is called.
	* That means that every time encrpytAES() is called at first new cipher output
	* is created by calling AES-encrypt(iv).
	* @param in input (plain text) bytes
	* @param out output (encrpyted) bytes
	* @param len len of input. because the cipher preserves the size,
	*													len of output=len of input
	* @retval E_SUCCESS
	*/
SINT32 CASymCipherCTR::crypt1(const UINT8* in,UINT8* out,UINT32 len)
{
#ifdef NO_ENCRYPTION
	CAMsg::printMsg(LOG_ERR,"Warning: - DO NULL encryption!\n");
	memmove(out, in, len);
	return E_SUCCESS;
#endif
			UINT32 i=2000;
			UINT8 tmpBuff[2000];
			int ret=EVP_DecryptUpdate(m_ctxAES1, tmpBuff, (int*)&i, in, len);
			/*if (ret != 1)
				{
					CAMsg::printMsg(LOG_ERR, "Error in CASymCipher::crypt1()\n ");
					return E_UNKNOWN;
				}*/
			memcpy(out, tmpBuff, len);
	return E_SUCCESS;
}

/** Decryptes in to out using iv2 and key2.
	* @param in input (encrypted) bytes
	* @param out output (decrpyted) bytes
	* @param len len of input. because the cipher preserves the size,
	*													len of output=len of input
	* @retval E_SUCCESS
	*/
SINT32 CASymCipherCTR::crypt2(const UINT8* in,UINT8* out,UINT32 len)
{
#ifdef NO_ENCRYPTION
	memmove(out, in, len);
	return E_SUCCESS;
#endif
			UINT32 i=2000;
			UINT8 tmpBuff[2000];

			EVP_EncryptUpdate(m_ctxAES2, tmpBuff, (int*)&i, in, len);
			memcpy(out, tmpBuff, len);
	return E_SUCCESS;
}


