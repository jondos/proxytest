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
#ifndef __CA_UTIL__
#define __CA_UTIL__
#include "CAASymCipher.hpp"

UINT32 strtrim(UINT8*);

SINT32 memtrim(UINT8* out,const UINT8* in,UINT32 len);

char* strins(const char* src,UINT32 pos,const char* ins);
char* strins(const char* src,const char * pos,const char* ins);

SINT32 getcurrentTimeMillis(BIGNUM *bnTime); 
SINT32 getcurrentTimeMillis(UINT64& u64Time);

SINT32 getRandom(UINT8* buff,UINT32 len);

SINT32 getRandom(UINT32* val);

SINT32 msSleep(UINT16 ms);

SINT32 sSleep(UINT16 sec);

UINT32 getMemoryUsage();

#ifndef _WIN32
SINT32 filelength(int handle);
#endif

SINT32 setDOMElementValue(DOM_Element& elem,UINT32 value);
SINT32 getDOMElementValue(DOM_Element& elem,UINT32* value);
SINT32 setDOMElementValue(DOM_Element& elem,UINT8* value);
SINT32 getDOMElementValue(DOM_Element& elem,UINT8* value,UINT32* len);

SINT32 setDOMElementAttribute(DOM_Element& elem,char* attr,int value);
SINT32 getDOMElementAttribute(DOM_Element& elem,char* attr,int* value);

SINT32 getDOMChildByName(const DOM_Node& node,const UINT8* const name,DOM_Node& child,bool deep=false);

SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, UINT8* xml, UINT32* xmllen,CAASymCipher* pRSA);
SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, const UINT8* const xml, UINT32 xmllen,CAASymCipher* pRSA);

inline void set64(UINT64& op1,UINT32 op2)
	{
#if !defined(_WIN32)&&!defined(__linux)
		op1.low=op2;
		op1.high=0;
#else
		op1=op2;
#endif
	}

inline void set64(UINT64& op1,UINT64 op2)
	{
#if !defined(_WIN32)&&!defined(__linux)
		op1.low=op2.low;
		op1.high=op2.high;
#else
		op1=op2;
#endif
	}

inline void add64(UINT64& op1,UINT32 op2)
	{
#if !defined(_WIN32)&&!defined(__linux)
		UINT32 t=op1.low;
		op1.low+=op2;
		if(op1.low<t)
			op1.high++;
#else
		op1+=op2;
#endif
	}

inline void inc64(UINT64& op1)
	{
#if !defined(_WIN32)&&!defined(__linux)
	op1.low++;
	if(op1.low==0)
		op1.high++;
#else
		op1++;
#endif
	}

inline UINT32 diff64(UINT64& bigop,UINT64& smallop)
	{
		#if !defined(_WIN32)&&!defined(__linux)
			return (UINT32) -1; //TODO!!!
		#else
			return (UINT32)(bigop-smallop);
		#endif
	}

inline UINT32 div64(UINT64& op1,UINT32 op2)
	{
#if !defined(_WIN32)&&!defined(__linux)
		return (UINT32) -1; //TODO!!!
#else
		return (UINT32)op1/op2;
#endif
	}

inline void print64(UINT8* buff,UINT64& op)
	{
#if defined(_WIN32)
		_ui64toa(op,(char*)buff,10);
#elif defined(__linux)
		sprintf((char*)buff,"%Lu",op);
#else
		sprintf((char*)buff,"(%lu:%lu)",op.high,op.low);
#endif
	}
#endif

UINT8* readFile(UINT8* name,UINT32* size);
