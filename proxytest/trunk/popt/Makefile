#CC=gcc
#CC=CC -mips4 -64
#INCLUDE = -I. -I/usr/local/ssl/include -I/usr/users/sya/sk13/openssl/include -I/home/imis/mix/openssl64/include -I/sun/ikt/sk13/openssl/include
#LIBS	= -L/usr/local/ssl/lib -L/usr/users/sya/sk13/openssl/lib -L/home/imis/mix/openssl64/lib -L/sun/ikt/sk13/openssl/lib ./popt/popt.a ./httptunnel/httptunnel.a ./xml/xml.a -lcrypto -lpthread
#CPPFLAGS =-O3 -Wall 
#CPPFLAGS =-O3 -D_REENTRANT
#DEBUG =-D_DEBUG -g

popt.a: popt.c poptparse.c popthelp.c
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) -DHAVE_STRERROR popt.c -o popt.o
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) -DHAVE_STRERROR poptparse.c -o poptparse.o
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) -DHAVE_STRERROR popthelp.c -o popthelp.o
	ar -rcs popt.a popt.o poptparse.o popthelp.o 

clean:
	rm -f *.o
	rm -f popt.a
