#
# Autoconfiguration files for proxytest (a MiX proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign

check_PROGRAMS = test.proxytest
test_proxytest_SOURCES = TCAUtil.cpp TCACertificate.cpp AllTests.cpp
test_proxytest_LDADD =-l@LIBGOOGLETEST@ \
		$(top_srcdir)/libproxytest.a \
	    $(top_srcdir)/xml/libxml.a \
	    $(top_srcdir)/popt/libpopt.a \
	    $(top_srcdir)/trio/libtrio.a \
	    $(top_srcdir)/tre/libtre.a \
	    $(top_srcdir)/gcm/libgcm.a \
	    $(top_srcdir)/TypeA/libmixa.a
	
TESTS = test.proxytest
                                                     
includes = @INCLUDES@