#
# Autoconfiguration files for proxytest (a MiX proxy)
# Konrad Rieck <kr@r0q.cx>
#

AUTOMAKE_OPTIONS = foreign
sbin_PROGRAMS = proxytest
proxytest_SOURCES = CAASymCipher.cpp \
										CABase64.cpp \
										CACacheLoadBalancing.cpp\
										CACmdLnOptions.cpp \
		   							CADatagramSocket.cpp \
										CAFirstMix.cpp \
										CAInfoService.cpp \
										CAIPList.cpp \
		   							CALastMix.cpp \
										CALocalProxy.cpp \
										CAMiddleMix.cpp \
		   							CAMix.cpp \
										CAMsg.cpp \
										CAMuxChannelList.cpp \
		   							CAMuxSocket.cpp \
										CAQueue.cpp \
										CARoundTripTime.cpp \
										CASignature.cpp \
		   							CASocket.cpp \
										CASocketAddrINet.cpp \
										CASocketAddrUnix.cpp \
										CASocketGroup.cpp \
		   							CASocketList.cpp \
										CASymCipher.cpp \
										CAUtil.cpp \
										StdAfx.cpp \
		   							proxytest.cpp \
		   	            CAASymCipher.hpp \
										CA_Base64.hpp \
										CACallback.hpp \
										CACacheLoadBalancing.hpp\
										CACmdLnOptions.hpp \
										CAFirstMix.hpp \
										CAInfoService.hpp \
										CAIPList.hpp \
										CALastMix.hpp \
										CALocalProxy.hpp \
										CAMiddleMix.hpp \
										CAMix.hpp \
										CAMsg.hpp \
										CAMuxChannelList.hpp \
										CAMuxSocket.hpp \
										CAQueue.hpp \
										CARoundTripTime.hpp \
										CASignature.hpp \
										CASocket.hpp \
										CASocketAddr.hpp \ 
										CASockerAddrINet.hpp \
										CASocketAddrUnix.hpp \
										CASocketGroup.hpp \
										CASocketList.hpp \
		   							CASymCipher.hpp \
		    						CAUtil.hpp \
		    						StdAfx.h 

proxytest_LDADD = $(top_srcdir)/xml/libxml.a \
		  $(top_srcdir)/aes/libaes.a \
		  $(top_srcdir)/popt/libpopt.a

EXTRA_DIST = README proxytest.doxy proxytest.dsp proxytest.dsw

SUBDIRS = popt xml aes
