#
# Autoconfiguration files for proxytest (a MiX proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign

if  USE_SGX # build programm which should run outside enclave
sbin_PROGRAMS = mix-outside-enclave
mix-outside-enclave_SOURCES = CASGXMixOutsideEnclave.cpp
mix-outside-enclave_CFLAGS = $(AM_CFLAGS)
mix-outside-enclave_CPPFLAGS = $(AM_CPPFLAGS)

endif # USE_SGX