.SUFFIXES:
.SUFFIXES: .o .cpp	

#CC=gcc
#
CC=CC -mips4 -64
INCLUDE = -I. -I/usr/users/sya/sk13/openssl/include -I/home/imis/mix/openssl64/include -I/sun/ikt/sk13/openssl/include
LIBS	= -L/usr/users/sya/sk13/openssl/lib -L/home/imis/mix/openssl64/lib -L/sun/ikt/sk13/openssl/lib ./popt/popt.a ./httptunnel/httptunnel.a ./xml/xml.a -lcrypto -lpthread
# -lstdc++
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
	proxytest.o

OBJS=$(OBJS_ALL)

.cpp.o:
	$(CC) -c $(CPPFLAGS) $(INCLUDE) $(DEBUG) $< $(LDFLAGS) -o $@


all: $(OBJS) popt.a httptunnel.a xml.a
	$(CC) -o proxytest $(OBJS) $(LIBS)

all_sun: $(OBJS) popt.a httptunnel.a xml.a
	$(CC) -o proxytest $(OBJS) $(LIBS) -lsocket -lnsl

all_sgi: $(OBJS) httptunnel.a popt.a xml.a
	$(CC) -o proxytest $(OBJS) $(LIBS) 
#	#-lsocket -lnsl

debug: $(OBJS) popt.a httptunnel.a xml.a
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

xml.a: ./xml/xmloutput.cpp  
	$(CC) -c  $(INCLUDE) $(DEBUG) ./xml/xmloutput.cpp -o ./xml/xmloutput.o
	ar -rcs ./xml/xml.a ./xml/*.o 

clean:
	- rm $(OBJS)
	- rm ./popt/*.o
	- rm ./popt/*.a
	- rm ./httptunnel/*.o
	- rm ./httptunnel/*.a
	- rm ./xml/*.o
	- rm ./xml/*.a
	- rm proxytest
