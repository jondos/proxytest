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

#define UNIVERSAL_ATTRIBUTE_LAST_UPDATE "lastUpdate"
#define UNIVERSAL_NODE_LAST_UPDATE "LastUpdate"

#define STR_VALUE_TRUE "true"
#define STR_VALUE_FALSE "false"

#define TMP_BUFF_SIZE 255
#define TMP_LOCALE_SIZE 3
#define TMP_DATE_SIZE 9

#define TEMPLATE_REFID_MAXLEN ((TMP_BUFF_SIZE) + (TMP_LOCALE_SIZE) + (TMP_DATE_SIZE) + 2)

#if _XERCES_VERSION >= 30001
	#define INTEGRATE_NOT_ALLOWED_POSITIONS \
		(DOMNode::DOCUMENT_POSITION_CONTAINED_BY | DOMNode::DOCUMENT_POSITION_CONTAINS )
#else
	#define INTEGRATE_NOT_ALLOWED_POSITIONS \
		(DOMNode::TREE_POSITION_ANCESTOR   | DOMNode::TREE_POSITION_DESCENDANT | \
		DOMNode::TREE_POSITION_EQUIVALENT | DOMNode::TREE_POSITION_SAME_NODE )
#endif

UINT32 strtrim(UINT8*);
UINT32 toLower(UINT8* a_string);

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

SINT32 msSleep(UINT32 ms);

SINT32 sSleep(UINT32 sec);

UINT32 getMemoryUsage();

inline SINT64 filesize64(int handle)
	{
		#ifdef _WIN32
			return _filelengthi64(handle);
		#elif defined (HAVE_FSTAT64)
			struct stat64 info;
			if(fstat64(handle, &info) != 0)
				return E_UNKNOWN;
			return info.st_size;
		#else
			struct stat info;
			if(fstat(handle, &info) != 0)
				return E_UNKNOWN;
			return info.st_size;
		#endif
	}

inline SINT32 filesize32(int handle)
	{
		#ifdef _WIN32
			return _filelength(handle);
		#else
			struct stat info;
			if(fstat(handle, &info) != 0)
				return E_UNKNOWN;
			return info.st_size;
		#endif
	}

/**
 * Parses  a buffer containing an XML document and returns this document.
 */
XERCES_CPP_NAMESPACE::DOMDocument* parseDOMDocument(const UINT8* const buff, UINT32 len);
/**
 * parses a file via path or URL
 */
XERCES_CPP_NAMESPACE::DOMDocument* parseDOMDocument(const UINT8* const pathOrURL);
void initDOMParser();
void releaseDOMParser();

SINT32 getDOMChildByName(const DOMNode* pNode,const XMLCh* const name,DOMNode* & child,bool deep=false);
SINT32 getDOMChildByName(const DOMNode* pNode,const char * const name,DOMNode* & child,bool deep=false);
SINT32 getDOMChildByName(const DOMNode* pNode,const char * const name,DOMElement* & child,bool deep=false);
SINT32 getSignatureElements(DOMNode* parent, DOMNode** signatureNodes, UINT32* length);

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

/** Gets the value from an DOM-Element as UINT32. If an error occurs, the default value is returned.
*/
SINT32 getDOMElementValue(const DOMElement * const pElem, UINT32& value, UINT32 defaultValue);

SINT32 getDOMElementValue(const DOMElement * const pElem, UINT32* value);


