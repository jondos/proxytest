httptunnel.a: common.cpp http.cpp tunnel.cpp  
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) common.cpp -o common.o
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) http.cpp -o http.o
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) tunnel.cpp -o tunnel.o
	ar -rcs httptunnel.a *.o 

clean:
	rm -f *.o
	rm -f httptunnel.a
