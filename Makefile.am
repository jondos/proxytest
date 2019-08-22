#
# Autoconfiguration files for proxytest (a Mix proxy)
# Konrad Rieck <kr@r0q.cx>
#
ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign

export SGXTOOLCHAIN=@SCONE_TOOLCHAIN@
mix_LDADD =

proxytest_sources = CAAbstractControlChannel.cpp\
		    CAAbstractXMLEncodable.cpp\
		    CAAbstractXMLSignable.cpp\
		    CAAccountingBIInterface.cpp\
		    CAAccountingControlChannel.cpp\
		    CAAccountingDBInterface.cpp\
		    CAAccountingInstance.cpp\
		    CAAccountingSettleThread.cpp\
		    CAASymCipher.cpp \
		    CABase64.cpp \
		    CACacheLoadBalancing.cpp\
		    CACertificate.cpp\
		    CACertStore.cpp\
		    CAClientSocket.cpp\
		    CACmdLnOptions.cpp \
		    CAConditionVariable.cpp\
		    CAControlChannelDispatcher.cpp\
		    CAControlChannelTest.cpp\
		    CADatabase.cpp\
		    CADatagramSocket.cpp \
		    CADataRetentionLog.cpp\
		    CADataRetentionLogFile.cpp\
		    CADynaNetworking.cpp \
		    CADynamicCascadeConfigurator.cpp \
		    CAFileSystemDirectory.cpp \
		    CAFirstMix.cpp \
		    CAFirstMixChannelList.cpp \
		    CAHttpClient.cpp \
		    CAInfoService.cpp \
		    CAInfoServiceAware.cpp \
		    CAIPList.cpp \
		    CAIPAddrWithNetmask.cpp \
		    CALastMix.cpp \
		    CALastMixChannelList.cpp\
		    CAListenerInterface.cpp\
		    CALibProxytest.cpp\
		    CALocalProxy.cpp \
		    CALogPacketStats.cpp\
		    CAMiddleMix.cpp \
		    CAMiddleMixChannelList.cpp \
		    CAMix.cpp \
		    CAMixWithReplayDB.cpp\
		    CAMsg.cpp \
		    CAMultiSignature.cpp\
		    CAMutex.cpp\
		    CAMuxSocket.cpp \
		    CAPriceInfo.cpp \
		    CAPool.cpp\
		    CAQueue.cpp \
		    CAReplayControlChannel.cpp\
		    CAReplayCtrlChannelMsgProc.cpp\
		    CASemaphore.cpp\
		    CASignature.cpp \
		    CASingleSocketGroup.cpp\
		    CASocket.cpp \
		    CASocketAddrINet.cpp \
		    CASocketAddrUnix.cpp \
		    CASocketGroup.cpp \
		    CASocketGroupEpoll.cpp\
		    CASocketList.cpp \
		    CAStatusManager.cpp \
		    CATLSClientSocket.cpp\
		    CASymCipher.cpp \
			CASymCipherCTR.cpp \
			CASymCipherGCM.cpp \
			CASymCipherOFB.cpp \
		    CASyncControlChannel.cpp\
		    CATempIPBlockList.cpp\
		    CAThread.cpp \
		    CAThreadPool.cpp\
		    CAThreadList.cpp \
		    CAUtil.cpp \
		    CAXMLBI.cpp \
		    CAXMLCostConfirmation.cpp\
		    CAXMLErrorMessage.cpp\
		    CAXMLPriceCert.cpp\
		    Hashtable.cpp\
		    TermsAndConditions.cpp \
		    packetintro.cpp \
		    StdAfx.cpp \
		    CAAbstractControlChannel.hpp\
		    CAAbstractXMLEncodable.hpp\
		    CAAbstractXMLSignable.hpp\
		    CAAccountingBIInterface.hpp\
		    CAAccountingControlChannel.hpp\
		    CAAccountingDBInterface.hpp\
		    CAAccountingInstance.hpp\
		    CAAccountingSettleThread.hpp\
		    CAASymCipher.hpp \
		    CABase64.hpp \
		    CACacheLoadBalancing.hpp\
		    CACertificate.hpp\
		    CACertStore.hpp\
		    CAConditionVariable.hpp\
		    CAControlChannelDispatcher.hpp\
		    CAControlChannelTest.hpp\
		    CACryptoBenchmark.cpp \
		    CACryptoBenchmark.hpp \
		    CACmdLnOptions.hpp \
		    CADatabase.hpp\
		    CADatagramSocket.hpp\
		    CADataRetentionLog.hpp\
		    CADataRetentionLogFile.hpp\
		    CADynaNetworking.hpp \
		    CADynamicCascadeConfigurator.hpp \
		    CAFileSystemDirectory.hpp \
		    CAFirstMix.hpp \
		    CAFirstMixChannelList.hpp \
		    CAHttpClient.hpp \
		    CAInfoService.hpp \
		    CAInfoServiceAware.hpp \
		    CAIPList.hpp \
		    CAIPAddrWithNetmask.hpp \
		    CALastMix.hpp \
		    CALastMixChannelList.hpp\
		    CAListenerInterface.hpp\
		    CALibProxytest.hpp\
		    CALocalProxy.hpp \
		    CALockAble.hpp\
		    CALogPacketStats.hpp\
		    CAMiddleMix.hpp \
		    CAMiddleMixChannelList.hpp \
		    CAMix.hpp \
		    CAMixWithReplayDB.hpp\
		    CAMsg.hpp \
		    CAMultiSignature.hpp\
		    CAMutex.hpp\
		    CAMuxSocket.hpp \
		    CAPriceInfo.hpp \
		    CAPool.hpp\
		    CAQueue.hpp \
		    CAReplayControlChannel.hpp\
		    CAReplayCtrlChannelMsgProc.hpp\
		    CASemaphore.hpp\
		    CASignature.hpp \
		    CASingleSocketGroup.hpp\
		    CASocket.hpp \
		    CASocketAddr.hpp \
		    CASocketAddrINet.hpp \
		    CASocketAddrUnix.hpp \
		    CASocketGroup.hpp \
		    CASocketGroupEpoll.hpp\
		    CASocketList.hpp \
		    CAStatusManager.hpp \
		    CATLSClientSocket.hpp\
		    CAClientSocket.hpp\
		    CASymCipher.hpp \
		    CASymCipherGCM.hpp \
		    CASymChannelCipher.hpp \
		    CASymChannelCipherFactory.hpp \
		    CASymCipher.hpp \
			CASymCipherCTR.hpp \
			CASymCipherGCM.hpp \
			CASymCipherOFB.hpp \
		    CASyncControlChannel.hpp \
		    CATempIPBlockList.hpp\
		    CAThread.hpp \
		    CAThreadPool.hpp\
		    CAThreadList.hpp \
		    CAUtil.hpp \
		    CAXMLBI.hpp \
		    CAXMLCostConfirmation.hpp\
		    CAXMLErrorMessage.hpp\
		    CAXMLPriceCert.hpp\
		    Hashtable.hpp\
		    TermsAndConditions.hpp \
		    packetintro.h\
		    StdAfx.h\
		    basetypedefs.h





