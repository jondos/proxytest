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
#include "CAUDPMixECCrypto.hpp"
#include "../CAUtil.hpp"

#define GENERATOR "04B70E0CBD6BB4BF7F321390B94A03C1D356C21122343280D6115C1D21BD376388B5F723FB4C22DFE6CD4375A05A07476444D5819985007E34"

CAUDPMixECCrypto::CAUDPMixECCrypto()
{
	m_ecgroupCurve=NULL;
	m_ecgroupCurve = EC_GROUP_new_by_curve_name(NID_secp224r1);
	BIGNUM *order = BN_new();
	BIGNUM *cofactor = BN_new();
	EC_POINT *generator = EC_POINT_new(m_ecgroupCurve);
	EC_POINT *ret = EC_POINT_hex2point(m_ecgroupCurve, GENERATOR, generator, NULL);
	EC_GROUP_get_order(m_ecgroupCurve, order, NULL);
	EC_GROUP_get_cofactor(m_ecgroupCurve, cofactor, NULL);
	EC_GROUP_set_generator(m_ecgroupCurve, generator, order, cofactor);
	UINT32 len;
	UINT8 *privbin = readFile((UINT8*)"C:/temp/Mix2.priv", &len);
	m_privKey = BN_new();
	BIGNUM *bret = BN_bin2bn(privbin, len, m_privKey);

}


SINT32 CAUDPMixECCrypto::getSharedSecretAndBlindedPublicKey(UDPMIXPACKET *pPacketIn, UDPMIXPACKET *pPacketOut, UINT8 *sharedSecret)
{
	EC_POINT *point = EC_POINT_new(m_ecgroupCurve);
	int ret=EC_POINT_oct2point(m_ecgroupCurve, point, pPacketIn->initHeader.eccPubElement, UDPMIXPACKET_INIT_ECC_PUB_ELEMENT_SIZE, NULL);
	EC_POINT *res = EC_POINT_new(m_ecgroupCurve);
	ret=EC_POINT_mul(m_ecgroupCurve, res, NULL, point, m_privKey, NULL);

	SHA256_CTX *sha256 = new SHA256_CTX;
	SHA256_Init(sha256);
	SHA256_Update(sha256,"aes_key:",8);
	UINT8 *buff;
	int len = EC_POINT_point2buf(m_ecgroupCurve, res, POINT_CONVERSION_UNCOMPRESSED, &buff, NULL);
	SHA256_Update(sha256, buff, len);
	UINT8 hash[20];
	SHA256_Final(hash,sha256);
	return E_SUCCESS;
}