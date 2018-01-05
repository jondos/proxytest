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
	memset(m_iv2,0,16);

#ifdef INTEL_IPP_CRYPTO
	ippsRijndael128Init(key, IppsRijndaelKey128,m_keyAES1);
	ippsRijndael128Init(key, IppsRijndaelKey128,m_keyAES2);
#else
	if(bEncrypt)
		{
#ifdef SYM_CIPHER_CTR
	EVP_DecryptInit_ex(m_ctxAES1,EVP_aes_128_ctr(), NULL, key, m_iv1);
	EVP_EncryptInit_ex(m_ctxAES2, EVP_aes_128_ctr(), NULL, key, m_iv2);
	memcpy(key1, key, 16);
	memcpy(key2, key, 16);

#endif
#ifdef AES_NI
	aesni_set_encrypt_key(key,128,m_keyAES1);
	aesni_set_encrypt_key(key,128,m_keyAES2);

#else
			AES_set_encrypt_key(key,128,m_keyAES1);
			AES_set_encrypt_key(key,128,m_keyAES2);
#endif
		}
	else
		{
#ifdef AES_NI
			aesni_set_decrypt_key(key,128,m_keyAES1);
			aesni_set_decrypt_key(key,128,m_keyAES2);
#else
			AES_set_decrypt_key(key,128,m_keyAES1);
			AES_set_decrypt_key(key,128,m_keyAES2);
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
			AES_set_encrypt_key(key+KEY_SIZE,128,m_keyAES2);
#endif
#endif
			memset(m_iv1,0,16);
			memset(m_iv2,0,16);
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
SINT32 CASymCipher::crypt1(const UINT8* in,UINT8* out,UINT32 len)
{
#ifdef NO_ENCRYPTION
	CAMsg::printMsg(LOG_ERR,"Warning: - DO NULL encryption!\n");
	memmove(out, in, len);
	return E_SUCCESS;
#endif
#ifdef SYM_CIPHER_CTR
	if (1/*(len % 16) == 0*/)
		{
			UINT32 i=len;
			int ret=EVP_DecryptUpdate(m_ctxAES1, out, (int*)&i, in, len);
			if (ret != 1)
				{
					CAMsg::printMsg(LOG_ERR, "Error in CASymCipher::crypt1()\n ");
					return E_UNKNOWN;
				}
		}
	else
		{
			UINT32 len1=len&0x0FFFFFF0;
			UINT32 i=len1;
			EVP_DecryptUpdate(m_ctxAES1, out, (int*)&i, in, len1);
			UINT32 index = len1;
			len1=len-len1;
			UINT8 tmpBuff[16];
			memcpy(tmpBuff,in+index, len1);
			EVP_DecryptUpdate(m_ctxAES1, tmpBuff,(int*)&i, tmpBuff, 16);
			memcpy(out+index,tmpBuff, len1);			
		}
	return E_SUCCESS;
#endif

#ifdef INTEL_IPP_CRYPTO
	UINT32 k=len&0xFFFFFFF0;
	ippsRijndael128EncryptOFB(in,out,k,16, m_keyAES1,m_iv1);
//				if((len%16)!=0)
//					{
	ippsRijndael128EncryptOFB(in+k,out+k,len%16,len%16, m_keyAES1,m_iv1);
//					}
	return E_SUCCESS;
#endif
	UINT32 i=0;
	while(i+15<len)
		{
#ifdef INTEL_IPP_CRYPTO
			ippsRijndael128EncryptECB(m_iv1,m_iv1,KEY_SIZE, m_keyAES1, IppsCPPaddingNONE);
#else
#ifdef AES_NI
//		int outlen = 16;
//		EVP_EncryptUpdate(m_ctxAES1, m_iv1, &outlen, m_iv1, 16);
			aesni_encrypt(m_iv1,m_iv1,m_keyAES1);
#else
			AES_encrypt(m_iv1,m_iv1,m_keyAES1);
#endif
#endif
			out[i]=in[i]^m_iv1[0];
			i++;
			out[i]=in[i]^m_iv1[1];
			i++;
			out[i]=in[i]^m_iv1[2];
			i++;
			out[i]=in[i]^m_iv1[3];
			i++;
			out[i]=in[i]^m_iv1[4];
			i++;
			out[i]=in[i]^m_iv1[5];
			i++;
			out[i]=in[i]^m_iv1[6];
			i++;
			out[i]=in[i]^m_iv1[7];
			i++;
			out[i]=in[i]^m_iv1[8];
			i++;
			out[i]=in[i]^m_iv1[9];
			i++;
			out[i]=in[i]^m_iv1[10];
			i++;
			out[i]=in[i]^m_iv1[11];
			i++;
			out[i]=in[i]^m_iv1[12];
			i++;
			out[i]=in[i]^m_iv1[13];
			i++;
			out[i]=in[i]^m_iv1[14];
			i++;
			out[i]=in[i]^m_iv1[15];
			i++;
		}
	if(i<len) //In this case len-i<16 !
		{
#ifdef INTEL_IPP_CRYPTO
			ippsRijndael128EncryptECB(m_iv1,m_iv1,KEY_SIZE, m_keyAES1, IppsCPPaddingNONE);
#else
#ifdef AES_NI
		//int outlen = 16;
		//EVP_EncryptUpdate(m_ctxAES1, m_iv1, &outlen, m_iv1, 16);
			aesni_encrypt(m_iv1,m_iv1,m_keyAES1);

#else
			AES_encrypt(m_iv1,m_iv1,m_keyAES1);
#endif
#endif
			len-=i;
			for(UINT32 k=0; k<len; k++)
				{
					out[i]=in[i]^m_iv1[k];
					i++;
				}
		}
	return E_SUCCESS;
}

/** Decryptes in to out using iv2 and key2.
	* @param in input (encrypted) bytes
	* @param out output (decrpyted) bytes
	* @param len len of input. because the cipher preserves the size,
	*													len of output=len of input
	* @retval E_SUCCESS
	*/
SINT32 CASymCipher::crypt2(const UINT8* in,UINT8* out,UINT32 len)
{
#ifdef NO_ENCRYPTION
	memmove(out, in, len);
	return E_SUCCESS;
#endif
#ifdef SYM_CIPHER_CTR
	if (1/*(len % 16) == 0*/)
		{
			UINT32 i=len;
			EVP_EncryptUpdate(m_ctxAES2, out, (int*)&i, in, len);
		}
	else
		{
			UINT32 len1=len&0x0FFFFFF0;
			UINT32 i=len1;
			EVP_EncryptUpdate(m_ctxAES2, out, (int*)&i, in, len1);
			UINT32 index = len1;
			len1=len-len1;
			UINT8 tmpBuff[16];
			memcpy(tmpBuff,in+index, len1);
			EVP_EncryptUpdate(m_ctxAES2, tmpBuff,(int*)&i, tmpBuff, 16);
			memcpy(out+index,tmpBuff, len1);
			
		}
	return E_SUCCESS;
#endif

	UINT32 i=0;
	while(i+15<len)
		{
#ifdef INTEL_IPP_CRYPTO
			ippsRijndael128EncryptECB(m_iv1,m_iv1,KEY_SIZE, m_keyAES2, IppsCPPaddingNONE);
#else
#ifdef AES_NI
			aesni_encrypt(m_iv2,m_iv2,m_keyAES2);
#else
			AES_encrypt(m_iv2,m_iv2,m_keyAES2);
#endif
#endif
			out[i]=in[i]^m_iv2[0];
			i++;
			out[i]=in[i]^m_iv2[1];
			i++;
			out[i]=in[i]^m_iv2[2];
			i++;
			out[i]=in[i]^m_iv2[3];
			i++;
			out[i]=in[i]^m_iv2[4];
			i++;
			out[i]=in[i]^m_iv2[5];
			i++;
			out[i]=in[i]^m_iv2[6];
			i++;
			out[i]=in[i]^m_iv2[7];
			i++;
			out[i]=in[i]^m_iv2[8];
			i++;
			out[i]=in[i]^m_iv2[9];
			i++;
			out[i]=in[i]^m_iv2[10];
			i++;
			out[i]=in[i]^m_iv2[11];
			i++;
			out[i]=in[i]^m_iv2[12];
			i++;
			out[i]=in[i]^m_iv2[13];
			i++;
			out[i]=in[i]^m_iv2[14];
			i++;
			out[i]=in[i]^m_iv2[15];
			i++;
		}
	if(i<len)
		{
#ifdef INTEL_IPP_CRYPTO
			ippsRijndael128EncryptECB(m_iv1,m_iv1,KEY_SIZE, m_keyAES2, IppsCPPaddingNONE);
#else
#ifdef AES_NI
			aesni_encrypt(m_iv2,m_iv2,m_keyAES2);
#else
			AES_encrypt(m_iv2,m_iv2,m_keyAES2);
#endif
#endif
			len-=i;
			for(UINT32 k=0; k<len; k++)
				{
					out[i]=in[i]^m_iv2[k];
					i++;
				}
		}
	return E_SUCCESS;
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
SINT32 CASymCipher::decrypt1CBCwithPKCS7(const UINT8* in,UINT8* out,UINT32* len)
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
SINT32 CASymCipher::encrypt1CBCwithPKCS7(const UINT8* in,UINT32 inlen,UINT8* out,UINT32* len)
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

#ifndef ONLY_LOCAL_PROXY
SINT32 CASymCipher::testSpeed()
{
	const UINT32 runs=1000000;
	CASymCipher* pCipher=new CASymCipher();
	CASymCipher* pCipher1=new CASymCipher();
	CASymCipher* pCipher2=new CASymCipher();
	CASymCipher* pCipher3=new CASymCipher();
	UINT8 key[16];
	UINT8* inBuff=new UINT8[1024];
	UINT8* outBuff=new UINT8[1024];
	getRandom(key,16);
	getRandom(inBuff,1024);
	UINT8* checkBuff=new UINT8[1024];
	memcpy(checkBuff, inBuff, 1024);
	pCipher->setKey(key);
	pCipher1->setKey(key);
	pCipher2->setKey(key);
	pCipher3->setKey(key,false);
	UINT64 start,end;

	AES_KEY* aeskey = new AES_KEY;
	AES_set_decrypt_key(key,128,aeskey);
	UINT8 iv[16];
	memset(iv, 0, 16);

	getcurrentTimeMillis(start);
	for(UINT32 i=0; i<runs; i++)
		{
			pCipher->crypt1(inBuff,inBuff,1023);
			pCipher1->crypt1(inBuff,inBuff,1023);
			if (memcmp(inBuff, checkBuff, 1023) != 0)
				{
						printf("CASymCiper::testSpeed() -- enc / dec mismatch!\n");
				}
			UINT32 outlen = 1024;
			pCipher2->encrypt1CBCwithPKCS7(inBuff, 1023, outBuff, &outlen);
			//pCipher3->decrypt1CBCwithPKCS7(outBuff, inBuff, &outlen);
			
			
			
				AES_cbc_encrypt(outBuff,inBuff,outlen,aeskey,iv,AES_DECRYPT);
			
			
			
			
			
			if (memcmp(inBuff, checkBuff, 1023) != 0)
				{
						printf("CASymCiper::testSpeed() -- CBC enc / dec mismatch!\n");
				}
		}
	getcurrentTimeMillis(end);
	UINT32 d=diff64(end,start);
	printf("CASymCiper::testSpeed() takes %u ms for %u * 1023 Bytes!\n",d,runs);
	return E_SUCCESS;
}
#endif

void CASymCipher::setGCMKeys(UINT8* keyRecv, UINT8* keySend)
{

#ifndef USE_OPENSSL_GCM
	m_pGCMCtxEnc = new gcm_ctx_64k;
	m_pGCMCtxDec = new gcm_ctx_64k;
#else
	//Note the have to provide *some* key (OpenSSL API enforced --> so the use the variables we have anyway..)
	// The Key will be overriden by a call to setKeyGCM in any case!
	AES_set_encrypt_key(m_iv1,128,m_keyAES1);
	m_pGCMCtxEnc = CRYPTO_gcm128_new(m_keyAES1,(block128_f)AES_encrypt);
	m_pGCMCtxDec = CRYPTO_gcm128_new(m_keyAES1,(block128_f)AES_encrypt);
#endif



#ifndef USE_OPENSSL_GCM
	if(m_pGCMCtxDec!=NULL)
		delete m_pGCMCtxDec;
	if(m_pGCMCtxEnc!=NULL)
		delete m_pGCMCtxEnc;

	m_pGCMCtxEnc = new gcm_ctx_64k;
	m_pGCMCtxDec = new gcm_ctx_64k;
	gcm_init_64k(m_pGCMCtxEnc, keySend, 128);
	gcm_init_64k(m_pGCMCtxDec, keyRecv, 128);
#else
	AES_set_encrypt_key(keyRecv,128,m_keyAES1);
	AES_set_encrypt_key(keySend,128,m_keyAES2);
	CRYPTO_gcm128_release(m_pGCMCtxEnc);
	CRYPTO_gcm128_release(m_pGCMCtxDec);
	m_pGCMCtxEnc=CRYPTO_gcm128_new(m_keyAES2,(block128_f)AES_encrypt);
	m_pGCMCtxDec=CRYPTO_gcm128_new(m_keyAES1,(block128_f)AES_encrypt);
#endif
	//reset IV
	m_nEncMsgCounter = 0;
	memset(m_pEncMsgIV, 0, 12);
	m_nDecMsgCounter = 0;
	memset(m_pDecMsgIV, 0, 12);
}

SINT32 CASymCipher::encryptMessage(const UINT8* const in, UINT32 inlen, UINT8* out)
{
	#ifdef NO_ENCRYPTION
	memmove(out, in, inlen);
	return E_SUCCESS;
#endif

	//m_pcsEnc->lock();
	m_pEncMsgIV[2] = htonl(m_nEncMsgCounter);
	m_nEncMsgCounter++;
#ifndef USE_OPENSSL_GCM
	gcm_encrypt_64k(m_pGCMCtxEnc, m_pEncMsgIV, in, inlen, out, (UINT32*)(out + inlen));
#else
	CRYPTO_gcm128_setiv(m_pGCMCtxEnc,(UINT8*)m_pEncMsgIV,12);
	CRYPTO_gcm128_encrypt(m_pGCMCtxEnc,in,out,inlen);
	CRYPTO_gcm128_tag(m_pGCMCtxEnc,out+inlen,16);
#endif
	//m_pcsEnc->unlock();
	return E_SUCCESS;
}

SINT32 CASymCipher::decryptMessage(const UINT8* in, UINT32 inlen, UINT8* out, bool integrityCheck)
{
#ifdef NO_ENCRYPTION
	memmove(out, in, inlen);
	return E_SUCCESS;
#endif

	SINT32 ret = E_UNKNOWN;
	//m_pcsDec->lock();
	m_pDecMsgIV[2] = htonl(m_nDecMsgCounter);
	if (integrityCheck)
		{
			m_nDecMsgCounter++;
#ifndef USE_OPENSSL_GCM
			ret = ::gcm_decrypt_64k(m_pGCMCtxDec, m_pDecMsgIV, in, inlen - 16, in + inlen - 16, out);
#else
			CRYPTO_gcm128_setiv(m_pGCMCtxDec,(UINT8*)m_pDecMsgIV,12);
			CRYPTO_gcm128_decrypt(m_pGCMCtxDec,in,out,inlen-16);
			ret=CRYPTO_gcm128_finish(m_pGCMCtxDec,in + inlen - 16,16);
#endif
		}
	else
		{
#ifndef USE_OPENSSL_GCM
			ret = ::gcm_decrypt_64k(m_pGCMCtxDec, m_pDecMsgIV, in, inlen, out);
#else
			CRYPTO_gcm128_setiv(m_pGCMCtxDec,(UINT8*)m_pDecMsgIV,12);
			ret=CRYPTO_gcm128_decrypt(m_pGCMCtxDec,in,out,inlen);
#endif
		}
	//m_pcsDec->unlock();
#ifndef USE_OPENSSL_GCM
	if(ret==0)
#else
	if(ret!=0)
#endif
		return E_UNKNOWN;
	return E_SUCCESS;
}
