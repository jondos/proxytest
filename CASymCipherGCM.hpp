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
#ifndef __CASYMCIPHERGCM__
#define __CASYMCIPHERGCM__

#define KEY_SIZE 16

#include "CALockAble.hpp"
#include "CAMutex.hpp"
/** This class could be used for encryption/decryption of data (streams) with
	* AES using 128bit GCM mode.
	*/
class CASymCipherGCM
#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_MIDDLE_MIX
	:public CALockAble
#endif
	{
		public:
			CASymCipherGCM()
				{
					m_bKeySet=false;


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

			~CASymCipherGCM()
				{
#ifndef ONLY_LOCAL_PROXY
					waitForDestroy();
#endif

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

			void setGCMKeys(UINT8* keyRecv, UINT8* keySend)
			{

#ifndef USE_OPENSSL_GCM
				m_pGCMCtxEnc = new gcm_ctx_64k;
				m_pGCMCtxDec = new gcm_ctx_64k;
#else
				//Note the have to provide *some* key (OpenSSL API enforced --> so the use the variables we have anyway..)
				// The Key will be overriden by a call to setKeyGCM in any case!
				AES_set_encrypt_key(m_iv1, 128, m_keyAES1);
				m_pGCMCtxEnc = CRYPTO_gcm128_new(m_keyAES1, (block128_f)AES_encrypt);
				m_pGCMCtxDec = CRYPTO_gcm128_new(m_keyAES1, (block128_f)AES_encrypt);
#endif



#ifndef USE_OPENSSL_GCM
				if (m_pGCMCtxDec != NULL)
					delete m_pGCMCtxDec;
				if (m_pGCMCtxEnc != NULL)
					delete m_pGCMCtxEnc;

				m_pGCMCtxEnc = new gcm_ctx_64k;
				m_pGCMCtxDec = new gcm_ctx_64k;
				gcm_init_64k(m_pGCMCtxEnc, keySend, 128);
				gcm_init_64k(m_pGCMCtxDec, keyRecv, 128);
#else
				AES_set_encrypt_key(keyRecv, 128, m_keyAES1);
				AES_set_encrypt_key(keySend, 128, m_keyAES2);
				CRYPTO_gcm128_release(m_pGCMCtxEnc);
				CRYPTO_gcm128_release(m_pGCMCtxDec);
				m_pGCMCtxEnc = CRYPTO_gcm128_new(m_keyAES2, (block128_f)AES_encrypt);
				m_pGCMCtxDec = CRYPTO_gcm128_new(m_keyAES1, (block128_f)AES_encrypt);
#endif
				//reset IV
				m_nEncMsgCounter = 0;
				memset(m_pEncMsgIV, 0, 12);
				m_nDecMsgCounter = 0;
				memset(m_pDecMsgIV, 0, 12);

			}
			SINT32 encryptMessage(const UINT8* in, UINT32 inlen, UINT8* out)
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
				CRYPTO_gcm128_setiv(m_pGCMCtxEnc, (UINT8*)m_pEncMsgIV, 12);
				CRYPTO_gcm128_encrypt(m_pGCMCtxEnc, in, out, inlen);
				CRYPTO_gcm128_tag(m_pGCMCtxEnc, out + inlen, 16);
#endif
				//m_pcsEnc->unlock();
				return E_SUCCESS;
			}
			SINT32 decryptMessage(const UINT8* in, UINT32 inlen, UINT8* out, bool integrityCheck)
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
					CRYPTO_gcm128_setiv(m_pGCMCtxDec, (UINT8*)m_pDecMsgIV, 12);
					CRYPTO_gcm128_decrypt(m_pGCMCtxDec, in, out, inlen - 16);
					ret = CRYPTO_gcm128_finish(m_pGCMCtxDec, in + inlen - 16, 16);
#endif
				}
				else
				{
#ifndef USE_OPENSSL_GCM
					ret = ::gcm_decrypt_64k(m_pGCMCtxDec, m_pDecMsgIV, in, inlen, out);
#else
					CRYPTO_gcm128_setiv(m_pGCMCtxDec, (UINT8*)m_pDecMsgIV, 12);
					ret = CRYPTO_gcm128_decrypt(m_pGCMCtxDec, in, out, inlen);
#endif
				}
				//m_pcsDec->unlock();
#ifndef USE_OPENSSL_GCM
				if (ret == 0)
#else
				if (ret != 0)
#endif
					return E_UNKNOWN;
				return E_SUCCESS;

			}


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
			bool m_bKeySet;
	};

#endif
