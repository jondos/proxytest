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
#include "CAUtil.hpp"
#include "CABase64.hpp"
#ifdef DEBUG
	#include "CAMsg.hpp"
#endif
#include "xml/DOM_Output.hpp"
/**
*	Removes leading and ending whitespaces (chars<=32) from a zero terminated string.
*		@param s input string (null terminated)
*		@return new size of string
*		@retval 0 means, that either s was NULL or that the new string has a zero length 
*											(that means, that the old string only contains whitespaces)
*/
UINT32 strtrim(UINT8* s)
	{
		if(s==NULL)
			return 0;
		UINT32 end=strlen((char*)s);
		if(end==0)
			return 0;
		end--;
		UINT32 start=0;
		UINT32 size;
		while(start<=end&&s[start]<=32)
			start++;
		if(start>end) //empty string....
			return 0;
		while(end>start&&s[end]<=32)
			end--;
		size=(end+1)-start;
		memmove(s,s+start,size);
		s[size]=0;
		return size;
	}

/**
*	Removes leading and ending whitespaces (chars<=32) from a byte array.
*		@param src input byte array
*		@param dest output byte array
*		@param size size of the input byte array
*		@retval E_UNSPECIFIED, if dest was NULL
*		@return	size of output otherwise
*		@todo replace UINT32 size with SINT32 size
*/
SINT32 memtrim(UINT8* dest,const UINT8* src,UINT32 size)
	{
		if(src==NULL||size==0)
			return 0;
		if(dest==NULL)
			return E_UNSPECIFIED;
		UINT32 start=0;
		UINT32 end=size-1;
		while(start<=end&&src[start]<=32)
			start++;
		if(start>end) //empty string....
			return 0;
		while(end>start&&src[end]<=32)
			end--;
		size=(end+1)-start;
		memmove(dest,src+start,size);
		return (SINT32)size;
	}

/** Inserts a String ins in a String src starting after pos chars.
	* Returns a newly allocated String which must be freed using delete.
	*/
char* strins(const char* src,UINT32 pos,const char* ins)
	{
		if(src==NULL||ins==NULL)
			return NULL;
		UINT32 srcLen=strlen(src);
		if(pos>srcLen)
			return NULL;
		UINT32 insLen=strlen(ins);
		char* newString=new char[srcLen+insLen+1];
		if(newString==NULL)
			return NULL;
		memcpy(newString,src,pos);
		memcpy(newString+pos,ins,insLen);
		memcpy(newString+pos+insLen,src+pos,srcLen-pos+1); //copy includes the \0
		return newString;
	}

/** Inserts a String ins in a String src starting at the char pos points to.
	* Returns a newly allocated String which must be freed using delete.
	*/
char* strins(const char* src,const char * pos,const char* ins)
	{
		if(pos==NULL||pos<src)
			return NULL;
		return strins(src,pos-src,ins);
	}

/** Gets the current Systemtime in milli seconds. 
	* @param bnTime - Big Number, in which the current time is placed
	* @retval E_UNSPECIFIED, if bnTime was NULL
	* @retval E_UNKNOWN, if an error occurs
	*	@retval	E_SUCCESS, otherwise
*/
/*SINT32 getcurrentTimeMillis(BIGNUM* bnTime)
	{
		if(bnTime==NULL)
			return E_UNSPECIFIED;
		#ifdef _WIN32
			struct _timeb timebuffer;
			_ftime(&timebuffer);
			// Hack what should be solved better...
			BN_set_word(bnTime,timebuffer.time);
			BN_mul_word(bnTime,1000);
			BN_add_word(bnTime,timebuffer.millitm);
			// end of hack..
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
	}*/

SINT32 getcurrentTime(timespec& t)
	{
		#ifdef _WIN32
			timeb timebuffer;
			ftime(&timebuffer);
			/* Hack what should be solved better...*/
			t.tv_sec=timebuffer.time;
			t.tv_nsec=timebuffer.millitm*1000000;
			/* end of hack..*/
			return E_SUCCESS;
	  #else //we dont use ftime due to a bug in glibc2.0
		//we use gettimeofday() in order to get the millis...
			struct timeval tv;
			gettimeofday(&tv,NULL); //getting millis...
			t.tv_sec=tv.tv_sec;
			t.tv_nsec=tv.tv_usec*1000;
			return E_SUCCESS;
	  #endif
	}

