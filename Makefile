.SUFFIXES:
.SUFFIXES: .o .cpp	

CC=gcc
INCLUDE = -I. -I/sun/ikt/sk13/openssl/include
LIBS	= -L/usr/local/ssl/lib -L/sun/ikt/sk13/openssl/lib 
#-lcrypto
#-lpthread 
CPPFLAGS =-O3 -Wall 
#-D_REENTRANT
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
	proxytest.o

OBJS=$(OBJS_ALL)

.cpp.o:
	$(CC) -c $(CPPFLAGS) $(INCLUDE) $(DEBUG) $< $(LDFLAGS) -o $@


all: $(OBJS) popt.a
	$(CC) -o proxytest $(OBJS) ./popt/popt.a $(LIBS)

all_sun: $(OBJS) 
	$(CC) -o proxytest $(OBJS) $(LIBS) ./popt/popt.a -lsocket -lnsl

debug: $(OBJS) popt.a
	$(CC) -o proxytest $(OBJS) ./popt/popt.a $(LIBS)

popt.a: ./popt/popt.c ./popt/poptparse.c 
	$(CC) -c  $(INCLUDE) $(DEBUG) -DHAVE_STRERROR ./popt/popt.c -o ./popt/popt.o
	$(CC) -c  $(INCLUDE) $(DEBUG) -DHAVE_STRERROR ./popt/poptparse.c -o ./popt/poptparse.o
#	$(CC) -c  $(INCLUDE) $(DEBUG) -DHAVE_STRERROR ./popt/findme.c -o ./popt/findme.o
	ar -rcs ./popt/popt.a ./popt/popt.o ./popt/poptparse.o 

clean:
	- rm $(OBJS)
	- rm proxytest
