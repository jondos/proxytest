#include "StdAfx.h"
int strtrim(char* s)
	{
		int inpos=0;
		int outpos=0;
		for(inpos=0;inpos<strlen(s);inpos++)
			{
				if(s[inpos]>' ')
					s[outpos++]=s[inpos];
			}
		s[outpos]=0;
		return outpos;
	}

int memtrim(const char* in,char* out,int len)
	{
		int inpos=0;
		int outpos=0;
		for(inpos=0;inpos<len;inpos++)
			{
				if(in[inpos]>' ')
					out[outpos++]=in[inpos];
			}
		return outpos;
	}