/** Gets the current Systemtime in milli seconds. 
	* @param u64Time - 64 bit Integer, in which the current time is placed
	* @retval E_UNKNOWN, if an error occurs
	*	@retval	E_SUCCESS, otherwise
*/
SINT32 getcurrentTimeMillis(UINT64& u64Time)
	{
		#ifdef _WIN32
			timeb timebuffer;
			ftime(&timebuffer);
			/* Hack what should be solved better...*/
			u64Time=((UINT64)timebuffer.time)*1000+((UINT64)timebuffer.millitm);
			/* end of hack..*/
			return E_SUCCESS;
	  #else //we dont use ftime due to a bug in glibc2.0
		//we use gettimeofday() in order to get the millis...
			struct timeval tv;
			gettimeofday(&tv,NULL); //getting millis...
			#ifdef HAVE_NATIVE_UINT64
				u64Time=((UINT64)tv.tv_sec)*1000+((UINT64)tv.tv_usec)/1000;
				return E_SUCCESS;
			#else
				return E_UNKNOWN;
			#endif
	  #endif
	}

/** Gets the current Systemtime in micros seconds. Depending on the operating system,
  * the time may not be so accurat. 
	* @param u64Time - 64 bit Integer, in which the current time is placed
	* @retval E_UNKNOWN, if an error occurs
	*	@retval	E_SUCCESS, otherwise
*/
SINT32 getcurrentTimeMicros(UINT64& u64Time)
	{
		#ifdef _WIN32
			timeb timebuffer;
			ftime(&timebuffer);
			/* Hack what should be solved better...*/
			u64Time=((UINT64)timebuffer.time)*1000000+((UINT64)timebuffer.millitm)*1000;
			/* end of hack..*/
			return E_SUCCESS;
	  #else //we dont use ftime due to a bug in glibc2.0
		//we use gettimeofday() in order to get the millis...
			struct timeval tv;
			gettimeofday(&tv,NULL); //getting millis...
			#ifdef HAVE_NATIVE_UINT64
				u64Time=((UINT64)tv.tv_sec)*1000000+((UINT64)tv.tv_usec);
				return E_SUCCESS;
			#else
				return E_UNKNOWN;
			#endif
	  #endif
	}

SINT32 initRandom()
	{
		#if _WIN32
			RAND_screen();
		#else
			#ifndef __linux
				unsigned char randbuff[255];
				getcurrentTime(*((timespec*)randbuff));
				RAND_seed(randbuff,sizeof(randbuff));
			#endif
		#endif
		return E_SUCCESS;
	}
/** Gets 32 random bits.
	@param val - on return the bits are random
	@retval E_UNKNOWN, if an error occured
	@retval E_SUCCESS, if successful
*/
SINT32 getRandom(UINT32* val)
	{
		ASSERT(val!=NULL,"VAL should be not NULL");
		if(RAND_bytes((UINT8*)val,4)!=1&&
			 RAND_pseudo_bytes((UINT8*)val,4)<0)
				return E_UNKNOWN;
		return E_SUCCESS;
	}

/** Gets some random bytes.
	@param buff - buff which is filled with randomness
	@param len - number of bytes requested
	@retval E_UNKNOWN, if an error occured
	@retval E_SUCCESS, if successful
*/
SINT32 getRandom(UINT8* buff,UINT32 len)
	{
		ASSERT(buff!=NULL,"BUFF should be not NULL")
		if(RAND_bytes(buff,len)!=1&&		
			 RAND_pseudo_bytes(buff,len)<0)
			return E_UNKNOWN;
		return E_SUCCESS;
	}

/** Sleeps ms milliseconds*/
SINT32 msSleep(UINT16 ms)
	{//Do not us usleep for this --> because it doesnt seam to work on irix, multithreaded
		#ifdef _WIN32
			Sleep(ms);
		#else
			struct timespec req;
			struct timespec rem;
			req.tv_sec=ms/1000;
			req.tv_nsec=(ms%1000)*1000000;
			while(nanosleep(&req,&rem)==-1)
				{
					req.tv_sec=rem.tv_sec;
					req.tv_nsec=rem.tv_nsec;
				}
		#endif
		return E_SUCCESS;
	}

/** Sleeps sec Seconds*/
SINT32 sSleep(UINT16 sec)
	{
		#ifdef _WIN32
			Sleep(sec*1000);
		#else
			struct timespec req;
			struct timespec rem;
			req.tv_sec=sec;
			req.tv_nsec=0;
			while(nanosleep(&req,&rem)==-1)
				{
					req.tv_sec=rem.tv_sec;
					req.tv_nsec=rem.tv_nsec;
				}
		#endif
		return E_SUCCESS;
	}

