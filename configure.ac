dnl
dnl Autoconfiguration files for proxytest (a MiX proxy)
dnl Konrad Rieck <kr@r0q.cx>
dnl

AC_INIT([AN.ON Mix],[00.11.001],[sk13@inf.tu-dresden.de],[mix])
AM_INIT_AUTOMAKE([subdir-objects])
AM_MAINTAINER_MODE
AC_PREREQ(2.59)
AC_CONFIG_SRCDIR([proxytest.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR([m4])

# Require PKG_CHECK_MODULES macro
m4_ifndef([PKG_CHECK_MODULES],
		  [m4_fatal([must install pkg-config before running autoconf])])

dnl Do we cross compile?
AC_RUN_IFELSE([], [], [], [CROSSCOMPILE="true"; AC_MSG_NOTICE([We do cross compile])])

dnl Checks for programs.
AM_PROG_AR

LT_INIT([disable-static])
AC_PROG_CC

dnl avoid the default flags "-g -o2"
SAVEDCXXFLAGS=$CXXFLAGS
CXXFLAGS=" "
AC_PROG_CXX 
CXXFLAGS=$SAVEDCXXFLAGS

AC_PROG_INSTALL

AC_LANG_PUSH(C++)
AC_REQUIRE_CPP
AC_COMPILE_IFELSE([AC_LANG_SOURCE([int main(){return 0;}])],[],[AC_MSG_ERROR([Could not run C++ compiler])])
AC_LANG_POP(C++)

OFLAGS=""

if test "x$GCC" = "xyes" ; then
   CFLAGS="$CFLAGS -Wall -D_REENTRANT"
   CXXFLAGS="$CXXFLAGS -Wall -D_REENTRANT"
else
   echo "You need the GNU C Compiler to compile proxytest."
   exit 
fi

DEFAULTLIBS="$LIBS" #-L/usr/lib -L/usr/local/lib"

case "$host" in
*-*-solaris*)
		DEFAULTLIBS="$LIBS -lnsl -lsocket -lposix4"
		;;
*-*-android*)
	DEFAULTLIBS="$LIBS -pthread"
	CXXFLAGS="$CXXFLAGS -fno-rtti -fno-exceptions"
	;;
*-*-linux*)
	DEFAULTLIBS="$LIBS -lpthread"
	;;
*-*-irix*)
	CFLAGS="$CFLAGS "
	;;
*-*-freebsd*)
	DEFAULTLIBS="$LIBS -L/usr/local/lib -liconv -lpthread";
	CFLAGS="$CFLAGS";
	CXXFLAGS="$CXXFLAGS";
	OFLAGS="-O3"
	;;
*-*-kfreebsd*)
	DEFAULTLIBS="$LIBS -lpthread";
	;;
*-*-netbsd*)
	DEFAULTLIBS="$LIBS -L/usr/pkg/lib -liconv -lpthread"
	;;
*-*-drops*)
	CFLAGS="$CFLAGS -DOS_TUDOS"
	CXXFLAGS="$CXXFLAGS -DOS_TUDOS";
	;;
*)
	;;
esac

case "$host" in
alpha*)
	CXXFLAGS="$CXXFLAGS -mieee"
	;;
m68k*)
		OFLAGS="-O0"
		;;	
*)
	if test "x$OFLAGS" == "x" ; then
		OFLAGS="-O3"
	fi
	;;
esac

dnl Architecture dependend compiler optimisations
MARCH=""
AC_ARG_WITH(arch,
	AS_HELP_STRING([--with-arch=i386|i486|i586|i686|pentium3|opteron],[Specify architecure for optimisation]),
	[
		if test "x$withval" != "xno" ; then
			MARCH="-march=${withval}"
		fi
	]
)

dnl bits for environment
AC_ARG_WITH(bits,
	AS_HELP_STRING([--with-bits=32|64],[Generate code for a 32-bit or 64-bit environment]),
	[
		if test "x$withval" != "xno" ; then
			MARCH="$MARCH -m${withval}"
		fi
	]
)

dnl auto selection --> skip if selecte manually
if test -z "$MARCH"; then
	case "$host_cpu" in
	i686)
		MARCH="-march=i686"
	;;
	esac
fi
OFLAGS="$OFLAGS $MARCH"

dnl Static?
AC_ARG_ENABLE(static,
		AS_HELP_STRING([--enable-static],[Static linking]),
		[
		if test "x$enableval" != "xno" ; then
			CFLAGS="$CFLAGS -static"
				CXXFLAGS="$CXXFLAGS -static"
		fi    
		])

dnl Without epoll?
USE_EPOLL=1
AC_ARG_ENABLE(epoll,
		AS_HELP_STRING([--disable-epoll],[do not use epoll]),
		[
		if test "x$enableval" == "xno" ; then
			USE_EPOLL=0
		fi    
		])
dnl pool mix ??
POOL=""
AC_ARG_ENABLE(pool,
	AS_HELP_STRING([--enable-pool=POOLSIZE],[Specify if to build a pool mix and optionally the poolsize]),
	[
		if test "x$enableval" != "xno" ; then
			POOL="-DUSE_POOL"
			if test "x$enableval" != "xyes" ; then	
			POOL="$POOL -DMIX_POOL_SIZE=$enableval"
			fi	
		fi
	]
)