if ONLY_LIB
lib_LTLIBRARIES = libmix.la
libmix_la_CPPFLAGS = -DSOMEFLAG
libmix_la_SOURCES = $(proxytest_sources)

libmix_la_LIBADD = $(top_builddir)/xml/libxml.a \
	    $(top_builddir)/popt/libpopt.a \
	    $(top_builddir)/trio/libtrio.a \
	    $(top_builddir)/tre/libtre.a \
	    $(top_builddir)/gcm/libgcm.a


else #only lib

if BUILD_AS_SHADOW_PLUGIN

lib_LTLIBRARIES = libmixshadow.la
libmixshadow_la_SOURCES = $(proxytest_sources) proxytest.cpp

libmixshadow_la_LIBADD = $(top_builddir)/xml/libxml.a \
	    $(top_builddir)/popt/libpopt.a \
	    $(top_builddir)/trio/libtrio.a \
	    $(top_builddir)/tre/libtre.a \
	    $(top_builddir)/gcm/libgcm.a \
	    $(top_builddir)/TypeA/libmixa.a \
	    $(top_builddir)/SquidLogHelper/libsquidloghelper.a 
libmixshadow_la_LDFLAGS = -Wl,--no-as-needed
else #BUILD_AS_SHADOW_PLUGIN

noinst_LIBRARIES = libproxytest.a
sbin_PROGRAMS = mix
libproxytest_a_CFLAGS = $(AM_CFLAGS) @USE_SGX_INSIDE@
libproxytest_a_CPPFLAGS = $(AM_CPPFLAGS) @USE_SGX_INSIDE@
libproxytest_a_SOURCES = $(proxytest_sources)


