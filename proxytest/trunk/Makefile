.SUFFIXES:
.SUFFIXES: .o .cpp	

CC=gcc
INCLUDE = -I. -I/sun/ikt/sk13/openssl/include
LIBS	= -L/usr/local/ssl/lib -L/sun/ikt/sk13/openssl/lib ./popt/popt.a ./httptunnel/httptunnel.a -lcrypto
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
	CAASymCipher.o\
	proxytest.o

OBJS=$(OBJS_ALL)

.cpp.o:
	$(CC) -c $(CPPFLAGS) $(INCLUDE) $(DEBUG) $< $(LDFLAGS) -o $@


all: $(OBJS) popt.a httptunnel.a
	$(CC) -o proxytest $(OBJS) $(LIBS)

all_sun: $(OBJS) 
	$(CC) -o proxytest $(OBJS) $(LIBS) -lsocket -lnsl

debug: $(OBJS) popt.a httptunnel.a
	$(CC) -o proxytest $(OBJS) $(LIBS)

popt.a: ./popt/popt.c ./popt/poptparse.c ./popt/popthelp.c
	$(CC) -c  $(INCLUDE) $(DEBUG) -DHAVE_STRERROR ./popt/popt.c -o ./popt/popt.o
	$(CC) -c  $(INCLUDE) $(DEBUG) -DHAVE_STRERROR ./popt/poptparse.c -o ./popt/poptparse.o
	$(CC) -c  $(INCLUDE) $(DEBUG) -DHAVE_STRERROR ./popt/popthelp.c -o ./popt/popthelp.o
	ar -rcs ./popt/popt.a ./popt/popt.o ./popt/poptparse.o ./popt/popthelp.o 

httptunnel.a: ./httptunnel/common.cpp  
	$(CC) -c  $(INCLUDE) $(DEBUG) ./httptunnel/common.cpp -o ./httptunnel/common.o
	$(CC) -c  $(INCLUDE) $(DEBUG) ./httptunnel/http.cpp -o ./httptunnel/http.o
	$(CC) -c  $(INCLUDE) $(DEBUG) ./httptunnel/tunnel.cpp -o ./httptunnel/tunnel.o
	ar -rcs ./httptunnel/httptunnel.a ./httptunnel/*.o 

clean:
	- rm $(OBJS)
	- rm ./popt/*.o
	- rm ./popt/*.a
	- rm ./httptunnel/*.o
	- rm ./httptunnel/*.a
	- rm proxytest
