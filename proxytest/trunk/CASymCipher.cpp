#include "StdAfx.h"
#include "CASymCipher.hpp"

int CASymCipher::setEncryptionKey(unsigned char* key)
	{
		BF_set_key(&keyEnc,16,key);
		return 0;
	}

int CASymCipher::setDecryptionKey(unsigned char* key)
	{
		BF_set_key(&keyDec,16,key);
		return 0;
	}

int CASymCipher::encrypt(unsigned char* in,int len)
	{
		unsigned char ivec[16];
		memset(ivec,0,sizeof(ivec));
		BF_cbc_encrypt(in,in,len,&keyEnc,ivec,1);
		return 0;
	}

int CASymCipher::decrypt(unsigned char* in,int len)
	{
		unsigned char ivec[16];
		memset(ivec,0,sizeof(ivec));
		BF_cbc_encrypt(in,in,len,&keyDec,ivec,0);
		return 0;
	}
	