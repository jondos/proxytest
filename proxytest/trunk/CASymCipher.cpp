#include "StdAfx.h"
#include "CASymCipher.hpp"

int CASymCipher::setEncryptionKey(unsigned char* key)
	{
		BF_set_key(&keyEnc,16,key);
		memcpy(rawKeyEnc,key,16);
		bEncKeySet=true;
		return 0;
	}

bool CASymCipher::isEncyptionKeyValid()
	{
		return bEncKeySet;
	}

int CASymCipher::generateEncryptionKey()
	{
		unsigned char key[16];
		RAND_bytes(key,16);
		return setEncryptionKey(key);
	}

int CASymCipher::getEncryptionKey(unsigned char* key)
	{
		if(bEncKeySet)
			{
				memcpy(key,rawKeyEnc,16);		
				return 0;
			}
		else
			return -1;
	}

int CASymCipher::setDecryptionKey(unsigned char* key)
	{
		BF_set_key(&keyDec,16,key);
		return 0;
	}

int CASymCipher::encrypt(unsigned char* in,int len)
	{
//		unsigned char ivec[16];
//		memset(ivec,0,sizeof(ivec));
		//BF_cbc_encrypt(in,in,len,&keyEnc,ivec,1);
		for(int i=0;i<len;i+=8)
			BF_ecb_encrypt(in+i,in+i,&keyEnc,BF_ENCRYPT);
		return 0;
	}

int CASymCipher::decrypt(unsigned char* in,unsigned char* out,int len)
	{
//		unsigned char ivec[16];
//		memset(ivec,0,sizeof(ivec));
//		BF_cbc_encrypt(in,in,len,&keyDec,ivec,0);
		for(int i=0;i<len;i+=8)
			BF_ecb_encrypt(in+i,out+i,&keyDec,BF_DECRYPT);
		return 0;
	}
	