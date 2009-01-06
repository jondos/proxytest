#
# Autoconfiguration files for proxytest (a Mix proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libproxytest.a
sbin_PROGRAMS = mix

libproxytest_a_SOURCES = CAAbstractControlChannel.cpp\
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
		    CAFirstMix.cpp \
		    CAFirstMixChannelList.cpp \
		    CAHttpClient.cpp \
		    CAInfoService.cpp \
		    CAInfoServiceAware.cpp \
		    CAIPList.cpp \
		    CALastMix.cpp \
		    CALastMixChannelList.cpp\
		    CAListenerInterface.cpp\
		    CALocalProxy.cpp \
		    CALogPacketStats.cpp\
		    CAMiddleMix.cpp \
		    CAMiddleMixChannelList.cpp \
		    CAMix.cpp \
		    CAMixWithReplayDB.cpp\
		    CAMsg.cpp \
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
		    packetintro.cpp
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
		    CACmdLnOptions.hpp \
		    CADatabase.hpp\
		    CADatagramSocket.hpp\
		    CADataRetentionLog.hpp\
		    CADataRetentionLogFile.hpp\
		    CADynaNetworking.hpp \
		    CADynamicCascadeConfigurator.hpp \
		    CAFirstMix.hpp \
		    CAFirstMixChannelList.hpp \
		    CAHttpClient.hpp \
		    CAInfoService.hpp \
		    CAInfoServiceAware.hpp \
		    CAIPList.hpp \
		    CALastMix.hpp \
		    CALastMixChannelList.hpp\
		    CAListenerInterface.hpp\
		    CALocalProxy.hpp \
		    CALockAble.hpp\
		    CALogPacketStats.hpp\
		    CAMiddleMix.hpp \
		    CAMiddleMixChannelList.hpp \
		    CAMix.hpp \
		    CAMixWithReplayDB.hpp\
		    CAMsg.hpp \
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
		    packetintro.h\
		    StdAfx.h\
		    basetypedefs.h

mix_SOURCES = proxytest.cpp

mix_LDADD = $(top_srcdir)/libproxytest.a \
	    $(top_srcdir)/xml/libxml.a \
	    $(top_srcdir)/popt/libpopt.a \
	    $(top_srcdir)/trio/libtrio.a \
	    $(top_srcdir)/tre/libtre.a \
	    $(top_srcdir)/gcm/libgcm.a \
	    $(top_srcdir)/TypeA/libmixa.a

EXTRA_DIST = README proxytest.doxy proxytest.dsp proxytest.dsw

if DOTESTS
    MAYBE_tests=test
endif    

SUBDIRS = popt xml trio tre TypeA gcm $(MAYBE_tests)
