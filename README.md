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

Instructions
------------

On Linux, at least, it should run and start making noises if you type
`make`.  MacOS support using CoreAudio is strongly desired but not yet
implemented.

hjkl to move the cursor; space and backspace, or + and -, to adjust
the things in the cells; d to toggle a debug dump.

Screenshot
---------

      160<<6           x                         
        t<<0  x        x  x                    x 
        t<<8     x                    x        x 
        t<<3                                   x 
    xa^xb^xc                                   x 
    pa*pb*pc                                   x 
    sa+sb+sc                       x             
           - sa sb sc pa pb pc xa xb xc audio<<8 

You can't see the highlight in this screenshot, but there's a
highlight.