#
# Autoconfiguration files for proxytest (a Mix proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign
sbin_PROGRAMS = mix
mix_SOURCES = CAAbstractControlChannel.cpp\
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
		    CACmdLnOptions.cpp \
		    CAConditionVariable.cpp\
		    CAControlChannelDispatcher.cpp\
		    CAControlChannelTest.cpp\
		    CADatabase.cpp\
		    CADatagramSocket.cpp \
		    CAFirstMix.cpp \
		    CAFirstMixChannelList.cpp \
		    CAHttpClient.cpp \
		    CAInfoService.cpp \
		    CAIPList.cpp \
		    CALastMix.cpp \
		    CALastMixChannelList.cpp\
		    CAListenerInterface.cpp\
		    CALocalProxy.cpp \
		    CALogPacketStats.cpp\
		    CAMiddleMix.cpp \
		    CAMiddleMixChannelList.cpp \
		    CAMix.cpp \
		    CAMsg.cpp \
		    CAMutex.cpp\
		    CAMuxSocket.cpp \
		    CAPayment.cpp\
		    CAPool.cpp\
		    CAQueue.cpp \
		    CASemaphore.cpp\
		    CASignature.cpp \
		    CASingleSocketGroup.cpp\
		    CASocket.cpp \
		    CASocketAddrINet.cpp \
		    CASocketAddrUnix.cpp \
		    CASocketGroup.cpp \
		    CASocketGroupEpoll.cpp\
		    CASocketList.cpp \
		    CATLSClientSocket.cpp\
		    CASSLContext.cpp\
		    CASymCipher.cpp \
		    CASyncControlChannel.cpp\
		    CATempIPBlockList.cpp\
		    CAThread.cpp \
		    CAThreadPool.cpp\
		    CAUtil.cpp \
		    CAXMLBI.cpp \
		    CAXMLCostConfirmation.cpp\
		    CAXMLErrorMessage.cpp\
		    StdAfx.cpp \
		    proxytest.cpp \
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
		    CAFirstMix.hpp \
		    CAFirstMixChannelList.hpp \
		    CAHttpClient.hpp \
		    CAInfoService.hpp \
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
		    CAMsg.hpp \
		    CAMutex.hpp\
		    CAMuxSocket.hpp \
		    CAPayment.hpp\
		    CAPool.hpp\
		    CAQueue.hpp \
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
		    CATLSClientSocket.hpp\
		    CASSLContext.hpp\
		    CASymCipher.hpp \
		    CASyncControlChannel.hpp \
		    CATempIPBlockList.hpp\
		    CAThread.hpp \
		    CAThreadPool.hpp\
		    CAUtil.hpp \
		    CAXMLBI.hpp \
		    CAXMLCostConfirmation.hpp\
		    CAXMLErrorMessage.hpp\
		    StdAfx.h \
		    basetypedefs.h


mix_LDADD = $(top_srcdir)/xml/libxml.a \
		  $(top_srcdir)/popt/libpopt.a \
		  $(top_srcdir)/trio/libtrio.a \
		  $(top_srcdir)/tre/libtre.a \
		  $(top_srcdir)/TypeA/libmixa.a

EXTRA_DIST = README proxytest.doxy proxytest.dsp proxytest.dsw

SUBDIRS = popt xml trio tre TypeA
