#include "StdAfx.h"
#include "CABase64.hpp"

SINT32 CABase64::decode(UINT8* in,UINT32 inlen,UINT8* out,UINT32 *outlen)
	{
		EVP_ENCODE_CTX oCTX;
		EVP_DecodeInit(&oCTX);
		UINT32 len;
		*outlen=0;
		EVP_DecodeUpdate(&oCTX,out,(int*)outlen,in,(int)inlen);
		EVP_DecodeFinal(&oCTX,out+(*outlen),(int*)&len);
		(*outlen)+=len;
		return E_SUCCESS;
	}

SINT32 CABase64::encode(UINT8* in,UINT32 inlen,UINT8* out,UINT32 *outlen)
	{
		EVP_ENCODE_CTX oCTX;
		EVP_EncodeInit(&oCTX);
		int len;
		*outlen=0;
		EVP_EncodeUpdate(&oCTX,out,(int*)outlen,in,(int)inlen);
		EVP_EncodeFinal(&oCTX,out+(*outlen),&len);
		(*outlen)+=len;
		return E_SUCCESS;
	}
