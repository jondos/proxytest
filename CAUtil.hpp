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

SINT32 getcurrentTime(timespec& t); 
SINT32 getcurrentTimeMillis(UINT64& u64Time);
SINT32 getcurrentTimeMicros(UINT64& u64Time);

SINT32 initRandom();
SINT32 getRandom(UINT8* buff,UINT32 len);

SINT32 getRandom(UINT32* val);

SINT32 msSleep(UINT16 ms);

SINT32 sSleep(UINT16 sec);

UINT32 getMemoryUsage();

#ifndef _WIN32
SINT32 filelength(int handle);
#endif

SINT32 setDOMElementValue(DOM_Element& elem,UINT32 value);

/**
 * Returns the content of the text node under elem
 * as 64bit unsigned integer.
 */
SINT32 getDOMElementValue(const DOM_Element& elem, UINT64 &value);

/**
 * Sets the decimal text representation of a 64bit integer as node value
 * TODO: implement this for non-64bit platforms
 */
SINT32 setDOMElementValue(DOM_Element & elem, const UINT64 text);

SINT32 getDOMElementValue(const DOM_Element& elem,UINT32* value);

SINT32 getDOMElementValue(const DOM_Element& elem,UINT16* value);

SINT32 setDOMElementValue(DOM_Element& elem,const UINT8* value);


/** 
 * Returns the content of the text node(s) under elem
 * as null-terminated C String. If there is no text node
 * len is set to 0.
 * 
 * TODO: Why is elem a DOM_Node and not a DOM_Element here?
 * @param DOM_Node the element which has a text node under it
 * @param value a buffer that gets the text value
 * @param len on call contains the buffer size, on return contains the number of bytes copied
 * @return E_SPACE if the buffer is too small, E_SUCCESS otherwise, E_UNKNOWN if the element is NULL
 */
SINT32 getDOMElementValue(const DOM_Node& elem,UINT8* value,UINT32* len);

SINT32 setDOMElementAttribute(DOM_Node& elem,const char* attrName,int value);
SINT32 getDOMElementAttribute(const DOM_Element& elem,const char* attrName,int* value);
SINT32 setDOMElementAttribute(DOM_Node& elem,const char* attrName,const UINT8* value);
SINT32 getDOMElementAttribute(const DOM_Node& elem,const char* attrName,bool& value);
SINT32 getDOMElementAttribute(const DOM_Element& elem,const char* attrName,UINT8* value,UINT32* len);

SINT32 getDOMChildByName(const DOM_Node& node,const UINT8* const name,DOM_Node& child,bool deep=false);

SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, UINT8* xml, UINT32* xmllen,CAASymCipher* pRSA);
SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, DOM_DocumentFragment& docFrag,CAASymCipher* pRSA);
SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, const UINT8* const xml, UINT32 xmllen,CAASymCipher* pRSA);
SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, DOM_Node & root,CAASymCipher* pRSA);

/** Replaces a DOM element with an encrypted version of this element*/
SINT32 encryptXMLElement(DOM_Node &, CAASymCipher* pRSA);

/** Replaces a DOM element with a deencrypted version of this element*/
SINT32 decryptXMLElement(DOM_Node &, CAASymCipher* pRSA);

inline void set64(UINT64& op1,UINT32 op2)
	{
#if !defined(HAVE_NATIVE_UINT64)
		op1.low=op2;
		op1.high=0;
#else
		op1=op2;
#endif
	}

inline void set64(UINT64& op1,UINT64 op2)
	{
#if !defined(HAVE_NATIVE_UINT64)
		op1.low=op2.low;
		op1.high=op2.high;
#else
		op1=op2;
#endif
	}

inline void setZero64(UINT64& op1)
	{
#if !defined(HAVE_NATIVE_UINT64)
		op1.low=0;
		op1.high=0;
#else
		op1=0;
#endif
	}
	
inline void add64(UINT64& op1,UINT32 op2)
	{
#if !defined(HAVE_NATIVE_UINT64)
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
#if !defined(HAVE_NATIVE_UINT64)
	op1.low++;
	if(op1.low==0)
		op1.high++;
#else
		op1++;
#endif
	}

inline UINT32 diff64(const UINT64& bigop,const UINT64& smallop)
	{
		#if !defined(HAVE_NATIVE_UINT64)
			return (UINT32) -1; //TODO!!!
		#else
			return (UINT32)(bigop-smallop);
		#endif
	}