#if !defined LOCAL_PROXY_ONLY || defined INCLUDE_MIDDLE_MIX 
	/** Creates an empty DOM DOcument.
	*/
	XERCES_CPP_NAMESPACE::DOMDocument* createDOMDocument();
	SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, UINT8* xml, UINT32* xmllen,CAASymCipher* pRSA);
	SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, DOMElement* & elemRootEncodedKey,XERCES_CPP_NAMESPACE::DOMDocument* docOwner,CAASymCipher* pRSA);
	SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, const UINT8* const xml, UINT32 xmllen,CAASymCipher* pRSA);
	SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, const DOMNode* pRoot,CAASymCipher* pRSA);
	SINT32 getDOMElementValue(const DOMElement * const pElem,UINT16* value);
	SINT32 setDOMElementValue(DOMElement* pElem,const UINT8* value);
	SINT32 setDOMElementValue(DOMElement* pElem, UINT32 value);
	SINT32 setDOMElementAttribute(DOMNode* pElem,const char* attrName, UINT32 value);
	SINT32 getDOMElementAttribute(const DOMNode * const pElem,const char* attrName,UINT32& value);
	SINT32 getDOMElementAttribute(const DOMNode * const pElem,const char* attrName,UINT8* value,UINT32* len);
	SINT32 getDOMElementAttribute(const DOMNode * const pElem,const char* attrName,SINT64& value);
	SINT32 getDOMElementAttribute(const DOMNode * const pElem,const char* attrName,bool& value);
	SINT32 getNodeName(const DOMNode * const pElem, UINT8* value,UINT32* valuelen);
	SINT32 getLastDOMChildByName(const DOMNode* pNode,const char * const name,DOMElement* & a_child);
	SINT32 getLastDOMChildByName(const DOMNode* pNode,const XMLCh* const name,DOMNode* & a_child);
	SINT32 getLastDOMChildByName(const DOMNode* pNode,const char * const name,DOMNode* & a_child);

	DOMNodeList* getElementsByTagName(DOMElement* pElem,const char* const name);


	/**
 * Clones an OpenSSL DSA structure
 */
inline DSA* DSA_clone(DSA* dsa)
	{
		if(dsa==NULL)
			return NULL;
		DSA* tmpDSA=DSA_new();
#if  OPENSSL_VERSION_NUMBER > 0x100020cfL
		BIGNUM* p = NULL;
		BIGNUM* q = NULL;
		BIGNUM* g = NULL;
		BIGNUM* pub_key = NULL;
		BIGNUM* priv_key = NULL;
		DSA_get0_pqg(dsa,(const BIGNUM**) &p,(const BIGNUM**) &q,(const BIGNUM**) &g);
		DSA_set0_pqg(tmpDSA,BN_dup(p), BN_dup(q), BN_dup(g));
		DSA_get0_key(dsa,(const BIGNUM**) &pub_key,(const BIGNUM**) &priv_key);
		DSA_set0_key(tmpDSA,BN_dup(pub_key), BN_dup(priv_key));
#else
		tmpDSA->g=BN_dup(dsa->g);
		tmpDSA->p=BN_dup(dsa->p);
		tmpDSA->q=BN_dup(dsa->q);
		tmpDSA->pub_key=BN_dup(dsa->pub_key);
		if(dsa->priv_key!=NULL)
			tmpDSA->priv_key=BN_dup(dsa->priv_key);
#endif
		return tmpDSA;
	}

/**
 * Clones an OpenSSL RSA structure
 */
inline RSA* RSA_clone(RSA* rsa)
	{
		if(rsa == NULL)
		{
			return NULL;
		}
#if  OPENSSL_VERSION_NUMBER >= 0x1000204fL
		return RSAPrivateKey_dup(rsa);
#else
		RSA* tmpRSA = RSA_new();
		tmpRSA->n = BN_dup(rsa->n);
		tmpRSA->e = BN_dup(rsa->e);
		if(rsa->d != NULL)
		{ //we have a private key
			tmpRSA->d = BN_dup(rsa->d);
			if(tmpRSA->p != NULL)
			{
				tmpRSA->p = BN_dup(rsa->p);
			}
			if(tmpRSA->q != NULL)
			{
				tmpRSA->q = BN_dup(rsa->q);
			}
		}
		if(tmpRSA->dmp1 != NULL)
		{
			tmpRSA->dmp1 = BN_dup(rsa->dmp1);
		}
		if(tmpRSA->dmq1 != NULL)
		{
			tmpRSA->dmq1 = BN_dup(rsa->dmq1);
		}
		if(tmpRSA->iqmp != NULL)
		{
			tmpRSA->iqmp = BN_dup(rsa->iqmp);
		}
		return tmpRSA;
#endif
	}


