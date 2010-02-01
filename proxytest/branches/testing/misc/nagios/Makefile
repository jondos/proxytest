all: check_mix

check_mix: check_mix.o
	gcc -o check_mix check_mix.o /usr/local/lib/libexpat.a
	
check_mix.o: check_mix.c check_mix.h ../../monitoringDefs.h
	gcc -c -pedantic -Wall check_mix.c
		
clean:
	rm -f *.o
	rm -f check_mix