dnl delay channels ??
BANDWIDTH_LIMIT="-DDELAY_CHANNELS -DDELAY_USERS"
AC_ARG_ENABLE(	[bandwidth-limitation],
				AS_HELP_STRING([--disable-bandwidth-limitation],[Specify if to build without bandwidth limitation. The limitation parameters can be specified in the mix configuration file.]),
	[
		if test "x$enableval" = "xno" ; then
			BANDWIDTH_LIMIT=""
dnl		    if test "x$enableval" != "xyes" ; then	
dnl			BANDWIDTH_LIMIT="$BANDWIDTH_LIMIT -DDELAY_CHANNEL_TRAFFIC=$enableval"
dnl		    fi	
		fi
	]
)

dnl BANDWIDTH_LIMIT_THROUGHPUT=""
dnl AC_ARG_ENABLE(bandwidth-limitation-throughput,
dnl 	AS_HELP_STRING([--enable-bandwidth-limitation-throughput=LIMITATION_THROUGHPUT],
dnl 			[Specify if to build with bandwidth limitation and optionally the limit in kByte/s of each communication channel]),
dnl 	[
dnl 		if test "x$enableval" != "xno" ; then
dnl 		    BANDWIDTH_LIMIT_THROUGHPUT="-DDELAY_CHANNELS"
dnl 		    if test "x$enableval" != "xyes" ; then	
dnl 			BANDWIDTH_LIMIT_THROUGHPUT="$BANDWIDTH_LIMIT_THROUGHPUT -DDELAY_CHANNEL_KBYTE_PER_SECOND=$enableval"
dnl 		    fi	
dnl 		fi
dnl 	]
dnl )

dnl payment ??
PAYMENT=""
AC_ARG_ENABLE(payment,
	AS_HELP_STRING([--enable-payment],
			[Specify if to build with payment support]),
	[
		if test "x$enableval" != "xno" ; then
			PAYMENT="-DPAYMENT"
		fi
	]
)

dnl infoserviceserial ??
INFOSERVICESERIAL=""
AC_ARG_ENABLE(infoserviceserial,
	AS_HELP_STRING([--infoservice-serial],
			[Use serial communication with infoservices]),
	[
		if test "x$enableval" != "xno" ; then
			INFOSERVICESERIAL="-DINFOSERVICESERIAL"
		fi
	]
)

dnl server monitoring ??
SERVER_MONITORING=""
AC_ARG_ENABLE(server_monitoring,
	AS_HELP_STRING([--enable-server_monitoring],
			[Specify if to build with state tracking for server monitoring]),
	[
		if test "x$enableval" != "xno" ; then
			SERVER_MONITORING="-DSERVER_MONITORING"
		fi
	]
)

dnl replay detection ??
REPLAY_DETECTION=""
AC_ARG_ENABLE(replay_detection,
	AS_HELP_STRING([--enable-replay_detection],
			[Specify if to build with replay detection support]),
	[
		if test "x$enableval" != "xno" ; then
			REPLAY_DETECTION="-DREPLAY_DETECTION"
		fi
	]
)

dnl type-b mixes ??
NEW_MIX_TYPE=""
AC_ARG_ENABLE(new_mix_type,
	AS_HELP_STRING([--enable-new_mix_type],
			[Specify if to build with type B mixes]),
	[
		if test "x$enableval" != "xno" ; then
			NEW_MIX_TYPE="-DNEW_MIX_TYPE"
		fi
	]
)

dnl dynamic mix ??
DYNAMIC=""
AC_ARG_ENABLE(dynamic,
	AS_HELP_STRING([--enable-dynamic],[Specify if to build a  mix for a dynamic cascade]),
	[
		if test "x$enableval" != "xno" ; then
			DYNAMIC="-DDYNAMIC_MIX"
		fi
	]
)

DIALOG=""
AC_ARG_ENABLE(dialog,
	AS_HELP_STRING([--enable-dialog],[]),
	[
		if test "x$enableval" != "xno" ; then
			DIALOG="-DLOG_DIALOG"
		fi
	]
)


LIBS="$DEFAULTLIBS"

dnl todo: instead of statically configuring the right pthread options --> try< to find them dynamically
dnl have_pthread=no
dnl PTHREAD_FLAGS=""
dnl PTHREAD_LIBS=""
dnl 
dnl if test "x$enable_threads" != xno; then
dnl 	AC_CHECK_HEADER(pthread.h, AC_DEFINE(HAVE_PTHREAD_H))
dnl 
dnl 	if test x$ac_cv_header_pthread_h = xyes; then
dnl 		dnl Check various threading options for the platforms we support
dnl 		for flag in -lpthreads -lpthread -pthread; do
dnl         		AC_MSG_CHECKING([for pthread_create using $flag])
dnl 			SAVELIBS="$LIBS"
dnl 			LIBS="$flag $LIBS"
dnl         		AC_TRY_LINK([#include <pthread.h>],
dnl 				[pthread_create(0, 0, 0, 0);],
dnl         			have_pthread=yes)
dnl         		AC_MSG_RESULT([$have_pthread])
dnl 			LIBS="$SAVELIBS"
dnl 
dnl 			if test $have_pthread = yes; then
dnl 				PTHREAD_FLAGS="-D_THREAD_SAFE -D_REENTRANT"
dnl 				PTHREAD_LIBS="$flag"
dnl 
dnl 				# Solaris requires -D_POSIX_PTHREAD_SEMANTICS to
dnl 				# be POSIX-compliant... :(
dnl 				if test $uname = SunOS; then
dnl 					PTHREAD_FLAGS="$PTHREAD_FLAGS -D_POSIX_PTHREAD_SEMANTICS"
dnl 				fi
dnl 				break
dnl 			fi
dnl			done
dnl 	fi
dnl fi



dnl Checks for Headers
AC_HEADER_STDC
AC_CHECK_HEADERS(sys/filio.h)
AC_CHECK_HEADERS(sys/epoll.h)
AC_CHECK_HEADER([malloc.h],AC_DEFINE([HAVE_MALLOC_H],[],[Define if you have a working version of malloc.h]))


dnl Check for some "standard" libraries
AC_CHECK_LIB(epoll,epoll_ctl)

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([vsnprintf])
AC_CHECK_FUNCS([snprintf],[], [AC_MSG_ERROR([function snprintf is not available])])
AC_CHECK_FUNCS([strnstr])
AC_CHECK_FUNCS([fstat64])
AC_CHECK_FUNCS([poll])
AC_CHECK_FUNCS([pthread_mutex_init])
AC_CHECK_FUNCS([pthread_cond_init])
AC_CHECK_FUNCS([sem_init])
AC_CHECK_FUNCS([sbrk])
AC_CHECK_FUNCS([mallinfo])
AC_CHECK_FUNCS([atoll])
AC_CHECK_FUNCS([strtoull])


dnl even if epoll exist it may be unfunctional...
if test "x$USE_EPOLL" != "x0" ; then
	AC_CHECK_FUNC([epoll_wait],
	[	
		AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include "sys/epoll.h"]],[[int handle;handle=epoll_create(1);if(handle==-1)exit(-1);]])],
		[
			AC_DEFINE([HAVE_EPOLL],[1],[Define if your OS support the epoll() syscall])
		],[],[echo "skip epoll run test because of cross compiling"])
		],[])
