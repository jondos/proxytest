/*************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 1998 Bjorn Reese and Daniel Stenberg.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS AND
 * CONTRIBUTORS ACCEPT NO RESPONSIBILITY IN ANY CONCEIVABLE MANNER.
 *
 *************************************************************************
 *
 * http://ctrio.sourceforge.net/
 *
 ************************************************************************/

#ifndef TRIO_TRIO_H
#define TRIO_TRIO_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

/*
 * Use autoconf defines if present. Packages using trio must define
 * HAVE_CONFIG_H as a compiler option themselves.
 */
#if defined(HAVE_CONFIG_H)
# include <config.h>
#endif

#if !defined(WITHOUT_TRIO)

//#ifdef __cplusplus
//extern "C" {
//#endif

/* make utility and C++ compiler in Windows NT fails to find this symbol */ 
#if defined(WIN32) && !defined(isascii)
//# define isascii ((unsigned)(x) < 0x80)
#endif

/* Error macros */
#define TRIO_ERROR_CODE(x) ((-(x)) & 0x00FF)
#define TRIO_ERROR_POSITION(x) ((-(x)) >> 8)
#define TRIO_ERROR_NAME(x) trio_strerror(x)

/*
 * Error codes.
 *
 * Remember to add a textual description to trio_strerror.
 */
enum {
  TRIO_EOF      = 1,
  TRIO_EINVAL   = 2,
  TRIO_ETOOMANY = 3,
  TRIO_EDBLREF  = 4,
  TRIO_EGAP     = 5,
  TRIO_ENOMEM   = 6,
  TRIO_ERANGE   = 7,
  TRIO_ERRNO    = 8
};

const char *trio_strerror(int);

/*************************************************************************
 * Print Functions
 */

int trio_printf(const char *format, ...);
int trio_vprintf(const char *format, va_list args);
int trio_printfv(const char *format, void **args);

int trio_fprintf(FILE *file, const char *format, ...);
int trio_vfprintf(FILE *file, const char *format, va_list args);
int trio_fprintfv(FILE *file, const char *format, void **args);

int trio_dprintf(int fd, const char *format, ...);
int trio_vdprintf(int fd, const char *format, va_list args);
int trio_dprintfv(int fd, const char *format, void **args);

/* trio_sprintf(target, format, ...)
 * trio_snprintf(target, maxsize, format, ...)
 *
 *   Build 'target' according to 'format' and succesive
 *   arguments. This is equal to the sprintf() and
 *   snprintf() functions.
 */
int trio_sprintf(char *buffer, const char *format, ...);
int trio_vsprintf(char *buffer, const char *format, va_list args);
int trio_sprintfv(char *buffer, const char *format, void **args);

int trio_snprintf(char *buffer, size_t max, const char *format, ...);
int trio_vsnprintf(char *buffer, size_t bufferSize, const char *format,
		   va_list args);
int trio_snprintfv(char *buffer, size_t bufferSize, const char *format,
		   void **args);

int trio_snprintfcat(char *buffer, size_t max, const char *format, ...);
int trio_vsnprintfcat(char *buffer, size_t bufferSize, const char *format,
                      va_list args);

#endif /* WITHOUT_TRIO */

#endif /* TRIO_TRIO_H */