#endif

#ifndef ONLY_LOCAL_PROXY

/** Creates a new DOMText with the given value which belongs to the DOMDocument owernDoc.
**/
DOMText* createDOMText(XERCES_CPP_NAMESPACE::DOMDocument* pOwnerDoc,const char * const text);

SINT32 setDOMElementValue(DOMElement* pElem, SINT32 value);
/**
 * Returns the content of the text node under elem
 * as 64bit unsigned integer.
 */

/**
 * Sets the decimal text representation of a 64bit integer as node value
 * TODO: implement this for non-64bit platforms
 */
SINT32 setDOMElementValue(DOMElement* pElem, const UINT64 text);
SINT32 setDOMElementValue(DOMElement* pElem, const SINT64 text);


SINT32 getDOMElementValue(const DOMNode * const pElem, UINT64 &value);
SINT32 getDOMElementValue(const DOMElement * const pElem, SINT64 &value);

SINT32 getDOMElementValue(const DOMElement * const pElem,SINT32* value);


SINT32 getDOMElementValue(const DOMElement * const pElem,double* value);

SINT32 setDOMElementAttribute(DOMNode* pElem,const char* attrName, bool value);
SINT32 setDOMElementAttribute(DOMNode* pElem,const char* attrName, SINT32 value);
SINT32 setDOMElementAttribute(DOMNode* pElem, const char* attrName, UINT64 value);
SINT32 setDOMElementAttribute(DOMNode* pElem, const char* attrName, SINT64 value);

SINT32 setDOMElementValue(DOMElement* pElem,double floatValue);
SINT32 setDOMElementValue(DOMElement* pElem, bool value);


SINT32 setCurrentTimeMilliesAsDOMAttribute(DOMNode *pElem);


SINT32 integrateDOMNode(const DOMNode *srcNode, DOMNode *dstNode, bool recursive, bool replace);

/** Replaces a DOM element with an encrypted version of this element*/
SINT32 encryptXMLElement(DOMNode* pElem , CAASymCipher* pRSA);

#endif //ONLY_LOCAL_PROXY

#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_FIRST_MIX

/** Replaces a DOM element with a deencrypted version of this element*/
SINT32 decryptXMLElement(DOMNode* pelem , CAASymCipher* pRSA);
//if not null the returned char pointer must be explicitely freed by the caller with 'delete []'
UINT8 *getTermsAndConditionsTemplateRefId(DOMNode *tcTemplateRoot);

#endif //!ONLY_LOCAL_PROXY or first

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

/** Sets the value of dst to the value of src
*/
inline void set64(SINT64& dst,SINT64 src)
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

inline void setZero64(SINT64& op1)
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

inline bool isGreater64(SINT64 op1,SINT64 op2)
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

inline bool isZero64(SINT64& op1)
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
					UINT digit=(UINT)(num/mask);
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


UINT8* readFile(const UINT8* const name,UINT32* size);
SINT32 saveFile(const UINT8* const name,const UINT8* const buff,UINT32 buffSize);

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
 * Parses a 64bit unsigned integer
 */
SINT32 parseU64(const UINT8 * str, UINT64& value);

/**
 * Parses a 64bit signed integer
 */
SINT32 parseS64(const UINT8 * str, SINT64& value);

/** Read a passwd (i.e. without echoing the chars typed in)
	*/
SINT32 readPasswd(UINT8* buff,UINT32 len);

#if !defined ONLY_LOCAL_PROXY || defined INCLUDE_LAST_MIX
void logMemoryUsage();
#endif //ONLY_LOCAL_PROXY

#endif
