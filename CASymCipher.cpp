#include "StdAfx.h"
#include "CASymCipher.hpp"

SINT32 CASymCipher::setEncryptionKey(UINT8* key)
	{
		BF_set_key(&keyEnc,16,key);
		memcpy(rawKeyEnc,key,16);
		bEncKeySet=true;
		return E_SUCCESS;
	}

bool CASymCipher::isEncyptionKeyValid()
	{
		return bEncKeySet;
	}

SINT32 CASymCipher::generateEncryptionKey()
	{
		UINT8 key[16];
		RAND_bytes(key,16);
		return setEncryptionKey(key);
	}

SINT32 CASymCipher::getEncryptionKey(UINT8* key)
	{
		if(bEncKeySet)
			{
				memcpy(key,rawKeyEnc,16);		
				return E_SUCCESS;
			}
		else
			return E_UNKNOWN;
	}

SINT32 CASymCipher::setDecryptionKey(UINT8* key)
	{
		BF_set_key(&keyDec,16,key);
		return E_SUCCESS;
	}

SINT32 CASymCipher::encrypt(UINT8* in,UINT32 len)
	{
		for(UINT32 i=0;i<len;i+=8)
			BF_ecb_encrypt(in+i,in+i,&keyEnc,BF_ENCRYPT);
		return E_SUCCESS;
	}

SINT32 CASymCipher::decrypt(UINT8* in,UINT8* out,UINT32 len)
	{
		for(UINT32 i=0;i<len;i+=8)
			BF_ecb_encrypt(in+i,out+i,&keyDec,BF_DECRYPT);
		return E_SUCCESS;
	}
	