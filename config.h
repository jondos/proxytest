/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define if you run the code on a big endian machine */
/* #undef BYTE_ORDER_BIG_ENDIAN */

/* Define if you run the code on a little endian machine */
#define BYTE_ORDER_LITTLE_ENDIAN 

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Define to 1 if you have the `atoll' function. */
#define HAVE_ATOLL 1

/* Define if your OS support the epoll() syscall */
#define HAVE_EPOLL 1

/* This is a flag to ioctl() to get the number of available bytes on a socket
   */
#define HAVE_FIONREAD 

/* Define to 1 if you have the `fstat64' function. */
#define HAVE_FSTAT64 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `epoll' library (-lepoll). */
/* #undef HAVE_LIBEPOLL */

/* Define to 1 if you have the <libpq-fe.h> header file. */
/* #undef HAVE_LIBPQ_FE_H */

/* Define to 1 if you have the `mallinfo' function. */
#define HAVE_MALLINFO 1

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <mysql.h> header file. */
/* #undef HAVE_MYSQL_H */

/* Define to 1 if you have the <mysql/mysql.h> header file. */
/* #undef HAVE_MYSQL_MYSQL_H */

/* Define if you mysql client library supports the MYSQL_RECONNECT option */
/* #undef HAVE_MYSQL_OPT_RECONNECT */

/* Define if you have OpenSSL (which is needed!) */
/* #undef HAVE_OPENSSL */

/* This is a flag to open() to do synchronized I/O */
#define HAVE_O_FSYNC 

/* This is a flag to open() to do synchronized I/O */
#define HAVE_O_SYNC 

/* Define to 1 if you have the <pgsql/libpq-fe.h> header file. */
/* #undef HAVE_PGSQL_LIBPQ_FE_H */

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define to 1 if you have the <postgresql/libpq-fe.h> header file. */
#define HAVE_POSTGRESQL_LIBPQ_FE_H 1

/* Define to 1 if you have the `pthread_cond_init' function. */
#define HAVE_PTHREAD_COND_INIT 1

/* Define to 1 if you have the `pthread_mutex_init' function. */
#define HAVE_PTHREAD_MUTEX_INIT 1

/* Define to 1 if you have the `sbrk' function. */
#define HAVE_SBRK 1

/* Define to 1 if you have the `sem_init' function. */
#define HAVE_SEM_INIT 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define if the type socklen_t is available */
#define HAVE_SOCKLEN_T 

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strnstr' function. */
/* #undef HAVE_STRNSTR */

/* Define to 1 if you have the `strtoull' function. */
#define HAVE_STRTOULL 1

/* Define to 1 if you have the <sys/epoll.h> header file. */
#define HAVE_SYS_EPOLL_H 1

/* Define to 1 if you have the <sys/filio.h> header file. */
/* #undef HAVE_SYS_FILIO_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define if your OS supports the TCP socket option TCP_KEEPALIVE */
/* #undef HAVE_TCP_KEEPALIVE */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if your OS supports the Unix Domain protocol */
#define HAVE_UNIX_DOMAIN_PROTOCOL 

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* This is the value, which is returned from inet_addr(), in case of an error
   */
/* #undef INADDR_NONE */

/* This is the maximum length of a file system path */
#define MAX_PATH 4096

/* This is a flag to read(), to tell the os does not wait for any bytes to
   arrive */
/* #undef MSG_DONTWAIT */

/* This is a flag to read(), to tell the os to get no signals */
/* #undef MSG_NOSIGNAL */

/* This flag is used in open(), in order to open a file in binary mode */
#define O_BINARY 0

/* Name of package */
#define PACKAGE "mix"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "sk13@inf.tu-dresden.de"

/* Define to the full name of this package. */
#define PACKAGE_NAME "AN.ON Mix"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "AN.ON Mix 00.07.001"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "mix"

/* Define to the version of this package. */
#define PACKAGE_VERSION "00.07.001"

/* The size of `signed char', as computed by sizeof. */
#define SIZEOF_SIGNED_CHAR 1

/* The size of `signed int', as computed by sizeof. */
#define SIZEOF_SIGNED_INT 4

/* The size of `signed long', as computed by sizeof. */
#define SIZEOF_SIGNED_LONG 4

/* The size of `signed short', as computed by sizeof. */
#define SIZEOF_SIGNED_SHORT 2

/* The size of `unsigned char', as computed by sizeof. */
#define SIZEOF_UNSIGNED_CHAR 1

/* The size of `unsigned int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_INT 4

/* The size of `unsigned long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG 4

/* The size of `unsigned long long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG_LONG 8

/* The size of `unsigned short', as computed by sizeof. */
#define SIZEOF_UNSIGNED_SHORT 2

/* The size of `__uint64_t', as computed by sizeof. */
#define SIZEOF___UINT64_T 8

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "00.07.001"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif
