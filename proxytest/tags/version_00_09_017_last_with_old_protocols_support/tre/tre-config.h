/* tre-config.h.in.  This file has all definitions that are needed in
   `regex.h'.  Note that this file must contain only the bare minimum
   of definitions without the TRE_ prefix to avoid conflicts between
   definitions here and definitions included from somewhere else. */

/* Define to 1 if you have the <libutf8.h> header file. */
#undef HAVE_LIBUTF8_H

/* Define to 1 if the system has the type `reg_errcode_t'. */
#undef HAVE_REG_ERRCODE_T

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <wchar.h> header file. */
#undef HAVE_WCHAR_H

/* Define if you want to enable approximate matching functionality. */
#undef TRE_APPROX

/* Define to enable multibyte character set support. */
#undef TRE_MULTIBYTE

/* Define to the absolute path to the system regex.h */
#undef TRE_SYSTEM_REGEX_H_PATH

/* Define to include the system regex.h from TRE regex.h */
#undef TRE_USE_SYSTEM_REGEX_H

/* Define to enable wide character (wchar_t) support. */
#undef TRE_WCHAR
/* TRE version string. */
#define TRE_VERSION "0.7.6"

/* TRE version level 1. */
#define TRE_VERSION_1 0

/* TRE version level 2. */
#define TRE_VERSION_2 7

/* TRE version level 3. */
#define TRE_VERSION_3 6

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <malloc.h> and it should be used. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#undef HAVE_DLFCN_H

/* Define if the GNU gettext() function is already present or preinstalled. */
#undef HAVE_GETTEXT

/* Define to 1 if you have the `isascii' function. */
#define HAVE_ISASCII 1

/* Define to 1 if you have the `iswctype' function. */
#define HAVE_ISWCTYPE 1

/* Define to 1 if you have the `iswlower' function. */
#define HAVE_ISWLOWER 1

/* Define to 1 if you have the `iswupper' function. */
#define HAVE_ISWUPPER 1

/* Define to 1 if you have the `mbrtowc' function. */
#define HAVE_MBRTOWC 1

/* Define to 1 if the system has the type `mbstate_t'. */
#define HAVE_MBSTATE_T 1

/* Define to 1 if you have the `mbtowc' function. */
#define HAVE_MBTOWC 1

/* Define to 1 if you have the <memory.h> header file. */
#undef HAVE_MEMORY_H

/* Define to 1 if you have the <regex.h> header file. */
#undef HAVE_REGEX_H
                                                                                                /* Define to 1 if the system has the type `reg_errcode_t'. */
#undef HAVE_REG_ERRCODE_T

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the `towlower' function. */
#undef HAVE_TOWLOWER

/* Define to 1 if you have the `towupper' function. */
#define HAVE_TOWUPPER 1

/* Define if you want to disable debug assertions. */
#ifndef NDEBUG
#define NDEBUG 1
#endif
/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to a field in the regex_t struct where TRE should store a pointer to
   the internal tre_tnfa_t structure */
#define TRE_REGEX_T_FIELD value

/* Define as `__inline' if that's what the C compiler calls it, or to nothing
   if it is not supported. */
#define inline __inline

