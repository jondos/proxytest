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
#include "CASymCipher.hpp"
#include "CAMsg.hpp"
//AES


#ifdef AES_NI
extern "C"
	{
		int aesni_set_encrypt_key(const unsigned char *userKey, int bits,AES_KEY *key);
		int aesni_set_decrypt_key(const unsigned char *userKey, int bits,AES_KEY *key);
		void aesni_encrypt(const unsigned char *in, unsigned char *out,
                   const AES_KEY *key);
		void aesni_cbc_encrypt(const unsigned char *in,
                       unsigned char *out,
                       size_t length,
                       const AES_KEY *key, unsigned char *ivec, int enc);
	}
#endif

/** Sets the key1 and key2 used for encryption/decryption. Also resets the IVs to zero!
	* @param key 16 random bytes used as key
	* @retval E_SUCCESS
	*/
SINT32 CASymCipher::setKey(const UINT8* key)
{
	return setKey(key,true);
}

/** Sets the key1 and key2 used for encryption/decryption to the same value of key. Also resets the IVs to zero!
	* @param key 16 random bytes used as key
	* @param bEncrypt if true, the key should be used for encryption (otherwise it will be used for decryption)
	* @retval E_SUCCESS
	*/
SINT32 CASymCipher::setKey(const UINT8* key,bool bEncrypt)
{
	memset(m_iv1,0,16);
#ifdef INTEL_IPP_CRYPTO
	ippsRijndael128Init(key, IppsRijndaelKey128,m_keyAES1);
#else
	if(bEncrypt)
		{
#ifdef AES_NI
	aesni_set_encrypt_key(key,128,m_keyAES1);

#else
			AES_set_encrypt_key(key,128,m_keyAES1);
#endif
		}
	else
		{
#ifdef AES_NI
			aesni_set_decrypt_key(key,128,m_keyAES1);
#else
			AES_set_decrypt_key(key,128,m_keyAES1);
#endif
		}
#endif
	m_bKeySet=true;
	return E_SUCCESS;
}

SINT32 CASymCipher::setKeys(const UINT8* key,UINT32 keysize)
{
	if(keysize==KEY_SIZE)
		{
			return setKey(key);
		}
	else if(keysize==2*KEY_SIZE)
		{
#ifdef INTEL_IPP_CRYPTO
			ippsRijndael128Init(key, IppsRijndaelKey128,m_keyAES1);
			ippsRijndael128Init(key+KEY_SIZE, IppsRijndaelKey128,m_keyAES2);
#else
#ifdef AES_NI
			aesni_set_encrypt_key(key,128,m_keyAES1);
			aesni_set_encrypt_key(key+KEY_SIZE,128,m_keyAES2);
#else
			AES_set_encrypt_key(key,128,m_keyAES1);
#endif
#endif
			memset(m_iv1,0,16);
#ifdef SYM_CIPHER_CTR
	EVP_DecryptInit_ex(m_ctxAES1,EVP_aes_128_ctr(), NULL, key, m_iv1);
	EVP_EncryptInit_ex(m_ctxAES2, EVP_aes_128_ctr(), NULL, key+KEY_SIZE, m_iv2);
	memcpy(key1, key, 16);
	memcpy(key2, key + KEY_SIZE, 16);
#endif
			m_bKeySet=true;
			return E_SUCCESS;
		}
	return E_UNKNOWN;
}



/** En-/Decryptes in to out using iv1 and key1. AES is used for en-/dcryption and the cryption
	* is done with CBC mode and PKCS7 padding.
	* @param in input (plain or ciphertext) bytes
	* @param out output (plain or ciphertext) bytes
	* @param len len of input. on return the output len,
	*													which is always <= len of input
	* @retval E_SUCCESS
	* @retval E_UNKNOWN, if error
	*/
SINT32 CASymCipher::decryptCBCwithPKCS7(const UINT8* in,UINT8* out,UINT32* len)
{
	if(in==NULL||out==NULL||len==NULL||*len==0)
		return E_UNKNOWN;
#ifdef INTEL_IPP_CRYPTO
#else
#ifndef AES_NI
	AES_cbc_encrypt(in,out,*len,m_keyAES1,m_iv1,AES_DECRYPT);
#else
	aesni_cbc_encrypt(in,out,*len,m_keyAES1,m_iv1,AES_DECRYPT);
#endif
	//Now remove padding
	UINT32 pad=out[*len-1];
	if(pad>16||pad>*len)
		return E_UNKNOWN;
	for(UINT32 i=*len-pad; i<*len-1; i++)
		if(out[i]!=pad)
			return E_UNKNOWN;
	*len-=pad;
#endif
	return E_SUCCESS;
}

/** En-/Decryptes in to out using IV1 and key1. AES is used for en-/decryption and the cryption
	* is done with CBC mode and PKCS7 padding.
	* @param in input (plain or ciphertext) bytes
	* @param inlen size of the input buffer
	* @param out output (plain or ciphertext) bytes
	* @param len on call len of output buffer; on return size of output buffer used,
	*													which is always > len of input
	* @retval E_SUCCESS
	*/
SINT32 CASymCipher::encryptCBCwithPKCS7(const UINT8* in,UINT32 inlen,UINT8* out,UINT32* len)
{
#ifdef INTEL_IPP_CRYPTO
#else
	UINT32 padlen=16-(inlen%16);
	if(inlen+padlen>(*len))
		{
			return E_SPACE;
		}
	UINT8* tmp=new UINT8[inlen+padlen];
	memcpy(tmp,in,inlen);
	for(UINT32 i=inlen; i<inlen+padlen; i++)
		{
			tmp[i]=(UINT8)padlen;
		}
#ifndef AES_NI
	AES_cbc_encrypt(tmp,out,inlen+padlen,m_keyAES1,m_iv1,AES_ENCRYPT);
#else
	aesni_cbc_encrypt(tmp,out,inlen+padlen,m_keyAES1,m_iv1,AES_ENCRYPT);
#endif
	delete[] tmp;
	tmp = NULL;
	*len=inlen+padlen;
#endif
	return E_SUCCESS;
}

