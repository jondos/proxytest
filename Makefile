CC=gcc
INCLUDE = 
LIBS	= -lpthread
CPPFLAGS = -D_REENTRANT -g

OBJS_ALL=CASocket.o\
	CASocketGroup.o\
	CAMixChannel.o\
	CAMixSocket.o\
	CASocketAddr.o\
	CASocketList.o\
	proxytest.o

OBJS=$(OBJS_ALL)

all: $(OBJS) 
	$(CC) -o proxytest $(OBJS) $(LIBS)

clean:
	- rm $(OBJS)
	- rm proxytest