UINT32 getMemoryUsage()
	{
#ifndef _WIN32
		struct rusage usage_self;
		if(getrusage(RUSAGE_SELF,&usage_self)==-1)
			return 0;
		struct rusage usage_children;
		if(getrusage(RUSAGE_CHILDREN,&usage_children)==-1)
			return 0;
		return usage_self.ru_idrss+usage_children.ru_idrss;
#else
	return 0;
#endif
	}

#ifndef _WIN32
SINT32 filelength(int handle)
	{
		struct stat st;
		if(fstat(handle,&st)==-1)
			return -1;
		return st.st_size;
	}
#endif

SINT32 setDOMElementValue(DOM_Element& elem,UINT32 text)
	{
		UINT8 tmp[10];
		sprintf((char*)tmp,"%u",text);
		setDOMElementValue(elem,tmp);
		return E_SUCCESS;
	}

SINT32 setDOMElementValue(DOM_Element& elem,const UINT8* value)
	{
		DOM_Text t=elem.getOwnerDocument().createTextNode(DOMString((char*)value));
		//Remove all "old" text Elements...
		DOM_Node child=elem.getFirstChild();
		while(child!=NULL)
			{
				if(child.getNodeType()==DOM_Node::TEXT_NODE)
					elem.removeChild(child);
				child=child.getNextSibling();
			}
		elem.appendChild(t);
		return E_SUCCESS;
	}

SINT32 setDOMElementAttribute(DOM_Node& elem,const char* attrName,const UINT8* value)
	{
		if(elem.isNull()||elem.getNodeType()!=DOM_Node::ELEMENT_NODE||
				attrName==NULL||value==NULL)
			return E_UNKNOWN;
		static_cast<DOM_Element&>(elem).setAttribute(attrName,DOMString((const char*)value));
		return E_SUCCESS;
	}

SINT32 setDOMElementAttribute(DOM_Node& elem,const char* attrName,int value)
	{
		if(elem.isNull()||elem.getNodeType()!=DOM_Node::ELEMENT_NODE||
				attrName==NULL)
			return E_UNKNOWN;
		UINT8 tmp[10];
		sprintf((char*)tmp,"%i",value);
		static_cast<DOM_Element&>(elem).setAttribute(attrName,DOMString((char*)tmp));
		return E_SUCCESS;
	}

SINT32 getDOMElementAttribute(const DOM_Element& elem,const char* attrName,UINT8* value,UINT32* len)
	{
		if(elem==NULL||attrName==NULL||value==NULL||len==NULL)
			return E_UNKNOWN;
		char* tmpStr=elem.getAttribute(attrName).transcode();
		UINT32 l=strlen(tmpStr);
		if(l>=*len)
			{
				delete[] tmpStr;
				return E_SPACE;
			}
		*len=l;
		memcpy(value,tmpStr,l+1);
		delete[] tmpStr;
		return E_SUCCESS;
	}

SINT32 getDOMElementAttribute(const DOM_Element& elem,const char* attrName,int* value)
	{
		if(elem==NULL||attrName==NULL||value==NULL)
			return E_UNKNOWN;
		char* tmpStr=elem.getAttribute(attrName).transcode();
		*value=atol(tmpStr);
		delete[] tmpStr;
		return E_SUCCESS;
	}

/*SINT32 getDOMElementTagName(const DOM_Element& elem, char ** tagName)
{
	if( elem==NULL || *tagName==NULL )
		return E_UNKNOWN;
	char * tmpStr = elem.getTagName().transcode();
	
}*/

SINT32 getDOMElementAttribute(const DOM_Node& elem,const char* attrName,bool& value)
	{
		if(	elem==NULL||elem.getNodeType()!=DOM_Node::ELEMENT_NODE||
				attrName==NULL)
			return E_UNKNOWN;
		SINT32 ret=E_UNSPECIFIED;
		char* tmpStr=static_cast<const DOM_Element&>(elem).getAttribute(attrName).transcode();
		if(stricmp(tmpStr,"true")==0)
			{
				value=true;
				ret=E_SUCCESS;
			}
		else if(stricmp(tmpStr,"false")==0)
			{
				value=false;
				ret=E_SUCCESS;
			}
		delete[] tmpStr;
		return ret;
	}
