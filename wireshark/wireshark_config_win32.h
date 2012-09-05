/* $Id: config.h.win32 39283 2011-10-05 23:02:31Z gerald $ */
/* config.h.win32.  Generated manually. :-) */
/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Generated Bison and Flex files test whether __STDC__ is defined
   in order to check whether to use ANSI C features such as "const".

   GCC defines it as 1 even if extensions that render the implementation
   non-conformant are enabled; Sun's C compiler (and, I think, other
   AT&T-derived C compilers) define it as 0 if extensions that render
   the implementation non-conformant are enabled; Microsoft Visual C++
   6.0 doesn't define it at all if extensions that render the implementation
   non-conformant are enabled.

   We define it as 0 here, so that those generated files will use
   those features (and thus not get type warnings when compiled with
   MSVC++). */
#ifndef __STDC__
#define __STDC__ 0
#endif

/*
 * Flex (v 2.5.35) uses this symbol to "exclude" unistd.h
 */
#define YY_NO_UNISTD_H

/* Use Unicode in Windows runtime functions. */
#define UNICODE 1
#define _UNICODE 1

/* Use threads */
#define USE_THREADS 1

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

/* Define if lex declares yytext as a char * by default, not a char[].  */
#define YYTEXT_POINTER 1

#define HAVE_PLUGINS		1

/* #undef HAVE_SA_LEN */

/* #undef HAVE_MKSTEMP */
/* #undef HAVE_MKDTEMP */

#define HAVE_LIBPCAP 1

#define HAVE_PCAP_FINDALLDEVS 1
#define HAVE_PCAP_DATALINK_NAME_TO_VAL 1
#define HAVE_PCAP_DATALINK_VAL_TO_NAME 1
#define HAVE_PCAP_DATALINK_VAL_TO_DESCRIPTION 1
#define HAVE_LIBWIRESHARKDLL 1

#define HAVE_PCAP_LIST_DATALINKS 1
#define HAVE_PCAP_FREE_DATALINKS 1
#define HAVE_PCAP_SET_DATALINK 1

#define HAVE_REMOTE 1
#define HAVE_PCAP_REMOTE 1
#define HAVE_PCAP_OPEN 1
#define HAVE_PCAP_OPEN_DEAD 1
#define HAVE_BPF_IMAGE 1
#define HAVE_PCAP_SETSAMPLING 1

/* availability of pcap_freecode() is handled at runtime */
#define HAVE_PCAP_FREECODE 1

/* `pcap_get_selectable_fd' is UN*X-only. */
/* #undef HAVE_PCAP_GET_SELECTABLE_FD */

#define HAVE_AIRPCAP 1

#define PCAP_NG_DEFAULT 1


/* define macro for importing variables from an dll
 * it depends on HAVE_LIBWIRESHARKDLL and _NEED_VAR_IMPORT_
 */
#if defined (_NEED_VAR_IMPORT_) && defined (HAVE_LIBWIRESHARKDLL)
#  define WS_VAR_IMPORT __declspec(dllimport) extern
#else
#  define WS_VAR_IMPORT extern
#endif

/*
 * Define WS_MSVC_NORETURN appropriately for declarations of routines that
 * never return (just like Charlie on the MTA).
 *
 * Note that MSVC++ expects __declspec(noreturn) to precede the function
 * name and GCC, as far as I know, expects __attribute__((noreturn)) to
 * follow the function name, so we need two different flavors of
 * noreturn tag.
 */
#define WS_MSVC_NORETURN	__declspec(noreturn)

/* Define if you have the gethostbyname2 function.  */
/* #undef HAVE_GETHOSTBYNAME2 */

/* Define if you have the getprotobynumber function.  */
/* #undef HAVE_GETPROTOBYNUMBER */

/* Define if you have the <arpa/inet.h> header file.  */
/* #undef HAVE_ARPA_INET_H */

/* Define if you have the <direct.h> header file.  */
#define HAVE_DIRECT_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <iconv.h> header file.  */
/* #undef HAVE_ICONV */

/* Define if you have the <netdb.h> header file.  */
/* #undef HAVE_NETDB_H */

/* Define if you have the <netinet/in.h> header file.  */
/* #define HAVE_NETINET_IN_H 1 */

/* Define if you have the <snmp/snmp.h> header file.  */
/* #undef HAVE_SNMP_SNMP_H */

