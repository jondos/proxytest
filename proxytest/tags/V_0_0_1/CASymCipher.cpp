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
//		unsigned char ivec[16];
//		memset(ivec,0,sizeof(ivec));
		//BF_cbc_encrypt(in,in,len,&keyEnc,ivec,1);
		for(int i=0;i<len;i+=8)
			BF_ecb_encrypt(in+i,in+i,&keyEnc,BF_ENCRYPT);
		return 0;
	}

int CASymCipher::decrypt(unsigned char* in,int len)
	{
//		unsigned char ivec[16];
//		memset(ivec,0,sizeof(ivec));
//		BF_cbc_encrypt(in,in,len,&keyDec,ivec,0);
		for(int i=0;i<len;i+=8)
			BF_ecb_encrypt(in+i,in+i,&keyDec,BF_DECRYPT);
		return 0;
	}
	