/*
  regex.h - POSIX.2 compatible regexp interface and TRE extensions

  Copyright (C) 2001-2003 Ville Laurikari <vl@iki.fi>.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License version 2 (June
  1991) as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

//#ifdef LOG_CRIME
#ifndef TRE_REGEX_H
#define TRE_REGEX_H 1

#include "tre-config.h"

//#ifdef HAVE_SYS_TYPES_H
//#include <sys/types.h>
//#endif /* HAVE_SYS_TYPES_H */

#ifdef TRE_USE_SYSTEM_REGEX_H
/* Include the system regex.h to make TRE ABI compatible with the
   system regex. */
#include TRE_SYSTEM_REGEX_H_PATH
#endif /* TRE_USE_SYSTEM_REGEX_H */

//#ifdef __cplusplus
//extern "C" {
//#endif

#ifdef TRE_USE_SYSTEM_REGEX_H

#ifndef REG_OK
#define REG_OK 0
#endif /* !REG_OK */

#ifndef HAVE_REG_ERRCODE_T
typedef int reg_errcode_t;
#endif /* !HAVE_REG_ERRCODE_T */

#else /* !TRE_USE_SYSTEM_REGEX_H */

/* If the we're not using system regex.h, we need to define the
   structs and enums ourselves. */

typedef int regoff_t;
typedef struct {
  size_t re_nsub;  /* Number of parenthesized subexpressions. */
  void *value;     /* For internal use only. */
} regex_t;

typedef struct {
  regoff_t rm_so;
  regoff_t rm_eo;
} regmatch_t;


typedef enum {
  REG_OK = 0,           /* No error. */
  /* POSIX regcomp() return error codes.  (In the order listed in the
     standard.)  */
  REG_NOMATCH,          /* No match. */
  REG_BADPAT,           /* Invalid regexp. */
  REG_ECOLLATE,         /* Unknown collating element. */
  REG_ECTYPE,           /* Unknown character class name. */
  REG_EESCAPE,          /* Trailing backslash. */
  REG_ESUBREG,          /* Invalid back reference. */
  REG_EBRACK,           /* "[]" imbalance */
  REG_EPAREN,           /* "\(\)" or "()" imbalance */
  REG_EBRACE,           /* "\{\}" or "{}" imbalance */
  REG_BADBR,            /* Invalid content of {} */
  REG_ERANGE,           /* Invalid use of range operator */
  REG_ESPACE,           /* Out of memory.  */
  REG_BADRPT
} reg_errcode_t;

/* POSIX regcomp() flags. */
#define REG_EXTENDED    1
#define REG_ICASE       (REG_EXTENDED << 1)
#define REG_NEWLINE     (REG_ICASE << 1)
#define REG_NOSUB       (REG_NEWLINE << 1)

/* POSIX regexec() flags. */
#define REG_NOTBOL 1
#define REG_NOTEOL (REG_NOTBOL << 1)

#endif /* !TRE_USE_SYSTEM_REGEX_H */

/* The maximum number of iterations in a bound expression. */
#undef RE_DUP_MAX
#define RE_DUP_MAX 255

/* The POSIX.2 regexp functions */
int regcomp(regex_t *preg, const char *regex, int cflags);
int regexec(const regex_t *preg, const char *string, size_t nmatch,
	    regmatch_t pmatch[], int eflags);
size_t regerror(int errcode, const regex_t *preg, char *errbuf,
		size_t errbuf_size);
void regfree(regex_t *preg);

#ifdef TRE_WCHAR
#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif /* HAVE_WCHAR_H */

/* Wide character versions (not in POSIX.2). */
int regwcomp(regex_t *preg, const wchar_t *regex, int cflags);
int regwexec(const regex_t *preg, const wchar_t *string, size_t nmatch,
	     regmatch_t pmatch[], int eflags);
#endif /* TRE_WCHAR */

/* Versions with a maximum length argument and therefore the capability to
   handle null characters in the middle of the strings (not in POSIX.2). */
int regncomp(regex_t *preg, const char *regex, size_t len, int cflags);
int regnexec(const regex_t *preg, const char *string, size_t len,
	     size_t nmatch, regmatch_t pmatch[], int eflags);
#ifdef TRE_WCHAR
int regwncomp(regex_t *preg, const wchar_t *regex, size_t len, int cflags);
int regwnexec(const regex_t *preg, const wchar_t *string, size_t len,
	      size_t nmatch, regmatch_t pmatch[], int eflags);
#endif /* TRE_WCHAR */

#ifdef TRE_APPROX

/* Approximate matching parameter struct. */
typedef struct {
  int cost_ins;        /* Default cost of an inserted character. */
  int cost_del;        /* Default cost of a deleted character. */
  int cost_subst;      /* Default cost of a substituted character. */
  int max_cost;        /* Maximum allowed cost of a match. */
} regaparams_t;

/* Approximate matching result struct. */
typedef struct {
  size_t nmatch;       /* Length of pmatch[] array. */
  regmatch_t *pmatch;  /* Submatch data. */
  int cost;            /* Cost of the match. */
} regamatch_t;


/* Approximate matching functions. */
int regaexec(const regex_t *preg, const char *string,
	     regamatch_t *match, regaparams_t params, int eflags);
int reganexec(const regex_t *preg, const char *string, size_t len,
	      regamatch_t *match, regaparams_t params, int eflags);
#ifdef TRE_WCHAR
/* Wide character approximate matching. */
int regawexec(const regex_t *preg, const wchar_t *string,
	      regamatch_t *match, regaparams_t params, int eflags);
int regawnexec(const regex_t *preg, const wchar_t *string, size_t len,
	       regamatch_t *match, regaparams_t params, int eflags);
#endif /* TRE_WCHAR */
#endif /* TRE_APPROX */

//#ifdef __cplusplus
//}
//#endif
int testTre();
#endif				/* TRE_REGEX_H */

//#endif //LOG_CRIME
/* EOF */
