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

SINT32 CASymCipher::setKeyAES(UINT8* key)
	{
		makeKey(*m_keyAES,(char*)key);
		memset(m_iv,0,16);
		memset(m_iv2,0,16);
		m_bEncKeySet=true;
		return E_SUCCESS;
	}


SINT32 CASymCipher::decryptAES(UINT8* in,UINT8* out,UINT32 len)
	{
		UINT32 i=0;
    while(i+15<len)
    	{
				rijndaelEncrypt (m_iv, m_iv, *m_keyAES);
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
				rijndaelEncrypt (m_iv, m_iv, *m_keyAES);
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

SINT32 CASymCipher::encryptAES(UINT8* in,UINT8* out,UINT32 len)
	{
		UINT32 i=0;
		while(i+15<len)
			{
				rijndaelEncrypt (m_iv, m_iv, *m_keyAES);

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
				rijndaelEncrypt (m_iv, m_iv, *m_keyAES);
				len-=i;
				for(UINT32 k=0;k<len;k++)
				 {
					 out[i]=in[i]^m_iv[k];
					 i++;
					}
			}
		return E_SUCCESS;
	}