SINT32 getDOMChildByName(const DOM_Node& node,const UINT8* const name,DOM_Node& child,bool deep)
	{
		if(node==NULL)
			return E_UNKNOWN;
		child=node.getFirstChild();
		while(child!=NULL)
			{
				if(child.getNodeName().equals((char*)name))
					return E_SUCCESS;
				if(deep)
					{
						DOM_Node n=child;
						if(getDOMChildByName(n,name,child,deep)==E_SUCCESS)
							return E_SUCCESS;
						child=n;
					}
				child=child.getNextSibling();
			}
		return E_UNKNOWN;
	}

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
SINT32 getDOMElementValue(const DOM_Node& elem,UINT8* value,UINT32* valuelen)
	{
		ASSERT(value!=NULL,"Value is null");
		ASSERT(valuelen!=NULL,"ValueLen is null");
		ASSERT(elem!=NULL,"Element is NULL");
		if(elem.isNull())
			return E_UNKNOWN;
		DOM_Node text=elem.getFirstChild();
		UINT32 spaceLeft=*valuelen;
		*valuelen=0;
		while(!text.isNull())
			{
				if(text.getNodeType()==DOM_Node::TEXT_NODE)
					{
						DOMString str=text.getNodeValue();
						if(str.length()>=spaceLeft)
							{
								*valuelen=str.length()+1;
								return E_SPACE;
							}
						char* tmpStr=str.transcode();
						memcpy(value+(*valuelen),tmpStr,str.length());
						*valuelen+=str.length();
						spaceLeft-=str.length();
						delete[] tmpStr;
					}
				text=text.getNextSibling();
			}
		value[*valuelen]=0;
		return E_SUCCESS;
	}

SINT32 getDOMElementValue(const DOM_Element& elem,UINT32* value)
	{
		ASSERT(value!=NULL,"Value is null");
		ASSERT(elem!=NULL,"Element is NULL");
		UINT8 buff[255];
		UINT32 buffLen=255;
		if(getDOMElementValue(elem,buff,&buffLen)!=E_SUCCESS)
			return E_UNKNOWN;
		*value=atol((char*)buff);
		
		return E_SUCCESS;
	}

SINT32 getDOMElementValue(const DOM_Element& elem, UINT64 &value)
{
	ASSERT(value!=NULL, "Value is null");
	ASSERT(elem!=NULL, "Element is NULL");
	UINT8 buf[256];
	UINT32 bufLen = 256;
	if(getDOMElementValue(elem,buf,&bufLen)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	if(parseU64(buf, value)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	
	return E_SUCCESS;
}


SINT32 getDOMElementValue(const DOM_Element& elem,UINT16* value)
	{
		UINT32 tmp;
		if(getDOMElementValue(elem,&tmp)!=E_SUCCESS)
			return E_UNKNOWN;
		if(tmp>0xFFFF)
			return E_UNKNOWN;
		*value=(UINT16)tmp;
		return E_SUCCESS;
	}


void __encryptKey(UINT8* key,UINT32 keylen,UINT8* outBuff,UINT32* outLen,CAASymCipher* pRSA)
	{
		UINT8 tmpBuff[1024];
		memset(tmpBuff,0,sizeof(tmpBuff));
		memcpy(tmpBuff+128-keylen,key,keylen);
		pRSA->encrypt(tmpBuff,tmpBuff);
		CABase64::encode(tmpBuff,128,outBuff,outLen);
		outBuff[*outLen]=0;
	}

SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, UINT8* xml, UINT32* xmllen,CAASymCipher* pRSA)
	{
#define XML_ENCODE_KEY_TEMPLATE "<EncryptedKey><EncryptionMethod Algorithm=\"RSA\"/><CipherData><CipherValue>%s</CipherValue></CipherData></EncryptedKey>"
		UINT8 tmpBuff[1024];
		UINT32 len=1024;
		__encryptKey(key,keylen,tmpBuff,&len,pRSA);
		sprintf((char*)xml,XML_ENCODE_KEY_TEMPLATE,tmpBuff);
		*xmllen=strlen((char*)xml);
		return E_SUCCESS;
	}

SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, DOM_DocumentFragment& docFrag,CAASymCipher* pRSA)
	{
		DOM_Document doc=DOM_Document::createDocument();
		DOM_Element root=doc.createElement(DOMString("EncryptedKey"));
		docFrag=doc.createDocumentFragment();
		docFrag.appendChild(root);
		DOM_Element elem1=doc.createElement("EncryptionMethod");
		setDOMElementAttribute(elem1,"Algorithm",(UINT8*)"RSA");
		root.appendChild(elem1);
		DOM_Element elem2=doc.createElement("CipherData");
		elem1.appendChild(elem2);
		elem1=doc.createElement("CipherValue");
		elem2.appendChild(elem1);
		UINT8 tmpBuff[1024];
		UINT32 tmpLen=1024;
		__encryptKey(key,keylen,tmpBuff,&tmpLen,pRSA);
		setDOMElementValue(elem1,tmpBuff);
		return E_SUCCESS;
	}


		
	

SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, const UINT8* const xml, UINT32 xmllen,CAASymCipher* pRSA)
	{
		MemBufInputSource oInput(xml,xmllen,"decodekey");
		DOMParser oParser;
		oParser.parse(oInput);
		DOM_Document doc=oParser.getDocument();
		DOM_Element root=doc.getDocumentElement();
		return decodeXMLEncryptedKey(key,keylen,root,pRSA);
	}

SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, DOM_Node & root,CAASymCipher* pRSA)
	{
		DOM_Element elemCipherValue;
		if(getDOMChildByName(root,(UINT8*)"CipherValue",elemCipherValue,true)!=E_SUCCESS)
			return E_UNKNOWN;
		UINT8 buff[2048];
		UINT32 bufflen=2048;
		if(getDOMElementValue(elemCipherValue,buff,&bufflen)!=E_SUCCESS)
			return E_UNKNOWN;
		CABase64::decode(buff,bufflen,buff,&bufflen);
		pRSA->decrypt(buff,buff);
		for(SINT32 i=127;i>=0;i--)
			{
				if(buff[i]!=0)
					if(i>32)
						*keylen=64;
					else if(i>16)
						*keylen=32;
					else 
						*keylen=16;
			}
		memcpy(key,buff+128-(*keylen),(*keylen));
		return E_SUCCESS;
	}

/** The resulting encrypted xml struct is as follows:
	* <EncryptedData Type='http://www.w3.org/2001/04/xmlenc#Element'>
	*		<EncryptionMethod Algorithm="http://www.w3.org/2001/04/xmlenc#aes128-cbc"/>
	*   <ds:KeyInfo xmlns:ds='http://www.w3.org/2000/09/xmldsig#'>
	*			<EncryptedKey>
	*				<EncryptionMethod Algorithm="http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p"/>
	*				<CipherData>
	*					<CipherValue>...</CipherValue>					
	*				</CipherData>
	*			</EncryptedKey>
	*		</ds:KeyInfo>
	*		<CipherData>
	*			<CipherValue>...</CipherValue>
	*		</CipherData>
	*	</EncryptedData>
	*/
SINT32 decryptXMLElement(DOM_Node & node, CAASymCipher* pRSA)
	{
		DOM_Document doc=node.getOwnerDocument();
		if(!node.getNodeName().equals("EncryptedData"))
			return E_UNKNOWN;
		DOM_Node elemKeyInfo;
		getDOMChildByName(node,(UINT8*)"ds:KeyInfo",elemKeyInfo,false);
		DOM_Node elemEncKey;
		getDOMChildByName(elemKeyInfo,(UINT8*)"EncryptedKey",elemEncKey,false);
		DOM_Node elemCipherValue;
		getDOMChildByName(elemEncKey,(UINT8*)"CipherValue",elemCipherValue,true);
		UINT8* cipherValue=new UINT8[1000];
		UINT32 len=1000;
		if(getDOMElementValue(elemCipherValue,cipherValue,&len)!=E_SUCCESS)
			{
				delete cipherValue;
				return E_UNKNOWN;
			}
		CABase64::decode(cipherValue,len,cipherValue,&len);
		if(	pRSA->decryptOAEP(cipherValue,cipherValue,&len)!=E_SUCCESS||
				len!=32)
			{
				delete cipherValue;
				return E_UNKNOWN;
			}
		CASymCipher *pSymCipher=new CASymCipher();
		pSymCipher->setKey(cipherValue,false);
		pSymCipher->setIVs(cipherValue+16);

		DOM_Element elemCipherData;
		getDOMChildByName(node,(UINT8*)"CipherData",elemCipherData,false);
		getDOMChildByName(elemCipherData,(UINT8*)"CipherValue",elemCipherValue,false);
		len=1000;
		if(getDOMElementValue(elemCipherValue,cipherValue,&len)!=E_SUCCESS)
			{
				delete pSymCipher;
				delete cipherValue;
				return E_UNKNOWN;
			}
		CABase64::decode(cipherValue,len,cipherValue,&len);
		SINT32 ret=pSymCipher->crypt1CBCwithPKCS7(cipherValue,cipherValue,&len);
		delete pSymCipher;
		if(ret!=E_SUCCESS)
			{
				delete cipherValue;
				return E_UNKNOWN;
			}
		//now the need to parse the plaintext...
		MemBufInputSource oInput(cipherValue,len,"decryptelement");
		DOMParser oParser;
		oParser.parse(oInput);
		delete cipherValue;		
		DOM_Document docPlain=oParser.getDocument();
		DOM_Node elemPlainRoot;
		if(docPlain==NULL||(elemPlainRoot=docPlain.getDocumentElement())==NULL)
			return E_UNKNOWN;
		elemPlainRoot=doc.importNode(elemPlainRoot,true);	
		DOM_Node parent=node.getParentNode();
		if(parent.getNodeType()==DOM_Node::DOCUMENT_NODE)
			{
				parent.removeChild(node);
				parent.appendChild(elemPlainRoot);
			}
		else	
			parent.replaceChild(elemPlainRoot,node);	
		return E_SUCCESS;
	}

