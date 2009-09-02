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

#ifdef LOG_CRIME
	#include "tre/regex.h"
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

/*
 * compares date1 with date2. Note: only the date is compared, not the time
 * returns:
 * 	-1 if date1 < date2
 *   0 if date1 == date2
 * 	 1 if date1 > date2
 */
SINT32 compDate(struct tm *date1, struct tm *date2)
{
	//year
	if(date1->tm_year != date2->tm_year)
	{
		return (date1->tm_year < date2->tm_year) ? -1 : 1;
	}
	if(date1->tm_mon != date2->tm_mon)
	{
		return (date1->tm_mon < date2->tm_mon) ? -1 : 1;
	}
	if(date1->tm_mday != date2->tm_mday)
	{
		return (date1->tm_mday < date2->tm_mday) ? -1 : 1;
	}
	return 0;
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

SINT32 getDOMChildByName(const DOMNode* pNode,const char * const name,DOMElement* & child,bool deep)
	{
		return getDOMChildByName(pNode,name,(DOMNode*&)child,deep);
	}
SINT32 getDOMChildByName(const DOMNode* pNode,const XMLCh* const name,DOMNode* & a_child,bool deep)
	{
		a_child=NULL;
		if(pNode==NULL)
			return E_UNKNOWN;
		DOMNode *pChild=pNode->getFirstChild();
		while(pChild!=NULL)
			{
				if(XMLString::equals(pChild->getNodeName(),name))
					{
						a_child=pChild;
						return E_SUCCESS;
					}
				if(deep)
					{
						if(getDOMChildByName(pChild,name,a_child,deep)==E_SUCCESS)
							return E_SUCCESS;
					}
				pChild=pChild->getNextSibling();
			}
		return E_UNKNOWN;
	}

SINT32 getDOMChildByName(const DOMNode* pNode,const char* const name,DOMNode* & a_child,bool deep)
	{
		a_child=NULL;
		if(pNode==NULL)
			return E_UNKNOWN;
		XMLCh* tmpName=XMLString::transcode((const char * const)name);
		SINT32 ret=getDOMChildByName(pNode,tmpName,a_child,deep);
		XMLString::release(&tmpName);
		return ret;
	}

SINT32 getSignatureElements(DOMNode* parent, DOMNode** signatureNodes, UINT32* length)
{
	if(parent == NULL)
	{
		return E_UNKNOWN;
	}

	DOMNode* child = parent->getFirstChild();
	UINT32 count = 0;

	while(child != NULL)
	{
		if(XMLString::equals(child->getNodeName(), XMLString::transcode("Signature")))
		{
			if(count < *length)
			{
				signatureNodes[count] = child;
				count++;
			}
			else
			{
				return E_UNKNOWN;
			}
		}
		child = child->getNextSibling();
	}
	*length = count;

	return E_SUCCESS;
}

/**
 * integrates the source node in the destination Node.
 *  TODO 1. test for XERCES >= 3.0.1
 *  	 2. specification
 */
SINT32 integrateDOMNode(const DOMNode *srcNode, DOMNode *dstNode, bool recursive, bool replace)
{
	if( (srcNode->getNodeType() != DOMNode::ELEMENT_NODE) ||
		(dstNode->getNodeType() != DOMNode::ELEMENT_NODE) )
	{
		return E_UNKNOWN;
	}

	DOMNodeList *srcList = srcNode->getChildNodes();
	XERCES_CPP_NAMESPACE::DOMDocument *srcOwnerDoc = srcNode->getOwnerDocument();
	XERCES_CPP_NAMESPACE::DOMDocument *dstOwnerDoc = dstNode->getOwnerDocument();

	short int pos =
#if _XERCES_VERSION >= 30001
		srcNode->compareDocumentPosition(dstNode);
#else
		srcNode->compareTreePosition(dstNode);
#endif
	if( (pos & INTEGRATE_NOT_ALLOWED_POSITIONS)  )
	{
		CAMsg::printMsg(LOG_ERR,"integrate impossible due to illegal tree positions, (pos: 0x%x)\n", pos);
		return E_UNKNOWN;
	}

	if(srcList->getLength() == 0)
	{
		return E_SUCCESS;
	}

	DOMElement *srcElem = (DOMElement *) srcNode;
	DOMElement *dstElem = (DOMElement *) dstNode;

	DOMNode *currSrcChild = NULL;
	XMLCh** nodeNames=new XMLCh*[srcList->getLength()];
	memset(nodeNames,0,sizeof(XMLCh*)*srcList->getLength());
	UINT32 nodeNamesIndex = 0;
	XMLCh *currSrcChildName = NULL;

	DOMNodeList *currSrcChildren = NULL;
	DOMNodeList *currDstChildren = NULL;
	bool nodeAlreadyFinished = false;

	for(XMLSize_t i = 0; i < srcList->getLength(); i++)
	{
		currSrcChild = srcList->item(i);
		if( currSrcChild->getNodeType() == DOMNode::ELEMENT_NODE )
		{
			nodeAlreadyFinished = false;
			currSrcChildName = (XMLCh *) ((DOMElement *) currSrcChild)->getTagName();
			/*UINT8 *tn = (UINT8 *) XMLString::transcode(currSrcChildName);
			CAMsg::printMsg(LOG_DEBUG,"handle %s\n", tn);
			XMLString::release(&tn);*/
			for(UINT32 i = 0; i < nodeNamesIndex; i++ )
			{
				if(XMLString::equals(currSrcChildName, nodeNames[i]))
				{
					nodeAlreadyFinished = true;
					break;
				}
			}

			if(nodeAlreadyFinished)
			{
				continue;
			}
			currDstChildren = dstElem->getElementsByTagName(currSrcChildName);
			currSrcChildren = srcElem->getElementsByTagName(currSrcChildName);

			for(XMLSize_t j = 0;
				j < currSrcChildren->getLength(); j++ )
			{
				if(j >= currDstChildren->getLength())
				{
					if( (dstOwnerDoc != NULL) && (srcOwnerDoc != dstOwnerDoc) )
					{
						dstNode->appendChild(dstOwnerDoc->importNode(currSrcChildren->item(j), true));
					}
					else
					{
						dstNode->appendChild(currSrcChildren->item(j)->cloneNode(true));
					}
				}
				else if(replace)
				{
					if( (dstOwnerDoc != NULL) && (srcOwnerDoc != dstOwnerDoc) )
					{
						dstElem->replaceChild(
							dstOwnerDoc->importNode(currSrcChildren->item(j),true),
							currDstChildren->item(j));
					}
					else
					{
						dstElem->replaceChild(
							dstOwnerDoc->cloneNode(currSrcChildren->item(j)),
							currDstChildren->item(j));
					}
					continue;
				}
				else if(recursive)
				{
					if(currSrcChildren->item(j)->hasChildNodes() )
					{
						integrateDOMNode(currSrcChildren->item(j), currDstChildren->item(j), true, false);
					}
				}
				nodeNames[nodeNamesIndex++] = currSrcChildName;
			}
		}
	}
	delete[] nodeNames;
	return E_SUCCESS;
}

bool equals(const XMLCh* const e1,const char* const e2)
	{
		XMLCh* e3=XMLString::transcode(e2);
		bool ret=XMLString::equals(e1,e3);
		XMLString::release(&e3);
		return ret;
	}

XercesDOMParser* theDOMParser=NULL;
CAMutex* theParseDOMDocumentLock = NULL;

void initDOMParser()
{
	theParseDOMDocumentLock = new CAMutex();
	theDOMParser = new XercesDOMParser();
}

XERCES_CPP_NAMESPACE::DOMDocument* parseDOMDocument(const UINT8* const buff, UINT32 len)
	{
		theParseDOMDocumentLock->lock();
		MemBufInputSource in(buff,len,"tmpBuff");
		theDOMParser->parse(in);
		XERCES_CPP_NAMESPACE::DOMDocument* ret=NULL;
		if(theDOMParser->getErrorCount()==0)
			ret=theDOMParser->adoptDocument();
		theParseDOMDocumentLock->unlock();
		return ret;
	}

/**
 * parses a file via path or URL
 */
XERCES_CPP_NAMESPACE::DOMDocument* parseDOMDocument(const UINT8* const pathOrURL)
{
	theParseDOMDocumentLock->lock();

	theDOMParser->parse((const char *const) pathOrURL);
	XERCES_CPP_NAMESPACE::DOMDocument* ret=NULL;
	if(theDOMParser->getErrorCount()==0)
	{
		ret=theDOMParser->adoptDocument();
	}
	theParseDOMDocumentLock->unlock();
	return ret;
}

void releaseDOMParser()
	{
		if(	theParseDOMDocumentLock!=NULL)
			{
				theParseDOMDocumentLock->lock();
				delete theDOMParser;
				theDOMParser=NULL;
				theParseDOMDocumentLock->unlock();
				delete theParseDOMDocumentLock;
				theParseDOMDocumentLock=NULL;
			}
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
SINT32 getDOMElementValue(const DOMNode * const pElem,UINT8* value,UINT32* valuelen)
	{
		ASSERT(value!=NULL,"Value is null");
		ASSERT(valuelen!=NULL,"ValueLen is null");
		ASSERT(pElem!=NULL,"Element is NULL");
		if(pElem==NULL)
			return E_UNKNOWN;
		DOMNode*  pText=pElem->getFirstChild();
		UINT32 spaceLeft=*valuelen;
		*valuelen=0;
		while(pText!=NULL)
			{
				if(pText->getNodeType()==DOMNode::TEXT_NODE)
					{
						const XMLCh* str=pText->getNodeValue();
						char* tmpStr=XMLString::transcode(str);
						UINT32 tmpStrLen=strlen(tmpStr);
						if(tmpStrLen>=spaceLeft)
							{
								*valuelen=tmpStrLen+1;
								XMLString::release(&tmpStr);
								return E_SPACE;
							}
						memcpy(value+(*valuelen),tmpStr,tmpStrLen);
						*valuelen+=tmpStrLen;
						spaceLeft-=tmpStrLen;
						XMLString::release(&tmpStr);
					}
				pText=pText->getNextSibling();
			}
		value[*valuelen]=0;
		return E_SUCCESS;
	}

SINT32 getDOMElementAttribute(const DOMNode * const elem,const char* attrName,UINT8* value,UINT32* len)
	{
		if(elem==NULL||attrName==NULL||value==NULL||len==NULL||elem->getNodeType()!=DOMNode::ELEMENT_NODE)
			return E_UNKNOWN;
		XMLCh* name=XMLString::transcode(attrName);
		const XMLCh* tmpCh=((DOMElement*)elem)->getAttribute(name);
		XMLString::release(&name);
		char* tmpStr=XMLString::transcode(tmpCh);
		UINT32 l=strlen(tmpStr);
		if(l>=*len)
			{
				XMLString::release(&tmpStr);
				return E_SPACE;
			}
		*len=l;
		memcpy(value,tmpStr,l+1);
		XMLString::release(&tmpStr);
		return E_SUCCESS;
	}

SINT32 getDOMElementAttribute(const DOMNode * const elem,const char* attrName,SINT32* value)
	{
		UINT8 val[50];
		UINT32 len=50;
		if(getDOMElementAttribute(elem,attrName,val,&len)!=E_SUCCESS)
			return E_UNKNOWN;
		*value=atol((char*)val);
		return E_SUCCESS;
	}

DOMElement* createDOMElement(XERCES_CPP_NAMESPACE::DOMDocument* pOwnerDoc,const char * const name)
	{
		XMLCh* n=XMLString::transcode(name);
		DOMElement* ret=pOwnerDoc->createElement(n);
		XMLString::release(&n);
		return ret;
	}

DOMText* createDOMText(XERCES_CPP_NAMESPACE::DOMDocument* pOwnerDoc,const char * const text)
	{
		XMLCh* t=XMLString::transcode(text);
		DOMText* ret= pOwnerDoc->createTextNode(t);
		XMLString::release(&t);
		return ret;
	}

SINT32 setDOMElementAttribute(DOMNode* pElem,const char* attrName,const UINT8* value)
{
	if(pElem==NULL||pElem->getNodeType()!=DOMNode::ELEMENT_NODE||attrName==NULL||value==NULL)
		return E_UNKNOWN;
	XMLCh* name=XMLString::transcode(attrName);
	XMLCh* val=XMLString::transcode((const char*)value);
	((DOMElement*)pElem)->setAttribute(name,val);
	XMLString::release(&name);
	XMLString::release(&val);
	return E_SUCCESS;
}

SINT32 setCurrentTimeMilliesAsDOMAttribute(DOMNode *pElem)
{
		if( pElem == NULL || pElem->getNodeType() != DOMNode::ELEMENT_NODE )
		{
			return E_UNKNOWN;
		}
		UINT64 currentMillis;
		getcurrentTimeMillis(currentMillis);
		UINT8 tmpStrCurrentMillis[50];
		print64(tmpStrCurrentMillis,currentMillis);
		setDOMElementAttribute
			(pElem,UNIVERSAL_ATTRIBUTE_LAST_UPDATE, tmpStrCurrentMillis );
		return E_SUCCESS;

}

//if not null the returned char pointer must be explicitely freed by the caller with 'delete []'
UINT8 *getTermsAndConditionsTemplateRefId(DOMNode *tcTemplateRoot)
{
	UINT32 tmpTypeLen = TMP_BUFF_SIZE;
	UINT8 tmpType[TMP_BUFF_SIZE];

	UINT32 tmpLocaleLen = TMP_LOCALE_SIZE;
	UINT8 tmpLocale[TMP_LOCALE_SIZE];

	UINT32 tmpDateLen = TMP_DATE_SIZE;
	UINT8 tmpDate[TMP_DATE_SIZE];
	memset(tmpDate, 0, TMP_DATE_SIZE);
	memset(tmpLocale, 0, TMP_LOCALE_SIZE);
	memset(tmpType, 0, TMP_BUFF_SIZE);

	//TODO replace magic strings
	if(getDOMElementAttribute(tcTemplateRoot, "type", tmpType, &tmpTypeLen) != E_SUCCESS)
	{
		return NULL;
	}
	else if(getDOMElementAttribute(tcTemplateRoot, "locale", tmpLocale, &tmpLocaleLen) != E_SUCCESS)
	{
		return NULL;
	}
	else if(getDOMElementAttribute(tcTemplateRoot, "date", tmpDate, &tmpDateLen) != E_SUCCESS)
	{
		return NULL;
	}
	if( (tmpTypeLen == 0) ||
		(tmpLocaleLen == 0) ||
		(tmpDateLen) == 0)
	{
		return NULL;
	}
	//reserve 2 more chars for the both underlines between the fields ...
	size_t templateRefIdLen = tmpTypeLen+tmpLocaleLen+tmpDateLen+2;
	//... and 1 more for zero termination.
	char *templateRefId = new char[templateRefIdLen+1];
	memset(templateRefId, 0, templateRefIdLen+1);
	snprintf(templateRefId, templateRefIdLen+1, "%s_%s_%s", (char *) tmpType, (char *) tmpLocale, (char *) tmpDate);

	return (UINT8 *) templateRefId;
}

#ifndef ONLY_LOCAL_PROXY
DOMNodeList* getElementsByTagName(DOMElement* pElem,const char* const name)
	{
		XMLCh* tmpCh=XMLString::transcode(name);
		DOMNodeList* ret=pElem->getElementsByTagName(tmpCh);
		XMLString::release(&tmpCh);
		return ret;
	}

SINT32 getLastDOMChildByName(const DOMNode* pNode,const char * const name,DOMElement* & a_child)
	{
		return getLastDOMChildByName(pNode,name,(DOMNode*&)a_child);
	}

XERCES_CPP_NAMESPACE::DOMDocument* createDOMDocument()
	{
		DOMImplementation* pImpl=DOMImplementation::getImplementation();
		return pImpl->createDocument();
	}

SINT32 setDOMElementValue(DOMElement* pElem, UINT32 value)
	{
		UINT8 tmp[11];
		memset(tmp, 0, 11);
		snprintf((char*)tmp, 10, "%u", value);
		setDOMElementValue(pElem,tmp);
		return E_SUCCESS;
	}

SINT32 setDOMElementValue(DOMElement* pElem, SINT32 value)
	{
		//One more char for the sign of negative numbers
		UINT8 tmp[12];
		memset(tmp, 0, 12);
		snprintf((char*)tmp, 11, "%d", value);
		setDOMElementValue(pElem,tmp);
		return E_SUCCESS;
	}

SINT32 setDOMElementValue(DOMElement* pElem,double floatValue)
	{
		char tmp[400];
		snprintf(tmp,400, "%.2f", floatValue);
		setDOMElementValue(pElem,(UINT8 *)tmp);
		return E_SUCCESS;
	}


/**
 * Sets the decimal text representation of a 64bit integer as node value
 * TODO: implement this for non-64bit platforms
 */
SINT32 setDOMElementValue(DOMElement* pElem, const UINT64 text)
	{
		UINT8 tmp[32];
		memset(tmp, 0, 32);
		print64(tmp,text);
		setDOMElementValue(pElem,tmp);
		return E_SUCCESS;
	}


SINT32 setDOMElementValue(DOMElement* pElem,const UINT8* value)
	{
		XMLCh* val=XMLString::transcode((const char *)value);
		DOMText* pText=pElem->getOwnerDocument()->createTextNode(val);
		XMLString::release(&val);
		//Remove all "old" text Elements...
		DOMNode* pChild=pElem->getFirstChild();
		while(pChild!=NULL)
		{
			if(pChild->getNodeType()==DOMNode::TEXT_NODE)
				{
					DOMNode* n=pElem->removeChild(pChild);
					if (n != NULL)
					{
						n->release();
						n = NULL;
					}
				}
			pChild=pChild->getNextSibling();
		}
		pElem->appendChild(pText);
		return E_SUCCESS;
	}

SINT32 setDOMElementAttribute(DOMNode* pElem, const char* attrName, UINT64 value)
{
	UINT8 tmp[50];
	print64(tmp, value);
	return setDOMElementAttribute(pElem, attrName, tmp);
}

SINT32 setDOMElementAttribute(DOMNode* pElem, const char* attrName, SINT64 value)
{
	UINT8 tmp[50];
	memset(tmp, 0, 50);
	snprintf((char *) tmp, 50, "%lld", value);
	return setDOMElementAttribute(pElem, attrName, tmp);
}

SINT32 setDOMElementAttribute(DOMNode* pElem,const char* attrName, SINT32 value)
{
	UINT8 tmp[10];
	sprintf((char*)tmp, "%i", value);
	return setDOMElementAttribute(pElem, attrName, tmp);
}

SINT32 setDOMElementAttribute(DOMNode* pElem,const char* attrName, UINT32 value)
{
	UINT8 tmp[10];
	sprintf((char*)tmp, "%u", value);
	return setDOMElementAttribute(pElem, attrName, tmp);
}

SINT32 setDOMElementAttribute(DOMNode* pElem, const char* attrName, bool value)
{
	return setDOMElementAttribute(pElem, attrName,
			((UINT8*) (value ? STR_VALUE_TRUE : STR_VALUE_FALSE)));
}

SINT32 setDOMElementValue(DOMElement* pElem, bool value)
{
	return setDOMElementValue(pElem,((UINT8*) (value ? STR_VALUE_TRUE : STR_VALUE_FALSE)));
}

SINT32 getDOMElementAttribute(const DOMNode * const elem,const char* attrName,SINT64& value)
{
	UINT8 val[50];
	UINT32 len=50;
	if(getDOMElementAttribute(elem,attrName,val,&len)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	if(parseS64(val,value)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}

SINT32 getDOMElementAttribute(const DOMNode * const elem,const char* attrName, UINT64& value)
{
	UINT8 val[50];
	UINT32 len=50;
	if(getDOMElementAttribute(elem,attrName,val,&len)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	if(parseU64(val,value)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}

SINT32 getDOMElementAttribute(const DOMNode * const elem,const char* attrName,UINT32& value)
{
	UINT8 val[50];
	UINT32 len=50;
	if(getDOMElementAttribute(elem,attrName,val,&len)!=E_SUCCESS)
		return E_UNKNOWN;
	long l=atol((char*)val);
	if(l<0)
		return E_UNKNOWN;
	value=(UINT32)l;
	return E_SUCCESS;
}

SINT32 getDOMElementAttribute(const DOMNode * const elem,const char* attrName,bool& value)
{
	UINT8 val[50];
	UINT32 len=50;
	if(getDOMElementAttribute(elem,attrName,val,&len) != E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	SINT32 ret = E_UNSPECIFIED;
	if(strncasecmp((char*)val, STR_VALUE_TRUE, strlen(STR_VALUE_TRUE)) == 0)
	{
		value = true;
		ret = E_SUCCESS;
	}
	else if(strncasecmp((char*)val, STR_VALUE_FALSE, strlen(STR_VALUE_FALSE)) == 0)
	{
		value = false;
		ret = E_SUCCESS;
	}
	return ret;
}

SINT32 getLastDOMChildByName(const DOMNode* pNode,const char* const name,DOMNode* & a_child)
{
	a_child=NULL;
	if(pNode==NULL)
		return E_UNKNOWN;
	XMLCh* tmpName=XMLString::transcode((const char * const)name);
	SINT32 ret=getLastDOMChildByName(pNode,tmpName,a_child);
	XMLString::release(&tmpName);
	return ret;
}

SINT32 getLastDOMChildByName(const DOMNode* pNode,const XMLCh* const name,DOMNode* & a_child)
{
	a_child=NULL;
	if(pNode==NULL)
		{
			return E_UNKNOWN;
		}
	DOMNode* pChild;
	pChild=pNode->getLastChild();
	while(pChild!=NULL)
		{
			if(XMLString::equals(pChild->getNodeName(),name))
				{
					a_child = pChild; // found a child
					return E_SUCCESS;
				}
			pChild=pChild->getPreviousSibling();
		}
	return E_UNKNOWN;
}




SINT32 getDOMElementValue(const DOMElement* const pElem,UINT32* value)
{
	ASSERT(value!=NULL,"Value is null");
	ASSERT(pElem!=NULL,"Element is NULL");
	UINT8 buff[255];
	UINT32 buffLen=255;
	if(getDOMElementValue(pElem,buff,&buffLen)!=E_SUCCESS)
		return E_UNKNOWN;
	*value=atol((char*)buff);

	return E_SUCCESS;
}

SINT32 getDOMElementValue(const DOMElement* const pElem,double* value)
{
	ASSERT(value!=NULL,"Value is null");
	ASSERT(pElem!=NULL,"Element is NULL");
	UINT8 buff[255];
	UINT32 buffLen=255;
	if(getDOMElementValue(pElem,buff,&buffLen)!=E_SUCCESS)
		return E_UNKNOWN;
	*value=atof((char*)buff);

	return E_SUCCESS;
}


SINT32 getDOMElementValue(const DOMElement* pElem,UINT32& value, UINT32 defaultValue)
{
	UINT32 v;
	if(getDOMElementValue(pElem,&v)!=E_SUCCESS)
		{
		value=defaultValue;
		}
	else
		value=v;
	return E_SUCCESS;
}

SINT32 getDOMElementValue(const DOMElement* pElem, UINT64 &value)
{
	ASSERT(pElem!=NULL, "Element is NULL");
	UINT8 buf[256];
	UINT32 bufLen = 256;
	if(getDOMElementValue(pElem,buf,&bufLen)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	if(parseU64(buf, value)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}

	return E_SUCCESS;
}

SINT32 getDOMElementValue(const DOMElement* const pElem, SINT64 &value)
{
	ASSERT(pElem!=NULL, "Element is NULL");
	UINT8 buf[256];
	UINT32 bufLen = 256;
	if(getDOMElementValue(pElem,buf,&bufLen)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	if(parseS64(buf, value)!=E_SUCCESS)
	{
		return E_UNKNOWN;
	}
	return E_SUCCESS;
}


SINT32 getDOMElementValue(const DOMElement* const pElem,UINT16* value)
{
	UINT32 tmp;
	if(getDOMElementValue(pElem,&tmp)!=E_SUCCESS)
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

SINT32 encodeXMLEncryptedKey(UINT8* key,UINT32 keylen, DOMElement* & elemRootEncodedKey,XERCES_CPP_NAMESPACE::DOMDocument* docOwner,CAASymCipher* pRSA)
{
	elemRootEncodedKey=createDOMElement(docOwner,"EncryptedKey");
	DOMElement* elem1=createDOMElement(docOwner,"EncryptionMethod");
	setDOMElementAttribute(elem1,"Algorithm",(UINT8*)"RSA");
	elemRootEncodedKey->appendChild(elem1);
	DOMElement* elem2=createDOMElement(docOwner,"CipherData");
	elem1->appendChild(elem2);
	elem1=createDOMElement(docOwner,"CipherValue");
	elem2->appendChild(elem1);
	UINT8 tmpBuff[1024];
	UINT32 tmpLen=1024;
	__encryptKey(key,keylen,tmpBuff,&tmpLen,pRSA);
	setDOMElementValue(elem1,tmpBuff);
	return E_SUCCESS;
}





SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen, const UINT8* const xml, UINT32 xmllen,CAASymCipher* pRSA)
{
	XERCES_CPP_NAMESPACE::DOMDocument* pDoc=parseDOMDocument(xml,xmllen);
	if(pDoc == NULL)
	{
		return E_UNKNOWN;
	}
	DOMElement* root=pDoc->getDocumentElement();
	if(root == NULL)
	{
		return E_UNKNOWN;
	}
	SINT32 ret=decodeXMLEncryptedKey(key,keylen,root,pRSA);
	if (pDoc != NULL)
	{
		pDoc->release();
		pDoc = NULL;
	}
	return ret;
}

SINT32 decodeXMLEncryptedKey(UINT8* key,UINT32* keylen,const DOMNode* root,CAASymCipher* pRSA)
{
	DOMNode* elemCipherValue=NULL;
	if(getDOMChildByName(root,"CipherValue",elemCipherValue,true)!=E_SUCCESS)
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
				{
					if(i>32)
						*keylen=64;
					else if(i>16)
						*keylen=32;
					else
						*keylen=16;
				}
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
SINT32 encryptXMLElement(DOMNode* node, CAASymCipher* pRSA)
{
	XERCES_CPP_NAMESPACE::DOMDocument* doc=NULL;
	DOMNode* parent=NULL;
	if(node->getNodeType()==DOMNode::DOCUMENT_NODE)
		{
			doc=(XERCES_CPP_NAMESPACE::DOMDocument*)node;
			parent=doc;
			node=doc->getDocumentElement();
		}
	else
		{
			doc=node->getOwnerDocument();
			parent=node->getParentNode();
		}
	DOMElement* elemRoot=createDOMElement(doc,"EncryptedData");
	DOMElement* elemKeyInfo=createDOMElement(doc,"ds:KeyInfo");
	elemRoot->appendChild(elemKeyInfo);
	DOMElement* elemEncKey=createDOMElement(doc,"EncryptedKey");
	elemKeyInfo->appendChild(elemEncKey);
	DOMElement* elemCipherData=createDOMElement(doc,"CipherData");
	elemEncKey->appendChild(elemCipherData);
	DOMElement* elemCipherValue=createDOMElement(doc,"CipherValue");
	elemCipherData->appendChild(elemCipherValue);
	UINT8 key[32];
	getRandom(key,32);
	UINT8* pBuff=new UINT8[1000];
	UINT32 bufflen=255;
	pRSA->encryptOAEP(key,32,pBuff,&bufflen);
	UINT8* pOutBuff=new UINT8[1000];
	UINT32 outbufflen=255;
	CABase64::encode(pBuff,bufflen,pOutBuff,&outbufflen);
	pOutBuff[outbufflen]=0;
	setDOMElementValue(elemCipherValue,pOutBuff);
	delete[] pOutBuff;
	pOutBuff = NULL;
	delete[] pBuff;
	pBuff = NULL;
	CASymCipher *pSymCipher=new CASymCipher();
	pSymCipher->setKey(key,true);
	pSymCipher->setIVs(key+16);
	elemCipherData=createDOMElement(doc,"CipherData");
	elemRoot->appendChild(elemCipherData);
	elemCipherValue=createDOMElement(doc,"CipherValue");
	elemCipherData->appendChild(elemCipherValue);
	UINT8* b=DOM_Output::dumpToMem(node,&bufflen);
	outbufflen=bufflen+1000;
	pOutBuff=new UINT8[outbufflen];
	pSymCipher->encrypt1CBCwithPKCS7(b,bufflen,pOutBuff,&outbufflen);
	delete[] b;
	b = NULL;
	bufflen=outbufflen*3/2+1000;
	pBuff=new UINT8[bufflen];
	CABase64::encode(pOutBuff,outbufflen,pBuff,&bufflen);
	pBuff[bufflen]=0;
	setDOMElementValue(elemCipherValue,pBuff);
	delete[] pOutBuff;
	pOutBuff = NULL;
	delete[] pBuff;
	pBuff = NULL;
	if(parent->getNodeType()==DOMNode::DOCUMENT_NODE)
		{
			DOMNode* n=parent->removeChild(node);
			if (n != NULL)
			{
				n->release();
				n = NULL;
			}
			parent->appendChild(elemRoot);
		}
	else
		{
			DOMNode* n=parent->replaceChild(elemRoot,node);
			if (n != NULL)
			{
				n->release();
				n = NULL;
			}
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
	CASymCipher* pSymCipher=new CASymCipher();
	pSymCipher->setKey(key,true);
	pSymCipher->setIVs(key+16);
	UINT32 msgoutbufflen=inlen+1000;
	UINT8* msgoutbuff=new UINT8[msgoutbufflen];
	pSymCipher->encrypt1CBCwithPKCS7(inbuff,inlen,msgoutbuff,&msgoutbufflen);
	delete pSymCipher;
	pSymCipher = NULL;
	UINT32 encmsgoutbufflen=msgoutbufflen*3/2+1000;
	UINT8* encmsgoutbuff=new UINT8[encmsgoutbufflen];
	CABase64::encode(msgoutbuff,msgoutbufflen,encmsgoutbuff,&encmsgoutbufflen);
	delete[] msgoutbuff;
	msgoutbuff = NULL;
	encmsgoutbuff[encmsgoutbufflen]=0;
	msgoutbufflen=encmsgoutbufflen+1000;
	msgoutbuff=new UINT8[msgoutbufflen];
	sprintf((char*)msgoutbuff,XML_ENC_TEMPLATE,keyoutbuff,encmsgoutbuff);
	outlen=strlen((char*)msgoutbuff);
	delete[] encmsgoutbuff;
	encmsgoutbuff = NULL;
	return msgoutbuff;
}

#ifndef ONLY_LOCAL_PROXY
SINT32 decryptXMLElement(DOMNode* node, CAASymCipher* pRSA)
{
	XERCES_CPP_NAMESPACE::DOMDocument* doc=node->getOwnerDocument();
	if(! equals(node->getNodeName(),"EncryptedData"))
		return E_UNKNOWN;
	DOMNode* elemKeyInfo=NULL;
	getDOMChildByName(node,"ds:KeyInfo",elemKeyInfo,false);
	DOMNode* elemEncKey=NULL;
	getDOMChildByName(elemKeyInfo,"EncryptedKey",elemEncKey,false);
	DOMNode* elemCipherValue=NULL;
	getDOMChildByName(elemEncKey,"CipherValue",elemCipherValue,true);
	UINT8* cipherValue=new UINT8[1000];
	UINT32 len=1000;
	if(getDOMElementValue(elemCipherValue,cipherValue,&len)!=E_SUCCESS)
	{
		delete[] cipherValue;
		cipherValue = NULL;
		return E_UNKNOWN;
	}
	CABase64::decode(cipherValue,len,cipherValue,&len);
	if(	pRSA->decryptOAEP(cipherValue,cipherValue,&len)!=E_SUCCESS||
			len!=32)
	{
		delete[] cipherValue;
		cipherValue = NULL;
		return E_UNKNOWN;
	}
	CASymCipher *pSymCipher=new CASymCipher();
	pSymCipher->setKey(cipherValue,false);
	pSymCipher->setIVs(cipherValue+16);

	DOMNode* elemCipherData=NULL;
	getDOMChildByName(node,"CipherData",elemCipherData,false);
	getDOMChildByName(elemCipherData,"CipherValue",elemCipherValue,false);
	len=1000;
	if(getDOMElementValue(elemCipherValue,cipherValue,&len)!=E_SUCCESS)
		{
			delete pSymCipher;
			pSymCipher = NULL;
			delete[] cipherValue;
			cipherValue = NULL;
			return E_UNKNOWN;
		}
	if(CABase64::decode(cipherValue,len,cipherValue,&len)!=E_SUCCESS)
		{
			delete pSymCipher;
			pSymCipher = NULL;
			delete[] cipherValue;
			cipherValue = NULL;
			return E_UNKNOWN;
		}
	SINT32 ret=pSymCipher->decrypt1CBCwithPKCS7(cipherValue,cipherValue,&len);
	delete pSymCipher;
	pSymCipher = NULL;
	if(ret!=E_SUCCESS)
		{
			delete[] cipherValue;
			cipherValue = NULL;
			return E_UNKNOWN;
		}
	//now the need to parse the plaintext...
	XERCES_CPP_NAMESPACE::DOMDocument* docPlain=parseDOMDocument(cipherValue,len);
	delete[] cipherValue;
	cipherValue = NULL;
	DOMNode* elemPlainRoot=NULL;
	if(docPlain==NULL)
		return E_UNKNOWN;
	if((elemPlainRoot=docPlain->getDocumentElement())==NULL)
		{
			if (docPlain != NULL)
			{
				docPlain->release();
				docPlain = NULL;
			}
			return E_UNKNOWN;
		}
	elemPlainRoot=doc->importNode(elemPlainRoot,true);
	DOMNode* parent=node->getParentNode();
	if(parent->getNodeType()==DOMNode::DOCUMENT_NODE)
		{
			DOMNode* n=parent->removeChild(node);
			if (n != NULL)
			{
				n->release();
				n = NULL;
			}
			parent->appendChild(elemPlainRoot);
		}
	else
		{
			DOMNode* n=parent->replaceChild(elemPlainRoot,node);
			if (n != NULL)
			{
				n->release();
				n = NULL;
			}
		}
	if (docPlain != NULL)
	{
		docPlain->release();
		docPlain = NULL;
	}
	return E_SUCCESS;
}
#endif //ONLY_LOCAL_PROXY

UINT8* readFile(UINT8* name,UINT32* size)
{
	int handle=open((char*)name,O_BINARY|O_RDONLY);
	if(handle<0)
		return NULL;
	*size=filesize32(handle);
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
	#ifdef	HAVE_STRTOULL
		value = strtoull((const char *) str, NULL, 0);
		return E_SUCCESS;
	#else
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
	#endif
}

/**
 * Parses a 64bit signed integer.
 * Note: If the value is out of range or not parseable an erro is returned.
 */
SINT32 parseS64(const UINT8 * str, SINT64& value)
{
	#ifdef	HAVE_ATOLL
		value = atoll((const char *) str);
		return E_SUCCESS;
	#else
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
				SINT64 s64 = 0;
				for (UINT32 i = 0; i < len; i++)
				{
					UINT8 c=str[i];
					if (c >= '0' && c <= '9')
					{
						s64 *= 10;
						s64 += c - '0';
					}
					else if (i != 0 || str[i] != '+'||str[i]!='-')
					{
						return E_UNKNOWN;
					}
				}
				if(str[0]=='-')
					value=-s64;
				else
					value = s64;
				return E_SUCCESS;
		#else
			#warning parseS64() is not implemented for platforms without native INT64 support!!!
			///@todo code if we do not have native UINT64
			return E_UNKNOWN;
		#endif
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
			buff[i]=(UINT8)c;
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
