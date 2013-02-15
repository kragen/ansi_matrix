CFLAGS=-Wall -Werror
test: ascii_interface
	./ascii_interface

ascii_interface: avr_bytebeat_interp.o ascii_interface.o sound.o

clean:
	rm ascii_interface *.o