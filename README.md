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

WARNING
-------

With a slightly earlier version,
I was trying to reproduce Madgarden's well-known bytebeat Starlost
(whose simplest form is `3*(x^x%255)`) and with the following noise:

       96<<6           x                          
        t<<0  x                                   
        t<<8     x                    x           
        t<<3                                      
    xa^xb^xc              x                       
    pa*pb*pc                                    x 
    sa+sb+sc                    x                 
           - sa sb sc pa pb pc xa xb xc audio<<19 

my laptop disk stopped working, and things that were trying to swap
hung, until I manually killed the `aplay` process:

    Feb 15 14:33:45 default-Aspire-one kernel: [ 1450.808402] ata1.00: exception Emask 0x0 SAct 0x3 SErr 0x0 action 0x0
    Feb 15 14:33:45 default-Aspire-one kernel: [ 1450.808645] ata1.00: irq_stat 0x40000008
    Feb 15 14:33:45 default-Aspire-one kernel: [ 1450.808740] ata1.00: failed command: READ FPDMA QUEUED
    Feb 15 14:33:45 default-Aspire-one kernel: [ 1450.808870] ata1.00: cmd 60/f0:00:78:89:8a/00:00:00:00:00/40 tag 0 ncq 122880 in
    Feb 15 14:33:45 default-Aspire-one kernel: [ 1450.808874]          res 51/10:f0:78:89:8a/00:00:00:00:00/40 Emask 0x481 (invalid argument) <F>
    Feb 15 14:33:45 default-Aspire-one kernel: [ 1450.809229] ata1.00: status: { DRDY ERR }
    Feb 15 14:33:45 default-Aspire-one kernel: [ 1450.809320] ata1.00: error: { IDNF }
    Feb 15 14:33:45 default-Aspire-one kernel: [ 1450.821155] ata1.00: configured for UDMA/133
    Feb 15 14:33:45 default-Aspire-one kernel: [ 1450.821198] ata1: EH complete

My best hypothesis is that the horrifying noise coming from the nearby
netbook speakers vibrated the disk to the point where it stopped
working!

(That version of Starlost *did* work if you compile to use 32-bit
longs instead of 16-bit shorts.  I redefined multiplication to make it
doable in 16 bits:

                3<<6           x                         
                t<<0  x                                  
                t<<8     x                    x          
                t<<3                                     
            xa^xb^xc                 x                   
    (pa*pb>>4)*pc>>4                                   x 
            sa+sb+sc                       x             
                   - sa sb sc pa pb pc xa xb xc audio<<5 


)
