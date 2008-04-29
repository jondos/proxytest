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

/** Converts the byte array to a hex string. the caller is responsible for deleting the string*/
UINT8* bytes2hex(const void* bytes,UINT32 len);

char* strins(const char* src,UINT32 pos,const char* ins);
char* strins(const char* src,const char * pos,const char* ins);

SINT32 getcurrentTime(timespec& t); 
SINT32 getcurrentTimeMillis(UINT64& u64Time);
SINT32 getcurrentTimeMicros(UINT64& u64Time);

SINT32 compDate(struct tm *date1, struct tm *date2);

SINT32 initRandom();
SINT32 getRandom(UINT8* buff,UINT32 len);

SINT32 getRandom(UINT32* val);

SINT32 getRandom(UINT64* val);

SINT32 msSleep(UINT16 ms);

SINT32 sSleep(UINT16 sec);

UINT32 getMemoryUsage();

#ifndef _WIN32
SINT32 filelength(int handle);
#endif

/** Parses  a buffer containing an XML document and returns this document.
	*/
XERCES_CPP_NAMESPACE::DOMDocument* parseDOMDocument(const UINT8* const buff, UINT32 len);
void releaseDOMParser();

SINT32 getDOMChildByName(const DOMNode* pNode,const XMLCh* const name,DOMNode* & child,bool deep=false);
SINT32 getDOMChildByName(const DOMNode* pNode,const char * const name,DOMNode* & child,bool deep=false);
SINT32 getDOMChildByName(const DOMNode* pNode,const char * const name,DOMElement* & child,bool deep=false);

/** 
 * Returns the content of the text node(s) under elem
 * as null-terminated C String. If there is no text node
 * len is set to 0.
 * 
 * @param DOM_Node the element which has a text node under it
 * @param value a buffer that gets the text value
 * @param len on call contains the buffer size, on return contains the number of bytes copied
 * @return E_SPACE if the buffer is too small, E_SUCCESS otherwise, E_UNKNOWN if the element is NULL
 */
SINT32 getDOMElementValue(const DOMNode * const pElem,UINT8* value,UINT32* len);

SINT32 setDOMElementAttribute(DOMNode* pElem,const char* attrName,const UINT8* value);

bool equals(const XMLCh* const e1,const char* const e2);

SINT32 getDOMElementAttribute(const DOMNode * const pElem,const char* attrName,SINT32* value);

/** Creates a new DOMElement with the given name which belongs to the DOMDocument owernDoc. 
**/
DOMElement* createDOMElement(XERCES_CPP_NAMESPACE::DOMDocument* pOwnerDoc,const char * const name);

/** Creates a new DOMText with the given value which belongs to the DOMDocument owernDoc. 
**/
DOMText* createDOMText(XERCES_CPP_NAMESPACE::DOMDocument* pOwnerDoc,const char * const text);

#ifndef ONLY_LOCAL_PROXY
/** Creates an empty DOM DOcument.
	*/
XERCES_CPP_NAMESPACE::DOMDocument* createDOMDocument();

/** Creates a new DOMText with the given value which belongs to the DOMDocument owernDoc. 
**/
DOMText* createDOMText(XERCES_CPP_NAMESPACE::DOMDocument* pOwnerDoc,const char * const text);


SINT32 setDOMElementValue(DOMElement* pElem,UINT32 value);

/**
 * Returns the content of the text node under elem
 * as 64bit unsigned integer.
 */
SINT32 getDOMElementValue(const DOMElement * const pElem, UINT64 &value);

/**
 * Sets the decimal text representation of a 64bit integer as node value
 * TODO: implement this for non-64bit platforms
 */
SINT32 setDOMElementValue(DOMElement* pElem, const UINT64 text);

SINT32 getDOMElementValue(const DOMElement * const pElem,UINT32* value);
/** Gets the value from an DOM-Element as UINT32. If an error occurs, the default value is returned.
*/
SINT32 getDOMElementValue(const DOMElement * const pElem,UINT32& value,UINT32 defaultValue);

SINT32 getDOMElementValue(const DOMElement * const pElem,UINT16* value);

SINT32 setDOMElementValue(DOMElement* pElem,const UINT8* value);


SINT32 getDOMElementValue(const DOMElement * const pElem,double* value);

