#include "StdAfx.h"
#include "CAASymCipher.hpp"

CAASymCipher::CAASymCipher()
	{
		rsa=NULL;
	}

CAASymCipher::~CAASymCipher()
	{
		RSA_free(rsa);
	}

/** Decrypts exactly one block which is stored in from. The result of the decryption is stored in to.
	*@param from one block of cipher text
	*@param to the decrypted plain text
	*@return E_UNKNOWN in case of an error
	*        E_SUCCESS otherwise
	*/
SINT32 CAASymCipher::decrypt(UINT8* from,UINT8* to)
	{
		if(RSA_private_decrypt(128,from,to,rsa,RSA_NO_PADDING)==-1)
			return E_UNKNOWN;
		else
			return E_SUCCESS;
	}

/** Encrypts exactly one block which is stored in from. The result of the encrpytion is stored in to.
	*@param from one block of plain text
	*@param to the encrypted cipher text
	*@return E_UNKNOWN in case of an error
	*        E_SUCCESS otherwise
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
	*@return E_UNKNOWN in case of an error
	*        E_SUCCESS otherwise
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

/** Stores the public key in buff. The format is as follows:
	*
	* SIZE-N [2 bytes] - number of bytes which are needed for the modulus n (in network byte order..)
	* N [SIZE-N bytes] - the modulus n as integer (in network byte order)
	* SIZE-E [2 bytes] - number of bytes which are needed for the exponent e (in network byte order..)
	* E [SIZE-E bytes] - the exponent e as integer (in network byte order)
	*
	*@param buff byte array in which the public key should be stored
	*@param len on input holds the size of buff, on return it contains the number 
	*           of bytes needed to store the public key
	*@return E_UNKNOWN in case of an error
	*        E_SUCCESS otherwise
	*@see getPublicKeysize
	*@see setPublicKey
	*/
SINT32 CAASymCipher::getPublicKey(UINT8* buff,UINT32 *len)
	{
		if(rsa==NULL||buff==NULL||(*len)<getPublicKeySize())
			return E_UNKNOWN;
		int aktIndex=0;
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

/** Sets the public key to the vaules sotred in key. The format must match the format described for getPublicKey(). 
	*@param key byte array which holds the new public key
	*@param len on input,size of key byte array, on successful return number of bytes 'consumed'
	*@return E_UNKNOWN in case of an error, the cipher is the uninitialized (no key is set)
	*        E_SUCCESS otherwise
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
	