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
#include "CASymCipher.hpp"

#ifdef HAVE_SBRK
	char* internal_sbrk_start=(char*)sbrk(0);
#endif

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

UINT8* bytes2hex(const void* bytes,UINT32 len)
	{
		if(bytes==NULL||len==0)
			return NULL;
		UINT8* buff=new UINT8[len*3+1];
		UINT32 aktInd=0;
		for(UINT32 i=0;i<len;i++)
			{
				UINT8 b1=((UINT8*)bytes)[i];
				UINT8 b=(b1>>4);
				if(b>9)
					b+=55;
				else
					b+=48;
				buff[aktInd++]=b;
				b=(b1&0x0F);
				if(b>9)
					b+=55;
				else
					b+=48;
				buff[aktInd++]=b;
				buff[aktInd++]=32;
			}
		buff[len*3]=0;
		return buff;
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

/** Log information about the current memory (heap) usage. */
void logMemoryUsage()
	{
#ifdef HAVE_SBRK
		CAMsg::printMsg(LOG_DEBUG,"Memory consumption reported by sbrk(): %u\n",(long)((char*)sbrk(0)-internal_sbrk_start));
#endif
#ifdef HAVE_MALLINFO
		struct mallinfo malli=mallinfo();
		//memset(&malli,0,sizeo(malli));
		CAMsg::printMsg(LOG_DEBUG,"Memory consumption reported by mallinfo():\n");
		CAMsg::printMsg(LOG_DEBUG,"\t Total size of memory allocated with sbrk() by malloc() [bytes]: %i\n",malli.arena);
		CAMsg::printMsg(LOG_DEBUG,"\t Number of chunks not in use: %i\n",malli.ordblks);
		CAMsg::printMsg(LOG_DEBUG,"\t Total number of chunks allocated with mmap(): %i\n",malli.hblks);
		CAMsg::printMsg(LOG_DEBUG,"\t Total size of memory allocated with mmap() [byte]: %i\n",malli.hblkhd);
		CAMsg::printMsg(LOG_DEBUG,"\t Total size of memory occupied by chunks handed out by malloc(): %i\n",malli.uordblks);
		CAMsg::printMsg(LOG_DEBUG,"\t Total size of memory occupied by free (not in use) chunks: %i\n",malli.fordblks);
		CAMsg::printMsg(LOG_DEBUG,"\t Size of the top-most releasable chunk that normally borders the end of the heap: %i\n",malli.keepcost);
#endif
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
	
SINT32 getRandom(UINT64* val)
	{
		ASSERT(val!=NULL,"VAL should be not NULL");
		if(RAND_bytes((UINT8*)val,sizeof(UINT64))!=1&&
			 RAND_pseudo_bytes((UINT8*)val,sizeof(UINT64))<0)
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

#ifndef ONLY_LOCAL_PROXY
SINT32 setDOMElementValue(DOM_Element& elem,UINT32 text)
	{
		UINT8 tmp[10];
		sprintf((char*)tmp,"%u",text);
		setDOMElementValue(elem,tmp);
		return E_SUCCESS;
	}

	
SINT32 setDOMElementValue(DOM_Element& elem,double floatValue)
	{
		UINT8 tmp[10];
		sprintf((char*)tmp,"%.2f",floatValue);
		setDOMElementValue(elem,tmp);
		return E_SUCCESS;
	}
	

/**
 * Sets the decimal text representation of a 64bit integer as node value
 * TODO: implement this for non-64bit platforms
 */
SINT32 setDOMElementValue(DOM_Element & elem, const UINT64 text)
	{
		UINT8 tmp[32];
		print64(tmp,text);
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

SINT32 setDOMElementAttribute(DOM_Node& elem,const char* attrName,SINT32 value)
	{
		if(elem.isNull()||elem.getNodeType()!=DOM_Node::ELEMENT_NODE||
				attrName==NULL)
			return E_UNKNOWN;
		UINT8 tmp[10];
		sprintf((char*)tmp,"%i",value);
		static_cast<DOM_Element&>(elem).setAttribute(attrName,DOMString((char*)tmp));
		return E_SUCCESS;
	}

SINT32 getDOMElementAttribute(const DOM_Node& elem,const char* attrName,UINT8* value,UINT32* len)
	{
		if(elem==NULL||attrName==NULL||value==NULL||len==NULL||elem.getNodeType()!=DOM_Node::ELEMENT_NODE)
			return E_UNKNOWN;
		char* tmpStr=static_cast<const DOM_Element&>(elem).getAttribute(attrName).transcode();
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

SINT32 getDOMElementAttribute(const DOM_Node& elem,const char* attrName,SINT32* value)
	{
		if(elem==NULL||attrName==NULL||value==NULL||elem.getNodeType()!=DOM_Node::ELEMENT_NODE)
			return E_UNKNOWN;
		char* tmpStr=static_cast<const DOM_Element&>(elem).getAttribute(attrName).transcode();
		*value=atol(tmpStr);
		delete[] tmpStr;
		return E_SUCCESS;
	}

SINT32 getDOMElementAttribute(const DOM_Node& elem,const char* attrName,UINT32& value)
	{
		if(elem==NULL||attrName==NULL||elem.getNodeType()!=DOM_Node::ELEMENT_NODE)
			return E_UNKNOWN;
		char* tmpStr=static_cast<const DOM_Element&>(elem).getAttribute(attrName).transcode();
		long l=atol(tmpStr);
		delete[] tmpStr;
		if(l<0)
			return E_UNKNOWN;
		value=(UINT32)l;
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
	
SINT32 getLastDOMChildByName(const DOM_Node& node,const UINT8* const name,DOM_Node& a_child)
	{
		if(node==NULL)
			{
				return E_UNKNOWN;
			}
		DOM_Node child;
		child=node.getLastChild();
		while(child!=NULL)
			{
				if(child.getNodeName().equals((char*)name))
					{
						a_child = child; // found a child
						return E_SUCCESS;
					}
				child=child.getPreviousSibling();
			}
		return E_UNKNOWN;
	}

/** 
 * Returns the content of the text node(s) under elem
 * as null-terminated C String. If there is no text node
 * len is set to 0.
 * 
 * TODO: Why is elem a DOM_Node and not a DOM_Element here?
 * @param elem the element which has a text node under it
 * @param value a buffer that gets the text value
 * @param valuelen on call contains the buffer size, on return contains the number of bytes copied
 * @retval E_SPACE if the buffer is too small
 * @retval E_UNKNOWN if the element is NULL
 * @retval E_SUCCESS otherwise
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
	
SINT32 getDOMElementValue(const DOM_Element& elem,double* value)
	{
		ASSERT(value!=NULL,"Value is null");
		ASSERT(elem!=NULL,"Element is NULL");
		UINT8 buff[255];
		UINT32 buffLen=255;
		if(getDOMElementValue(elem,buff,&buffLen)!=E_SUCCESS)
			return E_UNKNOWN;
		*value=atof((char*)buff);
		
		return E_SUCCESS;
	}
	
	
SINT32 getDOMElementValue(const DOM_Element& elem,UINT32& value, UINT32 defaultValue)
{
	UINT32 v;
	if(getDOMElementValue(elem,&v)!=E_SUCCESS)
		{
		value=defaultValue;
		}
	else
	value=v;	
	return E_SUCCESS;
}

SINT32 getDOMElementValue(const DOM_Element& elem, UINT64 &value)
{
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
@verbatim
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
	*@endverbatim
	*/
SINT32 encryptXMLElement(DOM_Node node, CAASymCipher* pRSA)
	{
		DOM_Document doc;
		DOM_Node parent;
		if(node.getNodeType()==DOM_Node::DOCUMENT_NODE)
			{
				doc=(DOM_Document&)node;
				parent=doc;
				node=doc.getDocumentElement();
			}
		else
			{
				doc=node.getOwnerDocument();
				parent=node.getParentNode();
			}
		DOM_Element elemRoot=doc.createElement("EncryptedData");
		DOM_Element elemKeyInfo=doc.createElement("ds:KeyInfo");
		elemRoot.appendChild(elemKeyInfo);
		DOM_Element elemEncKey=doc.createElement("EncryptedKey");
		elemKeyInfo.appendChild(elemEncKey);
		DOM_Element elemCipherData=doc.createElement("CipherData");
		elemEncKey.appendChild(elemCipherData);
		DOM_Element elemCipherValue=doc.createElement("CipherValue");
		elemCipherData.appendChild(elemCipherValue);
		UINT8 key[32];
		getRandom(key,32);
		UINT8 buff[1000];
		UINT32 bufflen=255;
		pRSA->encryptOAEP(key,32,buff,&bufflen);
		UINT8 outbuff[1000];
		UINT32 outbufflen=255;
		CABase64::encode(buff,bufflen,outbuff,&outbufflen);
		outbuff[outbufflen]=0;
		setDOMElementValue(elemCipherValue,outbuff);
		CASymCipher *pSymCipher=new CASymCipher();
		pSymCipher->setKey(key,true);
		pSymCipher->setIVs(key+16);
		elemCipherData=doc.createElement("CipherData");
		elemRoot.appendChild(elemCipherData);
		elemCipherValue=doc.createElement("CipherValue");
		elemCipherData.appendChild(elemCipherValue);
		UINT8* b=DOM_Output::dumpToMem(node,&bufflen);
		outbufflen=1000;
		pSymCipher->encrypt1CBCwithPKCS7(b,bufflen,outbuff,&outbufflen);
		delete []b;
		bufflen=1000;
		CABase64::encode(outbuff,outbufflen,buff,&bufflen);
		buff[bufflen]=0;
		setDOMElementValue(elemCipherValue,buff);
		if(parent.getNodeType()==DOM_Node::DOCUMENT_NODE)
			{
				parent.removeChild(node);
				parent.appendChild(elemRoot);
			}
		else
			{
				parent.replaceChild(elemRoot,node);
			}
		return E_SUCCESS;
	}
#endif //ONLY_LOCAL_PROXY

/** Encrypts an XML-Element by wrapping it with:
*@verbatim
	* \<EncryptedData Type='http://www.w3.org/2001/04/xmlenc#Element'>
	*		\<EncryptionMethod Algorithm="http://www.w3.org/2001/04/xmlenc#aes128-cbc"/>
	*   \<ds:KeyInfo xmlns:ds='http://www.w3.org/2000/09/xmldsig#'>
	*			\<EncryptedKey>
	*				\<EncryptionMethod Algorithm="http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p"/>
	*				\<CipherData\>
	*					\<CipherValue>...\</CipherValue>					
	*				\</CipherData>
	*			\</EncryptedKey>
	*		\</ds:KeyInfo>
	*		\<CipherData>
	*			\<CipherValue>...\</CipherValue>
	*		\</CipherData>
	*	\</EncryptedData>
	*@endverbatim
	@return a buffer with the encyrpted element (the caller is responsible for freeing it), or NULL
*/
UINT8* encryptXMLElement(UINT8* inbuff,UINT32 inlen,UINT32& outlen,CAASymCipher* pRSA)
	{
		const char* XML_ENC_TEMPLATE="<EncryptedData><ds:KeyInfo><EncryptedKey><CipherData><CipherValue>%s</CipherValue></CipherData></EncryptedKey></ds:KeyInfo><CipherData><CipherValue>%s</CipherValue></CipherData></EncryptedData>";
		UINT8 key[32];
		getRandom(key,32);
		UINT8 buff[1000];
		UINT32 bufflen=255;
		pRSA->encryptOAEP(key,32,buff,&bufflen);
		UINT8 keyoutbuff[1000];
		UINT32 keyoutbufflen=255;
		CABase64::encode(buff,bufflen,keyoutbuff,&keyoutbufflen);
		keyoutbuff[keyoutbufflen]=0;
		CASymCipher *pSymCipher=new CASymCipher();
		pSymCipher->setKey(key,true);
		pSymCipher->setIVs(key+16);
		UINT8* msgoutbuff=new UINT8[10000];
		UINT32 msgoutbufflen=10000;
		pSymCipher->encrypt1CBCwithPKCS7(inbuff,inlen,msgoutbuff,&msgoutbufflen);
		UINT8* encmsgoutbuff=new UINT8[10000];
		UINT32 encmsgoutbufflen=10000;
		CABase64::encode(msgoutbuff,msgoutbufflen,encmsgoutbuff,&encmsgoutbufflen);
		encmsgoutbuff[encmsgoutbufflen]=0;
		sprintf((char*)msgoutbuff,XML_ENC_TEMPLATE,keyoutbuff,encmsgoutbuff);
		outlen=strlen((char*)msgoutbuff);
		delete[] encmsgoutbuff;
		return msgoutbuff;
	}

#ifndef ONLY_LOCAL_PROXY
SINT32 decryptXMLElement(DOM_Node node, CAASymCipher* pRSA)
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
				delete[] cipherValue;
				return E_UNKNOWN;
			}
		CABase64::decode(cipherValue,len,cipherValue,&len);
		if(	pRSA->decryptOAEP(cipherValue,cipherValue,&len)!=E_SUCCESS||
				len!=32)
			{
				delete[] cipherValue;
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
				delete[] cipherValue;
				return E_UNKNOWN;
			}
		CABase64::decode(cipherValue,len,cipherValue,&len);
		SINT32 ret=pSymCipher->decrypt1CBCwithPKCS7(cipherValue,cipherValue,&len);
		delete pSymCipher;
		if(ret!=E_SUCCESS)
			{
				delete[] cipherValue;
				return E_UNKNOWN;
			}
		//now the need to parse the plaintext...
		MemBufInputSource oInput(cipherValue,len,"decryptelement");
		DOMParser oParser;
		oParser.parse(oInput);
		delete[] cipherValue;		
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
#endif //ONLY_LOCAL_PROXY

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
 * Parses a 64bit unsigned integer.
 * Note: If the value is out of range or not parseable an erro is returned.
 */
SINT32 parseU64(const UINT8 * str, UINT64& value)
{
	#ifdef HAVE_NATIVE_UINT64
			if (str == NULL)
			{
				return E_UNKNOWN;
			}
			UINT32 len=strlen((char*)str);
			if (len < 1)
			{
				return E_UNKNOWN;
			}
			UINT64 u64 = 0;
			for (UINT32 i = 0; i < len; i++)
			{
				UINT8 c=str[i];
				if (c >= '0' && c <= '9')
				{
					u64 *= 10;
					u64 += c - '0';
				}
				else if (i != 0 || str[i] != '+')
				{
					return E_UNKNOWN;
				}
			}
			value = u64;
			return E_SUCCESS;
	#else
		#warning parseU64() is not implemented for platforms without native UINT64 support!!!
		///@todo code if we do not have native UINT64
		return E_UNKNOWN;
	#endif
}

SINT32 readPasswd(UINT8* buff,UINT32 len)
	{
		if(len==0)
			return E_SUCCESS;

#ifndef _WIN32
		termios tmpTermios;
		UINT32 flags;
		bool bRestore=true;
		if(tcgetattr(STDIN_FILENO,&tmpTermios)!=0)
			{
				bRestore=false;
			}
		flags=tmpTermios.c_lflag;
		tmpTermios.c_lflag&=~(ECHO);
		if(bRestore)
			tcsetattr(STDIN_FILENO,TCSAFLUSH,&tmpTermios);
#endif

		UINT32 i=0;
		for(i=0;i<len-1;i++)
			{
#ifdef _WIN32
				int c=::getch();
#else
				int c=getchar();
#endif
				if(c<=0||c=='\r'||c=='\n')
					break;
				buff[i]=c;
			}
		buff[i]=0;

#ifndef _WIN32
		tmpTermios.c_lflag=flags;
		if(bRestore)
			tcsetattr(STDIN_FILENO,TCSAFLUSH,&tmpTermios);
#endif
		return E_SUCCESS;
	}	

/**
 * Parses a timestamp in JDBC timestamp escape format (as it comes from the BI)
 * and outputs the value in seconds since the epoch.
 *
 * @param strTimestamp the string containing the timestamp
 * @param seconds an integer variable that gets the seconds value.
 * @todo think about timezone handling 
 */
/*SINT32 parseJdbcTimestamp(const UINT8 * strTimestamp, SINT32& seconds)
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
}*/


/**
 * Converts a timestamp (in seconds) to the String JDBC timestamp
 * escape format (YYYY-MM-DD HH:MM:SS)
 * @param seconds integer value containing the timestamp in seconds since the epoch
 * @param strTimestamp a string buffer that gets the result
 * @param len the buffer length
 * @todo think about timezone handling
 */
/*SINT32 formatJdbcTimestamp(const SINT32 seconds, UINT8 * strTimestamp, const UINT32 len)
{
	struct tm * time;
	time = localtime((time_t *) (&seconds));
	// without this line, there are problems on 64 BIT machines!!
	CAMsg::printMsg( LOG_DEBUG, "Year: %d Month: %d\n", time->tm_year, time->tm_mon);
	
	if(strftime((char *)strTimestamp, len, "%Y-%m-%d %H:%M:%S", time) == 0)
	{
		return E_SPACE;
	}
	return E_SUCCESS;
}*/
