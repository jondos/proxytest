.SUFFIXES:
.SUFFIXES: .o .cpp	

#CC=gcc
CC=CC -mips4 -64
#CC=gcc -mcpu=ultrasparc
#INCLUDE = -I. -I/usr/local/ssl/include -I/usr/users/sya/sk13/openssl/include -I/home/imis/mix/openssl64/include -I/sun/ikt/sk13/openssl/include
INCLUDE = -I. -I/usr/users/sya/sk13/openssl/include -I/home/imis/mix/openssl64/include -I/sun/ikt/sk13/openssl/include
#LIBS	= -L/usr/local/ssl/lib -L/usr/users/sya/sk13/openssl/lib -L/home/imis/mix/openssl64/lib -L/sun/ikt/sk13/openssl/lib ./popt/popt.a ./httptunnel/httptunnel.a ./xml/xml.a -lcrypto -lpthread
LIBS	= -L/usr/users/sya/sk13/openssl/lib -L/home/imis/mix/openssl64/lib -L/sun/ikt/sk13/openssl/lib ./popt/popt.a ./httptunnel/httptunnel.a ./xml/xml.a -lcrypto -lpthread
#CPPFLAGS =-O3 -Wall 
CPPFLAGS =-O3 -D_REENTRANT
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


all: $(OBJS) popt.a httptunnel.a xml.a
	$(CC) -o proxytest $(OBJS) $(LIBS) -lstdc++

all_static: $(OBJS) popt.a httptunnel.a xml.a
	$(CC) -static -o proxytest $(OBJS) $(LIBS) -lstdc++

all_sun: $(OBJS) popt.a httptunnel.a xml.a
	$(CC) -o proxytest $(OBJS) $(LIBS) -lsocket -lnsl -lstdc++

all_sgi: $(OBJS) httptunnel.a popt.a xml.a
	$(CC) -o proxytest $(OBJS) $(LIBS) 

debug: $(OBJS) popt.a httptunnel.a xml.a
	$(CC) -o proxytest $(OBJS) $(LIBS)

popt.a:
	cd popt;$(MAKE) 'CC=$(CC)' 'INCLUDE=$(INCLUDE)' 'CPPFLAGS=$(CPPFLAGS)'

httptunnel.a:
	cd httptunnel;$(MAKE) 'CC=$(CC)' 'INCLUDE=$(INCLUDE)' 'CPPFLAGS=$(CPPFLAGS)';

xml.a: 
	cd xml;$(MAKE) 'CC=$(CC)' 'INCLUDE=$(INCLUDE)' 'CPPFLAGS=$(CPPFLAGS)';

clean:
	rm -f $(OBJS)
	cd popt; $(MAKE) clean;
	cd httptunnel;$(MAKE) clean;
	cd xml;$(MAKE) clean;
	rm -f proxytest
