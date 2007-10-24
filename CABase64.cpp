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
#include "CABase64.hpp"

/** ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
 Makes a Base64-Decoding on the input byte array. It decodes @c in to @c out.
	@param in input byte array
	@param inlen size of the input byte array
	@param out output byte array
	@param outlen on input must contain the size of the byte array for output,
	              on return it contains the size of the decoded output
	@retval E_SUCCESS if no error occurs
	@retval E_UNKNOWN if an error occurs
fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff */
SINT32 CABase64::decode(const UINT8*in, UINT32 inlen, UINT8*out, UINT32*outlen)
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
	int len=0;
	*outlen=0;
	//ensure that in and out are disjunct - otherwise copy in
	if(((out>=in)&&(in+inlen>out))||((out<in)&&(out+*outlen>in)))
	{
		UINT8*tmpIn=new UINT8[inlen];
		memcpy(tmpIn, in, inlen);
		EVP_DecodeUpdate(&oCTX, out, (int*) outlen, tmpIn, (int)inlen);
		delete[] tmpIn;
	}
	else
	{
		EVP_DecodeUpdate(&oCTX, out, (int*) outlen, (unsigned char*) in, (int)inlen);
	}
	EVP_DecodeFinal(&oCTX, out+(*outlen), &len);
	(*outlen)+=len;
	return E_SUCCESS;
}

/** ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
 Makes a Base64-Encoding on the input byte array. It encodes @c in to @c out.
	@param in input byte array
	@param inlen size of the input byte array
	@param out output byte array
	@param outlen on input must contain the size of the byte array for output,
	              on return it contains the size of the encoded output
	@retval E_SUCCESS if no error occurs
	@retval E_UNKNOWN if an error occurs

fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff */
SINT32 CABase64::encode(const UINT8*in, UINT32 inlen, UINT8*out, UINT32*outlen)
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

	//ensure that in and out are disjunct - otherwise copy in
	if(((out>=in)&&(in+inlen>out))||((out<in)&&(out+*outlen>in)))
	{
		UINT8*tmpIn=new UINT8[inlen];
		memcpy(tmpIn, in, inlen);
		EVP_EncodeUpdate(&oCTX, out, (int*) outlen, tmpIn, (int)inlen);
		delete[] tmpIn;
	}
	else
	{
		EVP_EncodeUpdate(&oCTX, out, (int*) outlen, (unsigned char*) in, (int)inlen);
	}
	EVP_EncodeFinal(&oCTX, out+(*outlen), (int*)&len);
	(*outlen)+=len;
	return E_SUCCESS;
}