mix_SOURCES = proxytest.cpp
mix_CFLAGS = $(AM_CFLAGS) @USE_SGX_INSIDE@
mix_CPPFLAGS = $(AM_CPPFLAGS) @USE_SGX_INSIDE@
mix_LDADD += $(top_builddir)/libproxytest.a \
	    $(top_builddir)/xml/libxml.a \
	    $(top_builddir)/popt/libpopt.a \
	    $(top_builddir)/trio/libtrio.a \
	    $(top_builddir)/tre/libtre.a \
	    $(top_builddir)/gcm/libgcm.a \
	    $(top_builddir)/TypeA/libmixa.a \
	    $(top_builddir)/SquidLogHelper/libsquidloghelper.a 


EXTRA_DIST = README proxytest.doxy proxytest.dsp proxytest.dsw



endif #BUILD_AS_SHADOW_PLUGIN

endif #only lib

if DOTESTS
    MAYBE_tests=test
endif    

if DOWIRESHARK
    MAYBE_wireshark=wireshark
endif    

if USE_SGX
	MAYBE_sgx=SGX
endif

if ALL_FROM_SOURCE
BUILT_SOURCES = external.h

mix_LDADD += $(top_builddir)/build/mxml/libmxml.a \
	$(top_builddir)/build/openssl/libssl.a \
	$(top_builddir)/build/openssl/libcrypto.a 

external.h:
#Make MXML
	mkdir -p $(top_builddir)/build/mxml
	wget https://github.com/michaelrsweet/mxml/releases/download/v2.12/mxml-2.12.tar.gz -O $(top_builddir)/build/mxml.tgz
	tar -xzf $(top_builddir)/build/mxml.tgz -C $(top_builddir)/build/mxml --strip-components=1
if USE_SCONE
	cd $(top_builddir)/build/mxml ; CC=@SCONE_GCC@ ./configure --enable-threads
else
	cd $(top_builddir)/build/mxml ; ./configure --enable-threads
endif
	cd $(top_builddir)/build/mxml ; make libmxml.a
#Make OpenSSL
	mkdir -p $(top_builddir)/build/openssl
	wget https://www.openssl.org/source/openssl-1.1.0k.tar.gz -O $(top_builddir)/build/openssl.tgz
	tar -xzf $(top_builddir)/build/openssl.tgz -C $(top_builddir)/build/openssl --strip-components=1
if USE_SCONE
	cd $(top_builddir)/build/openssl ; CC=@SCONE_GCC@ ./config no-async no-capieng no-cms no-comp no-ct no-deprecated no-dgram no-dso no-dtls no-gost no-ocsp no-stdio no-shared  no-tls no-ui no-ts   no-psk no-srp 
else
	cd $(top_builddir)/build/openssl ; ./config no-capieng no-async no-cms no-comp no-ct no-deprecated no-dgram no-dso no-dtls no-gost no-ocsp no-stdio no-shared  no-tls no-ui no-ts   no-psk no-srp 
endif
	cd $(top_builddir)/build/openssl ; make
endif #all from source


SUBDIRS = popt xml trio tre TypeA gcm $(MAYBE_tests) $(MAYBE_wireshark) SquidLogHelper $(MAYBE_sgx)