inline UINT32 div64(UINT64& op1,UINT32 op2)
	{
#if !defined(HAVE_NATIVE_UINT64)
		return (UINT32) -1; //TODO!!!
#else
		return (UINT32)(op1/op2);
#endif
	}

inline bool isGreater64(UINT64& op1,UINT64& op2)
	{
#if !defined(HAVE_NATIVE_UINT64)
		if(op1.high>op2.high)
			return true;
		if(op1.high==op2.high)
			return op1.low>op2.low;
		return false;
#else
		return op1>op2;
#endif
	}

inline bool isLesser64(UINT64& smallOp1,UINT64& bigOp2)
	{
#if !defined(HAVE_NATIVE_UINT64)
		if(smallOp1.high<bigOp2.high)
			return true;
		if(smallOp1.high==bigOp2.high)
			return smallOp1.low<bigOp2.low;
		return false;
#else
		return smallOp1<bigOp2;
#endif
	}

inline bool isEqual64(UINT64& op1,UINT64& op2)
	{
#if !defined(HAVE_NATIVE_UINT64)
		return (op1.high==op2.high)&&op1.low==op2.low;
#else
		return op1==op2;
#endif
	}
	
inline bool isZero64(UINT64& op1)
	{
#if !defined(HAVE_NATIVE_UINT64)
		return (op1.high==0)&&op1.low==0;
#else
		return op1==0;
#endif
	}
	
/*inline void print64(UINT8* buff,UINT64& op)
	{
		#if defined(HAVE_NATIVE_UINT64)
			#if defined(_WIN32)
				_ui64toa(op,(char*)buff,10);
			#elif defined(__linux)||defined(__sgi)||defined(__FreeBSD__) //TODO: check if for FreeBSD it is correct
				sprintf((char*)buff,"%Lu",op);
			#elif defined(__APPLE__) //TODO: Check if this is ok...
				sprintf((char*)buff,"%llu",op);
			#endif
		#else //no native UINT_64
			sprintf((char*)buff,"(%lu:%lu)",op.high,op.low);
		#endif
	}
*/

inline void print64(UINT8* buff,UINT64 num)
	{
		#ifdef HAVE_NATIVE_UINT64
			if(num==0)
				{
					buff[0]='0';
					buff[1]=0;
					return;
				}
			UINT64 mask=10000000000000000000ULL;
			UINT digit;
			UINT32 index=0;
			bool bprintZero=false;
			if(num>=mask)
				{
					buff[index++]='1';
					num-=mask;
					bprintZero=true;
				}
			while(mask>1)
				{
					mask/=10;
					digit=(UINT)(num/mask);
					if(digit>0||bprintZero)
						{
							buff[index++]=(UINT8)(digit+'0');
							num%=mask;
							bprintZero=true;
						}
				}
			buff[index]=0;
		#else //no native UINT_64
			sprintf((char*)buff,"(%lu:%lu)",op.high,op.low);
		#endif
	}


UINT8* readFile(UINT8* name,UINT32* size);

/*
 Timestamp related functions
 */
void currentTimestamp(UINT8* buff,bool bInNetworkByteOrder=false);
 
/*
 Checking for duplicates
*/
bool validTimestampAndFingerprint(UINT8* fingerprint, UINT16 len, UINT8* timestamp_buff);


/**
 * Parses a timestamp in JDBC timestamp escape format (as it comes from the BI)
 * and outputs the value in milliseconds since the epoch.
 *
 * @param strTimestamp the string containing the timestamp
 * @param value an integer variable that gets the milliseconds value.
 */
SINT32 parseJdbcTimestamp(const UINT8 * strTimestamp, SINT32& seconds);


/**
 * Converts a timestamp (in seconds) to the String JDBC timestamp
 * escape format (YYYY-MM-DD HH:MM:SS)
 * @param seconds integer value containing the timestamp in seconds since the epoch
 * @param strTimestamp a string buffer that gets the result
 * @param len the buffer length
 * @todo think about timezone handling
 */
SINT32 formatJdbcTimestamp(const SINT32 seconds, UINT8 * strTimestamp, const UINT32 len);


/**
 * Parses a 64bit integer
 */
SINT32 parseU64(const UINT8 * str, UINT64& value);

#endif
