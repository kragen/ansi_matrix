CFLAGS=-Wall -Werror
test: ascii_interface
	./ascii_interface

ascii_interface: avr_bytebeat_interp.o ascii_interface.o
