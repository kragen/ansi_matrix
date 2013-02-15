ansi_matrix
===========

At the Nonino Alberti Media Lab, we're working on a hardware bytebeat
synthesizer. The first prototype was a DHTML thing I hacked up; this
ANSI-art C program is the second prototype; the third prototype is
intended to be some of this same code running on an Arduino and hooked
up to some blinky lights and an R-2R ladder DAC; the final product, if
we get to it, will do the same thing but with discrete gates instead
of software on a microcontroller.  (The multiplier will be especially
tricky!)

On Linux, at least, it should run and start making noises if you type
`make`.  MacOS support using CoreAudio is strongly desired but not yet
implemented.