/* Define if you have the <snmp/version.h> header file.  */
/* #undef HAVE_SNMP_VERSION_H */

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <stddef.h> header file.  */
/* #undef HAVE_STDDEF_H */

/* Define if you have the <sys/ioctl.h> header file.  */
/* #undef HAVE_SYS_IOCTL_H */

/* Define if you have the <sys/socket.h> header file.  */
/* #undef HAVE_SYS_SOCKET_H */

/* Define if you have the <sys/sockio.h> header file.  */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file.  */
/* #define HAVE_SYS_TIME_H 1 */

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <sys/wait.h> header file.  */
/* #undef HAVE_SYS_WAIT_H */

/* Define if tm_zone field exists in struct tm */
/* #undef HAVE_TM_ZONE 1 */

/* Define if tzname array exists */
/* #undef HAVE_TZNAME */

/* Define if you have the <unistd.h> header file.  */
/* #define HAVE_UNISTD_H 1 */

/* Define if you have the <windows.h> header file.  */
#define HAVE_WINDOWS_H 1

/* Define if you have the <winsock2.h> header file.  */
#define HAVE_WINSOCK2_H 1

/* Define if <inttypes.h> defines PRI[doxu]64 macros */
/* #define INTTYPES_H_DEFINES_FORMATS */

/* Define if you have the z library (-lz).  */
#define HAVE_LIBZ 1
#ifdef HAVE_LIBZ
#define HAVE_INFLATEPRIME 1
#endif

/* Define to use GNU ADNS library */
#define HAVE_C_ARES 1

/* Define to use GNU ADNS library */
#ifndef HAVE_C_ARES

#define ADNS_JGAA_WIN32 1
#endif

/* Define to use the Nettle library */


/* Define to use the gnutls library */
#define HAVE_LIBGNUTLS 1

/* Define to use the libgcrypt library */
#define HAVE_LIBGCRYPT 1

/* Define to use mit kerberos for decryption of kerberos/sasl/dcerpc */
#define HAVE_MIT_KERBEROS 1
#ifdef HAVE_MIT_KERBEROS
#define HAVE_KERBEROS
#endif

/* Define to use Lua */
#define HAVE_LUA 1
#define HAVE_LUA_5_1 1

/* Define to use Python */


/* Define to use Portaudio library */
#define HAVE_LIBPORTAUDIO 1
/* Define  version of of the Portaudio library API */


/* Define to have SMI */
#define HAVE_LIBSMI 1

/* Define to use GeoIP library */
#define HAVE_GEOIP 1

/* Define if GeoIP supports IPv6 (GeoIP 1.4.5 and later) */
#define HAVE_GEOIP_V6 1

/* Define for IPv6 */
#define INET6 1

/* Define to have ntddndis.h */
#define HAVE_NTDDNDIS_H 1

#define NEED_INET_ATON_H    1
#define NEED_INET_V6DEFS_H  1
#define NEED_STRPTIME_H     1

#ifndef WIN32
#define WIN32			1
#endif

/* Visual C 9 (2008) & Visual C 10 (2010) need these prototypes */
#if _MSC_VER == 1500 || _MSC_VER == 1600
#define NTDDI_VERSION NTDDI_WIN2K
#define _WIN32_WINNT _WIN32_WINNT_WIN2K
#endif

#define strncasecmp		strnicmp
#define popen			_popen
#define pclose			_pclose

/* Define to use GTK */
#define HAVE_GTK

/* Name of package */
#define PACKAGE "wireshark"

/* Version number of package */
#define VERSION "1.7.0"

/* We shouldn't need this under Windows but we'll define it anyway. */
#define HTML_VIEWER "mozilla"

/* Check for the required _MSC_VER */
/*#if MSC_VER_REQUIRED != _MSC_VER
#define WS_TO_STRING2(x) #x
#define WS_TO_STRING(x) WS_TO_STRING2(x)
#pragma message( "_MSC_VER is:" WS_TO_STRING(_MSC_VER) " but required is:" WS_TO_STRING(MSC_VER_REQUIRED) )
#error Your MSVC_VARIANT setting in config.nmake doesn't match the MS compiler version!
#endif
*/
/* Disable Code Analysis warnings that result in too many false positives. */
/* http://msdn.microsoft.com/en-US/library/zyhb0b82.aspx */
#if _MSC_VER >= 1400
#pragma warning ( disable : 6011 )
#endif

typedef int ssize_t;
