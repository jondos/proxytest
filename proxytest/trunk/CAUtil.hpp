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
#include "xml/xmloutput.h"

UINT32 strtrim(UINT8*);

SINT32 memtrim(UINT8* out,const UINT8* in,UINT32 len);

SINT32 getcurrentTimeMillis(BIGNUM *bnTime); 

SINT32 getRandom(UINT8* buff,UINT32 len);

SINT32 getRandom(UINT32* val);

SINT32 msSleep(UINT16 ms);

SINT32 sSleep(UINT16 sec);

UINT32 getMemoryUsage();

#ifndef _WIN32
SINT32 filelength(int handle);
#endif

/**
	This class implements the Output Stream interface required by XML::Output. 
	It stores all the data in a memory buffer.
*/
class BufferOutputStream:public XMLOutputStream
	{
		public:
			/** Constructs a new buffered Output Stream.
			@param initsize	the inital size of the buffer that holds the streamed data
							If initsize=0 then the default value is 1 kilo byte (so initsize=1024)
			@param grow the number of bytes by which the buffer is increased, if needed. 
									If grow=0 then the initial size is used (so grow=initsize)
			**/
			BufferOutputStream(UINT32 initsize=0,UINT32 grow=0)
				{
					if(initsize==0)
						initsize=1024;
					m_buffer=(UINT8*)malloc(initsize);
					if(m_buffer==NULL)
						m_size=0;
					else
						m_size=initsize;
					m_used=0;
					if(grow>0)
						m_grow=grow;
					else
						m_grow=initsize;
				}
			
			/** Releases the memory for the buffer.**/
			virtual ~BufferOutputStream()
				{
					if(m_buffer!=NULL)
						free(m_buffer);
				}

	/** write up to bufLen characters to an output source (memory buffer).
		@param buf		source buffer
		@param bufLen	number of characters to write
		@return 		the number of characters actually written - if this
						number is less than bufLen, there was an error (which is acctually E_UNKNOWN)
	*/			
			int write(const char *buf, size_t bufLen)
				{
					if(m_size-m_used<bufLen)
						{
							UINT8* tmp=(UINT8*)realloc(m_buffer,m_size+m_grow);
							if(tmp==NULL)
								{
									return E_UNKNOWN;	
								}
							m_size+=m_grow;
							m_buffer=tmp;
						}
					memcpy(m_buffer+m_used,buf,bufLen);
					m_used+=bufLen;
					return bufLen;
				}

			/** Gets the buffer.
			@return a pointer to the buffer which holds the already streamed data. May be NULL.
			*/
			UINT8* getBuff()
				{
					return m_buffer;
				}
			
			/** Gets the number of bytes which are stored in the output buffer.
			@return number of bytes writen to the stream.
			*/
			UINT32 getBufferSize()
				{
					return m_used;	
				}

			/** Resets the stream. All data is trashed and the next output will be writen to the beginn of the buffer.
			@return always E_SUCCESS
			*/
			SINT32 reset()
				{
					m_used=0;
					return E_SUCCESS;
				}

		private:
			UINT8* m_buffer; ///The buffer which stores the data
			UINT32 m_size; ///The size of the buffer in which the streaming data is stored 
			UINT32 m_used; ///The number of bytes already used of the buffer
			UINT32 m_grow; ///The number of bytes by which the buffer should grow if neccesary
	};


class BufferInputStream:public XMLInputStream
	{
		public:
			BufferInputStream(UINT8* buff,UINT32 l)
				{
					buffer=buff;
					len=l;
					pos=0;
				}

		int read(XML_Char *buf, size_t bufLen)
			{
				UINT32 size=(UINT32)min(bufLen,len-pos);
				if(size==0)
					return 0;
				memcpy(buf,buffer+pos,size);
				pos+=size;
				return size;
			}

		UINT32 getPos()
			{
				return pos;
			}

		UINT32 getSize()
			{
				return len;
			}

		UINT8* getBuffer()
			{
				return buffer;
			}

		private:
			UINT8* buffer;
			UINT32 pos;
			UINT32 len;
	};

#endif