UINT8* readFile(UINT8* name,UINT32* size)
	{
		int handle=open((char*)name,O_BINARY|O_RDONLY);
		if(handle<0)
			return NULL;
		*size=filelength(handle);
		UINT8* buff=new UINT8[*size];
		read(handle,buff,*size);
		close(handle);
		return buff;
	}

/**
	Adds TIMESTAMP_SIZE bytes of timestamp at the parameter
	and returns this timestamp.
*/
void currentTimestamp(UINT8* buff,bool bInNetworkByteOrder)
	{
		// Assume 366 days per year to be on the safe side with leap years.
		time_t seconds_per_year = 60 * 60 * 24 * 366;
		time_t now = time(NULL);

		// January 1 of every year is the start of the epoch.
		struct tm tm_epoch = *localtime(&now);
		tm_epoch.tm_sec = tm_epoch.tm_min = tm_epoch.tm_hour = tm_epoch.tm_mday = tm_epoch.tm_mon = 0;
		time_t epoch = mktime(&tm_epoch);

		// timestamp = (seconds_passed_in_this_year / seconds_per_year) * 2^16
		// That is 0x0000 on January 1, 0:00; 0x0001 on January 1, 0:10; 0xFFFF on December 31, 23:59 (leap year)
		UINT16 timestamp = (UINT16) ((((double) (now - epoch)) / ((double) seconds_per_year)) * 0xFFFF);
		if(bInNetworkByteOrder)
			timestamp=htons(timestamp);
		if(buff != NULL)
			memcpy(buff, &timestamp, TIMESTAMP_SIZE);
	}

/**
	Encapsulates the check for duplicates in a specific time period.
	Most probably this should go somewhere else in the code.
*/

// HASH_SIZE = 4.000 messages per minute * 8 minutes (length of time interval)
//							* 1.33 (load factor of hash) * 16 bytes (fingerprint length)
//							* 10 (factor for scalability) = 6809600
#define HASH_SIZE 						6809600
// Flags for the slots in the hash table.
#define EMPTY_FLAG						0x00
#define COLLISION_FLAG				0xFF
// Which hash to use in checkAndInsert().
#define CURRENT_HASH					0
#define OLD_HASH							1

struct ListItem {
  UINT8 value[128];
  struct ListItem *next;
};

bool checkAndInsert(UINT8 hash, UINT8* value);

UINT8 hash_current[HASH_SIZE];
UINT8 hash_old[HASH_SIZE];
/**
Additional to the overflow lists in the hash tables, for each hash table
an additional list of all overflow items is kept. This make it possible to
delete a complete hash table in O(overflow items) rather than
O(max items in hash table * average size of overflow list).
*/
struct ListItem* hash_current_list;
struct ListItem* hash_old_list;
struct ListItem* hash_current_list_pos;
struct ListItem* hash_old_list_pos;
UINT16 hash_current_timestamp = 0;
UINT16 hash_old_timestamp = 0;
UINT16 length = 0;
bool initialized = false;

