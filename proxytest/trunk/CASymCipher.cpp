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
//AES

/** Sets the key used for encryption/decryption. Also resets the IV to zero!
	* @param key 16 random bytes used as key 
	* @retval E_SUCCESS
	*/
SINT32 CASymCipher::setKey(const UINT8* key)
	{
		makeKey(*m_keyAES,(char*)key);
		memset(m_iv1,0,16);
		memset(m_iv2,0,16);
		m_bKeySet=true;
		return E_SUCCESS;
	}

/** Encryptes/Decrpytes in to out using iv1. AES is used for encryption and the encryption
	* is done with a special
	* 128bit-OFB mode: In the case that (len mod 16 !=0) the unused cipher
	* output bits are discarded and NOT used next time encryptAES() is called.
	* That means that every time encrpytAES() is called at first new cipher output
	* is created by calling AES-encrypt(iv).
	* @param in input (plain text) bytes
	* @param out output (encrpyted) bytes
	* @param len len of input. because the cipher preserves the size, 
	*													len of output=len of input
	* @retval E_SUCCESS
	*/
SINT32 CASymCipher::crypt1(const UINT8* in,UINT8* out,UINT32 len)
	{
		UINT32 i=0;
    while(i+15<len)
    	{
				rijndaelEncrypt (m_iv1, m_iv1, *m_keyAES);
				out[i]=in[i]^m_iv1[0];
				i++;
				out[i]=in[i]^m_iv1[1];
				i++;
				out[i]=in[i]^m_iv1[2];
				i++;
				out[i]=in[i]^m_iv1[3];
				i++;
				out[i]=in[i]^m_iv1[4];
				i++;
				out[i]=in[i]^m_iv1[5];
				i++;
				out[i]=in[i]^m_iv1[6];
				i++;
				out[i]=in[i]^m_iv1[7];
				i++;
				out[i]=in[i]^m_iv1[8];
				i++;
				out[i]=in[i]^m_iv1[9];
				i++;
				out[i]=in[i]^m_iv1[10];
				i++;
				out[i]=in[i]^m_iv1[11];
				i++;
				out[i]=in[i]^m_iv1[12];
				i++;
				out[i]=in[i]^m_iv1[13];
				i++;
				out[i]=in[i]^m_iv1[14];
				i++;
				out[i]=in[i]^m_iv1[15];
				i++;
			}
		if(i<len) //In this case len-i<16 !
			{
				rijndaelEncrypt (m_iv1, m_iv1, *m_keyAES);
				len-=i;
				for(UINT32 k=0;k<len;k++)
				 {
					 out[i]=in[i]^m_iv1[k];
					 i++;
					}
			}
		return E_SUCCESS;
	}

/** Decryptes in to out using iv2.
	* @param in input (encrypted) bytes
	* @param out output (decrpyted) bytes
	* @param len len of input. because the cipher preserves the size, 
	*													len of output=len of input
	* @retval E_SUCCESS
	*/
SINT32 CASymCipher::crypt2(const UINT8* in,UINT8* out,UINT32 len)
	{
		UINT32 i=0;
		while(i+15<len)
			{
				rijndaelEncrypt (m_iv2, m_iv2, *m_keyAES);
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
				rijndaelEncrypt (m_iv2, m_iv2, *m_keyAES);
				len-=i;
				for(UINT32 k=0;k<len;k++)
				 {
					 out[i]=in[i]^m_iv2[k];
					 i++;
					}
			}
		return E_SUCCESS;
	}
/*
SINT32 CASymCipher::encrypt1(const UINT8* in,UINT8* out,UINT32 len)
	{
		UINT32 i=0;
		while(i+15<len)
			{
				rijndaelEncrypt (m_iv1, m_iv1, *m_keyAES);

				out[i]=in[i]^m_iv1[0];
				i++;
				out[i]=in[i]^m_iv1[1];
				i++;
				out[i]=in[i]^m_iv1[2];
				i++;
				out[i]=in[i]^m_iv1[3];
				i++;
				out[i]=in[i]^m_iv1[4];
				i++;
				out[i]=in[i]^m_iv1[5];
				i++;
				out[i]=in[i]^m_iv1[6];
				i++;
				out[i]=in[i]^m_iv1[7];
				i++;
				out[i]=in[i]^m_iv1[8];
				i++;
				out[i]=in[i]^m_iv1[9];
				i++;
				out[i]=in[i]^m_iv1[10];
				i++;
				out[i]=in[i]^m_iv1[11];
				i++;
				out[i]=in[i]^m_iv1[12];
				i++;
				out[i]=in[i]^m_iv1[13];
				i++;
				out[i]=in[i]^m_iv1[14];
				i++;
				out[i]=in[i]^m_iv1[15];
				i++;
			}
		if(i<len)
			{
				rijndaelEncrypt (m_iv1, m_iv1, *m_keyAES);
				len-=i;
				for(UINT32 k=0;k<len;k++)
				 {
					 out[i]=in[i]^m_iv1[k];
					 i++;
					}
			}
		return E_SUCCESS;
	}
*/