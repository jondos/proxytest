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
#include "CASymCipher.hpp"

/*SINT32 CASymCipher::setEncryptionKey(UINT8* key)
	{
		BF_set_key(&keyEnc,16,key);
		memcpy(rawKeyEnc,key,16);
		bEncKeySet=true;
		return E_SUCCESS;
	}
*/
bool CASymCipher::isEncyptionKeyValid()
	{
		return m_bEncKeySet;
	}

/*
SINT32 CASymCipher::generateEncryptionKey()
	{
		UINT8 key[16];
		RAND_bytes(key,16);
		return setKeyAES(key);
	}
*/
/*SINT32 CASymCipher::getEncryptionKey(UINT8* key)
	{
		if(bEncKeySet)
			{
				memcpy(key,rawKeyEnc,16);		
				return E_SUCCESS;
			}
		else
			return E_UNKNOWN;
	}
*//*
SINT32 CASymCipher::setDecryptionKey(UINT8* key)
	{
		BF_set_key(&keyDec,16,key);
		return E_SUCCESS;
	}
*/
/*SINT32 CASymCipher::encrypt(UINT8* in,UINT32 len)
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
*/	

//AES
/*
SINT32 CASymCipher::setEncryptionKeyAES(UINT8* key)
	{
		makeKey(&keyEncAES,DIR_ENCRYPT,KEY_SIZE*8,(char*)key);
		memcpy(rawKeyEnc,key,16);
		bEncKeySet=true;
		memset(iv,0,16);
		return E_SUCCESS;
	}
*/

SINT32 CASymCipher::setKeyAES(UINT8* key)
	{
		makeKey(&m_keyAES,/*DIR_ENCRYPT,*/KEY_SIZE*8,(char*)key);
		memset(m_iv,0,16);
		memset(m_iv2,0,16);
		m_bEncKeySet=true;
		return E_SUCCESS;
	}


SINT32 CASymCipher::decryptAES(UINT8* in,UINT8* out,UINT32 len)
	{
		UINT32 i=0;
		//while(i<len-15)
    while(i+15<len)
    	{
				rijndaelEncrypt (m_iv, m_iv, m_keyAES.keySched);
				out[i]=in[i]^m_iv[0];
				i++;
				out[i]=in[i]^m_iv[1];
				i++;
				out[i]=in[i]^m_iv[2];
				i++;
				out[i]=in[i]^m_iv[3];
				i++;
				out[i]=in[i]^m_iv[4];
				i++;
				out[i]=in[i]^m_iv[5];
				i++;
				out[i]=in[i]^m_iv[6];
				i++;
				out[i]=in[i]^m_iv[7];
				i++;
				out[i]=in[i]^m_iv[8];
				i++;
				out[i]=in[i]^m_iv[9];
				i++;
				out[i]=in[i]^m_iv[10];
				i++;
				out[i]=in[i]^m_iv[11];
				i++;
				out[i]=in[i]^m_iv[12];
				i++;
				out[i]=in[i]^m_iv[13];
				i++;
				out[i]=in[i]^m_iv[14];
				i++;
				out[i]=in[i]^m_iv[15];
				i++;
			}
		if(i<len) //In this case len-i<16 !
			{
				rijndaelEncrypt (m_iv, m_iv, m_keyAES.keySched);
				len-=i;
				for(UINT32 k=0;k<len;k++)
				 {
					 out[i]=in[i]^m_iv[k];
					 i++;
					}
			}
		return E_SUCCESS;
	}

SINT32 CASymCipher::decryptAES2(UINT8* in,UINT8* out,UINT32 len)
	{
		UINT32 i=0;
		while(i+15<len)
			{
				rijndaelEncrypt (m_iv2, m_iv2, m_keyAES.keySched);
				out[i]=in[i]^m_iv2[0];
				i++;
				out[i]=in[i]^m_iv2[1];
				i++;
				out[i]=in[i]^m_iv2[2];
				i++;
				out[i]=in[i]^m_iv2[3];
				i++;
				out[i]=in[i]^m_iv2[4];
				i++;
				out[i]=in[i]^m_iv2[5];
				i++;
				out[i]=in[i]^m_iv2[6];
				i++;
				out[i]=in[i]^m_iv2[7];
				i++;
				out[i]=in[i]^m_iv2[8];
				i++;
				out[i]=in[i]^m_iv2[9];
				i++;
				out[i]=in[i]^m_iv2[10];
				i++;
				out[i]=in[i]^m_iv2[11];
				i++;
				out[i]=in[i]^m_iv2[12];
				i++;
				out[i]=in[i]^m_iv2[13];
				i++;
				out[i]=in[i]^m_iv2[14];
				i++;
				out[i]=in[i]^m_iv2[15];
				i++;
			}
		if(i<len)
			{
				rijndaelEncrypt (m_iv2, m_iv2, m_keyAES.keySched);
				len-=i;
				for(UINT32 k=0;k<len;k++)
				 {
					 out[i]=in[i]^m_iv2[k];
					 i++;
					}
			}
		return E_SUCCESS;
	}

SINT32 CASymCipher::encryptAES(UINT8* in,UINT8* out,UINT32 len)
	{
		UINT32 i=0;
		while(i+15<len)
			{
				rijndaelEncrypt (m_iv, m_iv, m_keyAES.keySched);

				out[i]=in[i]^m_iv[0];
				i++;
				out[i]=in[i]^m_iv[1];
				i++;
				out[i]=in[i]^m_iv[2];
				i++;
				out[i]=in[i]^m_iv[3];
				i++;
				out[i]=in[i]^m_iv[4];
				i++;
				out[i]=in[i]^m_iv[5];
				i++;
				out[i]=in[i]^m_iv[6];
				i++;
				out[i]=in[i]^m_iv[7];
				i++;
				out[i]=in[i]^m_iv[8];
				i++;
				out[i]=in[i]^m_iv[9];
				i++;
				out[i]=in[i]^m_iv[10];
				i++;
				out[i]=in[i]^m_iv[11];
				i++;
				out[i]=in[i]^m_iv[12];
				i++;
				out[i]=in[i]^m_iv[13];
				i++;
				out[i]=in[i]^m_iv[14];
				i++;
				out[i]=in[i]^m_iv[15];
				i++;
			}
		if(i<len)
			{
				rijndaelEncrypt (m_iv, m_iv, m_keyAES.keySched);
				len-=i;
				for(UINT32 k=0;k<len;k++)
				 {
					 out[i]=in[i]^m_iv[k];
					 i++;
					}
			}
		return E_SUCCESS;
	}

	/*
SINT32 CASymCipher::generateEncryptionKeyAES()
	{
		UINT8 key[16];
		RAND_bytes(key,16);
		return setEncryptionKeyAES(key);
	}
*/