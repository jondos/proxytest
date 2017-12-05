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

#include "CALockAble.hpp"
#include "CAMutex.hpp"
/** This class could be used for encryption/decryption of data (streams) with
	* AES using 128bit OFB mode. Because of the OFB mode technical encryption
	* and decrpytion are the same (depending on the kind of input). Therefore
	* there is only a general crypt() function.
	* This class has a 2-in-1 feature: Two independent IVs are available. Therefore
	* we have crypt1() and crypt2() depending on the used IV.
	*/
#define AES_NI
#define SYM_CIPHER_CTR
class CASymCipher
#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX
	:public CALockAble
#endif
	{
		public:
			CASymCipher()
				{
					m_bKeySet=false;
#ifdef INTEL_IPP_CRYPTO
					int size=0;
					ippsRijndael128GetSize(&size);
					m_keyAES1=(IppsRijndael128Spec*)new UINT8[size];
					m_keyAES2=(IppsRijndael128Spec*)new UINT8[size];
#else
					m_keyAES1=new AES_KEY;
					m_keyAES2=new AES_KEY;
#endif
					m_iv1=new UINT8[16];
					m_iv2=new UINT8[16];

					m_nEncMsgCounter = 0;
					m_pEncMsgIV = new UINT32[3];
					memset(m_pEncMsgIV, 0, 12);
					m_nDecMsgCounter = 0;
					m_pDecMsgIV = new UINT32[3];
					memset(m_pDecMsgIV, 0, 12);

					m_pGCMCtxEnc = NULL;
					m_pGCMCtxDec = NULL;

					m_pcsEnc = new CAMutex();
					m_pcsDec = new CAMutex();
				}

			~CASymCipher()
				{
#ifndef ONLY_LOCAL_PROXY
					waitForDestroy();
#endif
#ifdef INTEL_IPP_CRYPTO
					delete[] (UINT8*)m_keyAES1;
					delete[] (UINT8*)m_keyAES2;
#else
					delete m_keyAES1;
					delete m_keyAES2;
#endif
					m_keyAES1 = NULL;
					m_keyAES2 = NULL;
					delete[] m_iv1;
					m_iv1 = NULL;
					delete[] m_iv2;
					m_iv2 = NULL;

					delete [] m_pEncMsgIV;
					m_pEncMsgIV = NULL;
					delete [] m_pDecMsgIV;
					m_pDecMsgIV = NULL;

#ifndef USE_OPENSSL_GCM
					delete m_pGCMCtxEnc;
					delete m_pGCMCtxDec;
#else
					CRYPTO_gcm128_release(m_pGCMCtxEnc);
					CRYPTO_gcm128_release(m_pGCMCtxDec);
#endif

					m_pGCMCtxEnc = NULL;
					m_pGCMCtxDec = NULL;

					delete m_pcsEnc;
					m_pcsEnc = NULL;
					delete m_pcsDec;
					m_pcsDec = NULL;
				}
			bool isKeyValid()
				{
					return m_bKeySet;
				}

			/** Sets the keys for crypt1() and crypt2() to the same key*/
			SINT32 setKey(const UINT8* key);	
			
			/** Sets the keys for crypt1() and crypt2() either to the same key (if keysize==KEY_SIZE) or to
			 * different values, if keysize==2* KEY_SIZE*/
			SINT32 setKeys(const UINT8* key,UINT32 keysize);	
			
			SINT32 setKey(const UINT8* key,bool bEncrypt);	

			/** Sets iv1 and iv2 to p_iv.
				* @param p_iv 16 random bytes used for new iv1 and iv2.
				* @retval E_SUCCESS
				*/
			SINT32 setIVs(const UINT8* p_iv)
				{
					memcpy(m_iv1,p_iv,16);
					memcpy(m_iv2,p_iv,16);
#ifdef SYM_CIPHER_CTR
					EVP_EncryptInit_ex(m_ctxAES1, EVP_aes_128_ctr(), NULL, key1, m_iv1);
					EVP_EncryptInit_ex(m_ctxAES2, EVP_aes_128_ctr(), NULL, key2, m_iv2);
#endif
					return E_SUCCESS;
				}

			/** Sets iv2 to p_iv.
				* @param p_iv 16 random bytes used for new iv2.
				* @retval E_SUCCESS
				*/
			SINT32 setIV2(const UINT8* p_iv)
				{
					memcpy(m_iv2,p_iv,16);
#ifdef SYM_CIPHER_CTR
					EVP_EncryptInit_ex(m_ctxAES2, EVP_aes_128_ctr(), NULL, key2, m_iv2);
#endif
					return E_SUCCESS;
				}

			SINT32 crypt1(const UINT8* in,UINT8* out,UINT32 len);
			SINT32 crypt2(const UINT8* in,UINT8* out,UINT32 len);
			SINT32 decrypt1CBCwithPKCS7(const UINT8* in,UINT8* out,UINT32* len);
			SINT32 encrypt1CBCwithPKCS7(const UINT8* in,UINT32 inlen,UINT8* out,UINT32* len);

			void setGCMKeys(UINT8* keyRecv, UINT8* keySend);
			SINT32 encryptMessage(const UINT8* in, UINT32 inlen, UINT8* out);
			SINT32 decryptMessage(const UINT8* in, UINT32 inlen, UINT8* out, bool integrityCheck);

			static SINT32 testSpeed();

		private:
			CAMutex* m_pcsEnc;
			CAMutex* m_pcsDec;
#ifndef USE_OPENSSL_GCM
			gcm_ctx_64k* m_pGCMCtxEnc;
			gcm_ctx_64k* m_pGCMCtxDec;
#else
			GCM128_CONTEXT* m_pGCMCtxEnc;
			GCM128_CONTEXT* m_pGCMCtxDec;
#endif
			UINT32 m_nEncMsgCounter;
			UINT32* m_pEncMsgIV;
			UINT32 m_nDecMsgCounter;
			UINT32* m_pDecMsgIV;

		protected:

#ifdef INTEL_IPP_CRYPTO
			IppsRijndael128Spec* m_keyAES1;
			IppsRijndael128Spec* m_keyAES2;
#else
			AES_KEY* m_keyAES1;
			AES_KEY* m_keyAES2;
#endif

#ifdef SYM_CIPHER_CTR
			EVP_CIPHER_CTX *m_ctxAES1;
			EVP_CIPHER_CTX *m_ctxAES2;
			UINT8 key1[16];
			UINT8 key2[16];
#endif


			UINT8* m_iv1;
			UINT8* m_iv2;
			bool m_bKeySet;
	};

#endif