fi
dnl Checks for declarations.
AC_CHECK_DECL(TCP_KEEPALIVE,[AC_DEFINE(HAVE_TCP_KEEPALIVE,,[Define if your OS supports the TCP socket option TCP_KEEPALIVE])],[],[
#include <sys/types.h>
#include <sys/socket.h>])
AC_CHECK_DECL(AF_UNIX,[AC_DEFINE(HAVE_UNIX_DOMAIN_PROTOCOL,,[Define if your OS supports the Unix Domain protocol])],[],[
#include <sys/types.h>
#include <sys/socket.h>])
AC_CHECK_DECL(O_BINARY,[],[AC_DEFINE(O_BINARY,0,[This flag is used in open(), in order to open a file in binary mode])],[#include <sys/fcntl.h>])
AC_CHECK_DECL(MAX_PATH,[],[AC_DEFINE(MAX_PATH,4096,[This is the maximum length of a file system path])])
AC_CHECK_DECL(INADDR_NONE,[],[AC_DEFINE(INADDR_NONE,-1,[This is the value, which is returned from inet_addr(), in case of an error])],[
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>])
AC_CHECK_DECL(FIONREAD,[AC_DEFINE(HAVE_FIONREAD,,[This is a flag to ioctl() to get the number of available bytes on a socket])],[],[#include <sys/ioctl.h>])
AC_CHECK_DECL(MSG_NOSIGNAL,[],[AC_DEFINE(MSG_NOSIGNAL,0,[This is a flag to read(), to tell the os to get no signals])],[
#include <sys/types.h>
#include <sys/socket.h>])
AC_CHECK_DECL(MSG_DONTWAIT,[],[AC_DEFINE(MSG_DONTWAIT,0,[This is a flag to read(), to tell the os does not wait for any bytes to arrive])],[
#include <sys/types.h>
#include <sys/socket.h>])
AC_CHECK_DECL(O_SYNC,[AC_DEFINE(HAVE_O_SYNC,,[This is a flag to open() to do synchronized I/O])],[],[#include <sys/stat.h>
#include <fcntl.h>])
AC_CHECK_DECL(O_FSYNC,[AC_DEFINE(HAVE_O_FSYNC,,[This is a flag to open() to do synchronized I/O])],[],[#include <sys/stat.h>
#include <fcntl.h>])

dnl Checks for types
AC_CHECK_TYPE(socklen_t,[AC_DEFINE(HAVE_SOCKLEN_T,,[Define if the type socklen_t is available])],[],[
#include <sys/types.h>
#include <sys/socket.h>])

dnl Checks for sizes of base types
AC_CHECK_SIZEOF(unsigned char)
AC_CHECK_SIZEOF(signed char)
AC_CHECK_SIZEOF(unsigned short)
AC_CHECK_SIZEOF(signed short)
AC_CHECK_SIZEOF(unsigned int)
AC_CHECK_SIZEOF(signed int)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(signed long)
AC_CHECK_SIZEOF(unsigned long long)
AC_CHECK_SIZEOF(__uint64_t)

dnl Check Endian
AC_C_BIGENDIAN([AC_DEFINE([BYTE_ORDER_BIG_ENDIAN],,[Define if you run the code on a big endian machine])], [AC_DEFINE([BYTE_ORDER_LITTLE_ENDIAN],,[Define if you run the code on a little endian machine])])
dnl AC_C_BIGENDIAN ( [AC_DEFINE([BYTE_ORDER_BIG_ENDIAN],,[Define if you run the code on a big endian machine])], [AC_DEFINE([BYTE_ORDER_LITTLE_ENDIAN],,[Define if you run the code on a little endian machine])] ,,)


allfromsource="false"
AC_ARG_ENABLE(all-from-source,
	AS_HELP_STRING([--enable-all-from-source],[Build all dependencies from source]),
	[
		if test "x$enableval" != "xno" ; then
			allfromsource="true"
		fi
	]
)


dnl Checks for libraries.


if test "x$allfromsource" != "xtrue" ; then

if test "x$CROSSCOMPILE" != "xtrue" ; then

dnl Check for OpenSSL libs and includes. Adopted from OpenSSH configure.in
dnl default place to look for OpenSSL --> may be overwriten by the option below!
tryssldir="/usr/local/openssl /usr/lib/openssl \
						 /usr/local/ssl /usr/lib/ssl /usr/local /usr/pkg \
						 /opt /opt/openssl $HOME/ssl $HOME/openssl"
AC_ARG_WITH(ssl-dir,
	AS_HELP_STRING([--with-ssl-dir=PATH],[Specify path to OpenSSL installation]),
	[
		if test "x$withval" != "xno" ; then
			tryssldir=$withval
		fi
	]
)

saved_LIBS="$LIBS"
saved_LDFLAGS="$LDFLAGS"
saved_CFLAGS="$CFLAGS"
saved_CXXFLAGS="$CXXFLAGS"
if test "x$prefix" != "xNONE" ; then
	tryssldir="$tryssldir $prefix"
fi
AC_CACHE_CHECK([for OpenSSL directory], ac_cv_openssldir, [

	for ssldir in "" $tryssldir ; do
		if test ! -z "$ssldir" ; then
			LDFLAGS="$saved_LDFLAGS -L$ssldir/lib -L$ssldir"
			CFLAGS="$saved_CFLAGS -I$ssldir/include"
			CXXFLAGS="$saved_CXXFLAGS -I$ssldir/include"
			if test ! -z "$need_dash_r" ; then
				LDFLAGS="$LDFLAGS -R$ssldir/lib -R$ssldir"
			fi
		else
			LDFLAGS="$saved_LDFLAGS"
		fi

		LIBS="$saved_LIBS -lcrypto"

		# Basic test to check for compatible version and correct 
				# linking, *does not* test for RSA - that comes later.
		AC_RUN_IFELSE([AC_LANG_SOURCE([[
				#include <string.h>
				#include <openssl/rand.h>
				#include <openssl/aes.h>
				int main(void) 
				{
	char a[2048];
	memset(a, 0, sizeof(a));
	RAND_add(a, sizeof(a), sizeof(a));
	return(RAND_status() <= 0);
				}
				]])],
				[
				found_crypto=1
			],[
			],[	
			if test -f "$ssldir/libssl.a" && test -f "$ssldir/libcrypto.a"; then
			  found_crypto=1
			  fi])
		if test ! -z "$found_crypto" ; then
			break;
		fi
	done

	if test -z "$found_crypto" ; then
		AC_MSG_ERROR([Could not find working SSLeay / OpenSSL libraries (>=0.9.7), please install])	
	fi
	if test -z "$ssldir" ; then
		ssldir="(system)"
	fi

	ac_cv_openssldir=$ssldir
])

if (test ! -z "$ac_cv_openssldir" && test "x$ac_cv_openssldir" != "x(system)") ; then
	AC_DEFINE(HAVE_OPENSSL,,[Define if you have OpenSSL (which is needed!)])
	dnl Need to recover ssldir - test above runs in subshell
	ssldir=$ac_cv_openssldir
	CFLAGS="$saved_CFLAGS -I$ssldir/include"
	CXXFLAGS="$saved_CXXFLAGS -I$ssldir/include"
	LDFLAGS="$saved_LDFLAGS -L$ssldir/lib -L$ssldir"
	if test ! -z "$need_dash_r" ; then
		LDFLAGS="$LDFLAGS -R$ssldir/lib -R$ssldir"
	fi
	if test ! -z "$blibpath" ; then
		blibpath="$blibpath:$ssldir:$ssldir/lib"
	fi
fi
LIBS="-lcrypto -lssl $saved_LIBS"

#dnl check if ECC crypto is available
	AC_LINK_IFELSE([AC_LANG_PROGRAM([
		#include "openssl/ec.h"
		], [EC_KEY_free(0);])],[AC_DEFINE([HAVE_ECC],[],[Elliptic curve cryptography supported])],
		[AC_MSG_WARN([Elliptic curve cryptography disabled])],[AC_MSG_RESULT([skip ECC test because of cross compiling])])

else
	dnl we do cross compile --> just do some simple checks...
	AC_CHECK_HEADERS([openssl/aes.h], [],[AC_MSG_ERROR([Could not find working SSLeay / OpenSSL headers (>=0.9.7), please install])])
	AC_CHECK_LIB(crypto, BF_ecb_encrypt,[LIBS="-lcrypto -lssl $LIBS"],[AC_MSG_ERROR([Could not find working SSLeay / OpenSSL libraries (>=0.9.7), please install])])
fi

else
dnl build all form source
AC_MSG_NOTICE([Will use OpenSSL from source])
CFLAGS="$CFLAGS -I$PWD/build/openssl/include"
CXXFLAGS="$CXXFLAGS -I$PWD/build/openssl/include"

fi
		
useminixml="false"
USE_MINIXML=""
AC_ARG_WITH(minixml,
	AS_HELP_STRING([--with-minixml],
			[Use Mini-XML instead of XERCES-C]),
	[
		if test "x$withval" != "xno" ; then
			useminixml="true"
			USE_MINIXML="-DMXML_DOM"
		fi
	]
)

AM_CONDITIONAL([USE_MINIXML], [test x$useminixml = xtrue])
		
if test "x$allfromsource" != "xtrue" ; then
AC_MSG_NOTICE([Mini-XML/Xerces-C tests])

dnl Check for Mini-XML/Xerces-C
AC_LANG_PUSH(C++)
dnl Check for XML includes.
tryxmlincludedir="/usr/local/include /usr/include"

AC_ARG_WITH(xml-include,
	AS_HELP_STRING([--with-xml-include=PATH],
			[Specify path to Mini-XML/XERCES-C include files (without a trailing '/xercesc')]),
	[
		if test "x$withval" != "xno" ; then
			tryxmlincludedir=$withval
		fi
	]
)

saved_CFLAGS="$CFLAGS"
saved_CXXFLAGS="$CXXFLAGS"
if test "x$prefix" != "xNONE" ; then
	tryxmlincludedir="$tryxmlincludedir $prefix"
fi

if test "x$useminixml" == "xtrue" ; then
	AC_MSG_CHECKING([for Mini-XML includes])

	for xmldir in "" $tryxmlincludedir ; do
		CFLAGS="$saved_CFLAGS -I$xmldir"
		CXXFLAGS="$saved_CXXFLAGS -I$xmldir"
		AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
					#include <mxml.h>
					int main(void) 
					{
						return 0;
					}
					]])],
					[
					AC_MSG_RESULT([found in $xmldir])
					found_xmldir=1
					break;
				],[],[if test -f "$xmldir/mxml.h"; then
					AC_MSG_RESULT([found in $xmldir])
					found_xmldir=1
					break;
				  fi
				])
	done
else
	AC_MSG_CHECKING([for xerces-c includes])

	if test "x$CROSSCOMPILE" != "xtrue" ; then
	for xmldir in "" $tryxmlincludedir ; do
		CFLAGS="$saved_CFLAGS -I$xmldir -I$xmldir/xercesc"
		CXXFLAGS="$saved_CXXFLAGS -I$xmldir -I$xmldir/xercesc"
		AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
					#include <dom/DOM.hpp>
					int main(void) 
					{
						return 0;
					}
					]])],
					[
					AC_MSG_RESULT([found in $xmldir])
					found_xmldir=1
					break;
				],[],[if test -f "$xmldir/xercesc/util/XMLString.hpp"; then
					AC_MSG_RESULT([found in $xmldir])
					found_xmldir=1
					break;
				  fi
				])
	done

	else
	dnl Cross Compile
	CFLAGS="$saved_CFLAGS -I=/usr/include/xercesc"
	CXXFLAGS="$saved_CXXFLAGS -I=/usr/include/xercesc"
	AC_CHECK_HEADERS([dom/DOM.hpp], [found_xmldir=1],[AC_MSG_ERROR([Could not find working Xerces-C headers, please install])])
	fi
fi	
if test -z "$found_xmldir"; then
	AC_MSG_ERROR([Mini-XML/Xerces-C includes NOT found - please use --with-xml-include])
fi

dnl Check for XML libs
saved_LIBS="$LIBS"
saved_LDFLAGS="$LDFLAGS"
tryxmllibdir="/usr/local/lib /usr/lib"
AC_ARG_WITH(xml-lib,
	AS_HELP_STRING([--with-xml-lib=PATH],[Specify path to Mini-XML/XERCES-C library files]),
	[
		if test "x$withval" != "xno" ; then
			tryxmllibdir=$withval
		fi
	]
)
if test "x$prefix" != "xNONE" ; then
	tryxmllibdir="$tryxmllibdir $prefix"
fi

if test "x$useminixml" == "xtrue" ; then
	AC_MSG_CHECKING([for libminixml])
	LIBS="-lmxml $saved_LIBS"
	for xmllibdir in "" $tryxmllibdir ; do
		LDFLAGS="$saved_LDFLAGS -L$xmllibdir"
		AC_LINK_IFELSE([AC_LANG_PROGRAM([
			#include <mxml.h>
			], [mxmlDelete(NULL)
	])],[
	AC_MSG_RESULT([found in $xmllibdir])
	found_xmllib=1 break;
	],[],[found_xmllib=1
				AC_MSG_RESULT([skip xml lib run test because of cross compiling])
				])
	done

else

	AC_MSG_CHECKING([for libxerces-c])


	if test "x$CROSSCOMPILE" != "xtrue" ; then
	dnl Note: the order is import for compiling on TUDOS
	LIBS="-lxerces-c $saved_LIBS"
	for xmllibdir in "" $tryxmllibdir ; do
		LDFLAGS="$saved_LDFLAGS -L$xmllibdir"
		AC_LINK_IFELSE([AC_LANG_PROGRAM([
			#include "util/XercesDefs.hpp"
			#include "util/PlatformUtils.hpp"
			#if (_XERCES_VERSION >= 20200)
			   XERCES_CPP_NAMESPACE_USE
			#endif
			], [XMLPlatformUtils::Initialize()
	])],[
	AC_MSG_RESULT([found in $xmllibdir])
	found_xmllib=1 break;
	],[],[found_xmllib=1
				AC_MSG_RESULT([skip xml lib run test because of cross compiling])
				])
	done

	else
	dnl Cross Compile
	AC_CHECK_LIB(xerces-c,printf ,[LIBS="-lxerces-c $LIBS"],[AC_MSG_ERROR([Could not find working Xerces-C libraries, please install])])
	found_xmllib=1
	fi

fi
if test -z "$found_xmllib"; then
	AC_MSG_ERROR([Mini-XML/Xerces-C libs NOT found - please use --with-xml-lib])
fi

else
dnl all from source
AC_MSG_NOTICE([Will use Mini-XML/Xerces-C from source])
CFLAGS="$CFLAGS -I$PWD/build/mxml"
CXXFLAGS="$CXXFLAGS -I$PWD/build/mxml"

fi dnl not make all from source


AC_LANG_POP(C++)


dnl Check for Postgresql libs if (needed if compiled with payment support)
if test "x$PAYMENT" != "x" ; then
	AC_MSG_NOTICE([PostgreSQL tests])
	dnl we check for <postgresql/libq-fe.h>,<pgsql/libpq-fe.h> and <libpq-fe.h>
	AC_CHECK_HEADERS([postgresql/libpq-fe.h],[],[
						AC_CHECK_HEADERS([pgsql/libpq-fe.h],[],[
											AC_CHECK_HEADERS([libpq-fe.h],[],[
													AC_MSG_ERROR([PostgreSQL include file libpq-fe.h not found!])
															]
													)		    
											]
								)			
						]
			)		    
	AC_MSG_CHECKING([for PostgreSQL libs])
	saved_LIBS="$LIBS"
	PAYMENT_LIB="-lpq"
	LIBS="$LIBS $PAYMENT_LIB"
	AC_LINK_IFELSE([AC_LANG_PROGRAM([
				#ifdef HAVE_POSTGRESQL_LIBPQ_FE_H
				#include <postgresql/libpq-fe.h>
				#elif defined(HAVE_PGSQL_LIBPQ_FE_H)
				#include <pgsql/libpq-fe.h>
				#else
				#include <libpq-fe.h>
				#endif
				], [PQfinish(0);
				])],[AC_MSG_RESULT(libpq found)],
				[dnl libpq not found trying libpq++
					PAYMENT_LIB="-lpq++"
					LIBS="$saved_LIBS $PAYMENT_LIB"
					AC_LINK_IFELSE([AC_LANG_PROGRAM([
					#ifdef HAVE_POSTGRESQL_LIBPQ_FE_H
					#include <postgresql/libpq-fe.h>
					#elif defined(HAVE_PGSQL_LIBPQ_FE_H)
					#include <pgsql/libpq-fe.h>
					#else
					#include <libpq-fe.h>
					#endif
					], [PQfinish(0);
					])],[AC_MSG_RESULT(libpq++ found)],
					[AC_MSG_ERROR(PostgreSQL library libpq or libpq++ NOT found - please install them)])
				])

LIBS="$saved_LIBS"
fi


dnl Check for Mysql libs if needed
if test "x$DIALOG" != "x" ; then
	AC_MSG_NOTICE([Mysql tests])
	AC_CHECK_HEADERS([mysql/mysql.h],[],[
						AC_CHECK_HEADERS([mysql/mysql.h],[],[
											AC_CHECK_HEADERS([mysql.h],[],[
													AC_MSG_ERROR([Mysql include file mysql.h not found!])
															]
													)		    
											]
								)			
						]
			)		    
	AC_MSG_CHECKING([for Mysql libs])
	trymysqllibdir="/usr/lib /usr/local/lib /usr/lib/mysql /usr/local/lib/mysql"
	saved_LIBS="$LIBS"
	saved_LDFLAGS="$LDFLAGS"
	MYSQL_LIB="-lmysqlclient_r"
	LIBS="$LIBS $MYSQL_LIB"
	for mysqllibdir in "" $trymysqllibdir ; do
	LDFLAGS="$saved_LDFLAGS -L$mysqllibdir"
	AC_LINK_IFELSE([AC_LANG_PROGRAM([
				#ifdef HAVE_MYSQL_MYSQL_H
				#include <mysql/mysql.h>
				#elif defined(HAVE_MYSQL_H)
				#include <mysql.h>
				#endif
				], [mysql_init(0);
				])],[
				AC_MSG_RESULT([found in $mysqllibdir])
				found_mysqllib=1 break;
				],[],[found_mysqllib=1
			AC_MSG_RESULT([skip mysql lib run test because of cross compiling])
			])
	done
	if test -z "$found_mysqllib"; then
		AC_MSG_ERROR([mysql libs NOT found - please use --with-mysql-lib])
	fi

	AC_CHECK_DECL([MYSQL_OPT_RECONNECT], [AC_DEFINE(HAVE_MYSQL_OPT_RECONNECT,,[Define if you mysql client library supports the MYSQL_RECONNECT option])]   ,[],[[
					#ifdef HAVE_MYSQL_MYSQL_H
				#include <mysql/mysql.h>
				#elif defined(HAVE_MYSQL_H)
				#include <mysql.h>
				#endif
				]])
	LIBS="$saved_LIBS"
fi



AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],[Enable debug compilation mode]),
	[
		if test "x$enableval" != "xno" ; then
		CFLAGS="$CFLAGS -ggdb3 -DNO_SIGSEV_CATCH -fno-inline -fno-default-inline"
		CXXFLAGS="$CXXFLAGS -ggdb3 -DNO_SIGSEV_CATCH -fno-inline -fno-default-inline"
		OFLAGS="-O0"
		else
		CFLAGS="$CFLAGS -g0"	
		CXXFLAGS="$CXXFLAGS -g0"	
		fi
	]
)

AC_ARG_ENABLE(debug-output,
	AS_HELP_STRING([--enable-debug-output],[Enable output of debug messages]),
	[
		if test "x$enableval" != "xno" ; then
		CFLAGS="$CFLAGS -D_DEBUG"
		CXXFLAGS="$CXXFLAGS -D_DEBUG"
		fi
	]
)

AC_ARG_ENABLE(dataretentionlog,
	AS_HELP_STRING([--enable-dataretentionlog],[Enable log messages according to German data retention]),
	[
		if test "x$enableval" != "xno" ; then
		CFLAGS="$CFLAGS -DDATA_RETENTION_LOG"
		CXXFLAGS="$CXXFLAGS -DDATA_RETENTION_LOG"
		fi
	]
)

AC_ARG_ENABLE(keyexport,
	AS_HELP_STRING([--enable-keyexport],[Enable the export of the private encryption keys (only for debug purposes)]),
	[
		if test "x$enableval" != "xno" ; then
		CFLAGS="$CFLAGS -DEXPORT_ASYM_PRIVATE_KEY"
		CXXFLAGS="$CXXFLAGS -DEXPORT_ASYM_PRIVATE_KEY"
		fi
	]
)

AC_ARG_ENABLE(with-integrity-check,
	AS_HELP_STRING([--enable-with-integrity-check],[Enable integrity check for data channels]),
	[
		if test "x$enableval" != "xno" ; then
		CFLAGS="$CFLAGS -DWITH_INTEGRITY_CHECK"
		CXXFLAGS="$CXXFLAGS -DWITH_INTEGRITY_CHECK"
		fi
	]
)

AC_ARG_ENABLE(google-cpuprofiler,
	AS_HELP_STRING([--enable-google-cpuprofiler],[Enable CPU profiling using Googles Perftools]),
	[
		if test "x$enableval" != "xno" ; then
		CFLAGS="$CFLAGS -DENABLE_GPERFTOOLS_CPU_PROFILER"
		CXXFLAGS="$CXXFLAGS -DENABLE_GPERFTOOLS_CPU_PROFILER"
		LIBS="$LIBS -lprofiler"
		fi
	]
)

AC_ARG_ENABLE(google-heapchecker,
	AS_HELP_STRING([--enable-google-heapchecker],[Enable heap checking using Googles Perftools]),
	[
		if test "x$enableval" != "xno" ; then
		CFLAGS="$CFLAGS -DENABLE_GPERFTOOLS_HEAP_CHECKER"
		CXXFLAGS="$CXXFLAGS -DENABLE_GPERFTOOLS_HEAP_CHECKER"
		LIBS="$LIBS -ltcmalloc"
		fi
	]
)


CRIME=""
logcrime="false"
AC_ARG_ENABLE(crime-detection,
	AS_HELP_STRING([--enable-crime-detection],[Enable crime detection mode]),
	[
		if test "x$enableval" != "xno" ; then
		CRIME="-DLOG_CRIME"
		logcrime="true"
		fi	
	]
)

DOTHETESTS=""
AC_ARG_ENABLE(tests,
	AS_HELP_STRING([--enable-tests],[Build also some functional tests]),
	[
		if test "x$enableval" != "xno" ; then
		DOTHETESTS="yes"
		fi	
	]
)
GOOGLETEST_LIB=""
AC_ARG_WITH(googletest-lib,
	AS_HELP_STRING([--with-googletest-lib=PATH],[Specify path to Google Test library files]),
	[
		if test "x$withval" != "xno" ; then
			GOOGLETEST_LIB="-L$withval"
		fi
	]
)



DOWITHWIRESHARK=""
AC_ARG_ENABLE(wireshark,
	AS_HELP_STRING([--enable-wireshark],[Build also the Wireshark AN.ON protocol dissector]),
	[
		if test "x$enableval" != "xno" ; then
		DOWITHWIRESHARK="yes"
		fi	
	]
)


AC_ARG_ENABLE(manioq,
	AS_HELP_STRING([--enable-manioq],[Switch for Manioq provider mixes]),
	[
		if test "x$enableval" != "xno" ; then
		PAYMENT=""
		CFLAGS="$CFLAGS -DMANIOQ"
		CXXFLAGS="$CXXFLAGS -DMANIOQ"
		fi
	]
)

onlylib="false"
AC_ARG_ENABLE(build-only-lib,
	AS_HELP_STRING([--enable-build-only-lib],[Build just the library, but not the executable]),
	[
		if test "x$enableval" != "xno" ; then
			onlylib="true"
		fi
	]
)

onlymiddlemix="false"
AC_ARG_ENABLE(only-middle-mix,
	AS_HELP_STRING([--enable-only-middle-mix],[Build just the code for operating a last or middle mix]),
	[
		if test "x$enableval" != "xno" ; then
			onlymiddlemix="true"
		fi
	]
)

sconetoolchain=""
USE_SCONE_CFLAGS=""
AC_ARG_WITH(SCONE,
	AS_HELP_STRING([--with-SCONE=SCONE_TOOLCHAIN_PATH],[Build the Mix so it runs entirely within the SCONE framework to let it run on Intel SGX. Give the path to the SCONE toolchain]),
	[
		if test "x$withval" != "xno" ; then
			sconetoolchain="$withval"
			USE_SCONE_CFLAGS="-DWITH_SGX"
		fi
	]
)

use_sgx="false"
AC_ARG_WITH(SGX,
	AS_HELP_STRING([--with-SGX=SCONE_TOOLCHAIN_PATH],[Build a Mix which uses Intel SGX but is splitted into a trusted (within enclave) and untrusted aprt (outsied enclave). Give the path to the SCONE toolchain]),
	[
		if test "x$withval" != "xno" ; then
			sconetoolchain="$withval"
			use_sgx="true"
		fi
	]
)

build_as_shadow_plugin="false"
BUILD_AS_SHADOW_PLUGIN_CFLAGS=""
AC_ARG_ENABLE(build-as-shadow-plugin,
	AS_HELP_STRING([--enable-build-as-shadow-plugin],[Build as Shadow simulator plugin]),
	[
		if test "x$enableval" != "xno" ; then
			build_as_shadow_plugin="true"
			BUILD_AS_SHADOW_PLUGIN_CFLAGS="-D__BUILD_AS_SHADOW_PLUGIN__"
		fi
	]
)

BUILD_BENCHMARKING_CFLAGS=""
build_benchmarking="false"
AC_ARG_ENABLE(mix-benchmarking,
	AS_HELP_STRING([--enable-mix-benchmarking],[Build special versions of the Mix used for benchmarking]),
	[
		if test "x$enableval" != "xno" ; then
			build_benchmarking="true"
			BUILD_BENCHMARKING_CFLAGS="-DDO_MIDDLE_MIX_BENCHMARK"
		fi
	]
)

TEST_CFLAGS=""

if test "x$DOTHETESTS" == "xyes" ; then
	AC_SUBST(LIBGOOGLETEST,gtest)
	TEST_CFLAGS="-D__GOOGLE_UNIT_TEST__"
fi




WIRESHARK_CFLAGS=""
AS_IF([test "x$DOWITHWIRESHARK" = "xyes"], [
   PKG_CHECK_MODULES([GLIB], [glib-2.0],[WIRESHARK_CFLAGS="$GLIB_CFLAGS -I/usr/include/wireshark"],[AC_MSG_ERROR([You need glib-2.0 in order to compile the wireshark plugin!])])
   savedCFLAGS="$CFLAGS"
   CFLAGS="$CFLAGS $WIRESHARK_CFLAGS"
   AC_CHECK_HEADER([wiretap/wtap.h],[],[AC_MSG_ERROR([Wireshark development header files not found! Symlink /usr/include/wireshark to your wireshark source code directory could solve this problem!])])
   CFLAGS="$savedCFLAGS"
])

CFLAGS="$CFLAGS $POOL $DYNAMIC $DIALOG $BANDWIDTH_LIMIT $BANDWIDTH_LIMIT_THROUGHPUT  $PAYMENT $INFOSERVICESERIAL $SERVER_MONITORING $REPLAY_DETECTION $NEW_MIX_TYPE $CRIME $USE_MINIXML $OFLAGS $WIRESHARK_CFLAGS $TEST_CFLAGS $BUILD_AS_SHADOW_PLUGIN_CFLAGS $USE_SCONE_CFLAGS $BUILD_BENCHMARKING_CFLAGS"
CXXFLAGS="$CXXFLAGS $POOL $DYNAMIC $DIALOG $BANDWIDTH_LIMIT $BANDWIDTH_LIMIT_THROUGHPUT $PAYMENT $INFOSERVICESERIAL $SERVER_MONITORING $REPLAY_DETECTION $NEW_MIX_TYPE $CRIME $USE_MINIXML $OFLAGS $WIRESHARK_CFLAGS $TEST_CFLAGS $BUILD_AS_SHADOW_PLUGIN_CFLAGS $USE_SCONE_CFLAGS $BUILD_BENCHMARKING_CFLAGS"
LIBS="$LIBS $PAYMENT_LIB $MYSQL_LIB $GOOGLETEST_LIB"



if test "x$onlylib" == "xtrue" || test "x$build_as_shadow_plugin" == "xtrue" ; then
	CXXFLAGS="$CXXFLAGS -fPIC -DPIC"
	CFLAGS="$CFLAGS -fPIC -DPIC"
fi

if test "x$onlymiddlemix" == "xtrue" ; then
	CXXFLAGS="$CXXFLAGS -DONLY_MIDDLE_MIX"
	CFLAGS="$CFLAGS -DONLY_MIDDLE_MIX"
fi

if test -n "$sconetoolchain"; then
		if test "x$sconetoolchain" == "xyes" ; then
    AC_SUBST(SCONE_TOOLCHAIN,"")
    AC_SUBST(SCONE_GCC,"$CC")
    AC_SUBST(SCONE_CXX,"$CXX")
		else
    AC_SUBST(SCONE_TOOLCHAIN,"$sconetoolchain")
    AC_SUBST(SCONE_GCC,"$sconetoolchain/x86_64-linux-musl/bin/x86_64-linux-musl-gcc")
    AC_SUBST(SCONE_CXX,"$sconetoolchain/x86_64-linux-musl/bin/x86_64-linux-musl-g++")
    fi
    AM_CONDITIONAL([USE_SCONE],true)
else
    AM_CONDITIONAL([USE_SCONE],false)
fi

if test "x$use_sgx" == "xtrue" ; then
 AC_SUBST(USE_SGX_INSIDE,"-DUSE_SGX_INSIDE")
fi

AM_CONDITIONAL([DOTESTS], [test x$DOTHETESTS = xyes])
AM_CONDITIONAL([DOWIRESHARK], [test x$DOWITHWIRESHARK = xyes])
AM_CONDITIONAL([ONLY_LIB], [test x$onlylib = xtrue])
AM_CONDITIONAL([ALL_FROM_SOURCE], [test x$allfromsource = xtrue])
AM_CONDITIONAL([BUILD_AS_SHADOW_PLUGIN], [test x$build_as_shadow_plugin = xtrue])
AM_CONDITIONAL([BUILD_MIX_BENCHMARKING], [test x$build_benchmarking = xtrue])
AM_CONDITIONAL([USE_SGX], [test x$use_sgx = xtrue])
AM_CONDITIONAL([BUILD_ACL_LOG_HELPER], [test x$logcrime = xtrue])

AC_CONFIG_FILES([Makefile popt/Makefile xml/Makefile trio/Makefile tre/Makefile TypeA/Makefile TypeB/Makefile test/Makefile gcm/Makefile wireshark/Makefile SquidLogHelper/Makefile AclLogHelper/Makefile SGX/Makefile benchmarking/Makefile])
AC_OUTPUT
