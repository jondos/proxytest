.SUFFIXES:
.SUFFIXES: .o .cpp	

CC=gcc
INCLUDE = 
LIBS	= -lpthread
CPPFLAGS = -D_REENTRANT -O3 -Wall
#DEBUG =-D_DEBUG -g

OBJS_ALL=CASocket.o\
	CASocketGroup.o\
	CAMixChannel.o\
	CAMixSocket.o\
	CASocketAddr.o\
	CASocketList.o\
	CACmdLnOptions.o\
	proxytest.o

OBJS=$(OBJS_ALL)

.cpp.o:
	$(CC) -c $(CPPFLAGS) $(DEBUG) $< $(LDFLAGS) -o $@

popt.a:
	$(CC) -c  $(DEBUG) -DHAVE_STRERROR ./popt/popt.c -o ./popt/popt.o
	$(CC) -c  $(DEBUG) -DHAVE_STRERROR ./popt/poptparse.c -o ./popt/poptparse.o
	$(CC) -c  $(DEBUG) -DHAVE_STRERROR ./popt/findme.c -o ./popt/findme.o
	ar -rcs ./popt/popt.a ./popt/popt.o ./popt/poptparse.o ./popt/findme.o

all: $(OBJS) popt.a
	$(CC) -o proxytest $(OBJS) ./popt/popt.a $(LIBS)

all_sun: $(OBJS) 
	$(CC) -o proxytest $(OBJS) $(LIBS) -lsocket -lnsl

debug: $(OBJS) popt.a
	$(CC) -o proxytest $(OBJS) ./popt/popt.a $(LIBS)

clean:
	- rm $(OBJS)
	- rm proxytest
