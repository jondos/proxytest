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

#ifndef __BASE_TYPE_DEFS
#define __BASE_TYPE_DEFS

#ifdef _WIN32
	#define HAVE_NATIVE_UINT64
		//UINT64 already defined by Windows!
		typedef _int64 SINT64;
		typedef signed long SINT32;
		typedef unsigned short UINT16;
		typedef signed short SINT16;
		typedef unsigned char UINT8;
		typedef signed char SINT8;
#else
  #ifdef HAVE_CONFIG_H
			#if SIZEOF_UNSIGNED_CHAR == 1 && !defined HAVE_UINT8
				#define HAVE_UINT8
				typedef unsigned char UINT8;
			#endif
			#if SIZEOF_SIGNED_CHAR == 1 && !defined HAVE_SINT8
				#define HAVE_SINT8
				typedef signed char SINT8;
			#endif
			#if SIZEOF_UNSIGNED_SHORT == 2 && !defined HAVE_UINT16
				#define HAVE_UINT16
				typedef unsigned short UINT16;
			#endif
			#if SIZEOF_SIGNED_SHORT == 2 && !defined HAVE_SINT16
				#define HAVE_SINT16
				typedef unsigned short SINT16;
			#endif
				#if SIZEOF_UNSIGNED_INT == 4 && !defined HAVE_UINT32
				#define HAVE_UINT32
				typedef unsigned int UINT32;
			#endif
			#if SIZEOF_SIGNED_INT == 4 && !defined HAVE_SINT32
				#define HAVE_SINT32
				typedef signed int SINT32;
			#endif
			#if SIZEOF___UINT64_T == 8 && !defined HAVE_UINT64
				#define HAVE_UINT64
				typedef unsigned long long UINT64;
				typedef signed long long SINT64;
			#endif
			#if SIZEOF_UNSIGNED_LONG_LONG == 8 && !defined HAVE_UINT64
				#define HAVE_UINT64
				typedef unsigned long long UINT64;
				typedef signed long long SINT64;
			#endif
			#ifdef HAVE_UINT64
							#define HAVE_NATIVE_UINT64
			#endif
	#else
		#if defined(__linux)
			#include <linux/types.h>
			#define HAVE_NATIVE_UINT64
			typedef unsigned long long UINT64;
			typedef signed long long SINT64;
			typedef __u32 UINT32;
			typedef __s32 SINT32;
			typedef __u16 UINT16;
			typedef __s16 SINT16;
			typedef __u8 UINT8;
			typedef __s8 SINT8;
		#elif defined(__sgi)
			#define HAVE_NATIVE_UINT64
			typedef __uint64_t UINT64;	
			typedef __int64_t SINT64;	
			typedef __uint32_t UINT32;
			typedef __int32_t SINT32;
			typedef unsigned short UINT16;
			typedef signed short SINT16;
			typedef unsigned char UINT8;
			typedef signed char SINT8;
		#elif defined(__sun)    	
			typedef uint32_t UINT32;
			typedef int32_t SINT32;
			typedef uint16_t UINT16;
			typedef int16_t SINT16;
			typedef uint8_t UINT8;
			typedef int8_t SINT8;
		#elif defined __APPLE_CC__ 	
			#define HAVE_NATIVE_UINT64
			typedef unsigned long long UINT64;
			typedef signed long long SINT64;
			typedef unsigned int UINT32;
			typedef signed int SINT32;
			typedef unsigned short UINT16;
			typedef signed short SINT16;
			typedef unsigned char UINT8;
			typedef signed char SINT8;
		#else     	
			#warning This seams to be a currently not supported plattform - may be things go wrong! 
			#warning Please report the plattform, so that it could be added 
			#ifdef __GNUC__ //TODO check if for all GNUC long long is 64 bit!!
				#define HAVE_NATIVE_UINT64
				typedef unsigned long long UINT64;
				typedef signed long long SINT64;
			#endif
			typedef unsigned int UINT32;
			typedef signed int SINT32;
			typedef unsigned short UINT16;
			typedef signed short SINT16;
			typedef unsigned char UINT8;
			typedef signed char SINT8;
		#endif
	#endif //HAVE_CONFIG_H?
#endif //WIN32?

#if !defined(HAVE_NATIVE_UINT64)
	typedef struct __UINT64__t_
		{
			UINT32 high;
			UINT32 low;
		} UINT64;	 

	typedef struct __SINT64__t_
		{
			SINT32 high;
			UINT32 low;
		} SINT64;	 
#endif

typedef unsigned int UINT;
typedef signed int SINT;
	
#endif//__BASE_TYPES_DEFS_H
