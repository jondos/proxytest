#
# Autoconfiguration files for proxytest (a MiX proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign
sbin_PROGRAMS = proxytest
proxytest_SOURCES = CAASymCipher.cpp CABase64.cpp CACmdLnOptions.cpp \
		    CADatagramSocket.cpp CAFirstMix.cpp CAInfoService.cpp \
		    CALastMix.cpp CALocalProxy.cpp CAMiddleMix.cpp \
		    CAMix.cpp CAMsg.cpp CAMuxChannelList.cpp \
		    CAMuxSocket.cpp CARoundTripTime.cpp CASignature.cpp \
		    CASocket.cpp CASocketAddr.cpp CASocketGroup.cpp \
		    CASocketList.cpp CASymCipher.cpp CAUtil.cpp StdAfx.cpp \
		    proxytest.cpp \
		    CAASymCipher.hpp CAMix.hpp CASocketList.hpp CABase64.hpp \
		    CAMsg.hpp CASymCipher.hpp CACmdLnOptions.hpp \
		    CAMuxChannelList.hpp CAUtil.hpp CADatagramSocket.hpp \
		    CAMuxSocket.hpp StdAfx.h CAFirstMix.hpp \
		    CARoundTripTime.hpp CAInfoService.hpp CASignature.hpp \
		    CALastMix.hpp CASocket.hpp CALocalProxy.hpp \
		    CASocketAddr.hpp CAMiddleMix.hpp CASocketGroup.hpp 

proxytest_LDADD = $(top_srcdir)/xml/libxml.a \
		  $(top_srcdir)/httptunnel/libhttptunnel.a \
		  $(top_srcdir)/popt/libpopt.a

EXTRA_DIST = README proxytest.doxy proxytest.dsp proxytest.dsw

SUBDIRS = popt xml httptunnel