SINT32 setDOMElementAttribute(DOMNode* pElem,const char* attrName,SINT32 value);
SINT32 setDOMElementValue(DOMElement* pElem,double floatValue);

SINT32 getDOMElementAttribute(const DOMNode * const pElem,const char* attrName,UINT32& value);
SINT32 getDOMElementAttribute(const DOMNode * const pElem,const char* attrName,bool& value);
SINT32 getDOMElementAttribute(const DOMNode * const pElem,const char* attrName,UINT8* value,UINT32* len);

DOMNodeList* getElementsByTagName(DOMElement* pElem,const char* const name);

SINT32 getLastDOMChildByName(const DOMNode* pNode,const XMLCh* const name,DOMNode* & a_child);
SINT32 getLastDOMChildByName(const DOMNode* pNode,const char * const name,DOMNode* & a_child);
SINT32 getLastDOMChildByName(const DOMNode* pNode,const char * const name,DOMElement* & a_child);


SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, UINT8* xml, UINT32* xmllen,CAASymCipher* pRSA);
SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, DOMElement* & elemRootEncodedKey,XERCES_CPP_NAMESPACE::DOMDocument* docOwner,CAASymCipher* pRSA);
SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, const UINT8* const xml, UINT32 xmllen,CAASymCipher* pRSA);
SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, const DOMNode* pRoot,CAASymCipher* pRSA);

/** Replaces a DOM element with an encrypted version of this element*/
SINT32 encryptXMLElement(DOMNode* pElem , CAASymCipher* pRSA);

/** Replaces a DOM element with a deencrypted version of this element*/
SINT32 decryptXMLElement(DOMNode* pelem , CAASymCipher* pRSA);

#endif //ONLY_LOCAL_PROXY

UINT8* encryptXMLElement(UINT8* inbuff,UINT32 inlen,UINT32& outlen,CAASymCipher* pRSA);

inline void set64(UINT64& op1,UINT32 op2)
	{
#if !defined(HAVE_NATIVE_UINT64)
		op1.low=op2;
		op1.high=0;
#else
		op1=op2;
#endif
	}

/** Sets the value of dst to the value of src
*/
inline void set64(UINT64& dst,UINT64 src)
	{
#if !defined(HAVE_NATIVE_UINT64)
		dst.low=src.low;
		dst.high=src.high;
#else
		dst=src;
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

/**
 * Parses a timestamp in JDBC timestamp escape format (as it comes from the BI)
 * and outputs the value in milliseconds since the epoch.
 *
 * @param strTimestamp the string containing the timestamp
 * @param value an integer variable that gets the milliseconds value.
 */
//SINT32 parseJdbcTimestamp(const UINT8 * strTimestamp, SINT32& seconds);


/**
 * Converts a timestamp (in seconds) to the String JDBC timestamp
 * escape format (YYYY-MM-DD HH:MM:SS)
 * @param seconds integer value containing the timestamp in seconds since the epoch
 * @param strTimestamp a string buffer that gets the result
 * @param len the buffer length
 * @todo think about timezone handling
 */
//SINT32 formatJdbcTimestamp(const SINT32 seconds, UINT8 * strTimestamp, const UINT32 len);


/**
 * Parses a 64bit integer
 */
SINT32 parseU64(const UINT8 * str, UINT64& value);

/** Read a passwd (i.e. without echoing the chars typed in)
	*/
SINT32 readPasswd(UINT8* buff,UINT32 len);

void logMemoryUsage();


#ifndef ONLY_LOCAL_PROXY
/** Clones an OpenSSL DSA structure
	*/
inline DSA* DSA_clone(DSA* dsa)
	{
		if(dsa==NULL)
			return NULL;
		DSA* tmpDSA=DSA_new();
		tmpDSA->g=BN_dup(dsa->g);
		tmpDSA->p=BN_dup(dsa->p);
		tmpDSA->q=BN_dup(dsa->q);
		tmpDSA->pub_key=BN_dup(dsa->pub_key);
		if(dsa->priv_key!=NULL)
			tmpDSA->priv_key=BN_dup(dsa->priv_key);
		return tmpDSA;
	}
#endif //ONLY_LOCAL_PROXY
#endif
