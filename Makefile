.SUFFIXES:
.SUFFIXES: .o .cpp	

#CC=gcc
#CC=CC -mips4 -64
#CC=gcc -mcpu=ultrasparc
#INCLUDE = -I. -I/usr/local/ssl/include -I/usr/users/sya/sk13/openssl/include -I/home/imis/mix/openssl64/include -I/sun/ikt/sk13/openssl/include
INCLUDE = -I. -I/usr/users/sya/sk13/openssl/include -I/home/imis/mix/openssl64/include -I/sun/ikt/sk13/openssl/include
#LIBS	= -L/usr/local/ssl/lib -L/usr/users/sya/sk13/openssl/lib -L/home/imis/mix/openssl64/lib -L/sun/ikt/sk13/openssl/lib ./popt/popt.a ./httptunnel/httptunnel.a ./xml/xml.a -lcrypto -lpthread
LIBDIR= -L/usr/users/sya/sk13/openssl/lib -L/home/imis/mix/openssl64/lib -L/sun/ikt/sk13/openssl/lib
LIBS	= ./popt/popt.a ./httptunnel/httptunnel.a ./xml/xml.a -lcrypto -lpthread
#CPPFLAGS =-O3 -Wall 
#CPPFLAGS =-O3 -D_REENTRANT
#DEBUG =-D_DEBUG -g

OBJS_ALL=CASocket.o\
	CASocketGroup.o\
	CASocketAddr.o\
	CASocketList.o\
	CACmdLnOptions.o\
	CAMsg.o\
	CAMuxSocket.o\
	CAMuxChannelList.o\
	CASymCipher.o\
	CAASymCipher.o\
	CAInfoService.o\
	CASignature.o\
	CABase64.o\
	CAUtil.o\
	proxytest.o

OBJS=$(OBJS_ALL)

.cpp.o:
	$(CC) -c $(CPPFLAGS) $(INCLUDE) $(DEBUG) $< $(LDFLAGS) -o $@


all: 
	@echo 'Please select a target:'
	@echo 'linux-x86-gcc, linux-x86-gcc-static, linux-x86-gcc-debug,'
	@echo 'solaris-ultrasparc-gcc, irix-64-cc'

linux-x86-gcc:
	$(MAKE) 'CC=gcc' 'CPPFLAGS=-O3 -D_REENTRANT' 'INCLUDE=-I. -I/usr/local/ssl/include' 'LIBS=$(LIBS) -lstdc++' 'LIBDIR=-L/usr/local/ssl/lib' _all 

linux-x86-gcc-static:
	$(MAKE) 'CC=gcc' 'CPPFLAGS=-static -O3 -D_REENTRANT' 'INCLUDE=-I. -I/usr/local/ssl/include' 'LIBS=$(LIBS) -lstdc++' 'LIBDIR=-L/usr/local/ssl/lib' _all 

linux-x86-gcc-debug:
	$(MAKE) 'CC=gcc' 'CPPFLAGS=-static -O3 -D_REENTRANT' 'DEBUG=-g -D_DEBUG' 'INCLUDE=-I. -I/usr/local/ssl/include' 'LIBS=$(LIBS) -lstdc++' 'LIBDIR=-L/usr/local/ssl/lib' _all 

solaris-ultrasparc-gcc:
	$(MAKE) 'CC=gcc -mcpu=ultrasparc' 'CPPFLAGS=-O3 -D_REENTRANT' 'LIBS=$(LIBS) -lstdc++' _all 

irix-64-cc:
	$(MAKE) 'CC=CC -r10000 -mips4 -64' 'CPPFLAGS=-fullwarn -Ofast=IP27 -D_REENTRANT' 'LIBS=$(LIBS)' _all 

_all: $(OBJS) httptunnel.a popt.a xml.a
	$(CC) -o proxytest $(DEBUG) $(CPPFLAGS) $(OBJS) $(LIBDIR) $(LIBS) 

popt.a:
	cd popt;$(MAKE) 'CC=$(CC)' 'DEBUG=$(DEBUG)' 'INCLUDE=$(INCLUDE)' 'CPPFLAGS=$(CPPFLAGS)'

httptunnel.a:
	cd httptunnel;$(MAKE) 'CC=$(CC)' 'DEBUG=$(DEBUG)' 'INCLUDE=$(INCLUDE)' 'CPPFLAGS=$(CPPFLAGS)';

xml.a: 
	cd xml;$(MAKE) 'CC=$(CC)' 'DEBUG=$(DEBUG)' 'INCLUDE=$(INCLUDE)' 'CPPFLAGS=$(CPPFLAGS)';

clean:
	rm -f $(OBJS)
	cd popt; $(MAKE) clean;
	cd httptunnel;$(MAKE) clean;
	cd xml;$(MAKE) clean;
	rm -f proxytest
