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

/**
*	Removes leading and ending whitespaces (chars<32) from a string.
*		@param s input string (null terminated)
*		@return new size of string
*		
*/
SINT32 strtrim(UINT8* s)
	{
		UINT32 inpos=0;
		UINT32 outpos=0;
		if(s==NULL)
			return 0;
		UINT32 size=strlen((char*)s);
		while(inpos<size&&s[inpos]<32)
			inpos++;
		while(inpos<size&&s[inpos]>32)
			s[outpos++]=s[inpos++];
		s[outpos]=0;
		return outpos;
	}

/**
*	Removes leading and ending whitespaces (chars<32) from a byte array.
*		@param src input byte array
*		@param dest output byte array
*		@param size size of the input byte array
*		@return E_UNKNOWN, if an error occurs
*				 size of output otherwise
*/
SINT32 memtrim(UINT8* dest,const UINT8* src,UINT32 size)
	{
		UINT32 inpos=0;
		UINT32 outpos=0;
		if(src==NULL||size==0)
			return 0;
		if(dest==NULL)
			return E_UNKNOWN;
		while(inpos<size&&src[inpos]<32)
			inpos++;
		while(inpos<size&&src[inpos]>32)
			dest[outpos++]=src[inpos++];
		return (SINT32)outpos;
	}

/** Gets the current Systemtime in milli seconds. 
	* @param bnTime - Big Number, in which the current time is placed
	* @return E_UNKNOWN, if an error occurs
	*					E_SUCCESS, otherwise
*/
SINT32 getcurrentTimeMillis(BIGNUM* bnTime)
	{
		#ifdef _WIN32
			struct _timeb timebuffer;
			_ftime(&timebuffer);
			/* Hack what should be solved better...*/
			BN_set_word(bnTime,timebuffer.time);
			BN_mul_word(bnTime,1000);
			BN_add_word(bnTime,timebuffer.millitm);
			/* end of hack..*/
			return E_SUCCESS;
	  #else //we dont use ftime due to a bug in glibc2.0
		//we use gettimeofday() in order to get the millis...
			struct timeval tv;
			gettimeofday(&tv,NULL); //getting millis...
			BN_set_word(bnTime,tv.tv_sec);
			BN_mul_word(bnTime,1000);
			BN_add_word(bnTime,tv.tv_usec/1000);
			return E_SUCCESS;
	  #endif
	}