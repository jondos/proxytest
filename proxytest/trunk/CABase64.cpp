#include "StdAfx.h"
#include "CABase64.hpp"

/** Makes a Base64-Decoding on the input byte array. It decodes in to out.
*	@param in input byte array
*	@param inlen size of the input byte array
*	@param out output byte array
*	@param outlen on input must contain the size of the byte array for output,
	              on return it contains the size of the decoded output
*	@return E_SUCCESS, if no error occurs
*	        E_UNKNOWN, if an error occurs
*/ 
SINT32 CABase64::decode(UINT8* in,UINT32 inlen,UINT8* out,UINT32 *outlen)
	{
		if(outlen==NULL)
			return E_UNKNOWN;
		if(in==NULL||inlen==0)
			{
				*outlen=0;
				return E_SUCCESS;
			}
		EVP_ENCODE_CTX oCTX;
		EVP_DecodeInit(&oCTX);
		UINT32 len;
		*outlen=0;
		if(EVP_DecodeUpdate(&oCTX,out,(int*)outlen,in,(int)inlen)==-1)
			return E_UNKNOWN;
		if(EVP_DecodeFinal(&oCTX,out+(*outlen),(int*)&len)==-1)
			return E_UNKNOWN;
		(*outlen)+=len;
		return E_SUCCESS;
	}

/** Makes a Base64-Encoding on the input byte array. It encodes in to out.
*	@param in input byte array
*	@param inlen size of the input byte array
*	@param out output byte array
*	@param outlen on input must contain the size of the byte array for output,
	              on return it contains the size of the decoded output
*	@return E_SUCCESS, if no error occurs
*	        E_UNKNOWN, if an error occurs
*
*/ 
SINT32 CABase64::encode(UINT8* in,UINT32 inlen,UINT8* out,UINT32 *outlen)
	{
		if(outlen==NULL)
			return E_UNKNOWN;
		if(in==NULL||inlen==0)
			{
				*outlen=0;
				return E_SUCCESS;
			}
		if(inlen>*outlen)
			return E_UNKNOWN;
		EVP_ENCODE_CTX oCTX;
		EVP_EncodeInit(&oCTX);
		UINT32 len;
		*outlen=0;
		EVP_EncodeUpdate(&oCTX,out,(int*)outlen,in,(int)inlen);
		EVP_EncodeFinal(&oCTX,out+(*outlen),&len);
		(*outlen)+=len;
		return E_SUCCESS;
	}
