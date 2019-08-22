#
# Autoconfiguration files for proxytest (a MiX proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign

if  USE_SGX # build programm which should run outside enclave
sbin_PROGRAMS=mixOutsideEnclave
mixOutsideEnclave_SOURCES = CASGXMixOutsideEnclave.cpp
mixOutsideEnclave_CFLAGS = $(AM_CFLAGS) -DUSE_SGX_OUTSIDE
mixOutsideEnclave_CPPFLAGS = $(AM_CPPFLAGS) -DUSE_SGX_OUTSIDE

endif # USE_SGX