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
//#define AES_NI
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

			/** Sets the keys for crypt1() and crypt2() to the same key*/
			SINT32 setKey(const UINT8* key);	
			

			void setGCMKeys(UINT8* keyRecv, UINT8* keySend);
			SINT32 encryptMessage(const UINT8* in, UINT32 inlen, UINT8* out);
			SINT32 decryptMessage(const UINT8* in, UINT32 inlen, UINT8* out, bool integrityCheck);


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
