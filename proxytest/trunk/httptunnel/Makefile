CC=gcc
#CC=CC -mips4 -64
INCLUDE = -I. -I/usr/local/ssl/include -I/usr/users/sya/sk13/openssl/include -I/home/imis/mix/openssl64/include -I/sun/ikt/sk13/openssl/include
LIBS	= -L/usr/local/ssl/lib -L/usr/users/sya/sk13/openssl/lib -L/home/imis/mix/openssl64/lib -L/sun/ikt/sk13/openssl/lib ./popt/popt.a ./httptunnel/httptunnel.a ./xml/xml.a -lcrypto -lpthread
#CPPFLAGS =-O3 -Wall 
CPPFLAGS =-O3 -D_REENTRANT
#DEBUG =-D_DEBUG -g

httptunnel.a: common.cpp http.cpp tunnel.cpp  
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) common.cpp -o common.o
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) http.cpp -o http.o
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) tunnel.cpp -o tunnel.o
	ar -rcs httptunnel.a *.o 

clean:
	rm -f *.o
	rm -f httptunnel.a