bool validTimestampAndFingerprint(UINT8* fingerprint, UINT16 len, UINT8* timestamp_buff)
	{
		UINT16 current_timestamp;
		currentTimestamp((UINT8*)&current_timestamp);
		if(!initialized)
			{
				// Initialize hashes and lists on first call.
				memset(hash_current, 0, HASH_SIZE);
				memset(hash_old, 0, HASH_SIZE);
				hash_current_list = hash_old_list = hash_current_list_pos = hash_old_list_pos = NULL;
				hash_current_timestamp = hash_old_timestamp = current_timestamp;
				length = len;
				initialized = true;
			}

		// It is illegal to change the length of the figerprint during operation.
		if(len != length) return false;

		if(hash_current_timestamp != current_timestamp)
			{
				// We have passed into the next time interval or the next epoch.
				// Move current hash to old hash, delete old overflow items.
				memcpy(hash_old, hash_current, HASH_SIZE);
				struct ListItem* tmp;
				struct ListItem* li = hash_old_list;
				while(li != NULL)
					{
						tmp = li;
						free(li);
						li = tmp->next;
					}
				hash_old_list = hash_current_list;
				hash_old_list_pos = hash_current_list_pos;
				hash_old_timestamp = hash_current_timestamp;
				// Empty current hash, reset global overflow list.
				memset(hash_current, 0, HASH_SIZE);
				hash_current_list_pos = hash_current_list_pos = NULL;
				hash_current_timestamp = current_timestamp;
			}

		UINT16 timestamp = 0;
		memcpy(&timestamp, timestamp_buff, TIMESTAMP_SIZE);
		timestamp=ntohs(timestamp);
		if(timestamp == hash_current_timestamp)
			// Timestamp is OK, check for duplicates.
			return checkAndInsert(CURRENT_HASH, fingerprint);
		else if(timestamp == hash_old_timestamp)
			// Timestamp is OK, check for duplicates.
			return checkAndInsert(OLD_HASH, fingerprint);
		else
			// Timestamp is invalid.
			return false;
	}

bool checkAndInsert(UINT8 hash, UINT8* value)
	{
		UINT32 value2 = 0;
		memcpy(&value2, value, 4);
		// This is OK if the value is the result of a digest function.
		// In case it's a symmetric key, tinkering with it would make the messages invalid.
		UINT32 position = value2 % (HASH_SIZE / length);

		if(hash == CURRENT_HASH)
			{
				if(hash_current[position * length] == EMPTY_FLAG)
					{
						// The hash table is empty here, simply add the fingerprint.
						memcpy(&hash_current[position * length], value, length);
						return true;
					}
				else if(hash_current[position * length] == COLLISION_FLAG)
					{
						// A overflow list is already in place, check whether it contians
						// duplicates of the current fingerprint.
						struct ListItem* li;
						memcpy(&li, &hash_current[position * length + 2], 4);
						while(li->next != NULL);
							{
								if(!memcmp(&li->value, value, length))
									// Found a duplicate -> this fingerprint is invalid.
									return false;
								li = li->next;
							}
						if(!memcmp(&li->value, value, length))
							// Found a duplicate -> this fingerprint is invalid.
							return false;
						// No duplicates found, add new item at the end of the list.
						li->next = (struct ListItem *)malloc(sizeof(struct ListItem));
						li = li->next;
						memcpy(&li->value, value, length);
						li->next = NULL;
						// Finally, add new item to the global overflow list.
						hash_current_list_pos->next = li;
						hash_current_list_pos = hash_current_list_pos->next;
						return true;
					}
				else
					{
						// The position in the hash table is already occupied, but no
						// overflow exists.
						if(!memcmp(&hash_current[position * length], value, length))
							// Found a duplicate -> this fingerprint is invalid.
							return false;
						// Create an overflow list for this position in the hash table
						// starting with the fingerprint from the hash table.
						struct ListItem* li = (struct ListItem *)malloc(sizeof(struct ListItem));
						memcpy(&li->value, &hash_current[position * length], length);
						// Add this item to the global overflow list, create list if empty.
						if(hash_current_list == NULL)
							{
								hash_current_list = hash_current_list_pos = li;
							}
						else
							{
								hash_current_list_pos->next = li;
								hash_current_list_pos = hash_current_list_pos->next;
							}
						// Now add the new fingerprint to the overflow list.
						li = (struct ListItem *)malloc(sizeof(struct ListItem));
						memcpy(&li->value, value, length);
						li->next = NULL;
						// Add the new item to the global overflow list, too.
						hash_current_list_pos->next = li;
						hash_current_list_pos = hash_current_list_pos->next;
						// Set the flag for overflow list.
						hash_current[position * length] = COLLISION_FLAG;
						// Store a pointer to the overflow list.
						memcpy(&hash_current[position * length + 2], &hash_current_list, 4);
						return true;
					}
			}
		else // (hash == OLD_HASH)
			{
				/*
					The code below is an exact duplicate of the code for CURRENT_HASH,
					except for "hash_old*" variables being used instead of "hash_current*".
				*/
				if(hash_old[position * length] == EMPTY_FLAG)
					{
						memcpy(&hash_old[position * length], value, length);
						return true;
					}
				else if(hash_old[position * length] == COLLISION_FLAG)
					{
						struct ListItem* li;
						memcpy(&li, &hash_old[position * length + 2], 4);
						while(li->next != NULL);
							{
								if(!memcmp(&li->value, value, length))
									return false;
								li = li->next;
							}
						if(!memcmp(&li->value, value, length))
							return false;
						li->next = (struct ListItem *)malloc(sizeof(struct ListItem));
						li = li->next;
						memcpy(&li->value, value, length);
						li->next = NULL;
						hash_old_list_pos->next = li;
						hash_old_list_pos = hash_old_list_pos->next;
						return true;
					}
				else
					{
						if(!memcmp(&hash_old[position * length], value, length))
							return false;
						struct ListItem* li = (struct ListItem *)malloc(sizeof(struct ListItem));
						memcpy(&li->value, &hash_old[position * length], length);
						if(hash_old_list == NULL)
							{
								hash_old_list = hash_old_list_pos = li;
							}
						else
							{
								hash_old_list_pos->next = li;
								hash_old_list_pos = hash_old_list_pos->next;
							}
						li = (struct ListItem *)malloc(sizeof(struct ListItem));
						memcpy(&li->value, value, length);
						li->next = NULL;
						hash_old_list_pos->next = li;
						hash_old_list_pos = hash_old_list_pos->next;
						hash_old[position * length] = COLLISION_FLAG;
						memcpy(&hash_old[position * length + 2], &hash_old_list, 4);
						return true;
					}
			}
	}

