.SUFFIXES:
.SUFFIXES: .o .cpp	

CC=gcc
INCLUDE = 
LIBS	= -lpthread
CPPFLAGS = -D_REENTRANT -O3 -Wall
DEBUG =-D_DEBUG -g

OBJS_ALL=CASocket.o\
	CASocketGroup.o\
	CAMixChannel.o\
	CAMixSocket.o\
	CASocketAddr.o\
	CASocketList.o\
	proxytest.o

OBJS=$(OBJS_ALL)

.cpp.o:
	$(CC) -c $(CPPFLAGS) $(DEBUG) $< $(LDFLAGS) -o $@

all: $(OBJS) 
	$(CC) -o proxytest $(OBJS) $(LIBS)

all_sun: $(OBJS) 
	$(CC) -o proxytest $(OBJS) $(LIBS) -lsocket -lnsl

debug: $(OBJS)
	$(CC) -o proxytest $(OBJS) $(LIBS)

clean:
	- rm $(OBJS)
	- rm proxytest
