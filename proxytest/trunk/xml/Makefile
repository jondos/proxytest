CC=gcc
#CC=CC -mips4 -64
INCLUDE = -I. -I/usr/local/ssl/include -I/usr/users/sya/sk13/openssl/include -I/home/imis/mix/openssl64/include -I/sun/ikt/sk13/openssl/include
#CPPFLAGS =-O3 -Wall 
CPPFLAGS =-O3 -D_REENTRANT
#DEBUG =-D_DEBUG -g

xml.a: xmloutput.cpp xmlinput.cpp xmlinput_c.c  
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) ./xml/xmloutput.cpp -o ./xml/xmloutput.o
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) ./xml/xmlinput.cpp -o ./xml/xmlinput.o
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) ./xml/xmlinput_c.c -o ./xml/xmlinput_c.o
	ar -rcs ./xml/xml.a ./xml/*.o 

clean:
	rm -f *.o
	rm -f xml.a

