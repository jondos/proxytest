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
#include "CAASymCipher.hpp"

CAASymCipher::CAASymCipher()
	{
		rsa=NULL;
	}

CAASymCipher::~CAASymCipher()
	{
		destroy();
	}

SINT32 CAASymCipher::destroy()
	{
		RSA_free(rsa);
		rsa=NULL;
		return E_SUCCESS;
	}

/** Decrypts exactly one block which is stored in @c from. 
	*The result of the decryption is stored in @c to.
	*@param from one block of cipher text
	*@param to the decrypted plain text
	*@retval E_UNKNOWN in case of an error
	*@retval E_SUCCESS otherwise
	*/
SINT32 CAASymCipher::decrypt(UINT8* from,UINT8* to)
	{
		if(RSA_private_decrypt(128,from,to,rsa,RSA_NO_PADDING)==-1)
			return E_UNKNOWN;
		else
			return E_SUCCESS;
	}

/** Encrypts exactly one block which is stored in @c from. 
 *The result of the encrpytion is stored in @c to.
	*@param from one block of plain text
	*@param to the encrypted cipher text
	*@retval E_UNKNOWN in case of an error
	*@retval E_SUCCESS otherwise
	*/
SINT32 CAASymCipher::encrypt(UINT8* from,UINT8* to)
	{
		if(RSA_public_encrypt(128,from,to,rsa,RSA_NO_PADDING)==-1)
			return E_UNKNOWN;
		else
			return E_SUCCESS;
	}

/** Generates a new random key-pair of size bits.
	*@param size keysize of the new keypair
	*@retval E_UNKNOWN in case of an error
	*@retval E_SUCCESS otherwise
	*/
SINT32 CAASymCipher::generateKeyPair(UINT32 size)
	{
		RSA_free(rsa);
		rsa=RSA_generate_key(size,65537,NULL,NULL);
		if(rsa==NULL)
			return E_UNKNOWN;
		else
			return E_SUCCESS;
	}

/** Stores the public key in \c buff. The format is as follows:
	*
	* \li \c SIZE-N [2 bytes] - number of bytes which are needed for the modulus n (in network byte order..)
	* \li \c N [SIZE-N bytes] - the modulus \c n as integer (in network byte order)
	* \li \c SIZE-E [2 bytes] - number of bytes which are needed for the exponent e (in network byte order..)
	* \li \c E [SIZE-E bytes] - the exponent \c e as integer (in network byte order)
	*
	*@param buff byte array in which the public key should be stored
	*@param len on input holds the size of buff, on return it contains the number 
	*           of bytes needed to store the public key
	*@retval E_UNKNOWN in case of an error
	*@retval E_SUCCESS otherwise
	*@see getPublicKeySize
	*@see setPublicKey
	*/
SINT32 CAASymCipher::getPublicKey(UINT8* buff,UINT32 *len)
	{
		if(rsa==NULL||buff==NULL)
			return E_UNKNOWN;
		SINT32 keySize=getPublicKeySize();
		if(keySize<=0||(*len)<(UINT32)keySize)
			return E_UNKNOWN;
		UINT32 aktIndex=0;
		UINT16 size=htons(BN_num_bytes(rsa->n));
		memcpy(buff,&size,sizeof(size));
		aktIndex+=sizeof(size);
		BN_bn2bin(rsa->n,buff+aktIndex);
		aktIndex+=BN_num_bytes(rsa->n);
		size=htons(BN_num_bytes(rsa->e));
		memcpy(buff+aktIndex,&size,sizeof(size));
		aktIndex+=sizeof(size);
		BN_bn2bin(rsa->e,buff+aktIndex);
		aktIndex+=BN_num_bytes(rsa->e);
		*len=aktIndex;
		return E_SUCCESS;
	}

/** Returns the number of bytes needed to store we public key. This is the number of bytes needed for a
	* call of getPublicKey().
	*@return E_UNKOWN in case of an error
	*        number of bytes otherwise
	*@see getPublicKey
	*/
SINT32 CAASymCipher::getPublicKeySize()
	{
		if(rsa==NULL||rsa->n==NULL||rsa->e==NULL)
			return E_UNKNOWN;
		return (SINT32)BN_num_bytes(rsa->n)+BN_num_bytes(rsa->e)+4;
	}

/** Sets the public key to the vaules stored in \c key. The format must match the format described for getPublicKey(). 
	*@param key byte array which holds the new public key
	*@param len on input,size of key byte array, on successful return number of bytes 'consumed'
	*@retval E_UNKNOWN in case of an error, the cipher is the uninitialized (no key is set)
	*@retval E_SUCCESS otherwise
	*@see getPublicKey
	*/
SINT32 CAASymCipher::setPublicKey(UINT8* key,UINT32* len)
	{
		UINT32 aktIndex;
		UINT32 availBytes;
		UINT16 size;

		if(key==NULL||len==NULL||(*len)<6) //the need at least 6 bytes: 4 for the sizes and 1 for n and 1 for e
			goto _ERROR;
		availBytes=(*len);
		RSA_free(rsa);
		rsa=RSA_new();
		if(rsa==NULL)
			goto _ERROR;
		memcpy(&size,key,2);
		size=ntohs(size);
		availBytes-=2;
		if(size>availBytes-3) //the need at least 3 bytes for the exponent...
			goto _ERROR;
		aktIndex=2;
		availBytes-=size;
		rsa->n=BN_new();
		if(rsa->n==NULL)
			goto _ERROR;
		BN_bin2bn(key+aktIndex,size,rsa->n);
		aktIndex+=size;
		memcpy(&size,key+aktIndex,2);
		size=ntohs(size);
		availBytes-=2;
		aktIndex+=2;
		if(size>availBytes) 
			goto _ERROR;
		rsa->e=BN_new();
		BN_bin2bn(key+aktIndex,size,rsa->e);
		aktIndex+=size;
		(*len)=aktIndex;
		return E_SUCCESS;
_ERROR:
		RSA_free(rsa);
		rsa=NULL;
		return E_UNKNOWN;
	}
	