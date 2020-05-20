#
# Autoconfiguration files for proxytest (a MiX proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign

if  BUILD_ACL_LOG_HELPER # build Squid ACl log helepr programm?

includes = @INCLUDES@

bin_PROGRAMS = aclloghelper

aclloghelper_SOURCES = AclLogHelper.cpp

aclloghelper_CFLAGS = $(AM_CFLAGS)
aclloghelper_CPPFLAGS = $(AM_CPPFLAGS)
aclloghelper_LDADD = $(top_builddir)/libproxytest.a \
	    $(top_builddir)/xml/libxml.a \
	    $(top_builddir)/popt/libpopt.a \
	    $(top_builddir)/trio/libtrio.a \
	    $(top_builddir)/tre/libtre.a \
	    $(top_builddir)/gcm/libgcm.a \
	    $(top_builddir)/TypeA/libmixa.a \
	    $(top_builddir)/SquidLogHelper/libsquidloghelper.a 

endif #BUILD_ACL_LOG_HELPER
