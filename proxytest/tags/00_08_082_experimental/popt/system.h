#ifdef __NeXT
/* access macros are not declared in non posix mode in unistd.h -
 don't try to use posix on NeXTstep 3.3 ! */
#include <libc.h>
#endif


#ifdef __ICC
	#define HAVE_ALLOCA_H
	//#define alloca(size) _alloca(size)
#else
/* AIX requires this to be the first thing in the file.  */ 
	#ifndef __GNUC__
		#ifdef HAVE_ALLOCA_H
			#include <alloca.h>
		#else
			#ifdef _AIX
				#pragma alloca
			#else
				#ifndef alloca /* predefined by HP cc +Olibcalls */
					char *alloca ();
				#endif
			#endif
		#endif
	#elif defined(__GNUC__) && defined(__STRICT_ANSI__)
		#define alloca __builtin_alloca
	#else
		#ifdef HAVE_ALLOCA_H
			#include <alloca.h>
		#endif
	#endif
	#ifdef _WIN32
		#define alloca _alloca
	#endif
#endif
/*@only@*/ char * xstrdup (const char *str);

#if HAVE_MCHECK_H && defined(__GNUC__)
#define	vmefail()	(fprintf(stderr, "virtual memory exhausted.\n"), exit(EXIT_FAILURE), NULL)
#define xstrdup(_str)   (strcpy((malloc(strlen(_str)+1) ? : vmefail()), (_str)))
#else
#define	xstrdup(_str)	strdup(_str)
#endif  /* HAVE_MCHECK_H && defined(__GNUC__) */
