#include "StdAfx.h"
#include "CABase64.hpp"

int CABase64::decode(char* in,unsigned int inlen,char* out,unsigned int *outlen)
	{
		EVP_ENCODE_CTX oCTX;
		EVP_DecodeInit(&oCTX);
		int len;
		*outlen=0;
		EVP_DecodeUpdate(&oCTX,(unsigned char*)out,(int*)outlen,(unsigned char*)in,(int)inlen);
		EVP_DecodeFinal(&oCTX,(unsigned char*)out+(*outlen),&len);
		(*outlen)+=len;
		return 0;
	}

int CABase64::encode(char* in,unsigned int inlen,char* out,unsigned int *outlen)
	{
		EVP_ENCODE_CTX oCTX;
		EVP_EncodeInit(&oCTX);
		int len;
		*outlen=0;
		EVP_EncodeUpdate(&oCTX,(unsigned char*)out,(int*)outlen,(unsigned char*)in,(int)inlen);
		EVP_EncodeFinal(&oCTX,(unsigned char*)out+(*outlen),&len);
		(*outlen)+=len;
		return 0;
	}