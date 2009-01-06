#
# Autoconfiguration files for proxytest (a MiX proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign

check_PROGRAMS = test.proxytest
test_proxytest_SOURCES = TCAUtil.cpp
test_proxytest_LDADD =-l @LIBCPPUNIT@ $(top_srcdir)/libproxytest.a $(top_srcdir)/xml/libxml.a \
	$(top_srcdir)/popt/libpopt.a
	
TESTS = test.proxytest
                                                     
includes = @INCLUDES@