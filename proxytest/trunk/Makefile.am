#
# Autoconfiguration files for proxytest (a MiX proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign
sbin_PROGRAMS = proxytest
proxytest_SOURCES = CAASymCipher.cpp \
		    CABase64.cpp \
		    CACacheLoadBalancing.cpp\
		    CACertificate.cpp\
		    CACertStore.cpp\
		    CACmdLnOptions.cpp \
		    CAConditionVariable.cpp\
		    CADatagramSocket.cpp \
		    CAFirstMix.cpp \
		    CAFirstMixChannelList.cpp \
		    CAInfoService.cpp \
		    CAIPList.cpp \
		    CALastMix.cpp \
		    CALastMixChannelList.cpp\
		    CALocalProxy.cpp \
		    CAMiddleMix.cpp \
		    CAMiddleMixChannelList.cpp \
		    CAMix.cpp \
		    CAMsg.cpp \
		    CAMutex.cpp\
		    CAMuxSocket.cpp \
		    CAQueue.cpp \
		    CASemaphore.cpp\
		    CASignature.cpp \
		    CASocket.cpp \
		    CASocketAddrINet.cpp \
		    CASocketAddrUnix.cpp \
		    CASocketGroup.cpp \
		    CASocketList.cpp \
		    CASymCipher.cpp \
		    CAThread.cpp \
		    CAUtil.cpp \
		    StdAfx.cpp \
		    proxytest.cpp \
		    CAASymCipher.hpp \
		    CABase64.hpp \
		    CACacheLoadBalancing.hpp\
		    CACertificate.hpp\
		    CACertStore.hpp\
		    CAConditionVariable.hpp\
		    CACmdLnOptions.hpp \
		    CAFirstMix.hpp \
		    CAFirstMixChannelList.hpp \
		    CAInfoService.hpp \
		    CAIPList.hpp \
		    CALastMix.hpp \
		    CALastMixChannelList.hpp\
		    CALocalProxy.hpp \
		    CALockAble.hpp\
		    CAMiddleMix.hpp \
		    CAMiddleMixChannelList.hpp \
		    CAMix.hpp \
		    CAMsg.hpp \
		    CAMutex.hpp\
		    CAMuxSocket.hpp \
		    CAQueue.hpp \
		    CASemaphore.hpp\
		    CASignature.hpp \
		    CASocket.hpp \
		    CASocketAddr.hpp \
		    CASockerAddrINet.hpp \
		    CASocketAddrUnix.hpp \
		    CASocketGroup.hpp \
		    CASocketList.hpp \
		    CASymCipher.hpp \
		    CAThread.hpp \
		    CAUtil.hpp \
		    StdAfx.h 

proxytest_LDADD = $(top_srcdir)/xml/libxml.a \
		  $(top_srcdir)/aes/libaes.a \
		  $(top_srcdir)/popt/libpopt.a\
		  $(top_srcdir)/trio/libtrio.a

EXTRA_DIST = README proxytest.doxy proxytest.dsp proxytest.dsw

SUBDIRS = popt xml aes trio
