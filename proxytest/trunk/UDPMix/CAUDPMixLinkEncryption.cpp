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
#include "../StdAfx.h"
#include "CAUDPMixLinkEncryption.hpp"

CAUDPMixLinkEncryption::CAUDPMixLinkEncryption()
{
	m_bKeySet = false;

	m_nEncMsgCounter = 0;
	m_pEncMsgIV = new UINT32[3];
	memset(m_pEncMsgIV, 0, 12);
	m_pDecMsgIV = new UINT32[3];
	memset(m_pDecMsgIV, 0, 12);
	m_pGCMCtxEnc = NULL;
	m_pGCMCtxDec = NULL;
}

	CAUDPMixLinkEncryption::~CAUDPMixLinkEncryption()
{

	delete[] m_pEncMsgIV;
	m_pEncMsgIV = NULL;
	delete[] m_pDecMsgIV;
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

}

SINT32 CAUDPMixLinkEncryption::setKeys(UINT8* keySend,UINT8* keyRecv)
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
	memset(m_pDecMsgIV, 0, 12);
	return E_SUCCESS;
}

SINT32 CAUDPMixLinkEncryption::encrypt(UDPMIXPACKET *plainPacketIn, UDPMIXPACKET *encPacketOut)
{
#ifdef NO_ENCRYPTION
	memmove(out, in, inlen);
	return E_SUCCESS;
#endif

	m_pEncMsgIV[0] = htonl(m_nEncMsgCounter);
	m_nEncMsgCounter++;
	memcpy(encPacketOut->linkHeader.aru8__LinkCounterPrefix, m_pEncMsgIV, UDPMIXPACKET_LINKHEADER_COUNTER_SIZE);
#ifndef USE_OPENSSL_GCM
	gcm_encrypt_64k(m_pGCMCtxEnc, m_pEncMsgIV, (UINT8 *)&plainPacketIn->linkHeader.channelID, UDPMIXPACKET_LINKHEADER_HEADERFIELDS_SIZE, (UINT8 *)&encPacketOut->linkHeader.channelID,(UINT32*) &encPacketOut->linkHeader.MAC);
#else
	CRYPTO_gcm128_setiv(m_pGCMCtxEnc, (UINT8 *)m_pEncMsgIV, 12);
	CRYPTO_gcm128_encrypt(m_pGCMCtxEnc, in, out, inlen);
	CRYPTO_gcm128_tag(m_pGCMCtxEnc, out + inlen, 16);
#endif
	return E_SUCCESS;
	}

SINT32 CAUDPMixLinkEncryption::decrypt(UDPMIXPACKET *encPacketIn, UDPMIXPACKET *decPacketOut)
	{
#ifdef NO_ENCRYPTION
	memmove(out, in, inlen);
	return E_SUCCESS;
#endif

	SINT32 ret = E_UNKNOWN;
	memcpy(m_pDecMsgIV, encPacketIn->linkHeader.aru8__LinkCounterPrefix, UDPMIXPACKET_LINKHEADER_COUNTER_SIZE);
#ifndef USE_OPENSSL_GCM
	ret = ::gcm_decrypt_64k(m_pGCMCtxDec, m_pDecMsgIV, ((UINT8 *)&encPacketIn->linkHeader.channelID), UDPMIXPACKET_LINKHEADER_HEADERFIELDS_SIZE, ((UINT8 *)&encPacketIn->linkHeader.MAC), (UINT8 *)&decPacketOut->linkHeader.channelID);
#else
	CRYPTO_gcm128_setiv(m_pGCMCtxDec, (UINT8 *)m_pDecMsgIV, 12);
	CRYPTO_gcm128_decrypt(m_pGCMCtxDec, in, out, inlen - 16);
	ret = CRYPTO_gcm128_finish(m_pGCMCtxDec, in + inlen - 16, 16);
#endif

#ifndef USE_OPENSSL_GCM
	if (ret == 0)
#else
	if (ret != 0)
#endif
		{
			return E_UNKNOWN;
		}
	return E_SUCCESS;
	}
