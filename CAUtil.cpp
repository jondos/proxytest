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
SINT32 getcurrentTimeMillis(BIGNUM* bnTime)
	{
		if(bnTime==NULL)
			return E_UNSPECIFIED;
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
	@prama len - number of bytes requested
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
		DOM_Text t=elem.getOwnerDocument().createTextNode(DOMString((char*)tmp));
		elem.appendChild(t);
		return E_SUCCESS;
	}

SINT32 setDOMElementValue(DOM_Element& elem,UINT8* value)
	{
		DOM_Text t=elem.getOwnerDocument().createTextNode(DOMString((char*)value));
		elem.appendChild(t);
		return E_SUCCESS;
	}

SINT32 setDOMElementAttribute(DOM_Element& elem,char* attr,int value)
	{
		UINT8 tmp[10];
		sprintf((char*)tmp,"%i",value);
		elem.setAttribute(attr,DOMString((char*)tmp));
		return E_SUCCESS;
	}

SINT32 getDOMChildByName(const DOM_Node& node,UINT8* name,DOM_Node& child)
	{
		child=node.getFirstChild();
		while(child!=NULL)
			{
				if(child.getNodeName().equals((char*)name))
					return E_SUCCESS;
				child=child.getNextSibling();
			}
		return E_UNKNOWN;
	}

SINT32 getDOMElementValue(DOM_Element& elem,UINT8* value,UINT32* valuelen)
	{
		DOM_Node text=elem.getFirstChild();
		if(!text.isNull())
			{
				DOMString str=text.getNodeValue();

				char* tmpStr=str.transcode();
				*valuelen=str.length();
				memcpy(value,tmpStr,*valuelen);
				return E_SUCCESS;
			}
		return E_UNKNOWN;
	}
SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, UINT8* xml, UINT32* xmllen,CAASymCipher* pRSA)
	{
#define XML_ENCODE_KEY_TEMPLATE "<EncryptedKey><EncryptionMethod Algorithm=\"RSA\"/><CipherData><CipherValue>%s</CipherValue></CipherData></EncryptedKey>"
		UINT8 tmpBuff[1024];
		memset(tmpBuff,0,sizeof(tmpBuff));
		memcpy(tmpBuff+128-keylen,key,keylen);
		pRSA->encrypt(tmpBuff,tmpBuff);
		UINT32 len=1024;
		CABase64::encode(tmpBuff,128,tmpBuff,&len);
		tmpBuff[len]=0;
		sprintf((char*)xml,XML_ENCODE_KEY_TEMPLATE,tmpBuff);
		*xmllen=strlen((char*)xml);
		return E_SUCCESS;
	}

SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, UINT8* xml, UINT32 xmllen,CAASymCipher* pRSA)
	{
		char* start=strstr((char*)xml,"<CipherValue>");
		start+=13;
		char* end=strstr(start,"</CipherValue>");
		UINT8 tmpBuff[1024];
		UINT32 len=1024;
		CABase64::decode((UINT8*)start,end-start,tmpBuff,&len);
		pRSA->decrypt(tmpBuff,tmpBuff);
		*keylen=16;
		memcpy(key,tmpBuff+128-(*keylen),(*keylen));
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
