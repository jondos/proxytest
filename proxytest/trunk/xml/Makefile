xml.a: xmloutput.cpp xmlinput.cpp xmlinput_c.c  
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) xmloutput.cpp -o xmloutput.o
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) xmlinput.cpp -o xmlinput.o
	$(CC) -c  $(CPPFLAGS) $(INCLUDE) $(DEBUG) xmlinput_c.c -o xmlinput_c.o
	ar -rcs xml.a *.o 

clean:
	rm -f *.o
	rm -f xml.a

