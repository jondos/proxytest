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
		struct _timeb timebuffer;
		_ftime(&timebuffer);
		/* Hack what should be solved better...*/
		BN_set_word(bnTime,timebuffer.time);
		BN_mul_word(bnTime,1000);
		BN_add_word(bnTime,timebuffer.millitm);
		/* end of hack..*/
		return E_SUCCESS;
	}