/**
 * Parses a 64bit integer
 */
SINT32 parseU64(const UINT8 * str, UINT64& value)
{
	#ifdef HAVE_NATIVE_UINT64
		#ifdef HAVE_ATOLL
			value = (UINT64) atoll((char *)str);
			return E_SUCCESS;
		#endif
		return E_UNKNOWN;
	#else
		///@todo code if we do not have native UINT64
		return E_UNKNOWN;
	#endif
}

/**
 * Parses a timestamp in JDBC timestamp escape format (as it comes from the BI)
 * and outputs the value in seconds since the epoch.
 *
 * @param strTimestamp the string containing the timestamp
 * @param seconds an integer variable that gets the seconds value.
 * @todo think about timezone handling 
 */
SINT32 parseJdbcTimestamp(const UINT8 * strTimestamp, SINT32& seconds)
{
	struct tm time;
	SINT32 rc;
	
	// parse the formatted string
	rc = sscanf((const char*)strTimestamp, "%d-%d-%d %d:%d:%d", 
			&time.tm_year, &time.tm_mon, &time.tm_mday, &time.tm_hour, 
			&time.tm_min, &time.tm_sec);
	if(rc!=6) return E_UNKNOWN; // parsing error
	
	// convert values to struct tm semantic
	if(time.tm_year<1970) return E_UNKNOWN;	
	time.tm_year-=1900;
	if(time.tm_mon<1 || time.tm_mon>12) return E_UNKNOWN;
	time.tm_mon-=1;
	seconds = (UINT32)mktime(&time);
	if(seconds<0) return E_UNKNOWN;
	
	return E_SUCCESS;
}


/**
 * Converts a timestamp (in seconds) to the String JDBC timestamp
 * escape format (YYYY-MM-DD HH:MM:SS)
 * @param seconds integer value containing the timestamp in seconds since the epoch
 * @param strTimestamp a string buffer that gets the result
 * @param len the buffer length
 * @todo think about timezone handling
 */
SINT32 formatJdbcTimestamp(const SINT32 seconds, UINT8 * strTimestamp, const UINT32 len)
{
	struct tm * time;
	time = localtime((time_t *) (&seconds));
	if(strftime((char *)strTimestamp, len, "%Y-%m-%d %H:%M:%S", time) == 0)
	{
		return E_SPACE;
	}
	return E_SUCCESS;
}
